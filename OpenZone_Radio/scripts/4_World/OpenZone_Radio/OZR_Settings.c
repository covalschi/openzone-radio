// Власні налаштування мода рації.
//
// Поле поки одне, і окремий файл заведений саме через це: рація більше не
// читає нічиїх конфігів. Раніше рівень діагностики приходив із ядра
// (Settings.Debug), і один вимикач на всю збірку -- річ правильна; але ядра
// тепер може не бути зовсім, а посилання на клас із незавантаженого мода в
// Enforce не існує навіть у мертвій гілці.
//
// Один вимикач на родину при цьому зберігається: склейка @OpenZone_Radio_PDA
// жорстко вимагає ядро й переставляє наш прапорець за ядерним при старті.
// Тобто друге місце не сперечається з першим, а повторює його.

class OZR_Settings : OZR_ConfigBase
{
    // НЕ "Debug". Так зветься клас рушія, і поле з таким іменем не
    // компілюється взагалі: "Variable name 'Debug' already used as type name".
    // Та сама пастка, що вже ловила Step() і OZR_IsLive у цьому ж моді.
    bool DebugLog = false;

    private static ref OZR_Settings s_Inst;

    static OZR_Settings Get()
    {
        return s_Inst;
    }

    override int LatestVersion()
    {
        return OZR_Const.SCHEMA_RADIO;
    }

    override void LoadDefaults()
    {
        Version = LatestVersion();
        DebugLog = false;
    }

    static void ServerLoad()
    {
        s_Inst = new OZR_Settings();
        OZR_ConfigLoader<OZR_Settings>.Load(OZR_Const.SETTINGS, "Radio", s_Inst);
        OZR_Log.SetDebug(s_Inst.DebugLog);
    }
}
