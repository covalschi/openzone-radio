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
            GetRPCManager().AddRPC(OZR_Const.MOD, OZR_Const.RPC_AUDIO_RES, this, SingleplayerExecutionType.Client);

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
        // Сітку віддаємо, ЛИШЕ якщо вона сітка.
        //
        // Без цієї перевірки сервер описував клієнтові ванільну вісімку як
        // рівну ґратку: база 87.800, "крок" (102.5 - 87.8) / 7 = 2.1000,
        // вісім ділень. Клієнт перевірити рівномірність не може -- йому їдуть
        // три числа, а не таблиця, -- тож він чесно рахував base + i*step для
        // індексів СПРАВЖНЬОЇ сітки. Рація, збережена на індексі 962 (у сітці
        // на 1281 ділення це 148.025 МГц), підписувалась як 2108.000 МГц, а
        // ванільна ручка крокувала її по 2.1 МГц за натиск.
        //
        // Спостережено на живому сервері 2026-09-01: після рестарту не
        // піднявся нативний патч, і рушій роздав ванільну вісімку.
        //
        // Нулі означають "ефіру немає", і кожен споживач на клієнті вже вміє
        // це читати: підпис падає на ванільний, клавіатура не відкривається.
        bool even = OZR_Grid.Ready();
        float gBase  = 0;
        float gStep  = 0;
        int   gCount = 0;
        if (even)
        {
            gBase  = OZR_Grid.MHzAt(0);
            gStep  = OZR_Grid.StepMHz();
            gCount = OZR_Grid.Count();
        }
        else
        {
            OZR_Log.Warn("ether asked for, but the engine's table is not an even grid - telling the client there is no ether instead of describing the vanilla eight as one");
        }

        GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_GRID_RES,
            new Param3<float, float, int>(gBase, gStep, gCount),
            true, sender);

        // Гучності їдуть тим самим запитом, бо питання те саме: «що цей
        // сервер про ефір думає». Окремим пакетом, а не полями в сітці, --
        // сітка може бути відсутньою, а гучності діють однаково завжди.
        OZR_Settings st = OZR_Settings.Get();
        if (st)
        {
            float mirror = 0;
            if (st.MirrorPtt)
                mirror = 1;

            int cargo = 0;
            if (st.PttFromCargo)
                cargo = 1;

            GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_AUDIO_RES,
                new Param4<float, float, int, int>(st.SquelchGain, mirror, st.SquelchRange, cargo),
                true, sender);
        }

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
    // ВІДКРИВАЄТЬСЯ ОДНА РАЦІЯ, а не всі, які гравець несе. Це заміна
    // попереднього рішення, і замінене воно свідомо.
    //
    // Було: відкрити все живе й профільне, бо так робить ваніль -- увімкнена
    // рація везе голос власника хоч із дна рюкзака. Ціна виявилась не тією,
    // на яку розраховували: гравець, що несе три рації на трьох частотах,
    // одним натисканням говорив у три ефіри одразу, і жодного способу
    // сказати «в цю, а не в ту» не було. Ваніль цієї біди не має лише тому,
    // що в неї немає кнопки: там рація везе голос ЗАВЖДИ, і носити три
    // увімкнені рації однаково безглуздо.
    //
    // Правило: та, що В РУКАХ; якщо руки порожні -- та, що НА СЛОТІ й
    // останньою була в руках. Карго не говорить ніколи: рація в рюкзаку --
    // це запасна, а не робоча.
    //
    // ЗАКРИВАЄМО ЗАВЖДИ ВСІ. Несиметрія навмисна: рацію можна прибрати в
    // рюкзак, не відпустивши кнопки, і тоді край відпускання не знайшов би
    // того, кого відкрив край натискання -- передавач лишився б відкритим
    // назавжди. Відкриваємо вузько, закриваємо широко.
    //
    // Ванільних і чужих передавачів обхід не чіпає в обидва боки: їхній ефір
    // цей мод не відкривав, і закривати його теж не його справа.
    void OZR_PttRadio(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Server || !sender)
            return;

        Param2<bool, bool> p = new Param2<bool, bool>(false, false);
        if (!ctx.Read(p))
            return;

        PlayerBase player = OZR_PlayerOf(sender);
        if (!player || !player.GetInventory())
            return;

        int touched = OZR_SetAll(player, p.param1, p.param2);

        string said = "ptt: " + sender.GetName();
        if (p.param1)
            said += " opens ";
        else
            said += " shuts ";
        said += touched.ToString() + " radio(s)";
        if (p.param2)
            said += " (latched)";
        OZR_Log.Dbg(said);
    }

    // Скільки передавачів перемкнули. Число повертається не для краси: «нуль»
    // -- це єдине, чим відрізняється «гравець натиснув кнопку без рації» від
    // «пакет не дійшов», і без нього обидва випадки виглядають у лозі однаково.
    private int OZR_SetAll(PlayerBase player, bool on, bool locked)
    {
        array<EntityAI> items = new array<EntityAI>();
        if (!player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items))
            return 0;

        // Закриття не вибирає: усе, що цей мод міг відкрити, він і закриває.
        if (!on)
        {
            int shut = 0;
            for (int j = 0; j < items.Count(); j++)
            {
                TransmitterBase c = TransmitterBase.Cast(items[j]);
                if (!c || !OZR_Profiles.For(c.GetType()))
                    continue;

                c.OZR_SetSpeaking(false, false);
                shut++;
            }
            return shut;
        }

        TransmitterBase pick = OZR_PickSpeaker(player, items);
        if (!pick)
            return 0;

        pick.OZR_SetSpeaking(true, locked);
        return 1;
    }

    // Кого саме відкрити.
    //
    // Руки за все: рація в долоні -- це вже сказане вголос «говорю в цю», і
    // жодне інше правило не мусить це перебивати.
    //
    // Далі -- слоти, і серед них найсвіжіша за часом останнього тримання.
    // Час веде саме тому, що місце не розрізняє: дві рації на двох слотах
    // виглядають однаково, а діставали одну.
    //
    // Порожня пам'ять -- окремий випадок, і мовчанням він закінчуватись не
    // може. Після рестарту сервера жодна рація в руках ще не була, і правило
    // «остання в руках» не має відповіді взагалі; відмовити тут означало б
    // зробити кнопку мертвою до першого перекладання. Тому без пам'яті
    // говорить перша ж рація на слоті.
    private TransmitterBase OZR_PickSpeaker(PlayerBase player, array<EntityAI> items)
    {
        bool cargo = false;
        OZR_Settings st = OZR_Settings.Get();
        if (st)
            cargo = st.PttFromCargo;

        TransmitterBase best     = null;
        int             bestRank = 0;
        int             latest   = 0;

        for (int i = 0; i < items.Count(); i++)
        {
            TransmitterBase t = TransmitterBase.Cast(items[i]);
            if (!t || !OZR_Profiles.For(t.GetType()))
                continue;

            int rank = t.OZR_SpeakRank(cargo);
            if (rank == 0)
                continue;

            // Руки б'ють усе й закінчують пошук: рація в долоні -- це вже
            // сказане вголос «говорю в цю».
            if (rank == 3)
                return t;

            int held = t.OZR_HeldAt();

            // Спершу за місцем, і лише в межах одного місця -- за свіжістю.
            // Надіта рація б'є ту, що в рюкзаку, навіть якщо рюкзачну
            // тримали пізніше: місце -- це намір, а час лише розводить
            // однакові.
            if (rank > bestRank || (rank == bestRank && held > latest))
            {
                best     = t;
                bestRank = rank;
                latest   = held;
            }
        }

        return best;
    }

    void OZR_GridRes(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param3<float, float, int> p = new Param3<float, float, int>(0, 0, 0);
        if (!ctx.Read(p))
            return;

        OZR_ClientGrid.SetGrid(p.param1, p.param2, p.param3);

        if (OZR_ClientGrid.Ready())
        {
            string got = "ether received: " + p.param3.ToString() + " divisions from ";
            got += OZR_Fmt.MHz(p.param1) + " MHz by " + OZR_Fmt.Step(p.param2);
            OZR_Log.Info(got);
        }
        else
        {
            // Не збій зв'язку, а відповідь: сервер каже, що ефіру немає.
            // Сказати це прямо треба тому, що зовні воно виглядає точно так
            // само, як пакет, який не доїхав.
            OZR_Log.Warn("no ether: the server reports no even frequency grid - radios stay vanilla and the keypad will not open");
        }

        if (m_PullTimer)
            m_PullTimer.Stop();
    }

    void OZR_AudioRes(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
    {
        if (type != CallType.Client)
            return;

        Param4<float, float, int, int> p = new Param4<float, float, int, int>(1.0, 1.0, OZR_Const.SQUELCH_RANGE_DEFAULT, 0);
        if (!ctx.Read(p))
            return;

        OZR_Audio.SetGains(p.param1, p.param2, p.param3, p.param4);

        string got = "audio: squelch x" + p.param1.ToString();
        got += " within " + OZR_Audio.SquelchRung().ToString() + " m";
        got += ", mirror ptt onto the voice key = " + p.param2.ToString();
        got += ", ptt from cargo = " + p.param4.ToString();
        OZR_Log.Info(got);
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
