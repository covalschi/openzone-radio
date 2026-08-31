// Клієнтська половина склейки.
//
// Дві речі, і обидві -- підключення до чужих точок розширення, а не правка
// чужих файлів: сказати фабриці сторінок КПК, хто малює «Рацію», і підписати
// слухача на клавішу PTT мода рації.
//
// На виділеному сервері MissionGameplay не створюється взагалі (там
// MissionServer), тож цей код туди просто не потрапляє.

modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        OZ_PdaPageFactory.Add(OZRP_Const.PAGE_RADIO, OZR_PageRadio);
        OZR_Ptt.Listen(new OZRP_PttToBoard());
    }
}

// Клавіша PTT рації веде й плату в КПК.
//
// Через договір сторінок, а не прямим викликом: плата -- це серверна сутність,
// і єдиний шлях до неї з клієнта той самий, яким ходить сама сторінка.
class OZRP_PttToBoard : OZR_PttListener
{
    override void OnPtt(bool on)
    {
        OZR_PttRef r = new OZR_PttRef();
        r.On = on;

        string json;
        string err;
        if (JsonFileLoader<OZR_PttRef>.MakeData(r, json, err, false))
            OZ_Rpc.Request(OZRP_Const.PAGE_RADIO, "ptt", json);
    }
}
