// Клавіша клавіатури частот.
//
// ВІДКРИВАЄ, і тільки. Закривають кнопки в самому меню -- «X» і «TUNE».
//
// Це не задум, а вимір. Задумувався перемикач, але поки меню відкрите, DayZ
// глушить інпути, і та сама клавіша більше не спрацьовує; Back на геймпаді теж
// не дійшов. Тому єдиний надійний шлях назовні -- віджет усередині меню, і
// саме тому кнопка «X» там є.
//
// Друге, що довелося з'ясувати: менеджер НЕ знаходить це меню за id.
// EnterScriptedMenu віддає екземпляр, ui-дерево показує його відкритим, а
// FindMenu(MENU_FREQ) одразу після цього повертає порожньо -- і CloseMenu(id)
// відповідно нічого не закриває. Тому меню закриває САМЕ СЕБЕ через Close(), а
// тут ми лише пам'ятаємо посилання.
//
// Сам UAInput кешувати НЕ можна: ваніль тримає обгортку й перечитує вказівник
// щокадру (radialmenu.c, actiontargets). Робимо так само.

class OZR_FreqInput
{
    // БЕЗ ref: UAIDWrapper -- нативний об'єкт із приватним деструктором, і
    // скрипт ним не володіє.
    private static UAIDWrapper s_Key;
    private static OZR_FreqMenu s_Menu;
    private static bool s_Warned = false;

    static void Init()
    {
        UAInput i = GetUApi().GetInputByName(OZR_Const.INPUT_FREQ);
        if (!i)
        {
            // Єдина діагностика, яку дає рушій: NULL. Причин рівно дві --
            // inputs.xml не завантажився (шлях у CfgMods) або ім'я написане
            // інакше, ніж у XML.
            if (!s_Warned)
            {
                s_Warned = true;
                OZR_Log.Error("input " + OZR_Const.INPUT_FREQ + " not found - check the CfgMods inputs= path and the name in inputs.xml");
            }
            return;
        }

        s_Key = i.GetPersistentWrapper();
        OZR_Log.Dbg("input " + OZR_Const.INPUT_FREQ + " bound");
    }

    static void Poll()
    {
        if (!s_Key)
            return;

        UAInput i = s_Key.InputP();
        if (!i)
            return;

        // Поки меню відкрите, клавіша ЗАКРИВАЄ його -- і Escape теж.
        //
        // Раніше опитування тут просто вимикалось, бо здавалось, що при
        // відкритому меню інпути однаково заглушені. Тепер це не так: ми
        // глушимо лише мишачі групи, клавіатура лишається живою, тож та сама
        // клавіша чесно доходить і може працювати перемикачем.
        if (s_Menu)
        {
            bool byKey  = i.LocalPress();
            bool byBack = GetUApi().GetInputByID(UAUIBack).LocalPress();

            if (byKey || byBack)
            {
                OZR_FreqMenu m = s_Menu;
                s_Menu = null;
                m.Close();
            }
            return;
        }

        if (!i.LocalPress())
            return;

        // Клавіша бачить сирі натискання незалежно від фокуса UI. Поле з
        // фокусом означає «клавіатура зайнята текстом» -- сусідній мод спіймав
        // це тим, що літера в назві мітки закривала йому меню посеред слова.
        Widget focused = GetFocus();
        if (focused)
        {
            if (EditBoxWidget.Cast(focused) || MultilineEditBoxWidget.Cast(focused))
                return;
        }

        Open();
    }

    // Меню саме каже, що його вже немає: покладатись на FindMenu тут не можна.
    static void Forget()
    {
        s_Menu = null;
    }

    // Гравець помер або згорнув гру -- меню лишилось би висіти.
    static void Drop()
    {
        if (!s_Menu)
            return;

        OZR_FreqMenu m = s_Menu;
        s_Menu = null;
        m.Close();
    }

    private static void Open()
    {
        // Не лізти поверх ЧУЖОГО меню -- але перевіряти саме інвентар, а не
        // GetMenu(): у геймплеї той буває не-null і без жодного відкритого
        // вікна, і тоді клавіатура мовчки не відкривалась би ніколи.
        if (GetGame().GetUIManager().IsMenuOpen(MENU_INVENTORY))
            return;

        if (!OZR_FreqMenu.CanOpen())
            return;

        UIScriptedMenu made = GetGame().GetUIManager().EnterScriptedMenu(OZR_Const.MENU_FREQ, null);
        if (!made)
        {
            OZR_Log.Error("freq keypad: EnterScriptedMenu returned nothing for id " + OZR_Const.MENU_FREQ.ToString());
            return;
        }

        // Реєстру id меню в рушії немає, і чужий мод міг зайняти наш номер --
        // 133 вже виявився зайнятий OZ_LinkMenu ядра, і відкривалось чуже,
        // мовчки. Тому перевіряємо, що екземпляр саме наш.
        s_Menu = OZR_FreqMenu.Cast(made);
        if (!s_Menu)
            OZR_Log.Error("freq keypad: menu id " + OZR_Const.MENU_FREQ.ToString() + " is taken by " + made.ClassName() + " - pick another id");
    }
}
