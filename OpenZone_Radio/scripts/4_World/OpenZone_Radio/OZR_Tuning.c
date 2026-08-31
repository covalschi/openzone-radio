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
        RegisterNetSyncVariableInt("m_OZR_Index", -1, 65535);
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

    // Крок профілю в діленнях сітки. Одиниця -- найдрібніше, що буває.
    private int OZR_Stride(OZR_RadioProfile p)
    {
        float gs = OZR_Grid.StepMHz();
        if (gs <= 0)
            return 1;

        int stride = Math.Round(p.StepMHz / gs);
        if (stride < 1)
            stride = 1;
        return stride;
    }

    // Наступне ділення в межах відрізка, з обгортанням по КОЛУ ВІДРІЗКА.
    // Обгортання, а не упор: ванільна поведінка -- це цикл, і рація, яка
    // дійшла до краю й перестала крутитись, читалась би як зламана.
    private int OZR_NextIndex(OZR_RadioProfile p, int current)
    {
        int lo     = OZR_Grid.IndexOf(p.MinMHz);
        int hi     = OZR_Grid.IndexOf(p.MaxMHz);
        int stride = OZR_Stride(p);

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
        if (!p || !OZR_Grid.Ready())
        {
            OZR_Publish();
            return;
        }

        int lo  = OZR_Grid.IndexOf(p.MinMHz);
        int hi  = OZR_Grid.IndexOf(p.MaxMHz);
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

        OZR_Publish();
        return true;
    }
}
