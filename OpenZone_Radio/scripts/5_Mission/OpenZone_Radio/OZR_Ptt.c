// Кнопка «говорити».
//
// PTT -- це НАТИСНУТО/ВІДПУСКАЮ, а не перемикач: рація, яка лишається
// відкритою після того, як ти відійшов від клавіатури, видає своїх власників
// швидше за будь-яку засідку.
//
// На сервер їде рівно дві події -- край натискання й край відпускання. Слати
// стан щокадру означало б слати шістдесят пакетів на секунду про те, що нічого
// не змінилось.
//
// Клавіша оголошена в inputs.xml КПК (UAOZPdaPtt), а не тут: файл входів у
// мода один, і другий такий самий рушій просто не читає.

class OZR_Ptt
{
    // БЕЗ ref: UAIDWrapper -- нативний об'єкт із приватним деструктором, і
    // скрипт ним не володіє. Ваніль тримає його так само (radialmenu.c:31).
    private static UAIDWrapper s_Key;
    private static bool s_Down    = false;
    private static bool s_Warned  = false;

    static void Init()
    {
        UAInput i = GetUApi().GetInputByName(OZ_PdaConst.INPUT_PTT);
        if (!i)
        {
            if (!s_Warned)
            {
                s_Warned = true;
                OZR_Log.Error("input " + OZ_PdaConst.INPUT_PTT + " not found - check the CfgMods inputs= path in OpenZone_PDA and the name in inputs.xml");
            }
            return;
        }

        s_Key = i.GetPersistentWrapper();
        OZR_Log.Dbg("input " + OZ_PdaConst.INPUT_PTT + " bound");
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
        if (down == s_Down)
            return;

        s_Down = down;
        Say(down);
    }

    // Клавішу могли тримати в мить, коли КПК закрили або гравець помер. Край
    // відпускання тоді не прийде ніколи, тому мовчання доводиться вмикати
    // самим -- інакше передавач лишиться відкритим назавжди.
    static void Drop()
    {
        if (!s_Down)
            return;

        s_Down = false;
        Say(false);
    }

    private static void Say(bool on)
    {
        OZR_PttRef r = new OZR_PttRef();
        r.On = on;

        string json;
        string err;
        if (JsonFileLoader<OZR_PttRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZR_Const.PAGE_RADIO, "ptt", json);
    }
}
