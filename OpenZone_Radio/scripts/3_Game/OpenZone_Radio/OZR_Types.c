// Форма, якою сторінка «Рація» говорить із сервером.
//
// У 3_Game, бо на неї дивляться обидва боки: сервер складає, клієнт читає.

class OZR_ChannelInfo
{
    string Id      = "";
    string Name    = "";
    int    Band    = 0;

    // Частота в тому вигляді, в якому її знає рушій. Показуємо її поруч з
    // ім'ям: гравець із ванільною рацією має змогу зійтись із власником КПК
    // на одній смузі, а без числа це неможливо.
    float  Freq    = 0;

    // Чи пустить сюди фракція. Заборонений канал ЛИШАЄТЬСЯ в переліку --
    // сховати його означало б приховати, що ефір узагалі існує.
    bool   Allowed = false;
    bool   Current = false;
}

class OZR_RadioState
{
    // Плата вставлена. Без неї сторінки взагалі не буває, але прийти сюди
    // можна саме в мить, коли її витягли.
    bool   HasBoard = false;
    bool   Powered  = false;

    // Антена. Нуль -- рація глуха й німа, як і транспондер без антени.
    float  RangeM   = 0;

    // Чи йде передача просто зараз (тобто чи тримають PTT).
    bool   Live     = false;

    string Current  = "";
    ref array<ref OZR_ChannelInfo> Items;

    void OZR_RadioState()
    {
        Items = new array<ref OZR_ChannelInfo>();
    }
}

class OZR_TuneRef
{
    string Id = "";
}

class OZR_PttRef
{
    bool On = false;
}
