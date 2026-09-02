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

        if (op == "book")
            return Book(sender, ok, error);

        if (op == "tune")
            return Tune(json, sender, ok, error);

        if (op == "save")
            return Save(json, sender, ok, error);

        if (op == "forget")
            return Forget(json, sender, ok, error);

        // Носій: чотири дієслова роду oz_radio_frequencies (ТЗ-5 R-E5.4),
        // ТУТ, а не в ядрі (R-E5.5): КПК про частоти не знає нічого.
        if (op == "chip_read")
            return ChipRead(sender, ok, error);
        if (op == "chip_write")
            return ChipWrite(sender, ok, error);
        if (op == "chip_import")
            return ChipImport(sender, ok, error);
        if (op == "chip_take")
            return ChipTake(json, sender, ok, error);

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

        // Ефір -- поза платою: він один на сервер, і сторінці він потрібен
        // навіть тоді, коли плати немає (щоб розібрати набране число й
        // сказати про це щось осмислене).
        if (OZR_Grid.Ready())
        {
            st.EtherBase = OZR_Grid.MHzAt(0);
            st.EtherStep = OZR_Grid.StepMHz();
        }

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

        // Місце в пам'яті приладу -- ЗАВЖДИ: за цим числом клієнт розуміє,
        // що книжку час перечитати. Самих рядків тут немає, бо книжка їде
        // своєю операцією: вона міняється рідко, а стан перепитують часто.
        // Три відповіді про пам'ять (ТЗ-5 R-E5.2): прилад не зберігає записів
        // узагалі (Limits.Memory == 0), пам'ять заповнена, місце є. Вільних
        // -- саме ВІЛЬНИХ, без тих, що книжка вже тримає: OZ_RoomFor рахує
        // «вільні плюс свої», і лічильник показував місце, якого нема (D102).
        st.StoresRecords = pda.OZ_Max() > 0;
        st.FreeCells     = pda.OZ_Free();

        return Body(st, ok, error);
    }

    // Книжка -- ОКРЕМИЙ документ і окрема операція: міняється вона рідко, а
    // стан сторінки перепитують часто, і возити її щоразу означало б щоразу
    // перемальовувати список і скидати вибраний рядок.
    private string Book(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        OZR_FreqBook book = new OZR_FreqBook();
        Read(pda, book);

        OZR_BookList list = new OZR_BookList();
        Fill(list, book, OZR_Set.BoardIn(pda));
        return Rows(list, ok, error);
    }

    // Рядки списку з книжки -- приладу чи чипа, однаково. Доступність
    // рахуємо проти профілю тієї плати, яка справді стоїть: частота,
    // записана з п'ятикілометрової, у смугу двохсотметрової не влазить, і
    // сказати про це треба до натискання.
    private void Fill(OZR_BookList list, OZR_FreqBook book, OZ_Module_Radio board)
    {
        int lo;
        int hi;
        int stride;
        bool window = false;

        OZR_RadioProfile bp;
        if (board)
            bp = OZR_Profiles.For(board.GetType());
        if (bp)
            window = OZR_Grid.Window(bp, lo, hi, stride);

        // Ready() -- ОДИН РАЗ, поза циклом (D14): усередині це прохід по всій
        // таблиці рушія, близько 10 500 переходів на кожен рядок книжки.
        bool gridOk = OZR_Grid.Ready();

        // РІШЕННЯ ПРО ДОСЯЖНІСТЬ РАХУЄТЬСЯ В ЛОКАЛЬНУ ЗМІННУ, А НЕ ПРЯМО В
        // ПОЛЕ. Це не стиль -- це обхід рушійної вади, знайденої діленням
        // навпіл 2026-09-01: ланцюжок «&&» із трьох членів, присвоєний
        // безпосередньо в поле об'єкта на купі, псує купу, і сервер помирає
        // на першій-ліпшій наступній роботі з нею (0xC0000005 у ntdll,
        // STATUS_HEAP_CORRUPTION у RtlFreeHeap -- місце щоразу інше).
        // Правило одне: складену логіку рахуємо в локальну змінну й у поле
        // кладемо вже готове значення.
        for (int b = 0; b < book.Items.Count(); b++)
        {
            OZR_FreqEntry e = book.Items[b];

            OZR_BookRow row = new OZR_BookRow();
            row.Name  = e.Name;
            row.Index = e.Index;
            if (gridOk)
                row.MHz = OZR_Grid.MHzAt(e.Index);

            bool reach = false;
            if (window)
            {
                if (e.Index >= lo && e.Index <= hi)
                    reach = true;
            }
            if (reach && stride > 0)
                reach = ((e.Index - lo) % stride) == 0;

            row.Reach = reach;

            list.Items.Insert(row);
        }
    }

    private string Rows(OZR_BookList list, out bool ok, out string error)
    {
        string body;
        string err;
        if (!JsonFileLoader<OZR_BookList>.MakeData(list, body, err, false))
        {
            OZR_Log.Error("radio page: cannot serialise the book: " + err);
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return body;
    }

    // Один вихід на всі шляхи List().
    private string Body(OZR_RadioState st, out bool ok, out string error)
    {
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
    // Книжка НЕ ЧЕРЕЗ JSON -- але НЕ ТОМУ, ЩО JSON ВИНЕН.
    //
    // 2026-08-31 здавалось, що винен: сервер помирав нативно наче просто в
    // JsonFileLoader.LoadData, на цілком коректних 37 байтах, і тип при цьому
    // структурно тотожний OZ_MarkerList, який розбирається роками. Насправді
    // LoadData був не причиною, а ЖЕРТВОЮ -- першою роботою з купою після
    // того, як її вже зіпсували (справжня причина -- нижче, над Read).
    // Ті самі падіння лягали то в Print() логера, то в MakeData, то в
    // ErrorModuleHandler -- усе це різні жертви одного псування.
    //
    // Власний формат лишається, бо він тут просто кращий: пара «ділення,
    // ім'я» коротша за JSON і не потребує екранування. Це вибір, а не обхід.
    // Запис
    // -- це пара «ділення, ім'я», і рядок на запис читається однозначно без
    // жодного екранування: ділення -- число до першого пропуску, решта рядка
    // -- ім'я. Іменам переводу рядка не буває (їх ріже NAME_MAX і сама форма
    // вводу), тож роздільник безпечний.
    private static const string BOOK_SEP = "\n";

    // КНИЖКУ СТВОРЮЄ ТОЙ, ХТО КЛИЧЕ, і це не стиль, а вимога рушія.
    //
    // Enforce не має збирача сміття: час життя -- це лічильник посилань. Річ,
    // створена всередині функції й віддана назовні НЕ-ref типом повернення, у
    // мить повернення не має власника взагалі, тому кадр забирає її з собою --
    // і той, хто кликав, тримає покажчик на звільнену пам'ять. Пам'ять при
    // цьому не затирається, тож перші читання ще дають правдоподібні числа, і
    // помилка виглядає як робоча.
    //
    // Саме тут вона й ставала фатальною. Book/Save/Forget не просто читали
    // мертву книжку -- вони ПИСАЛИ в неї (Items.Insert, Items.Set,
    // Items.Remove) і поруч виділяли нові OZR_BookRow, які лягали рівно в ті
    // самі звільнені блоки. Звідси й 0xC0000005 у купі ntdll, і той факт, що
    // падало не там, де псувалось, а на першій же наступній роботі з купою --
    // здебільшого в Print() логера або в MakeData.
    //
    // Тому книжку заводить викликач: поки живий ЙОГО кадр, живе й вона.
    private void Read(OZ_PDA_Base pda, OZR_FreqBook book)
    {
        if (!book || !pda)
            return;
        Parse(pda.OZ_KindRead(OZRP_Const.KIND_FREQS), book);
    }

    // Той самий формат і на чипі: рід один, читач один.
    private void Parse(string payload, OZR_FreqBook book)
    {
        if (!book || payload == "")
            return;

        array<string> lines = new array<string>();
        payload.Split(BOOK_SEP, lines);

        for (int i = 0; i < lines.Count(); i++)
        {
            string line = lines[i];
            if (line == "")
                continue;

            int gap = line.IndexOf(" ");
            if (gap <= 0)
                continue;

            OZR_FreqEntry e = new OZR_FreqEntry();
            e.Index = line.Substring(0, gap).ToInt();
            e.Name  = line.Substring(gap + 1, line.Length() - gap - 1);
            book.Items.Insert(e);
        }
    }

    private string Payload(OZR_FreqBook book)
    {
        string payload = "";
        for (int i = 0; i < book.Items.Count(); i++)
        {
            OZR_FreqEntry e = book.Items[i];
            if (i > 0)
                payload += BOOK_SEP;
            payload += e.Index.ToString() + " " + e.Name;
        }
        return payload;
    }

    private bool Store(OZ_PDA_Base pda, OZR_FreqBook book, out string error)
    {
        string payload = Payload(book);

        // ОДНА ЯЧЕЙКА НА ЧАСТОТУ, і пише це в ПАМ'ЯТЬ ПРИЛАДУ, а не на носій.
        // Прилад сам відмовить, якщо ячеек не лишилось.
        if (!pda.OZ_KindWrite(OZRP_Const.KIND_FREQS, payload, book.Items.Count()))
        {
            error = "STR_OZ_ERR_PDA_FULL";
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

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        string name = r.Name;
        if (name == "")
        {
            error = "STR_OZR_ERR_NO_NAME";
            return "";
        }
        if (name.Length() > OZRP_Const.NAME_MAX)
            name = name.Substring(0, OZRP_Const.NAME_MAX);

        OZR_FreqBook book = new OZR_FreqBook();
        Read(pda, book);
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

        if (!Store(pda, book, error))
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

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        OZR_FreqBook book = new OZR_FreqBook();
        Read(pda, book);
        for (int i = book.Items.Count() - 1; i >= 0; i--)
        {
            if (book.Items[i].Name == r.Name)
                book.Items.Remove(i);
        }

        if (!Store(pda, book, error))
            return "";

        ok = true;
        error = "";
        return "";
    }

    // ------------------------------------------------------------- носій
    //
    // Дім книжки -- ПРИЛАД (ТЗ-5 R-E5.1); чип -- спосіб її винести. Чотири
    // дієслова ті самі, що в міток і записок (R-E5.4), з тими самими правилами
    // (R-E5.6): забрати -- значить скопіювати, чип не порожніє; читається й
    // замкнений на запис чип, запис потребує Writable; місткість чипа
    // рахується за родами, перезапис свого роду не рахується двічі.

    // Книжка чипа -- тим самим списком, що й книжка приладу.
    private string ChipRead(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        OZR_FreqBook book = new OZR_FreqBook();
        Parse(c.OZ_Read(OZRP_Const.KIND_FREQS), book);
        if (book.Items.Count() == 0)
        {
            error = "STR_OZR_ERR_CHIP_EMPTY";
            return "";
        }

        OZ_Module_Radio board = null;
        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (pda)
            board = OZR_Set.BoardIn(pda);

        OZR_BookList list = new OZR_BookList();
        Fill(list, book, board);
        return Rows(list, ok, error);
    }

    // Книжку приладу -- на чип. Скільки влізе; решта лишається в приладі,
    // і звіт це каже.
    private string ChipWrite(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_DataCarrier_Base c = OZ_CarrierOps.ResolveWritable(sender, error);
        if (!c)
            return "";

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        OZR_FreqBook book = new OZR_FreqBook();
        Read(pda, book);
        int total = book.Items.Count();
        if (total == 0)
        {
            error = "STR_OZR_ERR_BOOK_EMPTY";
            return "";
        }

        int room = c.OZ_RoomFor(OZRP_Const.KIND_FREQS);
        if (room <= 0)
        {
            error = "STR_OZ_ERR_CARRIER_FULL";
            return "";
        }
        if (total > room)
            book.Items.Resize(room);

        if (!c.OZ_Write(OZRP_Const.KIND_FREQS, Payload(book), book.Items.Count()))
        {
            error = "STR_OZ_ERR_CARRIER_FULL";
            return "";
        }

        return Report(book.Items.Count(), total, ok, error);
    }

    // Усе з чипа -- у прилад. Капсула (заморожений прилад) не приймає нічого
    // нового -- те саме правило, що в міток і записок.
    private string ChipImport(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }
        if (OZ_PdaCapsule.IsFrozen(pda))
        {
            error = "STR_OZ_ERR_FROZEN";
            return "";
        }

        OZR_FreqBook chip = new OZR_FreqBook();
        Parse(c.OZ_Read(OZRP_Const.KIND_FREQS), chip);
        if (chip.Items.Count() == 0)
        {
            error = "STR_OZR_ERR_CHIP_EMPTY";
            return "";
        }

        return Merge(pda, chip, ok, error);
    }

    // Одну частоту з чипа -- у прилад. Індекс -- рядок списку чипа, який
    // клієнт щойно отримав тим самим chip_read.
    private string ChipTake(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZR_TuneRef r;
        string err;
        if (!JsonFileLoader<OZR_TuneRef>.LoadData(json, r, err) || !r || r.Index < 0)
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_DataCarrier_Base c = OZ_CarrierOps.Resolve(sender, error);
        if (!c)
            return "";

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }
        if (OZ_PdaCapsule.IsFrozen(pda))
        {
            error = "STR_OZ_ERR_FROZEN";
            return "";
        }

        OZR_FreqBook chip = new OZR_FreqBook();
        Parse(c.OZ_Read(OZRP_Const.KIND_FREQS), chip);
        if (r.Index >= chip.Items.Count())
        {
            error = "STR_OZR_ERR_PICK_ONE";
            return "";
        }

        OZR_FreqBook one = new OZR_FreqBook();
        one.Items.Insert(chip.Items[r.Index]);
        return Merge(pda, one, ok, error);
    }

    // Злиття в книжку приладу: те саме ім'я -- переписати (книжка -- не
    // журнал), нове -- дописати, поки є місце. Що не влізло, лишилось на чипі.
    private string Merge(OZ_PDA_Base pda, OZR_FreqBook incoming, out bool ok, out string error)
    {
        ok = false;

        OZR_FreqBook book = new OZR_FreqBook();
        Read(pda, book);

        int room  = pda.OZ_RoomFor(OZRP_Const.KIND_FREQS);
        int taken = 0;

        for (int i = 0; i < incoming.Items.Count(); i++)
        {
            OZR_FreqEntry e = incoming.Items[i];
            if (!e || e.Name == "")
                continue;

            int at = -1;
            for (int j = 0; j < book.Items.Count(); j++)
            {
                if (book.Items[j].Name == e.Name)
                {
                    at = j;
                    break;
                }
            }

            OZR_FreqEntry copy = new OZR_FreqEntry();
            copy.Name  = e.Name;
            copy.Index = e.Index;

            if (at >= 0)
            {
                book.Items.Set(at, copy);
                taken++;
                continue;
            }

            if (book.Items.Count() >= room)
                continue;

            book.Items.Insert(copy);
            taken++;
        }

        if (taken == 0)
        {
            error = "STR_OZ_ERR_PDA_FULL";
            return "";
        }

        if (!Store(pda, book, error))
            return "";

        return Report(taken, incoming.Items.Count(), ok, error);
    }

    private string Report(int taken, int total, out bool ok, out string error)
    {
        OZR_ChipReport rep = new OZR_ChipReport();
        rep.Taken = taken;
        rep.Total = total;

        string body;
        string err;
        if (!JsonFileLoader<OZR_ChipReport>.MakeData(rep, body, err, false))
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return body;
    }
}
