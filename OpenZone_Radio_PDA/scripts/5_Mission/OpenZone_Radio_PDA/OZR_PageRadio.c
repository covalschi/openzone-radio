// Сторінка «Рація» на клієнті.
//
// Малює те, що сказав сервер, і нічого не вирішує сама.
//
// НАЛАШТУВАННЯ ЧИСЛОМ, а не вибором зі списку. Іменовані канали були потрібні,
// поки частот було вісім; після зняття межі ефір міряється тисячами ділень, і
// список імен, яких ніхто не давав, показував би те, чого немає.
//
// Імена лишились, але тепер їх дає САМ ГРАВЕЦЬ, і живуть вони на носії в КПК.
// Книжку можна загубити з дискетою, зняти з тіла, переписати товаришу -- тобто
// ім'я частоти стало предметом, а не рядком у конфігу сервера.
//
// Різниця з ручною рацією лишається одна, і вона ігрова: щоб перебудувати цю,
// треба ВІДКРИТИ КПК. На бігу не вийде.

class OZR_PageRadio : OZ_PdaPage
{
    private EditBoxWidget     m_Freq;
    private EditBoxWidget     m_Name;
    private TextListboxWidget m_Book;
    private ButtonWidget      m_BtnTune;
    private ButtonWidget      m_BtnUp;
    private ButtonWidget      m_BtnDown;
    private ButtonWidget      m_BtnSave;
    private ButtonWidget      m_BtnForget;

    // Рядок, на який гравець НАЦІЛИВСЯ. Перемальовування списку скидає його,
    // бо після оновлення це вже інший рядок.
    private int m_Picked = -1;

    // Ділення, яке востаннє ВПИСАЛИ в поле вводу. Поле переписується лише
    // тоді, коли частота справді змінилась, і це не економія: Paint приходить
    // раз на секунду, і безумовний запис затирав би недонабране число --
    // а в найгіршому випадку між набором і натисканням TUNE, тобто настроїв
    // би на те, чого гравець не просив.
    private int m_FreqShown = -1;

    // Підпис носія, за яким малювали книжку востаннє. Поки він не змінився,
    // перепитувати книжку нема сенсу: вона змінитись не могла.
    private bool m_CarrierShown = false;
    private int  m_FreeShown    = -2;

    // Синхронізоване ділення, ЯКИМ МИ ЙОГО БАЧИЛИ МИНУЛОГО РАЗУ.
    //
    // Порівнювати клієнтське число з серверним НЕ МОЖНА, і саме на цьому
    // попередня спроба прибрати опитування провалилась: поки власна
    // синхрозмінна порожня, OZR_ShownIndex падає назад на рушій, а рушій на
    // клієнті відповідає за СВОЄЮ ванільною таблицею -- інше число, ніж у
    // сервера, і так назавжди. Розбіжність ставала вічною, і сторінка знову
    // питала сервер щосекунди, тільки вже з іншої причини.
    //
    // Питання тут інше: чи змінилось щось на предметі ВІДТОДІ, ЯК МИ МАЛЮВАЛИ.
    // На нього відповідає порівняння з самим собою.
    private int  m_SyncShown    = -2;

    private ref OZR_RadioState m_State;

    // Книжка живе ОКРЕМО від стану, бо окремо й приїжджає -- див. OZR_BookList.
    // Своя копія ще й переживає оновлення стану, тож вибраний рядок не стає
    // недійсним щоразу, коли сервер сказав про батарею.
    private ref OZR_BookList m_Rows;

    override string LayoutPath()
    {
        return "OpenZone_Radio_PDA/gui/layouts/ozr_page_radio.layout";
    }

    override void OnBuilt()
    {
        m_Freq      = EditBoxWidget.Cast(Wgt("FreqEdit"));
        m_Name      = EditBoxWidget.Cast(Wgt("NameEdit"));
        m_Book      = TextListboxWidget.Cast(Wgt("BookList"));
        m_BtnTune   = ButtonWidget.Cast(Wgt("BtnTune"));
        m_BtnUp     = ButtonWidget.Cast(Wgt("BtnUp"));
        m_BtnDown   = ButtonWidget.Cast(Wgt("BtnDown"));
        m_BtnSave   = ButtonWidget.Cast(Wgt("BtnSave"));
        m_BtnForget = ButtonWidget.Cast(Wgt("BtnForget"));

        SetText("BtnTuneText",   "#STR_OZR_TUNE");
        SetText("BtnUpText",     "+");
        SetText("BtnDownText",   "-");
        SetText("BtnSaveText",   "#STR_OZR_SAVE");
        SetText("BtnForgetText", "#STR_OZR_FORGET");
    }

    override void OnSelected()
    {
        // Зайшли наново -- і поле, і книжка заповнюються наново.
        m_FreqShown    = -1;
        m_FreeShown    = -2;
        m_SyncShown    = -2;
        m_CarrierShown = false;
        m_Rows         = null;
        Ask();
        AskBook();
    }

    // СЕРВЕР НЕ ОПИТУЄТЬСЯ ПРОСТО ТАК, і виправдань для цього не знайшлося
    // жодного -- перевірено по черзі:
    //
    //   «сяде батарея»  -- живлення КПК СИНХРОНІЗОВАНЕ на самому предметі
    //                      (m_IsOn, m_Charge01). Клієнт тримає його в руках
    //                      буквально; питати про це по мережі -- це запит і
    //                      відповідь заради числа, яке вже лежить поруч.
    //   «витягнуть плату» -- поки КПК відкритий, витягти її не можна взагалі.
    //   «зміниться смуга» -- смуга, крок і дальність з конфіга й за сесію не
    //                      міняються.
    //
    // Ділення плати теж синхронізоване (m_OZR_Index, читається через
    // OZR_ShownIndex). Отже все, що змінюється саме по собі, клієнт бачить
    // сам -- і опитування раз на секунду було чистою данню звичці.
    //
    // Лишається один випадок: намальоване РОЗІЙШЛОСЯ з тим, що синхронізовано
    // (плату перебудували не з цієї сторінки). Тоді -- і тільки тоді -- ми
    // питаємо. Книжку при цьому не чіпаємо: див. OZR_ListReq.
    override void OnRefresh()
    {
        if (Drifted())
            Ask();
    }

    // КПК, який зараз у руках, ОЧИМА КЛІЄНТА.
    private OZ_PDA_Base Held()
    {
        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p || !p.GetHumanInventory())
            return null;

        return OZ_PDA_Base.Cast(p.GetHumanInventory().GetEntityInHands());
    }

    private bool Drifted()
    {
        // Ще нічого не малювали -- питати треба.
        if (!m_State)
            return true;

        // КПК уже не в руках: меню закриється саме, і смикати сервер по дорозі
        // нема сенсу.
        OZ_PDA_Base pda = Held();
        if (!pda)
            return false;

        if (pda.OZ_IsOn() != m_State.Powered)
            return true;

        OZ_Module_Radio board = OZR_Set.BoardIn(pda);
        if ((board != null) != m_State.HasBoard)
            return true;

        int shown = -1;
        if (board)
            shown = board.OZR_ShownIndex();

        if (shown != m_SyncShown)
        {
            m_SyncShown = shown;
            return true;
        }

        return false;
    }

    // Книжку просимо навмисно: коли зайшли на сторінку, коли самі її змінили
    // (save/forget) і коли носій виявився іншим. Раз на секунду вона не
    // потрібна нікому -- див. OZR_ListReq.
    private void Ask()
    {
        OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "list", "{}");
    }

    // Книжка -- СВОЯ операція, а не поле стану. Просимо її навмисно: коли
    // зайшли на сторінку, коли самі її змінили і коли місця в пам'яті стало
    // інакше. Раз на секунду вона не потрібна нікому.
    private void AskBook()
    {
        OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "book", "{}");
    }

    override bool OnPageClick(Widget w, int x, int y)
    {
        if (w == m_BtnTune)
        {
            Tune(Typed());
            return true;
        }

        if (w == m_BtnUp)
        {
            Step(1);
            return true;
        }

        if (w == m_BtnDown)
        {
            Step(-1);
            return true;
        }

        if (w == m_BtnSave)
        {
            Remember();
            return true;
        }

        if (w == m_BtnForget)
        {
            Forget();
            return true;
        }

        return false;
    }

    override void OnResponse(string op, bool ok, string json, string error)
    {
        if (op == "list")
        {
            if (!ok)
            {
                Hint(error);
                return;
            }

            OZR_RadioState st;
            string err;
            if (JsonFileLoader<OZR_RadioState>.LoadData(json, st, err) && st)
            {
                m_State = st;
                Paint();
            }
            return;
        }

        if (op == "book")
        {
            if (!ok)
            {
                Hint(error);
                return;
            }

            OZR_BookList got;
            string berr;
            if (JsonFileLoader<OZR_BookList>.LoadData(json, got, berr) && got && got.Items)
            {
                m_Rows = got;
                PaintBook();
            }
            return;
        }

        if (op == "tune")
        {
            if (ok)
                Ask();
            else
                Hint(error);
            return;
        }

        // Ми самі щойно змінили книжку -- перечитуємо і стан, і її.
        if (op == "save" || op == "forget")
        {
            if (!ok)
            {
                Hint(error);
                return;
            }

            Ask();
            AskBook();
        }
    }

    // Вибір рядка книжки ОДРАЗУ настроює: книжка й існує для того, щоб не
    // набирати число руками. Друга дія («вибрав, тепер тисни») тут була б
    // роботою без приросту сенсу.
    override bool OnPageItemSelected(Widget w, int row)
    {
        if (!m_Book || w != m_Book)
            return false;

        m_Picked = row;
        if (!m_Rows || row < 0 || row >= m_Rows.Items.Count())
            return true;

        // Видиме виділення -- одразу, ще до того, як сервер щось відповість.
        Repaint();

        OZR_BookRow r = m_Rows.Items[row];

        // Прямо у віджет: SetText базової сторінки вміє лише TextWidget,
        // а поле вводу -- інший клас.
        if (m_Name)
            m_Name.SetText(r.Name);

        // Недосяжний рядок вибрати можна -- забути його або переписати ім'я
        // треба вміти, -- а настроїтись на нього не можна, і причина
        // називається одразу.
        if (!r.Reach)
        {
            Hint("STR_OZR_ERR_OUT_OF_REACH");
            return true;
        }

        Tune(r.Index);
        return true;
    }

    // ------------------------------------------------------------- дії

    // Що набрано в полі, у діленнях ефіру. -1, якщо набрано не число або
    // сервер ще не сказав, з чого рахувати.
    private int Typed()
    {
        if (!m_Freq)
            return -1;

        string text = m_Freq.GetText();
        if (text == "")
            return -1;

        float mhz = text.ToFloat();
        if (mhz <= 0)
            return -1;

        return IndexOf(mhz);
    }

    // МГц -> ділення ефіру, за тими двома числами, що приїхали в стані.
    private int IndexOf(float mhz)
    {
        if (!m_State || m_State.EtherStep <= 0)
            return -1;

        return Math.Round((mhz - m_State.EtherBase) / m_State.EtherStep);
    }

    // Ділення -> МГц, тим самим джерелом.
    private float MHzAt(int index)
    {
        if (!m_State || m_State.EtherStep <= 0)
            return 0;

        return m_State.EtherBase + index * m_State.EtherStep;
    }

    private void Tune(int index)
    {
        if (index < 0)
        {
            Hint("#STR_OZR_ERR_OUT_OF_BAND");
            return;
        }

        // Межі ЦІЄЇ ПЛАТИ, а не ефіру, і саме про це сторінка мовчала раніше.
        // Ефір широкий -- 86..152 МГц, -- а плата вузька, і гравець на екрані
        // бачить саме її смугу. Відмова мусить називати ту межу, яка написана
        // над кнопкою, інакше «поза смугою» звучить як брехня.
        //
        // Допуск -- пів ділення: 138.000 із поля й 86.000 + 4160 * 0.0125
        // це те саме число лише в математиці, а у float -- ні.
        if (m_State && m_State.EtherStep > 0 && m_State.MaxMHz > 0)
        {
            float mhz  = MHzAt(index);
            float slack = m_State.EtherStep * 0.5;
            if (mhz < m_State.MinMHz - slack || mhz > m_State.MaxMHz + slack)
            {
                Hint("#STR_OZR_ERR_OUT_OF_BAND");
                return;
            }
        }

        OZR_TuneRef r = new OZR_TuneRef();
        r.Index = index;

        string json;
        string err;
        if (JsonFileLoader<OZR_TuneRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "tune", json);
    }

    // Крок по ґратці ЦІЄЇ плати, а не по діленню сітки: між її каналами
    // стояти нема сенсу.
    private void Step(int dir)
    {
        if (!m_State || m_State.EtherStep <= 0)
            return;

        int stride = Math.Round(m_State.StepMHz / m_State.EtherStep);
        if (stride < 1)
            stride = 1;

        // Крокуємо від того, що НАБРАНО, якщо набрано щось осмислене: інакше
        // «набрав 140.5, тисну +» відкидало б назад до тієї частоти, на якій
        // стоїмо, і кнопка виглядала б зламаною вдруге.
        int from = Typed();
        if (from < 0)
            from = m_State.Index;
        if (from < 0)
            from = IndexOf(m_State.MinMHz);
        if (from < 0)
            return;

        Tune(from + dir * stride);
    }

    // ---------------------------------------------------------- малювання

    private void Paint()
    {
        if (!m_State)
            return;

        if (!m_State.HasBoard)
        {
            SetText("StatusText", "#STR_OZR_ERR_NO_BOARD");
            SetText("FreqText",   "---");
            SetText("BandText",   "");
            return;
        }

        if (!m_State.Powered)
        {
            SetText("StatusText", "#STR_OZ_ERR_OFF");
            SetText("FreqText",   "---");
            SetText("BandText",   "");
            return;
        }

        if (m_State.Live)
            SetText("StatusText", "#STR_OZR_ON_AIR");
        else
            SetText("StatusText", "#STR_OZR_READY");

        SetText("FreqText", OZR_Fmt.MHz(m_State.FreqMHz) + " MHz");

        // Поле вводу починає з тієї частоти, на якій стоїмо. Порожнє поле
        // означало «набери все наново», хоч потрібна зазвичай сусідня
        // частота; та й «+/-» від порожнього поля відштовхуватись нема від
        // чого. Пишемо ЛИШЕ на зміну частоти -- див. m_FreqShown.
        if (m_Freq && m_State.Index != m_FreqShown)
        {
            m_FreqShown = m_State.Index;
            m_Freq.SetText(OZR_Fmt.MHz(m_State.FreqMHz));
        }

        string band = OZR_Fmt.MHz(m_State.MinMHz) + " - " + OZR_Fmt.MHz(m_State.MaxMHz);
        band += " / " + OZR_Fmt.Step(m_State.StepMHz);
        band += "   " + Math.Round(m_State.RangeM).ToString() + " m";
        SetText("BandText", band);

        PaintBook();
    }

    // Носій той самий, що й був? Порівнюємо саме те, що сервер шле завжди.
    private bool CarrierChanged()
    {
        return m_State.HasMemory != m_CarrierShown || m_State.FreeCells != m_FreeShown;
    }

    // ------------------------------------------------------------ книжка

    private void Remember()
    {
        if (!m_Name)
            return;

        OZR_BookRef r = new OZR_BookRef();
        r.Name = m_Name.GetText();

        string json;
        string err;
        if (JsonFileLoader<OZR_BookRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "save", json);
    }

    private void Forget()
    {
        if (!m_Rows || m_Picked < 0 || m_Picked >= m_Rows.Items.Count())
        {
            Hint("STR_OZR_ERR_PICK_ONE");
            return;
        }

        OZR_BookRef r = new OZR_BookRef();
        r.Name = m_Rows.Items[m_Picked].Name;

        string json;
        string err;
        if (JsonFileLoader<OZR_BookRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "forget", json);
    }

    private void PaintBook()
    {
        if (!m_Book || !m_State)
            return;

        // Книжка ще не приїжджала -- малювати нема чого, і чіпати намальоване
        // НЕ МОЖНА. Саме це тримає працездатність FORGET: інакше список
        // перебудовувався б на кожному оновленні стану й забирав із собою
        // вибір гравця раніше, ніж той устигав натиснути кнопку.
        if (!m_Rows)
        {
            AskBook();
            return;
        }

        // Місця в пам'яті стало інакше -- отже, книжка змінилась не з цієї
        // сторінки (мітка, нотатка, інший прилад). Перечитуємо.
        if (CarrierChanged())
        {
            m_CarrierShown = m_State.HasMemory;
            m_FreeShown    = m_State.FreeCells;
            AskBook();
        }

        m_Book.ClearItems();
        m_Picked = -1;

        if (!m_State.HasMemory)
        {
            SetText("BookFree", "#STR_OZR_NO_CARRIER");
            return;
        }

        for (int i = 0; i < m_Rows.Items.Count(); i++)
            m_Book.AddItem(RowText(i), NULL, 0);

        Shade();

        if (m_State.FreeCells >= 0)
            SetText("BookFree", m_State.FreeCells.ToString() + " free");
        else
            SetText("BookFree", "");
    }

    // Один рядок списку, в одному місці. Маркер попереду -- це і є видиме
    // виділення: власна підсвітка віджета губиться під кольором, яким ми
    // гасимо недосяжні рядки, а вибір мусить бути видно завжди й однозначно.
    private string RowText(int i)
    {
        OZR_BookRow e = m_Rows.Items[i];

        string line = " ";
        if (i == m_Picked)
            line = ">";

        line += " " + e.Name + "   " + OZR_Fmt.MHz(e.MHz);
        if (!e.Reach)
            line += "   " + "#STR_OZR_OUT_OF_REACH";

        return line;
    }

    // Недосяжне ГАСНЕ. Не замість підпису, а разом із ним: колір сам по собі
    // нічого не пояснює, а підпис сам по собі губиться в рівному списку.
    private void Shade()
    {
        if (!m_Book || !m_Rows)
            return;

        for (int i = 0; i < m_Rows.Items.Count(); i++)
        {
            if (!m_Rows.Items[i].Reach)
                m_Book.SetItemColor(i, 0, ARGB(255, 115, 115, 125));
        }
    }

    // Перемалювати ЛИШЕ підписи, не чіпаючи ані складу списку, ані прокрутки.
    // Перебудова списку заради маркера скидала б і те, і те.
    private void Repaint()
    {
        if (!m_Book || !m_Rows)
            return;

        for (int i = 0; i < m_Rows.Items.Count(); i++)
            m_Book.SetItem(i, RowText(i), NULL, 0);

        Shade();
    }

    private void Hint(string key)
    {
        if (key != "")
            SetText("StatusText", "#" + key);
    }
}
