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

    // Куди гравець перетягнув клавіатуру. КЛІЄНТСЬКИЙ файл: $profile: на
    // клієнті -- це його власний профіль, і сервера це не стосується.
    static const string KEYPAD_POS = "$profile:OpenZone\\RadioKeypad.json";

    // Клавіша цифрової клавіатури частот. Оголошена у ВЛАСНОМУ inputs.xml
    // цього мода: ручна рація -- не модуль КПК.
    static const string INPUT_FREQ = "UAOZRadioFreq";

    // Ідентифікатор меню.
    //
    // МАЄ БУТИ МАЛИМ. Виміряно сусіднім модом і записано в OZ_PdaConst: із
    // великим числом EnterScriptedMenu просто повертає NULL, а
    // Mission.CreateScriptedMenu навіть не кличеться -- у лозі видно виклики
    // з ванільними 11 і 17, і жодного з шестизначним. Тому не «беремо високе,
    // щоб ні з ким не зіткнутись», а тиснемось поруч із КПК (131, 132).
    //
    // Реєстру id у рушії немає, тож зіткнення непереборне -- і воно ВЖЕ
    // сталося: 133 виявився зайнятий OZ_LinkMenu ядра, і меню відкривалось
    // чуже, мовчки. Виглядало це як «клавіатура не працює», бо super
    // .CreateScriptedMenu віддавав чужий екземпляр раніше, ніж черга доходила
    // до нашого. Тому OZR_FreqInput після відкриття перевіряє, що екземпляр
    // саме наш, і каже вголос, якщо ні.
    //
    // Зайнято сім'єю OpenZone: 131 КПК, 132 його HUD-редактор, 133 прив'язка.
    static const int    MENU_FREQ  = 134;

    // RPC цього мода. Імена глобальні на весь процес, тому з префіксом.
    static const string RPC_GRID_REQ = "OZR_GridReq";
    static const string RPC_GRID_RES = "OZR_GridRes";
    static const string RPC_TUNE     = "OZR_TuneReq";
    static const string RPC_PTT      = "OZR_PttRadio";
}
