// Клієнтська половина склейки: одне підключення до чужої точки розширення.
//
// Сказати фабриці сторінок КПК, хто малює «Рацію». Більше нічого: PTT плати
// веде спільний обхід у моді рації, а настройка йде сторінкою.
//
// На виділеному сервері MissionGameplay не створюється взагалі (там
// MissionServer), тож цей код туди просто не потрапляє.

modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();
        OZ_PdaPageFactory.Add(OZRP_Const.PAGE_RADIO, OZR_PageRadio);
        // Літеру вкладки теж підписуємо самі: перелік у КПК знав рядок
        // "radio", тобто ім'я чужої сторінки, якої може й не бути.
        OZ_PdaPageFactory.Letter(OZRP_Const.PAGE_RADIO, "R");
    }
}
