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
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        OZR_Ptt.Poll();
    }

    // Меню програло фокус, гравець помер, гру згорнули -- край відпускання
    // клавіші в такі миті не приходить, і відкритий передавач лишився б
    // відкритим. Тому мовчання вмикаємо самі.
    override void OnMissionFinish()
    {
        OZR_Ptt.Drop();
        super.OnMissionFinish();
    }
}
