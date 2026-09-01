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
    override void OnInit()
    {
        super.OnInit();

        // Порядок такий самий, як у решті модів OpenZone: спершу super, потім
        // підписки. Інакше CF не встигає зареєструвати модуль, і подія
        // приходить у порожнечу.
        EnableMissionStart();
        EnableMissionFinish();
    }

    // Клієнтський бік: тягне сітку і тримає таймер, поки не дотягне.
    private ref Timer m_PullTimer;
    private int m_PullsLeft = 0;
    private static const float PULL_INTERVAL = 2.0;

    // ДВІ ХВИЛИНИ, а не двадцять секунд. Тяга починається з OnMissionStart --
    // тобто ще під час завантаження, задовго до того, як гравець опиниться у
    // світі: на цьому стенді від старту місії до входу минає близько хвилини,
    // і всі перші пакети йдуть у нікуди, бо каналу ще немає. Десяти спроб на
    // це не вистачало, і сітка не приїжджала зовсім -- сторінка чесно писала
    // «сервер ще не сказав», а виглядало це як поломка.
    //
    // Ціна помилитись у другий бік -- один крихітний пакет на дві секунди,
    // і лише поки сітки немає. Тому запас великий.
    private static const int   PULL_TRIES    = 60;

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        // Клієнт ТЯГНЕ сам, а не чекає, поки штовхнуть. Причина та сама, що
        // записана в ядрі поруч із OZ_Rpc.Hello(): серверний хук на конекті
        // спрацьовує раніше, ніж клієнт устигає зареєструвати обробник, і
        // пакет іде в нікуди. Тяга від порядку не залежить.
        if (GetGame().IsClient())
        {
            GetRPCManager().AddRPC(OZR_Const.MOD, OZR_Const.RPC_GRID_RES, this, SingleplayerExecutionType.Client);
            GetRPCManager().AddRPC(OZR_Const.MOD, OZR_Const.RPC_PROF_RES, this, SingleplayerExecutionType.Client);

            m_PullsLeft = PULL_TRIES;
            m_PullTimer = new Timer(CALL_CATEGORY_SYSTEM);
            m_PullTimer.Run(PULL_INTERVAL, this, "PullTick", NULL, true);
            PullTick();
        }

        if (!GetGame().IsServer())
            return;

        GetRPCManager().AddRPC(OZR_Const.MOD, OZR_Const.RPC_GRID_REQ, this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC(OZR_Const.MOD, OZR_Const.RPC_TUNE,     this, SingleplayerExecutionType.Server);
        GetRPCManager().AddRPC(OZR_Const.MOD, OZR_Const.RPC_PTT,      this, SingleplayerExecutionType.Server);

        // Найперше: рівень діагностики стоїть саме тут, і рядки нижче вже
        // мають на нього зважати.
        OZR_Settings.ServerLoad();

        OZR_Bands.Probe();
        // Після проби: профілі перевіряються ПРОТИ виміряної сітки, і без неї
        // перевіряти нічим.
        OZR_Profiles.ServerLoad();
        // Після профілів: ефір виводиться з них, і виводити його до того,
        // як вони прочитані, нема з чого. Пише файл, який прочитає
        // НАСТУПНИЙ старт сервера, і каже, чи вже діє.
        OZR_EtherServer.Publish(OZR_Profiles.Get());

        int profiles = 0;
        OZR_Profiles cfg = OZR_Profiles.Get();
        if (cfg && cfg.Radios)
            profiles = cfg.Radios.Count();

        string summary = "radio loaded: bands=" + OZR_Bands.Count().ToString();
        summary += " profiles=" + profiles.ToString();
        OZR_Log.Info(summary);
    }

    override void OnMissionFinish(Class sender, CF_EventArgs args)
    {
        super.OnMissionFinish(sender, args);

        if (m_PullTimer)
            m_PullTimer.Stop();
    }

    // Просимо сітку, поки не отримаємо. Спроби скінченні: якщо сервер не
    // відповідає, це не привід сипати пакетами до кінця сесії -- підпис
    // просто лишиться порожнім, і це чесніше за вигадане число.
    void PullTick()
    {
        if (OZR_ClientGrid.Ready() || m_PullsLeft <= 0)
        {
            if (m_PullTimer)
                m_PullTimer.Stop();
            return;
        }

        m_PullsLeft--;
        if (m_PullsLeft == 0)
            OZR_Log.Warn("the ether never arrived from the server after " + PULL_TRIES.ToString() + " tries - frequencies will show as unknown");

        GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_GRID_REQ, new Param1<int>(OZR_Const.SCHEMA_PROFILES), true);
    }

    // Гравець за особою відправника. Ходимо по онлайну, бо іншого зв'язку
    // між PlayerIdentity й сутністю на сервері немає.
    private PlayerBase OZR_PlayerOf(PlayerIdentity who)
    {
        if (!who)
            return null;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players[i]);
            if (p && p.GetIdentity() && p.GetIdentity().GetId() == who.GetId())
                return p;
        }
        return null;
    }

    // ----------------------------------------------------------------- RPC
    //
    // Ім'я методу мусить збігатися з рядком у AddRPC посимвольно, метод --
    // не статичний, і саме з цими чотирма параметрами в цьому порядку.

    void OZR_GridReq(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server || !sender)
            return;

        // ЛОГУЄМО, бо без цього рядка «сітка не приїхала» не має жодного
        // сліду в жодному лозі: ретейл-клієнт не пише скриптових рядків у
        // .RPT ЗОВСІМ (перевірено 2026-08-31: нуль рядків SCRIPT за сесію),
        // тож єдине місце, де видно цей обмін, -- серверний бік.
        OZR_Log.Dbg("ether asked for by " + sender.GetName());

        // ЧИСЛАМИ, а не JSON-ом: рядок-значення рушій ріже на 1023 байтах, і
        // один пакет із сіткою та всіма профілями переріс цю межу на
        // одинадцятому профілі. Обробник падав із «String CORRUPTED», а
        // виглядало це як «сітка не приїхала».
        GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_GRID_RES,
            new Param3<float, float, int>(OZR_Grid.MHzAt(0), OZR_Grid.StepMHz(), OZR_Grid.Count()),
            true, sender);

        OZR_Profiles cfg = OZR_Profiles.Get();
        if (!cfg || !cfg.Radios)
            return;

        // По пакету на профіль. Їх десяток -- це десяток крихітних пакетів раз
        // на сесію, і жодної довжини, яку можна переростити.
        for (int i = 0; i < cfg.Radios.Count(); i++)
        {
            OZR_RadioProfile p = cfg.Radios[i];
            GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_PROF_RES,
                new Param4<string, float, float, float>(p.ClassName, p.MinMHz, p.MaxMHz, p.StepMHz),
                true, sender);
        }
    }

    // Пряма настройка на ділення. Клієнт присилає ЧИСЛО, і воно не має жодної
    // ваги, поки сервер не перевірив його проти профілю тієї рації, яка
    // справді в руках у цього гравця. Інакше клавіатура частот була б
    // способом сісти на чужу смугу, минаючи і профіль, і саму рацію.
    void OZR_TuneReq(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server || !sender)
            return;

        Param1<int> p = new Param1<int>(0);
        if (!ctx.Read(p))
            return;

        PlayerBase player = OZR_PlayerOf(sender);
        if (!player || !player.GetHumanInventory())
        {
            OZR_Log.Dbg("tune refused: no player for this identity");
            return;
        }

        TransmitterBase radio = TransmitterBase.Cast(player.GetHumanInventory().GetEntityInHands());
        if (!radio)
        {
            OZR_Log.Dbg("tune refused: nothing that transmits in hands");
            return;
        }

        if (!radio.OZR_IsPowered())
        {
            OZR_Log.Dbg("tune refused: the radio is switched off");
            return;
        }

        OZR_RadioProfile prof = OZR_Profiles.For(radio.GetType());
        if (!prof || !OZR_Grid.Ready())
        {
            OZR_Log.Dbg("tune refused: no profile for " + radio.GetType() + " or the grid is not even");
            return;
        }

        int want = p.param1;
        int lo;
        int hi;
        int stride;
        if (!OZR_Grid.Window(prof, lo, hi, stride))
        {
            OZR_Log.Dbg("tune refused: " + radio.GetType() + " does not overlap the running ether at all - restart the server");
            return;
        }

        if (want < lo || want > hi)
        {
            OZR_Log.Dbg("tune refused: index " + want.ToString() + " is outside " + lo.ToString() + ".." + hi.ToString());
            return;
        }

        // Ділення мусить лежати на ґратці САМОГО профілю, а не просто в його
        // межах: інакше рація стане між своїми каналами й не зустріне нікого.
        if (((want - lo) % stride) != 0)
        {
            OZR_Log.Dbg("tune refused: index " + want.ToString() + " is between this set's own channels");
            return;
        }

        // Через OZR_TuneTo, а не SetFrequencyByIndex: він же й розкаже про
        // нову частоту клієнтові. Прямий виклик лишив би її невидимою.
        radio.OZR_TuneTo(want);

        OZR_Log.Dbg("tuned " + radio.GetType() + " to index " + want.ToString() + " = " + OZR_Grid.MHzAt(want).ToString() + " MHz");
    }


    // Кнопка «говорити» на ручних рацій.
    //
    // Клієнт присилає НАМІР -- один біт, і більше нічого. Які саме передавачі
    // від цього відкриються, вирішує сервер, обійшовши інвентар цього гравця:
    // інакше пакет «говорю» став би способом розкрити чужу рацію.
    //
    // Відкриваються ВСІ живі профільні рації, які гравець несе, а не та, що в
    // руках. Так це працює у ванілі -- увімкнена рація везе голос власника
    // звідки завгодно, хоч із дна рюкзака -- і саме так рацією користуються:
    // вмикають, кладуть у розвантаження й лишають руки вільними. Кнопка міняє
    // те, КОЛИ ефір відкритий, а не те, звідки він чути.
    //
    // Ванільних і чужих передавачів обхід не чіпає в обидва боки: їхній ефір
    // цей мод не відкривав, і закривати його теж не його справа.
    void OZR_PttRadio(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server || !sender)
            return;

        Param1<bool> p = new Param1<bool>(false);
        if (!ctx.Read(p))
            return;

        PlayerBase player = OZR_PlayerOf(sender);
        if (!player || !player.GetInventory())
            return;

        int touched = OZR_SetAll(player, p.param1);

        string said = "ptt: " + sender.GetName();
        if (p.param1)
            said += " opens ";
        else
            said += " shuts ";
        OZR_Log.Dbg(said + touched.ToString() + " radio(s)");
    }

    // Скільки передавачів перемкнули. Число повертається не для краси: «нуль»
    // -- це єдине, чим відрізняється «гравець натиснув кнопку без рації» від
    // «пакет не дійшов», і без нього обидва випадки виглядають у лозі однаково.
    private int OZR_SetAll(PlayerBase player, bool on)
    {
        array<EntityAI> items = new array<EntityAI>();
        if (!player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items))
            return 0;

        int touched = 0;
        for (int i = 0; i < items.Count(); i++)
        {
            TransmitterBase t = TransmitterBase.Cast(items[i]);
            if (!t || !OZR_Profiles.For(t.GetType()))
                continue;

            t.OZR_SetSpeaking(on);
            touched++;
        }
        return touched;
    }

    void OZR_GridRes(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param3<float, float, int> p = new Param3<float, float, int>(0, 0, 0);
        if (!ctx.Read(p))
            return;

        OZR_ClientGrid.SetGrid(p.param1, p.param2, p.param3);

        string got = "ether received: " + p.param3.ToString() + " divisions from ";
        got += OZR_Fmt.MHz(p.param1) + " MHz by " + OZR_Fmt.Step(p.param2);
        OZR_Log.Info(got);

        if (m_PullTimer)
            m_PullTimer.Stop();
    }

    void OZR_ProfRes(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param4<string, float, float, float> p = new Param4<string, float, float, float>("", 0, 0, 0);
        if (!ctx.Read(p))
            return;

        OZR_ClientGrid.AddProfile(p.param1, p.param2, p.param3, p.param4);
    }
}
