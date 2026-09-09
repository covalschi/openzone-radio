// Серверна половина ефіру: покласти виведену сітку на диск і сказати, чи вона
// вже діє.
//
// Арифметика живе в 3_Game (OZR_Ether), бо її рахує і вкладка на клієнті. Тут
// лишається те, що має сенс лише на сервері: файл і порівняння з ВИМІРЯНОЮ
// сіткою рушія.
//
// Файл читає нативний патч при старті ПРОЦЕСУ, тому нова сітка вступає в дію
// тільки з наступним запуском сервера. Це сказано вголос -- і в лозі, і у
// вкладці: мовчазна відкладена дія гірша за відсутню.

// Скільки одному клієнтові дозволено питати.
//
// Запит сітки не мав жодної межі: змінений клієнт міг слати його щокадру, а
// сервер на кожен розсилає ефір, гучності й пакет на профіль (одинадцять RPC
// за замовчуванням). Півсекунди -- це вчетверо частіше, ніж шле власний клієнт
// (тяга раз на дві секунди), тобто чесного гравця межа не помічає взагалі.
//
// ПРОМІЖОК ГОДИТЬСЯ ЛИШЕ ІДЕМПОТЕНТНОМУ ЗАПИТОВІ, і саме тому тут лишився
// один рід, а не два.
//
// Такий самий проміжок стояв і на PTT (906c696), і він з'їдав клацання --
// власник почув це 2026-09-09, а до 2475baf, де межі не було, вони грали
// надійно. Довід був: «свій клієнт шле краї, а не стан щокадру, тож
// півсекунди в нього не забирають нічого». Неправда, і неправда рівно про
// краї: натиснути й відпустити швидше за півсекунди -- це звичайне клацання
// гашетки, і ДРУГИЙ його край проміжок відкидав мовчки. Наслідків два, і
// обидва гірші за флуд: сплеск закриття не грав, а ефір лишався ВІДКРИТИМ до
// наступного краю -- мікрофон, якого ніхто не вимикав.
//
// Різниця між двома родами саме в повторюваності. Сітку клієнт тягне сам, раз
// на дві секунди: викинутий запит нічого не коштує, наступний приїде. Край
// PTT не повторюється ніколи -- його або обробили, або втратили назавжди.
// Тому межа на ньому має бути не про ЧАС, а про ЗМІНУ -- див. Changed().
class OZR_Throttle
{
    private static ref map<string, int> s_Last;
    private static const int GAP_MS = 500;

    static bool Allow(PlayerIdentity who, string kind)
    {
        if (!who)
            return false;

        if (!s_Last)
            s_Last = new map<string, int>();

        string key = kind + ":" + who.GetPlainId();
        int now = GetGame().GetTime();

        int was;
        if (s_Last.Find(key, was) && now - was < GAP_MS)
            return false;

        s_Last.Set(key, now);
        return true;
    }

    // ПАКЕТ, ЯКИЙ ПРОСИТЬ ТЕ, ЩО ВЖЕ СТОЇТЬ.
    //
    // Це все, що від межі на PTT лишилось, і все, що на ньому можна відкинути
    // безпечно: край за визначенням МІНЯЄ стан, тож жоден край сюди не
    // потрапить. Свій клієнт дублікатів не шле взагалі (OZR_Ptt.Apply звіряє
    // з посланим), а змінений може слати їх скільки завгодно -- і кожен
    // коштував би обходу всього інвентаря гравця.
    //
    // Стан цілим числом, бо мапа тут одна: 0 -- закрито, 1 -- відкрито
    // клавішею, 2 -- відкрито замком. Замок від утримання відрізняти
    // обов'язково: кинуту рацію сервер лишає говорити лише із замком.
    private static ref map<string, int> s_Ptt;

    static bool Changed(PlayerIdentity who, bool on, bool locked)
    {
        if (!who)
            return false;

        if (!s_Ptt)
            s_Ptt = new map<string, int>();

        // ГІЛКОЮ, А НЕ ЛАНЦЮЖКОМ -- та сама пастка, що в m_OZR_Latched:
        // складене «&&» у цьому рушії вміє мовчки дати не ту відповідь.
        int state = 0;
        if (on)
        {
            state = 1;
            if (locked)
                state = 2;
        }

        string key = who.GetPlainId();

        int was;
        if (s_Ptt.Find(key, was) && was == state)
            return false;

        s_Ptt.Set(key, state);
        return true;
    }

    // Гравець пішов -- його рядки теж. Мапи без цього росли б увесь запуск.
    // За UID, а не за особою: на дисконекті особи вже може не бути, і саме
    // тому CF несе uid окремим полем події.
    //
    // ЧЕРЕЗ Contains, а не голим Remove: рідний map оголошує Remove без слова
    // про відсутній ключ, і сама ваніль про всяк випадок питає перед ним
    // (enscript.c, map.Replace). Дисконект -- поганий шлях, щоб про це
    // дізнатись.
    static void Forget(string uid)
    {
        if (uid == "")
            return;

        string grid = "grid:" + uid;
        if (s_Last && s_Last.Contains(grid))
            s_Last.Remove(grid);

        // Стан гашетки теж: наступний власник цього UID -- нове тіло з
        // закритим ефіром, і пам'ять про чужий замок відкинула б його перше
        // ж натискання.
        if (s_Ptt && s_Ptt.Contains(uid))
            s_Ptt.Remove(uid);
    }
}

class OZR_EtherServer
{
    private static ref OZR_EtherPlan s_Plan;

    // ЩО ЦЕЙ СЕРВЕР ДУМАЄ ПРО ЕФІР -- одному клієнтові.
    //
    // Раніше це був тілом обробника RPC, і саме тому адмінська правка
    // профілів не доїжджала до вже підключених: розіслати те саме було нема
    // чим, а клієнтська тяга зупиняється після першої вдалої сітки. Тепер
    // місце одне, і його кличуть обидва -- запит клієнта й Apply у вкладці.
    static void SendTo(PlayerIdentity who)
    {
        if (!who)
            return;

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
        float gBase  = 0;
        float gStep  = 0;
        int   gCount = 0;

        if (OZR_Grid.Ready())
        {
            gBase  = OZR_Grid.Base();
            gStep  = OZR_Grid.StepMHz();
            gCount = OZR_Grid.Count();
        }
        else
        {
            OZR_Log.Warn("ether asked for, but the engine's table is not an even grid - telling the client there is no ether instead of describing the vanilla eight as one");
        }

        GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_GRID_RES,
            new Param3<float, float, int>(gBase, gStep, gCount),
            true, who);

        // Гучності їдуть тим самим запитом, бо питання те саме: «що цей
        // сервер про ефір думає». Окремим пакетом, а не полями в сітці, --
        // сітка може бути відсутньою, а гучності діють однаково завжди.
        //
        // BOOL-АМИ, А НЕ ЧИСЛАМИ. Два прапорці їхали як float і int, і клієнт
        // порівнював їх із нулем назад -- при тому, що поруч, у пакеті PTT,
        // той самий модуль возить Param2<bool, bool>. Дві різні мови для
        // одного типу в одному файлі -- це запрошення переплутати.
        //
        // П'ЯТИМ ПОЛЕМ ЇДЕ РІВЕНЬ ДІАГНОСТИКИ, і без нього клієнтського лога
        // в цього мода не було ЗОВСІМ. OZR_Log.SetDebug кличеться рівно двічі
        // (OZR_Settings.ServerLoad і OZRP_Module.OnMissionStart), і обидва
        // рази під if (IsServer()) -- отже на клієнті прапорець лишався false
        // назавжди, а кожен OZR_Log.Dbg там був мертвим рядком. Саме на цьому
        // боці й живуть сплески squelch, тобто пояснити пропущене клацання
        // було нічим.
        //
        // Тим самим пакетом, а не своїм: питання те саме -- «що цей сервер
        // про себе каже», -- і другий пакет означав би другий шлях, який може
        // не доїхати окремо.
        //
        // Береться ЖИВИЙ прапорець логера, а не поле st.DebugLog: коли поруч
        // стоїть ядро, склейка @OpenZone_Radio_PDA переставляє наш рівень за
        // ядерним (OZRP_Module.OnMissionStart), і саме за переставленим
        // сервер пише. Клієнт мусить мовчати чи говорити разом із ним, а не
        // за іншим числом.
        OZR_Settings st = OZR_Settings.Get();
        if (st)
        {
            GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_AUDIO_RES,
                new Param5<float, bool, int, bool, bool>(st.SquelchGain, st.MirrorPtt, st.SquelchRange, st.PttFromCargo, OZR_Log.IsDebug()),
                true, who);
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
                true, who);
        }
    }

    // ...і всім, хто вже в грі.
    //
    // Потрібно рівно після адмінської правки профілів: клієнтська тяга сама
    // себе зупиняє на першій же вдалій сітці, тож без цієї розсилки кейпад і
    // PTT в онлайну лишались би зі старими смугами до переспоручення.
    static void Broadcast()
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            if (players[i])
                SendTo(players[i].GetIdentity());
        }

        OZR_Log.Info("ether re-sent to " + players.Count().ToString() + " player(s) after an admin edit");
    }

    // Порахувати й записати. Кличеться і при старті, і після кожної правки
    // профілів: файл, який відстав від профілів, -- це рівно та неузгодженість,
    // заради усунення якої все це й зроблено.
    static void Publish(OZR_Profiles cfg)
    {
        // ПРОФІЛІ, ЯКИХ АДМІН НЕ ПИСАВ, НЕ ВИВОДЯТЬ ЕФІРУ.
        //
        // OZ_Radio_Profiles.json міг не розібратись -- кома не там, редактор
        // обірвав запис, -- і тоді ми стоїмо на вбудованій драбині, а файл
        // лишається на диску недоторканим. Вивести сітку з ЦИХ чисел і
        // покласти її поверх робочого OZ_Radio_Frequencies.json означало б
        // після одного зіпсованого старту втратити всі власні смуги сервера
        // мовчки -- і виявилось би це аж наступним рестартом.
        if (!OZR_Profiles.Writable())
        {
            OZR_Log.Warn("ether not derived: the radio profiles on disk did not parse, so we are on built-in defaults - the frequency file is left as it was, fix OZ_Radio_Profiles.json and restart");
            return;
        }

        array<ref OZR_RadioProfile> radios;
        if (cfg)
            radios = cfg.Radios;

        OZR_EtherPlan plan = OZR_Ether.Derive(radios);
        s_Plan = plan;

        if (!plan.Ok)
        {
            // Файл НЕ чіпаємо. Профілі можуть бути тимчасово безглуздими --
            // адмін посеред правки, -- і затирати цим робочу сітку не можна.
            OZR_Log.Warn("ether not derived: " + plan.Why + " - the frequency file is left as it was");
            return;
        }

        // ПИШЕМО ТЕКСТ САМІ, а не через JsonFileLoader. Він серіалізує те, що
        // лишилось від числа у float32: 0.0125 виходить як
        // 0.012500000186264515, і файл, який має бути джерелом правди про крок,
        // виглядає як помилка. Виведений крок за побудовою кратний 0.0001 МГц
        // (див. UNITS_PER_MHZ), тож чотирьох знаків достатньо рівно завжди.
        string body = "{ \"base_mhz\": " + OZR_Fmt.Fixed(plan.BaseMHz, 4);
        body += ", \"step_mhz\": " + OZR_Fmt.Fixed(plan.StepMHz, 4);
        body += ", \"count\": " + plan.Count.ToString() + " }";

        FileHandle fh = OpenFile(OZR_Const.FREQUENCIES, FileMode.WRITE);
        if (fh == 0)
        {
            OZR_Log.Error("cannot open " + OZR_Const.FREQUENCIES + " for writing");
            return;
        }
        FPrintln(fh, body);
        CloseFile(fh);

        string said = "ether derived from profiles: " + OZR_Ether.Describe(plan);
        if (Matches())
            said += " (in effect)";
        else
            said += " - RESTART THE SERVER to apply; the running ether is still " + Running();

        OZR_Log.Info(said);
    }

    // Чи те, що ми вивели, збігається з тим, що рушій справді роздає зараз.
    // Порівняння і форматування живуть у 3_Game (OZR_Ether): їх робить і
    // вкладка на клієнті, а два підрахунки одного числа розійшлись би тихо.
    static bool Matches()
    {
        if (!OZR_Grid.Ready())
            return false;

        return OZR_Ether.Same(s_Plan, OZR_Grid.Base(), OZR_Grid.StepMHz(), OZR_Grid.Count());
    }

    static string Running()
    {
        if (!OZR_Grid.Ready())
            return "not an even grid";

        return OZR_Ether.DescribeGrid(OZR_Grid.Base(), OZR_Grid.StepMHz(), OZR_Grid.Count());
    }
}
