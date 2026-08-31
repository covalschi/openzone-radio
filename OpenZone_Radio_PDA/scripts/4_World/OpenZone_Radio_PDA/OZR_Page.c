// Сторінка «Рація»: стан плати й настройка на ділення.
//
// Тут немає нічого про звук, і це не пропуск. Голос веде рушій: він дивиться,
// який передавач у гравця й на що той налаштований. Наша справа -- поставити
// частоту; далі рушій робить усе сам.
//
// PTT тут теж НЕМАЄ, і це не пропуск удруге. Плата -- звичайна профільна
// рація, а спільний обхід інвентаря в моді рації відкриває ВСІ профільні
// рації гравця, хоч у руках, хоч усередині КПК. Окрема операція «ptt» на цій
// сторінці була б другим шляхом до того самого, і два шляхи рано чи пізно
// розійшлися б у тому, кого вважати увімкненим.

class OZ_PdaHandlerRadio : OZ_PageHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok    = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        if (op == "list")
            return List(sender, ok, error);

        if (op == "tune")
            return Tune(json, sender, ok, error);

        if (op == "save")
            return Save(json, sender, ok, error);

        if (op == "forget")
            return Forget(json, sender, ok, error);

        return "";
    }

    // --------------------------------------------------------------- стан

    private string List(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        OZ_Module_Radio board = OZR_Set.BoardIn(pda);

        OZR_RadioState st = new OZR_RadioState();
        st.HasBoard = (board != null);
        st.Powered  = pda.OZ_IsOn();

        if (board)
        {
            st.RangeM = OZR_Set.RangeOf(board);
            st.Live   = board.OZR_IsLive();
            st.Index  = board.GetTunedFrequencyIndex();

            if (OZR_Grid.Ready())
                st.FreqMHz = OZR_Grid.MHzAt(st.Index);

            OZR_RadioProfile p = OZR_Profiles.For(board.GetType());
            if (p)
            {
                st.MinMHz  = p.MinMHz;
                st.MaxMHz  = p.MaxMHz;
                st.StepMHz = p.StepMHz;
            }
        }

        // Книжка -- з носія, а не з сервера: вона річ гравця й їздить у його
        // кишені. Носія немає -- немає й книжки, і це не помилка.
        OZ_DataCarrier_Base carrier = OZ_DataCarrier_Base.Cast(pda.OZ_Attached(OZ_PdaConst.SLOT_CARRIER));
        if (carrier)
        {
            st.HasCarrier = true;
            st.FreeSlots  = carrier.OZ_RoomFor(OZRP_Const.KIND_FREQS);

            // Доступність рахуємо ТУТ, проти профілю тієї плати, яка справді
            // стоїть. Книжку носять між рацій: частота, записана з
            // п'ятикілометрової, у смугу двохсотметрової не влазить, і сказати
            // про це треба до натискання, а не після відмови.
            int lo;
            int hi;
            int stride;
            bool window = false;

            OZR_RadioProfile bp;
            if (board)
                bp = OZR_Profiles.For(board.GetType());
            if (bp)
                window = OZR_Grid.Window(bp, lo, hi, stride);

            OZR_FreqBook book = Read(carrier);
            for (int b = 0; b < book.Items.Count(); b++)
            {
                OZR_FreqEntry e = book.Items[b];

                OZR_BookRow row = new OZR_BookRow();
                row.Name  = e.Name;
                row.Index = e.Index;
                if (OZR_Grid.Ready())
                    row.MHz = OZR_Grid.MHzAt(e.Index);

                row.Reach = window && e.Index >= lo && e.Index <= hi;
                if (row.Reach && stride > 0)
                    row.Reach = ((e.Index - lo) % stride) == 0;

                st.Book.Insert(row);
            }
        }

        string body;
        string err;
        if (!JsonFileLoader<OZR_RadioState>.MakeData(st, body, err, false))
        {
            OZR_Log.Error("radio page: cannot serialise state: " + err);
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return body;
    }

    // ---------------------------------------------------------- настройка

    private string Tune(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZR_TuneRef r;
        string err;
        if (!JsonFileLoader<OZR_TuneRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_Module_Radio board;
        if (!Ready(sender, board, error))
            return "";

        // Ті самі перевірки, що й для ручної рації, і навмисно ті самі: плата
        // -- така сама профільна рація, і межі їй ставить її ж профіль.
        OZR_RadioProfile prof = OZR_Profiles.For(board.GetType());
        int lo;
        int hi;
        int stride;
        if (!prof || !OZR_Grid.Window(prof, lo, hi, stride))
        {
            error = "STR_OZR_ERR_NO_PROFILE";
            return "";
        }

        if (r.Index < lo || r.Index > hi)
        {
            error = "STR_OZR_ERR_OUT_OF_BAND";
            return "";
        }

        if (((r.Index - lo) % stride) != 0)
        {
            error = "STR_OZR_ERR_OFF_STEP";
            return "";
        }

        // Через OZR_TuneTo, а не SetFrequencyByIndex: він же й розкаже про нову
        // частоту клієнтові.
        board.OZR_TuneTo(r.Index);

        // Перечитуємо НАЗАД: присвоєння індексу -- прохання до рушія, а не
        // факт, і саме тут видно, чи він його прийняв.
        if (board.GetTunedFrequencyIndex() != r.Index)
        {
            OZR_Log.Warn("radio page: engine refused index " + r.Index.ToString());
            error = "STR_OZR_ERR_OUT_OF_BAND";
            return "";
        }

        ok = true;
        error = "";
        return "";
    }

    private bool Ready(PlayerIdentity sender, out OZ_Module_Radio board, out string error)
    {
        board = null;

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return false;
        }

        if (!pda.OZ_IsOn())
        {
            error = "STR_OZ_ERR_OFF";
            return false;
        }

        board = OZR_Set.BoardIn(pda);
        if (!board)
        {
            error = "STR_OZR_ERR_NO_BOARD";
            return false;
        }

        error = "";
        return true;
    }

    // ------------------------------------------------------------- книжка

    // Читання носія ніколи не падає: порожній, зіпсований і відсутній розділ
    // однаково означають «записів немає». Книжка -- не той документ, заради
    // якого варто зупиняти сторінку.
    private OZR_FreqBook Read(OZ_DataCarrier_Base carrier)
    {
        OZR_FreqBook book = new OZR_FreqBook();
        if (!carrier)
            return book;

        string payload = carrier.OZ_Read(OZRP_Const.KIND_FREQS);
        if (payload == "")
            return book;

        OZR_FreqBook parsed;
        string err;
        if (JsonFileLoader<OZR_FreqBook>.LoadData(payload, parsed, err) && parsed && parsed.Items)
            return parsed;

        OZR_Log.Warn("frequency book on " + carrier.GetType() + " does not parse: " + err);
        return book;
    }

    private bool Store(OZ_DataCarrier_Base carrier, OZR_FreqBook book, out string error)
    {
        string payload;
        string err;
        if (!JsonFileLoader<OZR_FreqBook>.MakeData(book, payload, err, false))
        {
            error = "STR_OZ_ERR_INTERNAL";
            return false;
        }

        // Один запис на частоту. Носій сам відмовить, якщо не влазить, --
        // і саме тому перевірка тут не дублюється.
        if (!carrier.OZ_Write(OZRP_Const.KIND_FREQS, payload, book.Items.Count()))
        {
            error = "STR_OZ_ERR_CARRIER_FULL";
            return false;
        }

        error = "";
        return true;
    }

    // Записати ПОТОЧНУ частоту під іменем. Індекс беремо з плати, а не з
    // прохання: клієнт міг відстати на тик, і збереглося б не те, що людина
    // бачила.
    private string Save(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZR_BookRef r;
        string err;
        if (!JsonFileLoader<OZR_BookRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_Module_Radio board;
        if (!Ready(sender, board, error))
            return "";

        OZ_DataCarrier_Base carrier;
        if (!Carrier(sender, carrier, error))
            return "";

        string name = r.Name;
        if (name == "")
        {
            error = "STR_OZR_ERR_NO_NAME";
            return "";
        }
        if (name.Length() > OZRP_Const.NAME_MAX)
            name = name.Substring(0, OZRP_Const.NAME_MAX);

        OZR_FreqBook book = Read(carrier);

        // Те саме ім'я переписується, а не дублюється: книжка -- не журнал.
        int at = -1;
        for (int i = 0; i < book.Items.Count(); i++)
        {
            if (book.Items[i].Name == name)
            {
                at = i;
                break;
            }
        }

        OZR_FreqEntry e = new OZR_FreqEntry();
        e.Name  = name;
        e.Index = board.GetTunedFrequencyIndex();

        if (at >= 0)
            book.Items.Set(at, e);
        else
            book.Items.Insert(e);

        if (!Store(carrier, book, error))
            return "";

        ok = true;
        error = "";
        return "";
    }

    private string Forget(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZR_BookRef r;
        string err;
        if (!JsonFileLoader<OZR_BookRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_DataCarrier_Base carrier;
        if (!Carrier(sender, carrier, error))
            return "";

        OZR_FreqBook book = Read(carrier);
        for (int i = book.Items.Count() - 1; i >= 0; i--)
        {
            if (book.Items[i].Name == r.Name)
                book.Items.Remove(i);
        }

        if (!Store(carrier, book, error))
            return "";

        ok = true;
        error = "";
        return "";
    }

    private bool Carrier(PlayerIdentity sender, out OZ_DataCarrier_Base carrier, out string error)
    {
        // Через ворота КПК, а не самотужки: право писати на носій -- його
        // правило, і другий екземпляр цього правила рано чи пізно розійшовся б
        // із першим.
        carrier = OZ_CarrierOps.ResolveWritable(sender, error);
        return carrier != null;
    }
}
