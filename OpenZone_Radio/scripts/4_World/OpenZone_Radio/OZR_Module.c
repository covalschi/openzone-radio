// Серверна половина мода рації.
//
// Мод НЕ несе власного пристрою: рація -- це плата в відсіку КПК. Тому все,
// що тут робиться при старті:
//
//   1. виміряти таблицю частот рушія (див. OZR_Bands: число «сім» ніде не
//      оголошене, і вигадувати його не можна);
//   2. прочитати канали, які описав адмін -- або скласти по каналу на смугу,
//      якщо він не описав нічого;
//   3. оголосити своє залізо в договорі КПК -- плату рації й довгу антену;
//   4. стати сторінкою «Рація» в реєстрі ядра;
//   5. сказати одним рядком, що з цього вийшло, щоб вердикт стенда мав за що
//      зачепитись.
//
// Порядок перших двох кроків значущий: канали за замовчуванням будуються з
// ВИМІРЯНОЇ таблиці, і до виміру будувати їх нема з чого.

[CF_RegisterModule(OZR_Module)]
class OZR_Module : CF_ModuleWorld
{
    // Прийом тримається в тон живленню КПК поза сторінкою -- рація, яка
    // починає чути лише коли на неї подивишся, це не рація. Дві секунди --
    // непомітно для гравця й дешево для сервера: обхід онлайну й нічого
    // більше.
    private ref Timer m_SyncTimer;
    private static const float SYNC_INTERVAL = 2.0;

    override void OnInit()
    {
        super.OnInit();

        // Порядок такий самий, як у решті модів OpenZone: спершу super, потім
        // підписки. Інакше CF не встигає зареєструвати модуль, і подія
        // приходить у порожнечу.
        EnableMissionStart();
        EnableMissionFinish();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        if (!GetGame().IsServer())
            return;

        OZR_Bands.Probe();
        OZR_Settings.ServerLoad();
        OZR_Hardware.Declare();

        OZ_PageRegistry.Register(OZR_Const.PAGE_RADIO,
                                 "#STR_OZR_PAGE_RADIO",
                                 "set:oz_pda image:radio",
                                 new OZ_PdaHandlerRadio());

        m_SyncTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_SyncTimer.Run(SYNC_INTERVAL, this, "SyncTick", NULL, true);

        OZR_Settings cfg = OZR_Settings.Get();
        int channels = 0;
        if (cfg && cfg.Channels)
            channels = cfg.Channels.Count();

        string summary = "radio loaded: bands=" + OZR_Bands.Count().ToString();
        summary += " channels=" + channels.ToString();
        summary += " modules=" + OZR_Hardware.Count().ToString();
        OZR_Log.Info(summary);
    }

    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        super.OnMissionFinish(sender, args);

        if (m_SyncTimer)
            m_SyncTimer.Stop();
    }

    // Кличеться таймером на ім'я -- метод мусить бути видимим (не private).
    void SyncTick()
    {
        OZR_Set.Sync();
    }
}
