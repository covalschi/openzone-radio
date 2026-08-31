// Сторінка «Рація» на клієнті.
//
// Малює те, що сказав сервер, і нічого не вирішує сама. Заборонений канал
// показано ЗАВЖДИ -- сірим і з поміткою: сховати чужу смугу означало б
// приховати, що вона взагалі є, а знати про існування «Борга» в ефірі й не
// мати туди доступу -- це і є гра.

class OZR_PageRadio : OZ_PdaPage
{
    private TextListboxWidget m_List;
    private ButtonWidget m_BtnTune;

    private ref OZR_RadioState m_State;

    // Рядок, на який гравець НАЦІЛИВСЯ, а не той, на якому стоїть рація.
    private int m_Picked = -1;

    override string LayoutPath()
    {
        return "OpenZone_Radio/gui/layouts/ozr_page_radio.layout";
    }

    override void OnBuilt()
    {
        m_List    = TextListboxWidget.Cast(Wgt("ChannelList"));
        m_BtnTune = ButtonWidget.Cast(Wgt("BtnTune"));

        SetText("BtnTuneText", "#STR_OZR_TUNE");
    }

    override void OnSelected()
    {
        Ask();
    }

    override void OnRefresh()
    {
        // Стан ефіру міняється не лише від наших натискань: сіла батарея,
        // витягли антену, хтось інший вийшов на зв'язок. Раз на секунду.
        Ask();
    }

    private void Ask()
    {
        OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "list", "{}");
    }

    override bool OnPageItemSelected(Widget w, int row)
    {
        if (!m_List || w != m_List)
            return false;

        m_Picked = row;
        Paint();
        return true;
    }

    override bool OnPageClick(Widget w, int x, int y)
    {
        if (!w || w != m_BtnTune)
            return false;

        if (!m_State || m_Picked < 0 || m_Picked >= m_State.Items.Count())
        {
            SetText("RadioHint", "#STR_OZR_PICK_FIRST");
            return true;
        }

        OZR_TuneRef r = new OZR_TuneRef();
        r.Id = m_State.Items[m_Picked].Id;

        string json;
        string err;
        if (JsonFileLoader<OZR_TuneRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "tune", json);

        return true;
    }

    override void OnResponse(string op, bool ok, string json, string error)
    {
        if (!ok)
        {
            SetText("RadioHint", "#" + error);
            return;
        }

        if (op == "list")
        {
            string lerr;
            OZR_RadioState st;
            if (!JsonFileLoader<OZR_RadioState>.LoadData(json, st, lerr))
            {
                OZ_Log.Error("radio state unreadable: " + lerr);
                return;
            }
            m_State = st;
            Paint();
            return;
        }

        if (op == "tune")
        {
            SetText("RadioHint", "");
            Ask();
            return;
        }

        // "ptt" відповіді не малює: кнопку тримають, а не натискають, і
        // писати щось на кожне натиснення означало б миготіти.
    }

    private void Paint()
    {
        if (!m_List)
            return;

        // Виділення переживає перемальовку: список перечитується щосекунди, і
        // без цього приціл гравця збивався б рівно тоді, коли він у нього
        // цілиться.
        int keep = m_Picked;

        m_List.ClearItems();

        int n = 0;
        if (m_State && m_State.Items)
            n = m_State.Items.Count();

        for (int i = 0; i < n; i++)
        {
            OZR_ChannelInfo c = m_State.Items[i];

            string row = c.Name;
            if (c.Current)
                row = "> " + row;
            if (!c.Allowed)
                row += "  " + Widget.TranslateString("#STR_OZR_LOCKED");

            m_List.AddItem(row, NULL, 0);
        }

        if (keep >= 0 && keep < n)
            m_List.SelectRow(keep);

        PaintState();
    }

    private void PaintState()
    {
        if (!m_State)
            return;

        OZR_ChannelInfo here = null;
        for (int i = 0; i < m_State.Items.Count(); i++)
        {
            if (m_State.Items[i].Current)
                here = m_State.Items[i];
        }

        if (here)
        {
            SetText("RadioTitle", here.Name);

            // Частоту показуємо числом: власник ванільної рації інакше не
            // зможе зійтись із власником КПК на одній смузі.
            string band = Widget.TranslateString("#STR_OZR_BAND");
            band += "  " + here.Freq.ToString();
            SetText("RadioBand", band);
        }
        else
        {
            SetText("RadioTitle", "#STR_OZR_NOT_TUNED");
            SetText("RadioBand", "");
        }

        string power = Widget.TranslateString("#STR_OZR_POWER");
        if (m_State.Powered)
            power += "  " + Widget.TranslateString("#STR_OZR_ON");
        else
            power += "  " + Widget.TranslateString("#STR_OZR_OFF");
        SetText("RadioPower", power);

        string ant = Widget.TranslateString("#STR_OZR_ANTENNA");
        if (m_State.RangeM > 0)
            ant += "  " + m_State.RangeM.ToString() + " m";
        else
            ant += "  " + Widget.TranslateString("#STR_OZR_NONE");
        SetText("RadioAntenna", ant);

        if (m_State.Live)
            SetText("RadioLive", "#STR_OZR_ON_AIR");
        else
            SetText("RadioLive", "");

        if (!m_State.HasBoard)
            SetText("RadioHint", "#STR_OZR_ERR_NO_BOARD");
        else if (m_State.RangeM <= 0)
            SetText("RadioHint", "#STR_OZR_ERR_NO_ANTENNA");
    }
}
