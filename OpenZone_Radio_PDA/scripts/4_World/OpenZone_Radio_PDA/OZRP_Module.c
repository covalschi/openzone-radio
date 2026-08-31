// Серверна половина склейки «рація в КПК».
//
// Все, що раніше стояло в модулі самої рації й вимагало КПК або ядра, живе
// тепер тут. Мод рації про це нічого не знає: він лише оголошує точки, до яких
// можна підключитись, а підключається до них цей pbo.
//
// Що саме тут робиться:
//   1. рівень діагностики зводиться до ядерного -- один вимикач на всю родину,
//      коли родина взагалі стоїть;
//   2. читаються канали (це поняття КПК, не рації);
//   3. плата й довга антена оголошуються в договорі заліза КПК;
//   4. реєструється сторінка «Рація»;
//   5. вмикається звірка живлення, щоб приймач тримався в тон КПК;
//   6. підписується слухач PTT, який веде плату.
//
// Порядок значущий рівно в одному місці: діагностика ставиться першою, щоб
// рядки нижче вже на неї зважали.

[CF_RegisterModule(OZRP_Module)]
class OZRP_Module : CF_ModuleWorld
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

        EnableMissionStart();
        EnableMissionFinish();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        if (!GetGame().IsServer())
            return;

        // Один вимикач на родину. Рація тримає свій прапорець, бо може жити
        // без ядра; коли ядро є -- воно й вирішує.
        OZR_Log.SetDebug(OZ_Log.IsDebug());

        OZR_ChannelCfg.ServerLoad();
        OZRP_AdminCfg.Declare();

        OZR_Hardware.Declare();

        OZ_PageRegistry.Register(OZRP_Const.PAGE_RADIO,
                                 "#STR_OZR_PAGE_RADIO",
                                 "set:oz_pda image:radio",
                                 new OZ_PdaHandlerRadio());

        m_SyncTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_SyncTimer.Run(SYNC_INTERVAL, this, "SyncTick", NULL, true);

        OZR_ChannelCfg cfg = OZR_ChannelCfg.Get();
        int channels = 0;
        if (cfg && cfg.Channels)
            channels = cfg.Channels.Count();

        string summary = "radio in the pda: channels=" + channels.ToString();
        summary += " modules=" + OZR_Hardware.Count().ToString();
        OZR_Log.Info(summary);

        // Нуль -- не поломка, а порожній перелік, який нікому не належить,
        // крім адміна. Кажемо, ДЕ його заповнювати: інакше «channels=0»
        // читається як несправність.
        if (channels == 0)
            OZR_Log.Info("no radio channels described - name them in " + OZRP_Const.CHANNELS + " or in the admin console; the page shows what you put there");
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
