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

    private ref OZR_RadioState m_State;

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
        Ask();
    }

    override void OnRefresh()
    {
        // Стан міняється не лише від наших натискань: сіла батарея, витягли
        // плату, хтось вимкнув КПК. Раз на секунду.
        Ask();
    }

    private void Ask()
    {
        OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "list", "{}");
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

        if (op == "tune" || op == "save" || op == "forget")
        {
            if (ok)
                Ask();
            else
                Hint(error);
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
        if (!m_State || row < 0 || row >= m_State.Book.Count())
            return true;

        OZR_BookRow r = m_State.Book[row];

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

    // Що набрано в полі, у діленнях сітки. -1, якщо набрано не число або
    // сітки ще немає.
    private int Typed()
    {
        if (!m_Freq || !OZR_ClientGrid.Ready())
            return -1;

        string text = m_Freq.GetText();
        if (text == "")
            return -1;

        float mhz = text.ToFloat();
        if (mhz <= 0)
            return -1;

        return OZR_ClientGrid.IndexOf(mhz);
    }

    private void Tune(int index)
    {
        if (index < 0)
        {
            Hint("#STR_OZR_ERR_OUT_OF_BAND");
            return;
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
        if (!m_State || !OZR_ClientGrid.Ready())
            return;

        float gs = OZR_ClientGrid.StepMHz();
        if (gs <= 0)
            return;

        int stride = Math.Round(m_State.StepMHz / gs);
        if (stride < 1)
            stride = 1;

        int from = m_State.Index;
        if (from < 0)
            from = OZR_ClientGrid.IndexOf(m_State.MinMHz);

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

        string band = OZR_Fmt.MHz(m_State.MinMHz) + " - " + OZR_Fmt.MHz(m_State.MaxMHz);
        band += " / " + OZR_Fmt.Step(m_State.StepMHz);
        band += "   " + Math.Round(m_State.RangeM).ToString() + " m";
        SetText("BandText", band);

        PaintBook();
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
        if (!m_State || m_Picked < 0 || m_Picked >= m_State.Book.Count())
        {
            Hint("STR_OZR_ERR_PICK_ONE");
            return;
        }

        OZR_BookRef r = new OZR_BookRef();
        r.Name = m_State.Book[m_Picked].Name;

        string json;
        string err;
        if (JsonFileLoader<OZR_BookRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "forget", json);
    }

    private void PaintBook()
    {
        if (!m_Book || !m_State)
            return;

        m_Book.ClearItems();
        m_Picked = -1;

        if (!m_State.HasCarrier)
        {
            SetText("BookFree", "#STR_OZR_NO_CARRIER");
            return;
        }

        for (int i = 0; i < m_State.Book.Count(); i++)
        {
            OZR_BookRow e = m_State.Book[i];

            // Частоту рахує сервер: у книжці лежить позиція, а не число, і
            // клієнт міг ще не отримати сітку -- список має читатись однаково.
            string line = e.Name + "   " + OZR_Fmt.MHz(e.MHz);
            if (!e.Reach)
                line += "   " + "#STR_OZR_OUT_OF_REACH";

            m_Book.AddItem(line, NULL, 0);

            // Недосяжне ще й ГАСНЕ. Не замість підпису, а разом із ним: колір
            // сам по собі нічого не пояснює, а підпис сам по собі губиться в
            // рівному списку.
            if (!e.Reach)
                m_Book.SetItemColor(i, 0, ARGB(255, 115, 115, 125));
        }

        if (m_State.FreeSlots >= 0)
            SetText("BookFree", m_State.FreeSlots.ToString() + " free");
        else
            SetText("BookFree", "");
    }

    private void Hint(string key)
    {
        if (key != "")
            SetText("StatusText", "#" + key);
    }
}
