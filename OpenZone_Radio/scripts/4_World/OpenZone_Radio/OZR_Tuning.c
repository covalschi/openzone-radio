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

    // Чи відкритий ефір. Синхронізується, бо squelch мусять чути ВСІ поруч.
    private bool m_OZR_Air = false;

    // Чим саме він відкритий: утриманням клавіші чи защіпкою.
    //
    // Різниця не косметична. Кинута рація з УТРИМАННЯМ мусить замовкнути --
    // край відпускання по неї вже не прийде. Кинута рація із ЗАЩІПКОЮ мусить
    // говорити далі: замок ставлять навмисне, і підкинута відкрита рація --
    // це інструмент, а не недогляд (рішення власника 2026-09-02).
    private bool m_OZR_Latched = false;

    // Що клієнт уже намалював звуком. Порівнюємо з m_OZR_Air, щоб зіграти на
    // КРАЮ, а не щоразу, коли предмет синхронізувався з якоїсь іншої причини.
    private bool m_OZR_AirHeard = false;

    // Коли рація востаннє була в руках. Час рушія, серверний бік.
    //
    // Нуль означає «за цю сесію не тримали». Це не те саме, що «не носять»:
    // після рестарту сервера пам'ять порожня в усіх, і PTT не має через це
    // замовкнути -- див. OZR_Module.OZR_SetAll.
    private int m_OZR_HeldAt = 0;

    // «Це моя робоча рація» -- біт, який ПЕРЕЖИВАЄ вихід і рестарт.
    //
    // Ставиться, коли рація потрапляє в руки, і в ту саму мить знімається з
    // усіх інших рацій цього гравця: робоча одна.
    //
    // Возиться в двох напрямках, і обидва потрібні.
    //
    // КЛІЄНТУ -- бо рація, якої не вибирали, не говорить зовсім, і іконка PTT
    // лишається єдиним способом дізнатись про це до натискання. Вивести біт
    // сам клієнт не може: m_OZR_HeldAt серверний і на клієнті вічно нуль.
    //
    // У ЗБЕРЕЖЕННЯ -- бо без цього правило «не тримав, не говориш» означало б
    // мертву гашетку після КОЖНОГО входу в гру, а не лише після рестарту
    // сервера: предмети перестворюються при кожному конекті (видно по
    // "PlayerBase OnStoreLoad SUCCESS" у серверному лозі), і поле в пам'яті
    // не переживає цього.
    //
    // Зберігається саме БІТ, а не час. m_OZR_HeldAt -- це GetGame().GetTime(),
    // відлік від старту місії; числа з різних сесій незрівнянні, і зберігати
    // їх означало б порівнювати вчорашній полудень із сьогоднішнім ранком.
    private bool m_OZR_Chosen = false;

    // Не ref: EffectSound належить SEffectManager, і ваніль тримає свій
    // m_SoundLoop так само (transmitterbase.c:7).
    private EffectSound m_OZR_Squelch;

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

        // Ефір їде окремим бітом, і возити його доводиться з тієї ж причини,
        // що й індекс: EnableBroadcast -- рушійний стан, і SetSynchDirty про
        // нього нічого не знає. Без цього біта squelch чув би лише той, хто
        // натиснув, бо звук грає КЛІЄНТ, а рішення приймає сервер.
        RegisterNetSyncVariableBool("m_OZR_Air");
        RegisterNetSyncVariableBool("m_OZR_Chosen");
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

    // Рація потрапила в руки -- запам'ятовуємо КОЛИ.
    //
    // Гравець носить кілька рацій, а говорить в одну: ту, яку діставав. Місце
    // саме по собі цього не каже -- дві рації на двох слотах виглядають
    // однаково, -- тому потрібен час, а не лише розташування.
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);

        if (!GetGame() || !GetGame().IsServer())
            return;

        if (newLoc.GetType() == InventoryLocationType.HANDS)
        {
            m_OZR_HeldAt = GetGame().GetTime();
            OZR_ClaimFrom(GetHierarchyRootPlayer());
        }

        // Рація змінила місце з відкритим ефіром -- закриваємо, якщо його
        // тримала КЛАВІША, і лишаємо, якщо замок.
        //
        // Чому це взагалі тут. Край відпускання обходить інвентар ГРАВЦЯ, а
        // рації, яку щойно кинули, там уже немає -- отже закрити її нема кому,
        // і вона лишалась би відкритою на землі назавжди, транслюючи все, що
        // скажуть поруч. Відкритий мікрофон, якого ніхто не вимкне. Саме від
        // цього PTT і заводили, тож дірка була в найгіршому місці.
        //
        // Із замком це, навпаки, робоча поведінка: підкинути відкриту рацію --
        // намір, а не помилка.
        if (m_OZR_Air && !m_OZR_Latched)
        {
            EnableBroadcast(false);
            OZR_PublishAir(false);
            OZR_Log.Dbg("ptt gate: " + GetType() + " left its place with the air open on a held key - shut");
        }
    }

    int OZR_HeldAt()
    {
        return m_OZR_HeldAt;
    }

    bool OZR_Chosen()
    {
        return m_OZR_Chosen;
    }

    // Чи відкритий ефір -- ФАКТ, а не намір. Синхронізується заради squelch,
    // тож на клієнті це справжній стан передавача, а не те, що клієнт про
    // нього думає.
    bool OZR_AirOpen()
    {
        return m_OZR_Air;
    }

    void OZR_SetChosen(bool on)
    {
        if (m_OZR_Chosen == on)
            return;

        m_OZR_Chosen = on;
        SetSynchDirty();
    }

    // Наскільки ця рація «на видноті» -- більше значить ближче до рук.
    //
    // Одне місце на обидва боки: сервер вирішує, кого відкрити, а клієнт
    // малює іконку, і розходження між ними -- це кнопка, яка світиться й
    // нічого не робить.
    //
    // 3 руки, 2 слот, 1 карго, 0 не тут. Карго повертає нуль, коли сервер
    // його не дозволив: тоді рація в рюкзаку не існує для гашетки взагалі.
    int OZR_SpeakRank(bool cargoAllowed)
    {
        if (!GetInventory())
            return 0;

        InventoryLocation loc = new InventoryLocation;
        if (!GetInventory().GetCurrentInventoryLocation(loc))
            return 0;

        int kind = loc.GetType();

        if (kind == InventoryLocationType.HANDS)
            return 3;

        if (kind == InventoryLocationType.ATTACHMENT)
            return 2;

        if (kind == InventoryLocationType.CARGO && cargoAllowed)
            return 1;

        return 0;
    }

    // Стати єдиною робочою рацією цього гравця.
    //
    // Обхід інвентаря коштує тут дешево: він трапляється, лише коли рацію
    // беруть у руки -- кілька разів за сесію, -- і всередині це каст на
    // предмет, а дорогий OZR_Profiles.For питається тільки в передатчиків,
    // яких у гравця одиниці. Для масштабу: той самий обхід клієнт робить
    // п'ять разів на СЕКУНДУ, поки тримають гашетку.
    private void OZR_ClaimFrom(Man owner)
    {
        OZR_SetChosen(true);

        if (!owner || !owner.GetInventory())
            return;

        array<EntityAI> items = new array<EntityAI>();
        if (!owner.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items))
            return;

        for (int i = 0; i < items.Count(); i++)
        {
            TransmitterBase t = TransmitterBase.Cast(items[i]);
            if (!t || t == this)
                continue;

            if (!OZR_Profiles.For(t.GetType()))
                continue;

            t.OZR_SetChosen(false);
        }
    }

    // ------------------------------------------------------- persistence
    //
    // Через CF_ModStorage, а не власним OnStoreSave: свій запис у потік
    // ламає предмети, збережені попередньою версією мода, бо ctx.Read на
    // полі, якого в них немає, обриває читання. ModStorage несе кожен мод
    // окремим блоком і чужі блоки береже, тож ані додавання поля, ані зняття
    // мода з сервера не псує сейв.
    //
    // ЦІНА, і вона назавжди: ім'я класу в CfgMods стає частиною формату
    // збереження. CF мітить блок парою хешів цього імені (name.Hash() і
    // name.Reverse().Hash(), modstructure.c:53); перейменування дає інші
    // хеші, старий блок стає «unknown<a,b>» і поле читається умовчанням.
    // Байти при цьому цілі -- CF переписує чужі блоки назад (m_UnloadedMods),
    // -- але дістати їх можна лише повернувши старе ім'я.
    override void CF_OnStoreSave(CF_ModStorageMap storage)
    {
        super.CF_OnStoreSave(storage);

        CF_ModStorage mine = storage.Get(OZR_Const.MOD_CLASS);
        if (!mine)
            return;

        mine.Write(m_OZR_Chosen);

        // Ефір теж, і лише коли він тримається ЗАЩІПКОЮ.
        //
        // Утримання зберігати нічого: клавішу ніхто не тримає крізь
        // перезапуск сервера, і рація, що повернулась би вести передачу через
        // натиснуту вчора кнопку, -- це не відновлений стан, а вигаданий.
        // Замок навпаки: його ставлять, щоб лишити, і підкинута відкрита
        // рація мусить пережити рестарт, інакше «жучок» живе до першого
        // планового перезапуску й ні для чого не годиться.
        bool keptOpen = m_OZR_Air && m_OZR_Latched;
        mine.Write(keptOpen);
    }

    override bool CF_OnStoreLoad(CF_ModStorageMap storage)
    {
        if (!super.CF_OnStoreLoad(storage))
            return false;

        CF_ModStorage mine = storage.Get(OZR_Const.MOD_CLASS);
        if (!mine)
            return true;

        // Читання, що не вдалось, -- не привід валити предмет: рація,
        // збережена до появи цього поля, просто прийде невибраною, і гравець
        // вибере її рукою. Втрата тут -- один жест, а не рація.
        if (!mine.Read(m_OZR_Chosen))
            m_OZR_Chosen = false;

        bool wasOpen = false;
        if (!mine.Read(wasOpen))
            wasOpen = false;

        // ВІДКЛАДЕНО НА КАДР, і це не обережність, а порядок подій.
        //
        // Живлення відновлюється РАНІШЕ за наш блок: SwitchOn() робиться у
        // ванільному EntityAI.OnStoreLoad, тобто всередині super, а CF читає
        // мод-хранилище вже після нього. Отже OnWorkStart устигає закрити
        // ефір (це наше ж правило: рація прокидається слухаючою) ще до того,
        // як ми дізнаємось, що він був відкритий. Ставити біт тут означало б
        // сперечатися з подією, яка вже відбулася.
        //
        // Нуль мілісекунд -- «наступний кадр»: предмет на той час зібраний
        // цілком, з батареєю й енергоменеджером.
        if (wasOpen)
            GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(OZR_RestoreAir, 0, false);

        return true;
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
    void OZR_SetSpeaking(bool on, bool locked)
    {
        if (!OZR_IsPowered())
        {
            EnableBroadcast(false);
            OZR_PublishAir(false);
            m_OZR_Latched = false;
            OZR_Log.Dbg("ptt gate: " + GetType() + " asked to speak while dead - refused");
            return;
        }

        m_OZR_Latched = on && locked;

        EnableBroadcast(on);
        OZR_PublishAir(on);

        string said = "ptt gate: " + GetType() + " air ";
        if (on)
            said += "OPEN";
        else
            said += "shut";
        OZR_Log.Dbg(said + ", broadcasting=" + IsBroadcasting().ToString());
    }

    // Повернути ефір, який пережив перезапуск.
    //
    // Тільки для защіпки -- інше сюди й не збережеться. Живлення питаємо
    // заново: батарея могла сісти, поки сервер лежав, і відкритий передавач
    // на мертвій коробці -- стан, якого не буває.
    //
    // m_OZR_Latched теж ставимо: без нього перше ж переміщення рації закрило
    // б ефір за правилом «кинули на утриманні», тобто підкинутий жучок
    // замовк би, щойно його хтось зрушив.
    private void OZR_RestoreAir()
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        if (!OZR_IsPowered())
        {
            OZR_Log.Dbg("ptt gate: " + GetType() + " came back with a latched air but no power - left shut");
            return;
        }

        m_OZR_Latched = true;
        EnableBroadcast(true);
        OZR_PublishAir(true);

        OZR_Log.Info(GetType() + " came back still transmitting - the latch survived the restart");
    }

    // ------------------------------------------------------------- squelch
    //
    // Сплеск статики на відкриття й на закриття ефіру -- те, чим справжня
    // рація видає себе сусідам, і єдиний спосіб ПОЧУТИ, що поруч хтось
    // вийшов в ефір, не бачачи його екрана.
    //
    // Звук грає на самій рації через SEffectManager.PlaySoundOnObject, тобто
    // тривимірно, з її позиції. Тому його чують УСІ, хто поруч, а не власник
    // у навушниках: кожен клієнт, у чиєму пузирі ця рація є, програє його сам.
    //
    // Саме тому рішення й возиться бітом. Якби squelch грали там, де
    // натиснули кнопку, його чув би рівно одна людина -- та, якій він і так
    // не потрібен.
    void OZR_PublishAir(bool on)
    {
        if (!GetGame() || !GetGame().IsServer())
            return;

        if (m_OZR_Air == on)
            return;

        m_OZR_Air = on;
        SetSynchDirty();
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();

        if (m_OZR_Air == m_OZR_AirHeard)
            return;

        bool opening = m_OZR_Air;
        m_OZR_AirHeard = m_OZR_Air;
        OZR_Squelch(opening);
    }

    // Власний семпл, власний рівень -- див. CfgSoundShaders у config.cpp.
    //
    // Одноразово, без таймера: набір не зациклений, тож рушій сам зупинить
    // звук на кінці семпла й прибере його (SetAutodestroy у PlaySoundSet).
    // Раніше тут грав зациклений ванільний шип, який доводилось обривати
    // через CallLater -- і та довжина була нашою вигадкою, а не властивістю
    // звуку.
    private void OZR_Squelch(bool opening)
    {
        if (!GetGame() || GetGame().IsDedicatedServer())
            return;

        // IsWorking(), а не IsSwitchedOn(): у вимкненої коробки ефіру не
        // буває, і сплеск від неї був би звуком події, якої не сталося.
        if (!OZR_IsPowered())
            return;

        // Набір вибирається за НАПРЯМОМ, а не за станом: до цього рядка
        // m_OZR_Air уже новий, і питати його вдруге означало б питати те
        // саме двічі й отримати правильну відповідь випадково.
        // НЕ "set": так зветься контейнер рушія, і змінна з таким іменем не
        // компілюється -- "Variable name 'set' already used as type name".
        // Той самий рід пастки, що вже ловив Debug і Step у цьому ж моді.
        string soundSet = OZR_Const.SquelchSet(OZR_Audio.SquelchRung(), opening);

        if (!PlaySoundSet(m_OZR_Squelch, soundSet, 0, 0, false))
        {
            // Рівень Info, і лише на ВІДМОВІ. Набір, якого рушій не знайшов,
            // і набір, який просто тихий, зовні однакові -- обидва мовчать, --
            // а лікуються протилежно. Один рядок раз на сесію того вартий.
            OZR_Log.Warn("squelch: the engine would not play " + soundSet + " - check CfgSoundSets and requiredAddons");
            return;
        }

        // Гучність ставимо ПІСЛЯ створення: набір ванільний, і його власний
        // рівень підібраний під тихий фоновий шум увімкненої рації, а не під
        // короткий сплеск, який мусить бути помітним. Множник приходить із
        // сервера, бо це питання балансу, а не вух.
        if (m_OZR_Squelch)
            m_OZR_Squelch.SetSoundVolume(OZR_Audio.SquelchGain());
    }
}
