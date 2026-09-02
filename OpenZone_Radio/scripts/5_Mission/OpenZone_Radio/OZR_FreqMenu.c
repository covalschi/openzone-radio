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
    private float  m_Since = 0;

    // Підказка тримається В ПОЛІ, а не пишеться просто у віджет. Інакше
    // наступний же Paint затирає її звичайним текстом -- саме так відмова
    // «частота поза смугою» жодного разу не потрапила гравцеві на очі.
    private string m_HintKey = "#STR_OZR_KEYPAD_HINT";

    // Перетягування: зсув між курсором і кутом картки в мить захоплення.
    private bool   m_Dragging = false;
    private float  m_GrabX = 0;
    private float  m_GrabY = 0;

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

        // LockControls -- це і є «забрати мишу». База вміє це сама (див.
        // UIScriptedMenu.LockControls: ChangeGameFocus(1, INPUT_DEVICE_MOUSE)
        // плюс ShowUICursor), але САМА НЕ КЛИЧЕ -- меню мусить покликати.
        // Без цього курсор не з'являється й клікати нема чим; спіймано тим,
        // що автотест бив по обробнику напряму й миші не торкався зовсім.
        //
        // Клавіатуру не чіпаємо: UseKeyboard() лишається false, тож
        // ChangeGameFocus для неї не викликається, і біг живий.
        LockControls();
        SetFocus(layoutRoot);

        // Без цього клік доходить і до меню, І ДО ГРИ: кнопка натискається, а
        // разом із нею спрацьовує дія в світі -- та сама рація вмикається й
        // вимикається. LockControls дає курсор, але не забирає у гри саму дію.
        //
        // Глушимо РІВНО мишачі групи. "movement" навмисне НЕ чіпаємо -- саме
        // на цьому тримається вимога бігати з відкритою клавіатурою, і саме
        // цим ми відрізняємось від КПК, який глушить усе гуртом ("menu").
        GetGame().GetMission().AddActiveInputExcludes(Excludes());

        Place();
        Grab();
        Paint();
    }

    // Рівно мишачі групи. "movement" тут немає навмисне -- див. OnShow.
    private static array<string> Excludes()
    {
        array<string> a = new array<string>();
        a.Insert("aiming");
        a.Insert("actions");
        a.Insert("optics");
        a.Insert("hotkey");
        return a;
    }

    override void OnHide()
    {
        super.OnHide();

        m_Dragging = false;
        // БЕЗ true. Другий аргумент форсує скидання вводу, і затиснута W
        // губиться -- персонаж зупиняється рівно в мить закриття вікна.
        // Ваніль і КПК ставлять true, бо їм байдуже: вони й так знерухомили
        // гравця. Нам не байдуже, у цьому вся суть цього меню.
        GetGame().GetMission().RemoveActiveInputExcludes(Excludes(), false);
        UnlockControls();

        // Кажемо опитувачу самі: FindMenu(MENU_FREQ) це меню не бачить, тож
        // питати менеджера, чи ми ще відкриті, марно.
        OZR_FreqInput.Forget();
    }

    // Куди поставити картку: туди, куди її перетягнули востаннє, а якщо ще
    // нікуди -- по центру.
    private void Place()
    {
        if (!m_Card)
            return;

        // LoadFile віддає ще й текст помилки -- третій параметр обов'язковий.
        // Відсутній файл тут не помилка, а перший запуск, тож мовчимо.
        OZR_KeypadPos saved;
        string err;
        if (JsonFileLoader<OZR_KeypadPos>.LoadFile(OZR_Const.KEYPAD_POS, saved, err) && saved && saved.Set)
        {
            m_Card.SetPos(saved.X, saved.Y);
            Clamp();
            return;
        }

        Centre();
    }

    private void SavePos()
    {
        if (!m_Card)
            return;

        float x, y;
        m_Card.GetPos(x, y);

        OZR_KeypadPos p = new OZR_KeypadPos();
        p.Set = true;
        p.X   = x;
        p.Y   = y;
        string err;
        if (!JsonFileLoader<OZR_KeypadPos>.SaveFile(OZR_Const.KEYPAD_POS, p, err))
            OZR_Log.Warn("keypad position not saved: " + err);
    }

    // Не даємо картці піти за край: вікно, за яке більше не вхопитись, --
    // це вікно, яке більше не закрити.
    private void Clamp()
    {
        float cw, ch, x, y;
        m_Card.GetScreenSize(cw, ch);
        m_Card.GetPos(x, y);

        int sw, sh;
        GetScreenSize(sw, sh);

        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > sw - cw) x = sw - cw;
        if (y > sh - ch) y = sh - ch;

        m_Card.SetPos(x, y);
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
        return WhyNotOpen() == "";
    }

    // Чому не відкриється -- словами, і порожній рядок означає «відкриється».
    //
    // Мовчазна відмова тут коштувала дорожче за все інше в цьому файлі:
    // клавіша, яка нічого не робить, читається як зламаний мод, і відрізнити
    // «немає ефіру» від «рація вимкнена» чи «профіль не приїхав» не міг ніхто
    // -- ні гравець, ні той, кому він про це напише. П'ять різних причин
    // виглядали однаково.
    static string WhyNotOpen()
    {
        if (!OZR_ClientGrid.Ready())
            return "no ether: the server has not sent a usable frequency grid";

        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p || !p.GetHumanInventory())
            return "no player inventory";

        TransmitterBase t = TransmitterBase.Cast(p.GetHumanInventory().GetEntityInHands());
        if (!t)
            return "nothing in hands that can transmit";

        // Вимкнена рація нічого не вміє, і клавіатура над нею -- обіцянка,
        // якої ніхто не виконає.
        if (!t.OZR_IsPowered())
            return t.GetType() + " is not powered";

        if (!OZR_ClientGrid.For(t.GetType()))
            return "no profile for " + t.GetType() + " (" + OZR_ClientGrid.ProfileCount().ToString() + " profile(s) received)";

        return "";
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
                shown = OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(m_Radio.OZR_ShownIndex()));
            m_Freq.SetText(shown);
        }

        if (m_Band && m_Profile)
        {
            string band = OZR_Fmt.MHz(m_Profile.MinMHz) + " - " + OZR_Fmt.MHz(m_Profile.MaxMHz);
            band += "   step " + OZR_Fmt.MHz(m_Profile.StepMHz);
            m_Band.SetText(band);
        }

        if (m_Hint)
            m_Hint.SetText(m_HintKey);
    }

    // Перетягування за верхню смугу. Тягнемо КАРТКУ, а не окремі віджети:
    // вони всі її діти, тож рухаються разом.
    override bool OnMouseButtonDown(Widget w, int x, int y, int button)
    {
        if (w && button == 0 && w.GetName() == "DragBar")
        {
            float cx, cy;
            m_Card.GetPos(cx, cy);
            m_GrabX    = x - cx;
            m_GrabY    = y - cy;
            m_Dragging = true;
            return true;
        }
        return super.OnMouseButtonDown(w, x, y, button);
    }

    override bool OnMouseButtonUp(Widget w, int x, int y, int button)
    {
        if (m_Dragging)
        {
            m_Dragging = false;
            Clamp();
            SavePos();
            return true;
        }
        return super.OnMouseButtonUp(w, x, y, button);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (!w)
            return false;

        string name = w.GetName();

        // Тимчасова діагностика: чи доходить клік і під яким іменем. Оглядом
        // розкладки й обробника причину знайти не вдалося, а здогадуватись
        // удруге про те саме -- марна трата вечора.
        OZR_Log.Dbg("freq keypad: click on \"" + name + "\"");

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

        if (name == "BtnUp")
        {
            Nudge(1);
            return true;
        }

        if (name == "BtnDown")
        {
            Nudge(-1);
            return true;
        }

        // BACKSPACE, а не «стерти все». Промахнувся однією цифрою -- втрачати
        // через це весь набір безглуздо, а очистити можна й затиснувши.
        if (name == "BtnClear")
        {
            int n = m_Typed.Length();
            if (n > 0)
                m_Typed = m_Typed.Substring(0, n - 1);
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
            // Почав набирати -- відмова більше не актуальна.
            m_HintKey = "#STR_OZR_KEYPAD_HINT";

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
        if (!m_Radio || !m_Profile)
        {
            OZR_Log.Dbg("freq keypad: commit ignored - no radio or no profile");
            return;
        }

        // Нічого не набрано -- значить частоту вже виставили стрілками, і
        // підтверджувати нема чого. TUNE тут означає просто «готово», і
        // мовчазна кнопка на цьому місці читається як зламана.
        if (m_Typed == "")
        {
            OZR_Log.Dbg("freq keypad: nothing typed, closing");
            Close();
            return;
        }

        float mhz = m_Typed.ToFloat();
        int   idx = OZR_ClientGrid.IndexOf(mhz);

        int lo = OZR_ClientGrid.IndexOf(m_Profile.MinMHz);
        int hi = OZR_ClientGrid.IndexOf(m_Profile.MaxMHz);

        if (idx < lo || idx > hi)
        {
            OZR_Log.Dbg("freq keypad: commit refused - " + m_Typed + " is outside this set's band");
            m_Typed   = "";
            m_HintKey = "#STR_OZR_KEYPAD_OUT";
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

        Send(idx);

        // Закриваємось одразу. Показати «нову» частоту тут однаково нічим:
        // індекс на клієнті оновиться лише коли сервер його поверне, і
        // домальовувати очікуване значення означало б показати те, чого ще
        // немає -- а на відмову сервера воно й не з'явиться.
        m_Typed = "";
        OZR_Log.Dbg("freq keypad: commit done, closing");
        Close();
    }

    private void Send(int idx)
    {
        GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_TUNE, new Param1<int>(idx), true);
        OZR_Log.Dbg("freq keypad: asked for index " + idx.ToString() + " (" + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(idx)) + ")");
    }

    // Крок на один СВІЙ канал, по колу відрізка. Набирати частоту цілком
    // заради сусіднього каналу безглуздо, а що таке «сусідній», профіль уже
    // знає.
    private void Nudge(int dir)
    {
        if (!m_Radio || !m_Profile)
            return;

        int lo     = OZR_ClientGrid.IndexOf(m_Profile.MinMHz);
        int hi     = OZR_ClientGrid.IndexOf(m_Profile.MaxMHz);
        int stride = OZR_Stride();

        int cur = m_Radio.OZR_ShownIndex();
        if (cur < lo || cur > hi)
            cur = lo;

        // Спершу прилипаємо до ґратки профілю, потім крокуємо: рація могла
        // стояти між своїми каналами, якщо профіль щойно змінили.
        float rel = cur - lo;
        float st  = stride;
        int   k   = Math.Round(rel / st) + dir;

        int last = (hi - lo) / stride;
        if (k < 0)
            k = last;
        if (k > last)
            k = 0;

        string dbg = "freq keypad: step " + dir.ToString();
        dbg += " cur=" + cur.ToString() + " lo=" + lo.ToString() + " hi=" + hi.ToString();
        dbg += " stride=" + stride.ToString() + " k=" + k.ToString();
        dbg += " -> " + (lo + k * stride).ToString();
        OZR_Log.Dbg(dbg);

        m_Typed = "";
        Send(lo + k * stride);
    }

    // Малюємо те, що СПРАВДІ на рації, а не те, що попросили: сервер може
    // відмовити, і домальоване очікування було б брехнею. Відповідь приходить
    // із мережею, тож перемальовуємо за часом, а не за кліком -- і лише коли
    // гравець нічого не набирає, інакше набір затиралося б.
    override void Update(float timeslice)
    {
        super.Update(timeslice);

        // Тягнемо за курсором. Події «миша поїхала» у віджета немає, тож
        // позицію читаємо самі, поки кнопку тримають.
        if (m_Dragging && m_Card)
        {
            int mx, my;
            GetMousePos(mx, my);
            m_Card.SetPos(mx - m_GrabX, my - m_GrabY);
            return;
        }

        m_Since = m_Since + timeslice;
        if (m_Since < 0.25)
            return;
        m_Since = 0;

        // Рацію могли вимкнути або прибрати з рук, поки вікно відкрите.
        // Клавіатура над мертвою коробкою нічого не значить.
        if (!m_Radio || !m_Radio.OZR_IsPowered())
        {
            OZR_Log.Dbg("freq keypad: the radio is gone or switched off, closing");
            Close();
            return;
        }

        if (m_Typed == "")
            Paint();
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
