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

        // Плата в КПК -- така сама рація, тільки в відсіку, і профіль їй
        // потрібен той самий. Клас оголошує @OpenZone_Radio_PDA; без нього
        // рядок просто нікому не відповідає, і це нікому не шкодить.
        //
        // Смуга як у 2000-метрової: КПК не мисливська рація й не штабна
        // станція, а середина драбини. Настройка при цьому дорожча -- її не
        // зробиш на бігу, треба відкрити КПК.
        Add("OZ_Module_Radio", 138.0, 151.0, 0.0250);
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

    // Migrate тут НЕ перевизначений навмисно. Схема одна (SCHEMA_PROFILES == 1),
    // і override, який побайтово повторював базову реалізацію, лише обіцяв
    // переніс, якого не буває. Повернути його -- справа того дня, коли схема
    // рушить із місця; OZR_Settings живе на тій самій базі й без нього.

    override void Validate(out int warnings)
    {
        warnings = 0;

        if (!Radios)
            Radios = new array<ref OZR_RadioProfile>();

        // ЩО ТУТ ВИКИДАЄТЬСЯ, А ЩО ЛИШЕ ЗГАДУЄТЬСЯ.
        //
        // Викидається те, з чого не можна вивести НІЧОГО: порожній елемент,
        // профіль без імені, порожня чи перевернута смуга, нульовий крок,
        // межа понад стелю. Такий профіль зламав би і виведення ефіру, і саму
        // рацію. Перелік не тут -- він один на весь мод, у
        // OZR_RadioProfile.Problem.
        //
        // ПЕРШИМ, І НЕЗАЛЕЖНО ВІД СІТКИ. Раніше цей цикл стояв ПІСЛЯ виходу
        // «сітка нерівна», тобто на непропатченому сервері не виконувався
        // взагалі -- і порожній елемент масиву доживав до першого ж
        // OZR_GridReq, який розіменовував його без перевірки.
        for (int i = Radios.Count() - 1; i >= 0; i--)
        {
            OZR_RadioProfile p = Radios[i];

            if (!p)
            {
                OZR_Log.Warn("an empty entry in the radio profile list dropped");
                Radios.Remove(i);
                warnings++;
                continue;
            }

            string bad = p.Problem();
            if (bad != "")
            {
                OZR_Log.Warn("profile " + p.Named() + " " + bad + " - dropped");
                Radios.Remove(i);
                warnings++;
            }
        }

        // Далі -- лише те, що має сенс проти ЖИВОЇ сітки. Без рівномірної
        // сітки рахувати нічим; це не поломка, а те, як виглядає
        // непропатчений сервер із ванільною нерівною вісімкою. Кажемо про це
        // ОДИН раз і лишаємо профілі як є -- вони просто не застосуються.
        if (!OZR_Grid.Ready())
        {
            string flat = "the engine's frequency table is not an even grid (";
            flat += OZR_Bands.Count().ToString();
            flat += " bands) - radio profiles stay unapplied; this is what an unpatched server looks like";
            OZR_Log.Warn(flat);
            warnings++;
            return;
        }

        float lo = OZR_Grid.Base();
        float hi = OZR_Grid.MHzAt(OZR_Grid.Count() - 1);

        // Межі лишаються як написано. Раніше тут обрізали їх під сітку й
        // округляли крок -- і це було правильно, поки сітка була чимось
        // зовнішнім. Тепер сітка ВИВОДИТЬСЯ з цих самих чисел, і переписати їх
        // під стару сітку означало б знищити те, з чого будують нову: адмін
        // просить 86 МГц, ми обрізаємо до 136, ефір виводиться з 136, і 86 не
        // настає ніколи. Тому тут лише кажуть вголос, що профіль випереджає
        // ефір, а обрізає ВИКОРИСТАННЯ (OZR_Grid.Window) -- до наступного
        // старту сервера.
        for (int k = 0; k < Radios.Count(); k++)
        {
            OZR_RadioProfile q = Radios[k];
            if (q.MinMHz >= lo && q.MaxMHz <= hi)
                continue;

            string ahead = "profile " + q.ClassName + " asks for ";
            ahead += OZR_Fmt.MHz(q.MinMHz) + ".." + OZR_Fmt.MHz(q.MaxMHz);
            ahead += " MHz while the running ether is " + OZR_Fmt.MHz(lo);
            ahead += ".." + OZR_Fmt.MHz(hi);
            ahead += " - it works on the overlap until the server is restarted";
            OZR_Log.Warn(ahead);
            warnings++;
        }
    }

    // ОБ'ЄКТ ІЗ ЗАВАНТАЖУВАЧА ЖИВЕ РІВНО ДО КІНЦЯ РОЗБОРУ -- далі тільки копія.
    //
    // JsonFileLoader<T> -- тонка обгортка над нативним JsonSerializer, тож
    // об'єкт створює РУШІЙ: ані конструктор, ані ініціалізатори полів
    // (`= ""`, `= 0`) не виконуються, а сам розбір присвоює лише ті члени, для
    // яких знайшов значення. Решта лишається тим, що лежало за цим зміщенням.
    // Одразу після розбору сторінка ще свіжо занулена, тому перші читання
    // виглядають правильними; за хвилину купа зрушила, і те саме поле віддає
    // чуже сміття -- при цьому `if (x != "")` на ньому ІСТИННЕ. Зміряно на
    // стенді фракцій 2026-09-06 (рядок звання читався то "3", то "$").
    //
    // s_Inst живе весь запуск сервера, і його поля читає кожен тюн, кожен
    // спавн і кожен запит сторінки. Тому одразу після Load ми переписуємо
    // все в об'єкти, створені через new -- один раз, поки числа ще правдиві.
    OZR_Profiles Copy()
    {
        OZR_Profiles c = new OZR_Profiles();
        c.Version = Version;
        c.Radios  = new array<ref OZR_RadioProfile>();

        if (!Radios)
            return c;

        for (int i = 0; i < Radios.Count(); i++)
        {
            OZR_RadioProfile p = Radios[i];
            if (!p)
                continue;
            c.Radios.Insert(p.Copy());
        }
        return c;
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

    // Чи можна писати похідне від цього файлу.
    //
    // false означає одне: файл на диску є, і ми його НЕ ЗРОЗУМІЛИ, тобто
    // працюємо на дефолтах, яких адмін не писав. Виводити з них ефір і класти
    // його поверх робочого OZ_Radio_Frequencies.json -- значить після одного
    // зіпсованого старту втратити всі власні смуги мовчки. Той самий прапорець
    // і з тієї ж причини стоїть у ядрі (OZ_Settings.Writable).
    private static bool s_Writable = true;

    static bool Writable()
    {
        return s_Writable;
    }

    static void ServerLoad()
    {
        OZR_Profiles loaded = new OZR_Profiles();
        s_Writable = OZR_ConfigLoader<OZR_Profiles>.Load(OZR_Const.PROFILES, "RadioProfiles", loaded);

        // КОПІЯ, поки поля ще читаються правильно -- див. Copy() вище.
        s_Inst = loaded.Copy();
    }
}
