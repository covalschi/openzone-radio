// Сторінка «Рація» на клієнті.
//
// Малює те, що сказав сервер, і нічого не вирішує сама.
//
// НАЛАШТУВАННЯ ЧИСЛОМ, а не вибором зі списку. Іменовані канали були потрібні,
// поки частот було вісім; після зняття межі ефір міряється тисячами ділень, і
// список імен, яких ніхто не давав, показував би те, чого немає.
//
// Різниця з ручною рацією лишається одна, і вона ігрова: щоб перебудувати цю,
// треба ВІДКРИТИ КПК. На бігу не вийде.

class OZR_PageRadio : OZ_PdaPage
{
    private EditBoxWidget m_Freq;
    private ButtonWidget  m_BtnTune;
    private ButtonWidget  m_BtnUp;
    private ButtonWidget  m_BtnDown;

    private ref OZR_RadioState m_State;

    override string LayoutPath()
    {
        return "OpenZone_Radio_PDA/gui/layouts/ozr_page_radio.layout";
    }

    override void OnBuilt()
    {
        m_Freq    = EditBoxWidget.Cast(Wgt("FreqEdit"));
        m_BtnTune = ButtonWidget.Cast(Wgt("BtnTune"));
        m_BtnUp   = ButtonWidget.Cast(Wgt("BtnUp"));
        m_BtnDown = ButtonWidget.Cast(Wgt("BtnDown"));

        SetText("BtnTuneText", "#STR_OZR_TUNE");
        SetText("BtnUpText",   "+");
        SetText("BtnDownText", "-");
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

        if (op == "tune")
        {
            if (ok)
                Ask();
            else
                Hint(error);
        }
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
    }

    private void Hint(string key)
    {
        if (key != "")
            SetText("StatusText", "#" + key);
    }
}
