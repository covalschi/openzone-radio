// Серверна половина склейки «рація в КПК».
//
// Все, що раніше стояло в модулі самої рації й вимагало КПК або ядра, живе
// тепер тут. Мод рації про це нічого не знає: він лише оголошує точки, до яких
// можна підключитись, а підключається до них цей pbo.
//
// Що саме тут робиться:
//   1. рівень діагностики зводиться до ядерного -- один вимикач на всю родину,
//      коли родина взагалі стоїть;
//   2. плата оголошується модулем відсіку в договорі заліза КПК;
//   3. реєструється її поведінка -- нею приходять вставляння плати й «прилад
//      працює», тобто те, що раніше добували обходом усіх гравців;
//   4. реєструється сторінка «Рація»;
//   5. заводиться звірка -- на єдиний випадок, події для якого немає:
//      знеструмлений прилад із платою всередині.
//
// PTT тут немає: плата -- звичайна профільна рація, і спільний обхід у моді
// рації відкриває її разом із рештою, хоч вона й лежить усередині КПК.
//
// Порядок значущий рівно в одному місці: діагностика ставиться першою, щоб
// рядки нижче вже на неї зважали.

[CF_RegisterModule(OZRP_Module)]
class OZRP_Module : CF_ModuleWorld
{
    // Прийом тримається в тон живленню КПК поза сторінкою -- рація, яка
    // починає чути лише коли на неї подивишся, це не рація.
    //
    // Таймер лишився РІВНО для одного випадку, на який договір заліза КПК
    // події не дає: прилад знеструмився, а плата лежить у відсіку. Все інше
    // -- вставляння, вмикання, виймання -- приходить подіями (див. OZR_Set).
    // Тому й ходить він тепер по реєстру живих плат, а не по всіх гравцях
    // сервера: у спокої це порожній цикл.
    private ref Timer m_WakeTimer;
    private static const float WAKE_INTERVAL = 2.0;

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

        OZR_Hardware.Declare();

        // Поведінка плати: вставляння й «прилад працює» приходять звідси, а
        // не з обходу онлайну.
        OZ_PdaModules.Register(new OZR_BoardBehaviour());

        OZ_PageRegistry.Register(OZRP_Const.PAGE_RADIO,
                                 "#STR_OZR_PAGE_RADIO",
                                 "set:oz_pda image:radio",
                                 new OZ_PdaHandlerRadio());

        m_WakeTimer = new Timer(CALL_CATEGORY_SYSTEM);
        m_WakeTimer.Run(WAKE_INTERVAL, this, "WakeTick", NULL, true);

        OZR_Log.Info("radio in the pda: modules=" + OZR_Hardware.Count().ToString());
    }

    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        super.OnMissionFinish(sender, args);

        if (m_WakeTimer)
            m_WakeTimer.Stop();
    }

    // Кличеться таймером на ім'я -- метод мусить бути видимим (не private).
    void WakeTick()
    {
        OZR_Set.Sweep();
    }
}
