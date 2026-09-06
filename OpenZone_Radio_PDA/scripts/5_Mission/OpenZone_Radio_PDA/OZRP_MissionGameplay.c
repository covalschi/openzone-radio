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

        // Свій значок і свій підпис -- тими самими входами й з тієї ж
        // причини. Набір oz_pda_icons уже несе tab_radio, але КПК не має
        // права роздавати спрайти сторінкам, яких у ньому немає: без цих
        // двох рядків вкладка малювалась запасним "tab_page", а підписом
        // їй лишалась сама літера рейки.
        OZ_PdaPageFactory.Sprite(OZRP_Const.PAGE_RADIO, "tab_radio");
        OZ_PdaPageFactory.Caption(OZRP_Const.PAGE_RADIO, "#STR_OZR_PAGE_RADIO");
    }
}
