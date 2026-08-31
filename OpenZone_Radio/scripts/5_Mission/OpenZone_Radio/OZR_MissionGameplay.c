// Клієнтська точка входу мода рації.
//
// Дві речі, і обидві -- через договір КПК, а не правкою його файлів: сказати
// фабриці сторінок, хто малює «Рацію», і почати опитувати клавішу PTT.
//
// На виділеному сервері MissionGameplay не створюється взагалі (там
// MissionServer), тож цей код туди просто не потрапляє.

modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        OZ_PdaPageFactory.Add(OZR_Const.PAGE_RADIO, OZR_PageRadio);
        OZR_Ptt.Init();
        OZR_FreqInput.Init();
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        OZR_Ptt.Poll();
        OZR_FreqInput.Poll();
    }

    override UIScriptedMenu CreateScriptedMenu(int id)
    {
        // super ПЕРШИЙ і вихід одразу, якщо він щось віддав: саме це тримає
        // сумісність з іншими модами, що чіпали той самий клас.
        UIScriptedMenu menu = super.CreateScriptedMenu(id);
        if (menu)
            return menu;

#ifndef NO_GUI
        if (id == OZR_Const.MENU_FREQ)
            return new OZR_FreqMenu();
#endif

        return null;
    }

    // Меню програло фокус, гравець помер, гру згорнули -- край відпускання
    // клавіші в такі миті не приходить, і відкритий передавач лишився б
    // відкритим. Тому мовчання вмикаємо самі.
    override void OnMissionFinish()
    {
        OZR_Ptt.Drop();
        OZR_FreqInput.Drop();
        super.OnMissionFinish();
    }
}
