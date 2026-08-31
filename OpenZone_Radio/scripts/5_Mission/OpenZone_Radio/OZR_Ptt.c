// Кнопка «говорити».
//
// Одна клавіша на ВСІ рації гравця -- ручні й ту, що всередині КПК.
//
// Списку рацій цей файл не має й не потребує. Він шле серверу один біт наміру,
// а сервер обходить інвентар гравця й відкриває кожну ПРОФІЛЬНУ рацію, яку там
// знайде -- байдуже, в руках вона, в рюкзаку чи у відсіку КПК.
//
// Через це тут немає ані згадки про КПК, ані слухачів, які тут якийсь час
// стояли. Точка розширення виявилась іншою й кращою: мод, що хоче свій
// передавач, оголошує йому ПРОФІЛЬ -- і ця клавіша починає його відкривати,
// нікого ні про що не питаючи.
//
// ЧОМУ РУЧНІЙ РАЦІЇ ВЗАГАЛІ ПОТРІБЕН PTT. У ванілі його немає: TransmitterBase
// .OnWorkStart відкриває передавач разом із живленням (EnableBroadcast(true)),
// і далі увімкнена рація везе твій голос завжди -- хоч у руках, хоч у рюкзаку.
// Тому «додати PTT» -- це не надбудова над ванільною поведінкою, а заміна їй:
// ефір тримається закритим, і клавіша його відкриває. Робиться це лише для
// ПРОФІЛЬНИХ рацій (див. OZR_Tuning.OnWorkStart), щоб чужі й ванільні
// лишились такими, якими їх зробили їхні автори.
//
// РАЦІЯ НЕ МУСИТЬ БУТИ В РУКАХ. Ваніль возить голос через КОЖНУ увімкнену
// рацію, яку гравець несе, хоч із дна рюкзака -- і саме так рацією і
// користуються: вмикають, кладуть у розвантаження й лишають руки вільними.
// Тому кнопка відкриває весь набір, а не ту коробку, що зараз у долоні.
//
// ЗАЩІПКА. Одинарне натиснення -- утримання, як і було. Друге натиснення
// протягом DOUBLE_MS від першого -- замок: рація говорить далі й після того,
// як клавішу відпустили. Будь-яке наступне натиснення замок знімає.
//
// Замок -- саме те, чого перша версія цього файлу боялась вголос: відкритий
// передавач, залишений без нагляду, видає своїх власників. Тому він не мовчазний
// -- поки він стоїть, у HUD горить окремим кольором іконка (OZR_PttHud).
//
// На сервер їдуть КРАЇ, а не стан щокадру: сорок пакетів на секунду про те,
// що нічого не змінилось, -- це не PTT, а флуд. Який саме передавач відкрити,
// сервер вирішує сам, тож підміна рації посеред розмови нічого тут не ламає.

class OZR_Ptt
{
    // БЕЗ ref: UAIDWrapper -- нативний об'єкт із приватним деструктором, і
    // скрипт ним не володіє. Ваніль тримає його так само (radialmenu.c:31).
    private static UAIDWrapper s_Key;
    private static bool s_Down    = false;
    private static bool s_Warned  = false;

    // Защіпка й вимір подвійного натиснення.
    private static bool s_Latched   = false;
    private static int  s_LastPress = 0;
    private static const int DOUBLE_MS = 400;

    // Що вже сказано серверові.
    private static bool s_Sent = false;

    // Обхід інвентаря коштує помітно більше за читання клавіші, а відповідь
    // «чи є жива рація» між кадрами не міняється. Тому він робиться п'ять
    // разів на секунду й ЛИШЕ поки кнопку тримають: у спокої іконки немає, і
    // питати нема про що.
    private static bool s_HasRadio  = false;
    private static int  s_LookedAt  = 0;
    private static const int LOOK_EVERY_MS = 200;

    static void Init()
    {
        UAInput i = GetUApi().GetInputByName(OZR_Const.INPUT_PTT);
        if (!i)
        {
            if (!s_Warned)
            {
                s_Warned = true;
                OZR_Log.Error("input " + OZR_Const.INPUT_PTT + " not found - check the CfgMods inputs= path and the name in inputs.xml");
            }
            return;
        }

        s_Key = i.GetPersistentWrapper();
        OZR_Log.Dbg("input " + OZR_Const.INPUT_PTT + " bound");
    }

    static void Poll()
    {
        if (!s_Key)
            return;

        // Сам UAInput кешувати НЕ можна: ваніль тримає обгортку й перечитує
        // вказівник щокадру (radialmenu.c, actiontargets). Робимо так само.
        UAInput i = s_Key.InputP();
        if (!i)
            return;

        bool down = i.LocalValue() > 0;
        if (down != s_Down)
        {
            s_Down = down;

            if (down)
                Pressed();
        }

        // Перерахунок щокадру, але НЕ відправка: рація могла вимкнутись,
        // випасти з рук чи змінитись без жодного натиснення.
        Apply();
    }

    // Клавішу могли тримати в мить, коли КПК закрили або гравець помер. Край
    // відпускання тоді не прийде ніколи, тому мовчання доводиться вмикати
    // самим -- інакше передавач лишиться відкритим назавжди. Защіпку знімаємо
    // з тієї ж причини, і вона тут головна: вона переживе що завгодно.
    static void Drop()
    {
        s_Latched   = false;
        s_LastPress = 0;

        if (s_Sent)
            Send(false);

        s_Sent      = false;
        s_HasRadio  = false;
        s_LookedAt  = 0;

        OZR_PttHud.Drop();
    }

    private static void Pressed()
    {
        int now = GetGame().GetTime();

        // Замок стоїть -- будь-яке натиснення його знімає, і подвійним воно
        // вже не рахується. Інакше «двічі, щоб зняти» плуталося б із «двічі,
        // щоб поставити», і клавіша ставала б лотереєю.
        if (s_Latched)
        {
            s_Latched   = false;
            s_LastPress = now;
            OZR_Log.Dbg("ptt: latch off");
            return;
        }

        if (s_LastPress > 0 && (now - s_LastPress) <= DOUBLE_MS)
        {
            s_Latched = true;
            OZR_Log.Dbg("ptt: latch on");
        }

        s_LastPress = now;
    }

    // Чи несе гравець хоч одну живу профільну рацію -- де завгодно, включно з
    // руками (руки теж інвентар, окремо їх перебирати не треба).
    //
    // Три умови, і жодну не можна прибрати: не передавач -- нема чим говорити;
    // вимкнений -- нема чим живитись; не профільний -- його поведінку визначає
    // не цей мод, і ваніль на ньому й так відкрита.
    //
    // Це рішення тільки про ІКОНКУ. Кого насправді відкрити, вирішує сервер за
    // своїм списком: клієнтові вірити в такому не можна.
    private static bool LookForRadio()
    {
        PlayerBase p = PlayerBase.Cast(GetGame().GetPlayer());
        if (!p || !p.GetInventory())
            return false;

        array<EntityAI> items = new array<EntityAI>();
        if (!p.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items))
            return false;

        for (int i = 0; i < items.Count(); i++)
        {
            TransmitterBase t = TransmitterBase.Cast(items[i]);
            if (t && t.OZR_IsPowered() && OZR_ClientGrid.For(t.GetType()))
                return true;
        }
        return false;
    }

    private static void Apply()
    {
        bool asked = s_Down || s_Latched;

        if (asked)
        {
            int now = GetGame().GetTime();
            if (now - s_LookedAt >= LOOK_EVERY_MS || s_LookedAt == 0)
            {
                s_LookedAt = now;
                s_HasRadio = LookForRadio();
            }
        }
        else
        {
            // Наступне натиснення мусить дивитись наново, а не вірити тому, що
            // бачили минулого разу.
            s_LookedAt = 0;
        }

        bool want = asked && s_HasRadio;

        if (want != s_Sent)
        {
            Send(want);
            s_Sent = want;
        }

        // Іконка горить РІВНО поки йде передача. Постійна лампочка «рація при
        // тобі» нічого не повідомляє: рацію видно в інвентарі й так, а те, що
        // світиться завжди, перестає читатись саме тоді, коли має значення.
        int mode = OZR_PttHud.MODE_NONE;
        if (want)
        {
            mode = OZR_PttHud.MODE_LIVE;
            if (s_Latched)
                mode = OZR_PttHud.MODE_LOCK;
        }
        OZR_PttHud.Set(mode);
    }

    // Ручна рація: свій RPC, бо це не сторінка КПК і предмет тут інший.
    private static void Send(bool on)
    {
        GetRPCManager().SendRPC(OZR_Const.MOD, OZR_Const.RPC_PTT, new Param1<bool>(on), true);
    }
}
