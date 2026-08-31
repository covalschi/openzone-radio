// Цифрова клавіатура частот -- і чому вона не заважає бігти.
//
// Меню в DayZ забирає ввід, і гравець стає стовпом. Але ваніль уміє інакше:
// колесо жестів відкрите, а персонаж біжить. Різниця в одному -- воно
// перевизначає UseMouse() і НЕ ЧІПАЄ клавіатуру. Миша йде меню, клавіатура
// лишається грі, тому WASD живі.
//
// Звідси й форма клавіатури: цифри КЛІКАЮТЬСЯ. Забери меню клавіатуру заради
// набору з цифрового ряду -- і бігти стане нічим. Це не компроміс, а єдина
// форма, в якій обидві вимоги власника уживаються разом.

class OZR_FreqMenu extends UIScriptedMenu
{
    private TextWidget m_Title;
    private TextWidget m_Freq;
    private TextWidget m_Band;
    private TextWidget m_Hint;
    private Widget     m_Card;

    // Набране гравцем, як рядок: «145.1» -- це стан набору, а не число.
    // Числом воно стає лише в мить підтвердження.
    private string m_Typed = "";

    private TransmitterBase m_Radio;
    private ref OZR_RadioProfile m_Profile;

    override Widget Init()
    {
        layoutRoot = GetGame().GetWorkspace().CreateWidgets("OpenZone_Radio/gui/layouts/ozr_freq.layout");

        if (!layoutRoot)
            OZR_Log.Error("freq keypad: the layout produced no widgets");
        else
            OZR_Log.Dbg("freq keypad: layout built");

        m_Card  = layoutRoot;
        m_Title = TextWidget.Cast(layoutRoot.FindAnyWidget("TitleText"));
        m_Freq  = TextWidget.Cast(layoutRoot.FindAnyWidget("FreqText"));
        m_Band  = TextWidget.Cast(layoutRoot.FindAnyWidget("BandText"));
        m_Hint  = TextWidget.Cast(layoutRoot.FindAnyWidget("HintText"));

        return layoutRoot;
    }

    // Миша -- нам. Клавіатура НЕ згадується навмисне: саме її мовчання й
    // лишає гравцеві біг.
    override bool UseMouse()
    {
        return true;
    }

    override void OnShow()
    {
        super.OnShow();

        OZR_Log.Dbg("freq keypad: shown");
        GetGame().GetUIManager().ShowUICursor(true);
        Centre();
        Grab();
        Paint();
    }

    override void OnHide()
    {
        super.OnHide();
        GetGame().GetUIManager().ShowUICursor(false);

        // Кажемо опитувачу самі: FindMenu(MENU_FREQ) це меню не бачить, тож
        // питати менеджера, чи ми ще відкриті, марно.
        OZR_FreqInput.Forget();
    }

    // Картку центрує скрипт, а не розкладка: вирівнювання по екрану залежить
    // від роздільної здатності, і скрипт її знає, а розкладка ні.
    private void Centre()
    {
        if (!m_Card)
            return;

        // Два різні GetScreenSize, і плутати їх не варто: у Widget це ЙОГО
        // власний розмір у пікселях (enwidgets.c), а голий -- розмір екрана
        // (глобальний proto в 1_core/ensystem.c). Тут потрібні обидва.
        float cw, ch;
        m_Card.GetScreenSize(cw, ch);

        int sw, sh;
        GetScreenSize(sw, sh);

        m_Card.SetPos((sw - cw) * 0.5, (sh - ch) * 0.5);
    }

    // Рація в руках -- і тільки вона. Клавіатура без рації нічого не значить.
    private void Grab()
    {
        m_Radio   = null;
        m_Profile = null;

        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p || !p.GetHumanInventory())
            return;

        m_Radio = TransmitterBase.Cast(p.GetHumanInventory().GetEntityInHands());
        if (!m_Radio)
            return;

        m_Profile = OZR_ClientGrid.For(m_Radio.GetType());
    }

    static bool CanOpen()
    {
        if (!OZR_ClientGrid.Ready())
            return false;

        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p || !p.GetHumanInventory())
            return false;

        TransmitterBase t = TransmitterBase.Cast(p.GetHumanInventory().GetEntityInHands());
        if (!t)
            return false;

        return OZR_ClientGrid.For(t.GetType()) != null;
    }

    private void Paint()
    {
        if (m_Title)
        {
            string title = "";
            if (m_Radio)
                title = m_Radio.GetDisplayName();
            m_Title.SetText(title);
        }

        if (m_Freq)
        {
            string shown = m_Typed;
            if (shown == "" && m_Radio)
                shown = OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(m_Radio.GetTunedFrequencyIndex()));
            m_Freq.SetText(shown);
        }

        if (m_Band && m_Profile)
        {
            string band = OZR_Fmt.MHz(m_Profile.MinMHz) + " - " + OZR_Fmt.MHz(m_Profile.MaxMHz);
            band += "   step " + OZR_Fmt.MHz(m_Profile.StepMHz);
            m_Band.SetText(band);
        }

        if (m_Hint)
            m_Hint.SetText("#STR_OZR_KEYPAD_HINT");
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w)
            return false;

        string name = w.GetName();

        // Закриття мусить жити ВСЕРЕДИНІ меню. Клавіша-перемикач цього не
        // може: поки меню відкрите, DayZ глушить інпути, і та сама клавіша
        // більше не спрацьовує -- перевірено на стенді, разом із Back на
        // геймпаді. Меню, яке не закрити, гірше за відсутнє.
        if (name == "BtnClose")
        {
            Close();
            return true;
        }

        if (name == "BtnGo")
        {
            Commit();
            return true;
        }

        if (name == "BtnClear")
        {
            m_Typed = "";
            Paint();
            return true;
        }

        if (name == "BtnDot")
        {
            if (m_Typed != "" && m_Typed.IndexOf(".") < 0)
                m_Typed = m_Typed + ".";
            Paint();
            return true;
        }

        // Btn0..Btn9 -- останній символ імені і є цифрою.
        if (name.Length() == 4 && name.Substring(0, 3) == "Btn")
        {
            // Довжину обмежуємо: «1451250000» не частота, а промах по клавіші,
            // помножений на десять.
            if (m_Typed.Length() < 8)
                m_Typed = m_Typed + name.Substring(3, 1);
            Paint();
            return true;
        }

        return super.OnClick(w, x, y, button);
    }

    // Підтвердження. Рахуємо ділення, перевіряємо, що воно в смузі й на
    // ґратці профілю, і лише тоді просимо сервер. Сервер перевірить те саме
    // ще раз -- клієнтові тут не вірять, і правильно.
    private void Commit()
    {
        if (!m_Radio || !m_Profile || m_Typed == "")
            return;

        float mhz = m_Typed.ToFloat();
        int   idx = OZR_ClientGrid.IndexOf(mhz);

        int lo = OZR_ClientGrid.IndexOf(m_Profile.MinMHz);
        int hi = OZR_ClientGrid.IndexOf(m_Profile.MaxMHz);

        if (idx < lo || idx > hi)
        {
            m_Typed = "";
            if (m_Hint)
                m_Hint.SetText("#STR_OZR_KEYPAD_OUT");
            Paint();
            return;
        }

        // Прилипаємо до найближчого СВОГО каналу, а не відмовляємо: гравець
        // набрав 145.13, а рація крокує по 0.05 -- він мав на увазі 145.15, і
        // сказати йому «ні» замість того, щоб довести, це вередливість.
        int   stride = OZR_Stride();
        float rel    = idx - lo;
        float st     = stride;
        int   k      = Math.Round(rel / st);
        idx          = lo + k * stride;
        if (idx > hi)
            idx = hi;

        GetRPCManager().SendRPC(OZ_Const.MOD, OZR_Const.RPC_TUNE, new Param1<int>(idx), true);
        OZR_Log.Dbg("freq keypad: asked for index " + idx.ToString() + " (" + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(idx)) + ")");

        // Закриваємось одразу. Показати «нову» частоту тут однаково нічим:
        // індекс на клієнті оновиться лише коли сервер його поверне, і
        // домальовувати очікуване значення означало б показати те, чого ще
        // немає -- а на відмову сервера воно й не з'явиться.
        m_Typed = "";
        Close();
    }

    private int OZR_Stride()
    {
        int stride = 1;
        float gs = 0;

        if (OZR_ClientGrid.Count() > 1)
            gs = OZR_ClientGrid.MHzAt(1) - OZR_ClientGrid.MHzAt(0);

        if (gs > 0 && m_Profile)
            stride = Math.Round(m_Profile.StepMHz / gs);

        if (stride < 1)
            stride = 1;
        return stride;
    }
}
