// Константи склейки «рація в КПК».
//
// Тут те, чого немає сенсу тримати в самій рації: вона про КПК не знає, і
// сторінка, вид модуля та список каналів -- поняття цього стику, а не її.

class OZRP_Const
{
    // Сторінка «Рація» в КПК.
    static const string PAGE_RADIO = "radio";

    // Вид модуля в договорі OZ_PdaHardware.
    static const string MOD_RADIO = "radio";

    // Класнейм плати. Скриптовий клас живе в моді рації; сюди її оголошує
    // конфіг цього pbo, бо успадковується вона від класу КПК.
    static const string BOARD_CLASS = "OZ_Module_Radio";

    // Канали адмін описує тут. Ім'я НЕ змінилось при розділенні навмисно: файл
    // уже лежить на серверах, і перейменувати його означало б мовчки лишити
    // адміна без налаштувань. Переїхав власник файлу, а не файл.
    static const string CHANNELS     = "$profile:OpenZone\\Radio.json";
    static const int    SCHEMA_RADIO = 1;
    static const int    CH_NAME_MAX  = 24;
}
