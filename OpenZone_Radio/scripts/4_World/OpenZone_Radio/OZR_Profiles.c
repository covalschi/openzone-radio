// Профілі ручних рацій -- те, що адмін описує сам.
//
// Профіль каже про ОДИН класнейм: який відрізок ефіру йому доступний і через
// скільки МГц він крокує. Дальність сюди не входить: вона живе в config.cpp
// класу (`range`), бо її читає сам рушій, а не ми.
//
// Відрізки МАЮТЬ перетинатись, інакше рації різних тирів ніколи не почують
// одна одну і кожна стане окремою грою. Але перетинатись повністю їм теж не
// варто: саме шматок, куди дешева рація не дістає, і робить дорогу вартою
// того, щоб її шукати.

class OZR_Profiles : OZR_ConfigBase
{
    ref array<ref OZR_RadioProfile> Radios;

    private static ref OZR_Profiles s_Inst;

    static OZR_Profiles Get()
    {
        return s_Inst;
    }

    override int LatestVersion()
    {
        return OZR_Const.SCHEMA_PROFILES;
    }

    // Типово -- десять сходинок, які мод приносить із собою. Дальність кожної
    // стоїть у config.cpp її класу (рушій читає її звідти й нізвідки більше);
    // тут -- лише ефір, який їй дозволено.
    //
    // Драбина зростає в один бік по обох осях: що далі рація бере, то ШИРШИЙ
    // її відрізок і то ДРІБНІШИЙ крок. Через це дешева рація сидить у тісній
    // середині, де всі одне одному заважають, а дорога дістає туди, куди
    // дешева не чує, -- і саме це робить її вартою пошуку.
    //
    // Кроки всі кратні 0.0125, тобто кроку сітки: рація з некратним кроком
    // стала б МІЖ діленнями й не зійшлася б ні з ким. Межі теж лежать на
    // діленнях.
    //
    // Перетини навмисне неповні, але СУЦІЛЬНІ по драбині: кожна сходинка
    // перетинається з сусідніми, тож ланцюжок від 50 м до 10 км ніде не
    // рветься. Домовитись можна завжди -- але не будь-де.
    override void LoadDefaults()
    {
        Version = LatestVersion();
        Radios  = new array<ref OZR_RadioProfile>();

        Add("OZ_Radio_50m",    145.0, 145.5, 0.1000);
        Add("OZ_Radio_100m",   144.0, 146.0, 0.1000);
        Add("OZ_Radio_200m",   145.0, 148.0, 0.0500);
        Add("OZ_Radio_250m",   144.0, 147.0, 0.0500);
        Add("OZ_Radio_500m",   142.0, 147.0, 0.0250);
        Add("OZ_Radio_750m",   143.0, 148.0, 0.0250);
        Add("OZ_Radio_1000m",  140.0, 150.0, 0.0250);
        Add("OZ_Radio_2000m",  138.0, 151.0, 0.0250);
        Add("OZ_Radio_5000m",  136.0, 152.0, 0.0125);
        Add("OZ_Radio_10000m", 136.0, 152.0, 0.0125);
    }

    private void Add(string cls, float lo, float hi, float step)
    {
        OZR_RadioProfile p = new OZR_RadioProfile();
        p.ClassName = cls;
        p.MinMHz    = lo;
        p.MaxMHz    = hi;
        p.StepMHz   = step;
        Radios.Insert(p);
    }

    override bool Migrate(int from)
    {
        Version = LatestVersion();
        return true;
    }

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Radios)
            Radios = new array<ref OZR_RadioProfile>();

        // Без рівномірної сітки профілі рахувати нічим. Це не поломка: так
        // виглядає непропатчений сервер, де рушій дає свою нерівну вісімку.
        // Кажемо про це ОДИН раз і лишаємо профілі як є -- вони просто не
        // застосуються.
        if (!OZR_Grid.Ready())
        {
            string flat = "the engine's frequency table is not an even grid (";
            flat += OZR_Bands.Count().ToString();
            flat += " bands) - radio profiles stay unapplied; this is what an unpatched server looks like";
            OZR_Log.Warn(flat);
            warnings++;
            return;
        }

        float lo = OZR_Grid.MHzAt(0);
        float hi = OZR_Grid.MHzAt(OZR_Grid.Count() - 1);

        // ЩО ТУТ ВИКИДАЄТЬСЯ, А ЩО ЛИШЕ ЗГАДУЄТЬСЯ.
        //
        // Викидається те, з чого не можна вивести НІЧОГО: профіль без імені,
        // порожня чи перевернута смуга, нульовий крок. Такий профіль зламав би
        // і виведення ефіру, і саму рацію.
        //
        // Все інше лишається як написано. Раніше тут обрізали межі під сітку й
        // округляли крок до її кроку -- і це було правильно, поки сітка була
        // чимось зовнішнім. Тепер сітка ВИВОДИТЬСЯ з цих самих чисел, і
        // переписати їх під стару сітку означало б знищити те, з чого будують
        // нову: адмін просить 86 МГц, ми обрізаємо до 136, ефір виводиться з
        // 136, і 86 не настає ніколи. Тому тут лише кажуть вголос, що профіль
        // випереджає ефір, а обрізає використання (OZR_Grid.Window) -- до
        // наступного старту сервера.
        for (int i = Radios.Count() - 1; i >= 0; i--)
        {
            OZR_RadioProfile p = Radios[i];

            if (!p || p.ClassName == "")
            {
                OZR_Log.Warn("radio profile without a classname dropped");
                Radios.Remove(i);
                warnings++;
                continue;
            }

            if (p.MinMHz >= p.MaxMHz)
            {
                string empty = "profile " + p.ClassName + " has an empty band (";
                empty += OZR_Fmt.MHz(p.MinMHz) + ".." + OZR_Fmt.MHz(p.MaxMHz) + ") - dropped";
                OZR_Log.Warn(empty);
                Radios.Remove(i);
                warnings++;
                continue;
            }

            if (p.StepMHz <= 0)
            {
                string nostep = "profile " + p.ClassName + " has no step - dropped";
                OZR_Log.Warn(nostep);
                Radios.Remove(i);
                warnings++;
                continue;
            }

            // Не помилка, а стан: ефір ще не наздогнав профіль. Рація працює на
            // тій частині смуги, яка вже існує, і на всій -- після рестарту.
            if (p.MinMHz < lo || p.MaxMHz > hi)
            {
                string ahead = "profile " + p.ClassName + " asks for ";
                ahead += OZR_Fmt.MHz(p.MinMHz) + ".." + OZR_Fmt.MHz(p.MaxMHz);
                ahead += " MHz while the running ether is " + OZR_Fmt.MHz(lo);
                ahead += ".." + OZR_Fmt.MHz(hi);
                ahead += " - it works on the overlap until the server is restarted";
                OZR_Log.Warn(ahead);
                warnings++;
            }
        }
    }

    // Профіль за класнеймом, або порожньо. Порожньо означає «ця рація не наша»
    // -- ванільні й чужі рації лишаються з ванільною поведінкою.
    static OZR_RadioProfile For(string className)
    {
        if (!s_Inst || !s_Inst.Radios)
            return null;

        for (int i = 0; i < s_Inst.Radios.Count(); i++)
        {
            if (s_Inst.Radios[i].ClassName == className)
                return s_Inst.Radios[i];
        }
        return null;
    }

    static void ServerLoad()
    {
        s_Inst = new OZR_Profiles();
        OZR_ConfigLoader<OZR_Profiles>.Load(OZR_Const.PROFILES, "RadioProfiles", s_Inst);
    }
}
