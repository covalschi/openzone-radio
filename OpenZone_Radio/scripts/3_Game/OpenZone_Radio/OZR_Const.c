// Константи мода рації.

class OZR_Const
{
    static const string LOG_PREFIX = "[OpenZone/Radio] ";

    // Сторінка «Рація» в КПК. Її вмикає модуль рації, а не профіль: без
    // плати рації сторінці нема що показувати.
    static const string PAGE_RADIO = "radio";

    // Вид модуля в договорі OZ_PdaHardware.
    static const string MOD_RADIO = "radio";

    // Скільки індексів частот перебирати, шукаючи межу рушія.
    //
    // Було 32 -- «завідомо більше за очікуване», коли відповіддю була вісімка.
    // Після того, як межу зняли патчем, це перестало бути правдою: сітка тепер
    // буває на тисячу з гаком ділень, і проба чесно міряла б 32, поки сервер
    // віддає більше. Тому тут тепер не оцінка, а ЗАПОБІЖНИК від зациклення --
    // число, більше за будь-яку осмислену сітку. Ціна ітерації -- один
    // SetFrequencyByIndex і два читання на предметі поза світом.
    static const int BAND_PROBE_MAX = 4096;

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

    // Профілі ручних рацій: діапазон і крок на класнейм. Окремий файл, бо це
    // окреме рішення адміна -- канали КПК і те, куди дістає ручна рація, не
    // мають нічого спільного, крім ефіру.
    static const string PROFILES         = "$profile:OpenZone\\RadioProfiles.json";
    static const int    SCHEMA_PROFILES  = 1;
}
