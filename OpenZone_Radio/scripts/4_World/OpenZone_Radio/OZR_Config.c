// Канали -- те, що адмін описує сам.
//
// Канал не є частотою. Частот у рушія рівно стільки, скільки їх виміряв
// OZR_Bands, і більше взятись їм нізвідки. Канал -- це ІМ'Я поверх однієї з
// них плюс правило, кому туди можна. Тому «Борг» і «Свобода» можуть сидіти на
// сусідніх смугах, а сервер, якому потрібні просто «канал 1..8», не описує
// нічого й отримує їх сам.
//
// Кілька каналів на одній смузі -- не помилка, а рішення адміна: рушій їх не
// розрізняє, і сидітимуть вони в одному ефірі. Ми про це попереджаємо один
// раз, але не забороняємо: буває, що саме цього й хочуть.

class OZR_Channel
{
    string Id      = "";
    string Name    = "";
    int    Band    = 0;

    // Порожньо -- канал для всіх. Інакше -- id фракції, і тоді налаштуватись
    // на нього може лише її людина.
    string Faction = "";
}

class OZR_Settings : OZ_ConfigBase
{
    ref array<ref OZR_Channel> Channels;

    private static ref OZR_Settings s_Inst;

    static OZR_Settings Get()
    {
        return s_Inst;
    }

    override int LatestVersion()
    {
        return OZR_Const.SCHEMA_RADIO;
    }

    // Типово -- по каналу на кожну ВИМІРЯНУ смугу. Не сім і не вісім: скільки
    // рушій дав, стільки й буде, і на іншій збірці число зміниться саме.
    override void LoadDefaults()
    {
        Version  = LatestVersion();
        Channels = new array<ref OZR_Channel>();

        for (int i = 0; i < OZR_Bands.Count(); i++)
        {
            OZR_Channel c = new OZR_Channel();
            c.Id      = "ch" + (i + 1).ToString();
            c.Name    = "Channel " + (i + 1).ToString();
            c.Band    = i;
            c.Faction = "";
            Channels.Insert(c);
        }
    }

    override bool Migrate(int from)
    {
        Version = LatestVersion();
        return true;
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Channels)
            Channels = new array<ref OZR_Channel>();

        int bands = OZR_Bands.Count();

        // Йдемо з кінця: викидати з масиву, яким ідеш уперед, означає
        // пропускати сусіда викинутого.
        for (int i = Channels.Count() - 1; i >= 0; i--)
        {
            OZR_Channel c = Channels[i];

            if (!c || c.Id == "")
            {
                OZR_Log.Warn("channel without an id dropped");
                Channels.Remove(i);
                warnings++;
                continue;
            }

            // Смуга поза виміряною таблицею -- це канал, на який рушій просто
            // не налаштується. Мовчки лишити його означало б дати гравцеві
            // кнопку, яка нічого не робить.
            if (c.Band < 0 || c.Band >= bands)
            {
                string bad = "channel \"" + c.Id;
                bad += "\" asks for band " + c.Band.ToString();
                bad += ", but the engine has " + bands.ToString();
                OZR_Log.Warn(bad);
                Channels.Remove(i);
                warnings++;
                continue;
            }

            if (c.Name == "")
                c.Name = c.Id;
            if (c.Name.Length() > OZR_Const.CH_NAME_MAX)
                c.Name = c.Name.Substring(0, OZR_Const.CH_NAME_MAX);
        }

        if (Channels.Count() == 0 && bands > 0)
            OZR_Log.Warn("no usable channels - the radio page will be empty");
    }

    // Канал за id, або порожньо.
    static OZR_Channel ById(string id)
    {
        if (!s_Inst || !s_Inst.Channels)
            return null;

        for (int i = 0; i < s_Inst.Channels.Count(); i++)
        {
            if (s_Inst.Channels[i].Id == id)
                return s_Inst.Channels[i];
        }
        return null;
    }

    // Перший канал на цій смузі. Саме так пристрій згадує, де він стояв:
    // частоту зберігає сам предмет (TransmitterBase.OnStoreSave), а ім'я
    // каналу ми знаходимо назад по ній.
    static OZR_Channel ByBand(int band)
    {
        if (!s_Inst || !s_Inst.Channels)
            return null;

        for (int i = 0; i < s_Inst.Channels.Count(); i++)
        {
            if (s_Inst.Channels[i].Band == band)
                return s_Inst.Channels[i];
        }
        return null;
    }

    static void ServerLoad()
    {
        s_Inst = new OZR_Settings();
        OZ_ConfigLoader<OZR_Settings>.Load(OZR_Const.SETTINGS, "Radio", s_Inst);
    }
}
