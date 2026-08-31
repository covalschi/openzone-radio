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
    }
}
