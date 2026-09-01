// Профіль вступає в дію тут -- у ванільному кроці настройки. І тут же живе
// синхронізація частоти, без якої клієнт її просто не бачить.
//
// Обидві ванільні дії, в руках і на землі, сходяться в TransmitterBase.
// SetNextFrequency, і обидві -- у OnFinishProgressServer, тобто на СЕРВЕРІ.
// Це один вузол на всю гру, і саме тому обмеження живе тут, а не в двох діях і
// не в сторінці КПК: рацію крутять дією, а не вкладкою.
//
// ЧОМУ ІНДЕКС ДОВОДИТЬСЯ ВОЗИТИ САМИМ. Рушій тримає налаштований індекс у
// власному нативному стані предмета, а не в скриптовій змінній. SetSynchDirty
// будить синхронізацію СКРИПТОВИХ змінних -- і до рушійного поля не має
// стосунку. Через це на клієнті лишалося старе число, доки предмет не
// синхронізувався з якоїсь іншої причини: увімкнули-вимкнули рацію -- і
// підпис «стрибав». Тому ми заводимо власну синхрозмінну й кладемо індекс у
// неї щоразу, коли він змінився.
//
// Рація без профілю лишається ванільною в частині КРОКУ, але індекс возить
// однаково: підпис має бути правдивим у будь-якої рації.

modded class TransmitterBase
{
    // -1 -- «сервер ще нічого не сказав». Нуль не годиться: нульовий індекс --
    // це справжній канал, і сплутати «канал 0» з «невідомо» не можна.
    private int m_OZR_Index = -1;

    void TransmitterBase()
    {
        // БЕЗ МЕЖ, і це навмисна відмова від попереднього рішення.
        //
        // Було: RegisterNetSyncVariableInt("m_OZR_Index", -1, INDEX_MAX), де
        // INDEX_MAX = 65535. Задум був чесний -- прив'язати ширину синхронізації
        // до стелі ефіру, щоб сітка не могла стати ширшою за те, що доїде до
        // власника рації. Ціна виявилась іншою, ніж здавалось.
        //
        // Два аргументи -- це КВАНТУВАННЯ: рушій рахує з діапазону кількість
        // біт. Діапазон -1..65535 -- це 65537 значень, тобто СІМНАДЦЯТЬ біт,
        // на один більше за слово. Ваніль такого не робить ніде: найширше, що
        // в неї є, -- 0..9999 у кодовому замку, решта це десятки й сотні, а
        // там, де діапазон не малий (хеші, m_GateState, m_SyncParts01), вона
        // бере саме цю форму -- без меж, повним int.
        //
        // Прив'язку до стелі ефіру це не втрачає, а робить непотрібною: повний
        // int возить будь-яке ділення, яке взагалі може існувати.
        RegisterNetSyncVariableInt("m_OZR_Index");
    }

    // Єдине місце, де частота міняється. Все інше кличе саме його, щоб не
    // лишилось шляху, який змінює індекс і забуває про це сказати.
    void OZR_TuneTo(int index)
    {
        SetFrequencyByIndex(index);
        OZR_Publish();
    }

    void OZR_Publish()
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        m_OZR_Index = GetTunedFrequencyIndex();
        SetSynchDirty();
    }

    // Індекс, якому можна вірити на обох боках. На сервері істина -- рушій;
    // на клієнті -- те, що прислали, і лише поки не прислали, доводиться
    // питати рушій (він там відповість по СВОЇЙ ванільній таблиці, тобто
    // майже напевно неправду -- але це краще за порожнечу).
    int OZR_ShownIndex()
    {
        if (GetGame() && GetGame().IsServer())
            return GetTunedFrequencyIndex();

        if (m_OZR_Index >= 0)
            return m_OZR_Index;

        return GetTunedFrequencyIndex();
    }

    // Чи рація ЖИВА -- увімкнена й живиться. Вимкнена рація не крутиться:
    // ручка на мертвій коробці не має що зрушити, і показувати клавіатуру для
    // неї теж нема сенсу.
    //
    // Питаємо енергоменеджер, а не IsReceiving: приймання вмикається у
    // OnWorkStart, тобто вже НАСЛІДОК живлення, і на клієнті воно може
    // відставати. IsWorking -- це саме «увімкнена й має чим працювати».
    bool OZR_IsPowered()
    {
        ComponentEnergyManager em = GetCompEM();
        if (em)
            return em.IsWorking();

        return IsReceiving() || IsBroadcasting();
    }

    // Наступне ділення в межах відрізка, з обгортанням по КОЛУ ВІДРІЗКА.
    // Обгортання, а не упор: ванільна поведінка -- це цикл, і рація, яка
    // дійшла до краю й перестала крутитись, читалась би як зламана.
    //
    // Межі беруться з OZR_Grid.Window -- тобто вже перетнуті з тим, що рушій
    // роздає ЗАРАЗ. Профіль може випереджати ефір (його щойно правили, а
    // сервер ще не перезапускали), і тоді рація крутиться по спільній частині.
    private int OZR_NextIndex(OZR_RadioProfile p, int current)
    {
        int lo;
        int hi;
        int stride;
        if (!OZR_Grid.Window(p, lo, hi, stride))
            return GetTunedFrequencyIndex();

        if (current < lo || current > hi)
            return lo;

        // Прив'язуємось до ґратки САМОГО профілю, а не до сітки рушія: рація
        // могла стояти між своїми діленнями, якщо профіль щойно змінили.
        float rel = current - lo;
        float st  = stride;
        int   k   = Math.Round(rel / st);

        int next = lo + (k + 1) * stride;
        if (next > hi)
            next = lo;
        return next;
    }

    override void SetNextFrequency(PlayerBase player = NULL)
    {
        // Вимкнену рацію не крутимо взагалі -- ні нашу, ні ванільну.
        if (!OZR_IsPowered())
            return;

        OZR_RadioProfile p = OZR_Profiles.For(GetType());

        // Не наша рація, або сітка не годиться для профілів (непропатчений
        // сервер) -- крок хай буде ванільний, але сказати про нього все одно
        // треба.
        if (!p || !OZR_Grid.Ready())
        {
            super.SetNextFrequency(player);
            OZR_Publish();
            return;
        }

        OZR_TuneTo(OZR_NextIndex(p, GetTunedFrequencyIndex()));
    }

    // Свіжа рація прокидається на нулі сітки, а нуль лежить поза відрізком
    // майже кожного профілю. Гравець узяв би її в руки й побачив частоту, на
    // якій його рація фізично не працює -- тож заводимо її одразу в свою смугу.
    override void EEInit()
    {
        super.EEInit();

        if (!GetGame() || !GetGame().IsServer())
            return;

        OZR_RadioProfile p = OZR_Profiles.For(GetType());
        int lo;
        int hi;
        int stride;
        if (!p || !OZR_Grid.Window(p, lo, hi, stride))
        {
            OZR_Publish();
            return;
        }

        int cur = GetTunedFrequencyIndex();
        if (cur < lo || cur > hi)
            OZR_TuneTo(lo);
        else
            OZR_Publish();
    }

    // Предмет щойно підняли зі збереження -- рушій уже поставив збережений
    // індекс, і про нього теж треба сказати, інакше після рестарту клієнт
    // побачив би не ту частоту, на якій рація насправді стоїть.
    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;

        // Збережений індекс міг пережити сітку, в якій він щось означав.
        //
        // Ефір виводиться з профілів, тож після їх правки й рестарту сітка
        // буває іншою: рація прокидається за межами власної смуги, мовчить, а
        // ванільна ручка від смуги тільки віддаляє -- ваніль крокує +1 і про
        // смуги не знає. EEInit це вміє, але відпрацьовує ДО
        // super.OnStoreLoad, тобто до того, як збережений індекс поставили;
        // для піднятого зі збереження предмета з нього користі немає.
        //
        // Коли сітки нема взагалі, чіпати нічого: рушій візьме
        // table[index & 7], і це справжній ванільний канал.
        OZR_RadioProfile p = OZR_Profiles.For(GetType());
        int lo;
        int hi;
        int stride;
        if (p && OZR_Grid.Window(p, lo, hi, stride))
        {
            int cur = GetTunedFrequencyIndex();
            if (cur < lo || cur > hi)
            {
                string said = GetType() + " woke up on index " + cur.ToString();
                said += ", outside its band " + lo.ToString() + ".." + hi.ToString();
                said += " - brought back to " + lo.ToString();
                OZR_Log.Info(said);

                // OZR_TuneTo сам розкаже клієнтові -- окремий OZR_Publish тут
                // був би другим повідомленням про ту саму зміну.
                OZR_TuneTo(lo);
                return true;
            }
        }

        OZR_Publish();
        return true;
    }

    // --------------------------------------------------------------- PTT
    //
    // Ваніль відкриває передавач разом із живленням: OnWorkStart кличе
    // EnableBroadcast(true), і далі увімкнена рація везе голос власника
    // завжди -- хоч із рюкзака. Для рації з кнопкою «говорити» це рівно
    // навпаки, тож ефір доводиться ЗАКРИВАТИ одразу після ванільного кроку.
    //
    // Тільки для ПРОФІЛЬНИХ рацій. Ванільна PersonalRadio і чужі передавачі
    // лишаються ванільними: міняти поведінку предметів, яких цей мод не
    // робив, значить ламати їх для модів, які на неї розраховують.
    override void OnWorkStart()
    {
        super.OnWorkStart();

        if (!GetGame() || !GetGame().IsServer())
            return;

        if (OZR_Profiles.For(GetType()))
        {
            EnableBroadcast(false);

            // Видно в лозі стенда. Інакше «ефір закритий» -- твердження без
            // жодного спостережуваного наслідку: рація, яка мовчить, і рація,
            // якої гейт не торкнувся, зовні виглядають однаково доти, доки
            // хтось не спробує в неї заговорити.
            OZR_Log.Dbg("ptt gate: " + GetType() + " powered up with the air shut");
        }
    }

    // Клієнт сказав, що гравець тримає (або защіпнув) кнопку.
    //
    // Перевірка живлення тут не зайва, хоч клієнт її вже робив: вимкнути
    // рацію можна між пакетом і його обробкою, а відкритий передавач на
    // знеструмленій коробці -- це стан, якого не буває.
    void OZR_SetSpeaking(bool on)
    {
        if (!OZR_IsPowered())
        {
            EnableBroadcast(false);
            OZR_Log.Dbg("ptt gate: " + GetType() + " asked to speak while dead - refused");
            return;
        }

        EnableBroadcast(on);

        string said = "ptt gate: " + GetType() + " air ";
        if (on)
            said += "OPEN";
        else
            said += "shut";
        OZR_Log.Dbg(said + ", broadcasting=" + IsBroadcasting().ToString());
    }
}
