// Константи мода рації.

class OZR_Const
{
    static const string LOG_PREFIX = "[OpenZone/Radio] ";

    // Сторінка «Рація» в КПК. Її вмикає модуль рації, а не профіль: без
    // плати рації сторінці нема що показувати.
    static const string PAGE_RADIO = "radio";

    // Вид модуля в договорі OZ_PdaHardware.
    static const string MOD_RADIO = "radio";

    // Скільки індексів частот перебирати, шукаючи межу рушія. Число завідомо
    // більше за очікуване: межу шукаємо, а не припускаємо.
    static const int BAND_PROBE_MAX = 32;

    // Клас предмета, на якому міряємо таблицю частот. Ванільна рація --
    // єдиний ItemTransmitter, який точно є на будь-якому сервері.
    static const string BAND_PROBE_CLASS = "PersonalRadio";

    // Класнейм плати рації. Знати його треба: саме цей предмет і є
    // передавачем, який рушій бачить у гравця.
    static const string BOARD_CLASS = "OZ_Module_Radio";

    // Канали адмін описує тут. Файл лежить поруч із рештою конфігів OpenZone.
    static const string SETTINGS       = "$profile:OpenZone\\Radio.json";
    static const int    SCHEMA_RADIO   = 1;
    static const int    CH_NAME_MAX    = 24;
}
