// Плата рації як предмет.
//
// ЧОМУ ВОНА -- ПЕРЕДАВАЧ, а не звичайний модуль. Голос по рації веде сам
// рушій: він шукає в гравця предмет-передавач і дивиться, на що той
// налаштований. Скриптом голос нікуди не перекласти -- тому пристрій, який
// має звучати, мусить БУТИ передавачем, а не вдавати його.
//
// Тип предмета рушій бере з ІЄРАРХІЇ СКРИПТОВОГО КЛАСУ, а не з конфіга:
// ванільний Transmitter_Base у config.cpp -- порожній Inventory_Base зі
// scope=0, і передавачем PersonalRadio робить саме те, що його скриптовий
// клас походить від TransmitterBase. Тому в конфізі плата лишається звичайним
// модулем відсіку, а передавачем стає тут.
//
// Власного живлення в плати НЕМАЄ. Ванільна рація вмикається своїм
// EnergyManager і в OnWorkStart сама вмикає прийом і передачу; у нас живить
// КПК, а вмикає -- сторінка. Тому подіїживлення перекриті порожніми: без
// цього перший же дотик до GetCompEM() пішов би в нікуди.

class OZ_Module_Radio extends TransmitterBase
{
    override void OnSwitchOn() { }
    override void OnWorkStart() { }
    override void OnWorkStop()  { }

    // Чи тримають зараз PTT. Тримаємо ЦЕ, а не питаємо рушій: живлення
    // звіряється подіями, і без власної пам'яті звірка або затикала б людину
    // посеред фрази, або лишала б рот відкритим.
    private bool m_Speaking;

    // ПЛАТИ, ЯКІ ЦЕЙ МОД УВІМКНУВ. Реєстр живих, а не всіх.
    //
    // Потрібен рівно для одного питання, на яке подій НЕ БУВАЄ: КПК
    // знеструмився, а плата лишилась у відсіку. Договір заліза КПК дає
    // вставляння, виймання й тік «поки прилад працює» -- і жодного слова про
    // те, що прилад працювати перестав (тік просто зупиняється, а зупинку
    // спостерігати нема чим). Тому звірка лишається, але дивиться вона на
    // цей список, а не на весь онлайн: живих плат на сервері одиниці, тоді як
    // гравців сотні.
    //
    // Посилання СЛАБКІ (array без ref): реєстр не має продовжувати життя
    // предмета. Знятись із нього -- обов'язок самої плати, і робиться це і в
    // OZR_Wake(false), і в EEDelete, як у ванільного Land_Underground_Panel.
    private static ref array<OZ_Module_Radio> s_Live;

    static array<OZ_Module_Radio> OZR_LiveBoards()
    {
        if (!s_Live)
            s_Live = new array<OZ_Module_Radio>();
        return s_Live;
    }

    // Скільки нативних перемикань цей мод справді зробив. Лічильник, а не
    // рядок на кожен виклик: питання «чи мовчить цей шлях у спокої» має
    // числову відповідь, і саме її забирає звірка раз на хвилину.
    private static int s_Natives = 0;

    static int OZR_NativeCalls()
    {
        return s_Natives;
    }

    override void EEDelete(EntityAI parent)
    {
        OZR_Forget();
        super.EEDelete(parent);
    }

    private void OZR_Remember()
    {
        array<OZ_Module_Radio> live = OZR_LiveBoards();
        if (live.Find(this) < 0)
            live.Insert(this);
    }

    private void OZR_Forget()
    {
        if (!s_Live)
            return;

        int at = s_Live.Find(this);
        if (at >= 0)
            s_Live.Remove(at);
    }

    // Плата пішла з відсіку -- мовчить одразу, не чекаючи звірки.
    //
    // Про КПК тут не сказано жодного слова навмисно: мод рації про нього не
    // знає. Достатньо факту «я більше не причеплена»: рація, яка лежить у
    // рюкзаку сама по собі, живлення не має нізвідки.
    override void EEItemLocationChanged(notnull InventoryLocation oldLoc, notnull InventoryLocation newLoc)
    {
        super.EEItemLocationChanged(oldLoc, newLoc);

        if (!GetGame() || !GetGame().IsServer())
            return;

        if (newLoc.GetType() != InventoryLocationType.ATTACHMENT)
            OZR_Wake(false);
    }

    // Приймач вмикається разом із живленням КПК і слухає постійно.
    // Передавач -- лише поки тримають PTT.
    //
    // Рот закритий ЗА ЗАМОВЧУВАННЯМ, і це не перестраховка: увімкнена плата
    // сама починає віщати -- зміряно на стенді, де КПК показував ON AIR, хоч
    // клавіші ніхто не торкався. Рація, яка передає все, що ти кажеш, поки ти
    // цього не просив, видає своїх власників швидше за будь-яку засідку.
    //
    // ПИТАЄМО, ПЕРШ НІЖ ПИСАТИ. Раніше всі три нативні виклики йшли
    // безумовно, і на звірці раз на дві секунди це були три записи в рушій на
    // кожну плату за тик -- при тому, що змінюється тут майже ніколи нічого.
    // Тепер виклик робиться лише на РІЗНИЦІ, і лічильник вище рахує саме їх.
    void OZR_Wake(bool on)
    {
        if (IsOn() != on)
        {
            SwitchOn(on);
            s_Natives++;
        }

        if (IsReceiving() != on)
        {
            EnableReceive(on);
            s_Natives++;
        }

        // Знеструмлений КПК закриває рот тим самим шляхом, що й клавіша: так
        // синхрозмінна ефіру й защіпка гаснуть разом із рушійним бітом.
        if (!on)
        {
            if (m_Speaking)
                OZR_SetSpeaking(false, false);
            m_Speaking = false;

            if (IsBroadcasting())
            {
                EnableBroadcast(false);
                s_Natives++;
            }

            OZR_Forget();
            return;
        }

        if (IsBroadcasting() != m_Speaking)
        {
            EnableBroadcast(m_Speaking);
            s_Natives++;

            // Рядок лише на КРАЮ й лише поки говорять: саме цей шлях до D96
            // закривав ефір мовчки.
            if (m_Speaking)
                OZR_Log.Dbg("board: air re-opened by the power check - broadcasting=" + IsBroadcasting().ToString());
        }

        OZR_Remember();
    }

    // ЄДИНИЙ вхід для «говорити» -- той самий, що в ручної рації (D96).
    //
    // Раніше PTT ішов через OZR_SetSpeaking модованого TransmitterBase і
    // m_Speaking не торкався, а звірка живлення КПК раз на дві секунди кликала
    // OZR_Wake, який ставив EnableBroadcast(on && m_Speaking) -- тобто
    // закривав ефір посеред фрази. Плата замовкала через дві секунди
    // утримання, і жодного рядка про це ніде не було.
    override bool OZR_SetSpeaking(bool on, bool locked)
    {
        bool did = super.OZR_SetSpeaking(on, locked);

        // ГІЛКОЮ, А НЕ ЛАНЦЮЖКОМ -- та сама пастка, що й у m_OZR_Latched
        // поруч: «&&», покладене прямо в поле об'єкта на купі, або псує
        // купу, або тихо дає false при двох істинних операндах. Тут ціна
        // брехні -- плата, яка замовкає посеред фрази на наступній звірці
        // живлення, тобто рівно та вада, заради якої m_Speaking і завели.
        m_Speaking = false;
        if (did)
            m_Speaking = on;

        return did;
    }
}
