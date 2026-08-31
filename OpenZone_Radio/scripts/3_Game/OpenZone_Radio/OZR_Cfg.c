// Версійований JSON-конфіг -- ВЛАСНИЙ, а не ядерний.
//
// Це майже дослівна копія OZ_ConfigBase/OZ_ConfigLoader з OpenZone_Core, і
// копія тут зроблена свідомо. Причина не в тому, що ядро погане, а в тому, що
// в DayZ немає необов'язкових посилань: клас із незавантаженого мода не можна
// згадати навіть у мертвій гілці, а порядок компіляції задається лише
// requiredAddons -- тобто ЖОРСТКОЮ залежністю. Отже «користуватись ядром,
// якщо воно є» неможливо: або жорстко залежати, або мати своє.
//
// Ручна рація ядра не потребує ні для чого іншого, і платити за дев'яносто
// три рядки блокуючим вікном «requires addon OpenZone_Core» на сервері, де
// ядра свідомо немає, -- погана угода. Тому вони тут.
//
// Що саме дає ця обгортка, крім читання файлу: версію схеми й міграцію. Конфіг
// без версії -- це файл, який після зміни формату мовчки читається як сміття;
// з версією він або мігрує, або скаржиться, і в обох випадках адмін дізнається
// про це від нас, а не від наслідків.

class OZR_ConfigBase
{
    int Version = 0;

    // Перевизначаються нащадком. База навмисне робить найтихіше, що можна:
    // конфіг, який нічого не переносить і нічого не перевіряє, -- законний.
    int  LatestVersion()            { return 1; }
    void LoadDefaults()             { Version = LatestVersion(); }
    bool Migrate(int from)          { Version = LatestVersion(); return true; }
    void Validate(out int warnings) { warnings = 0; }
}

class OZR_ConfigLoader<Class T>
{
    // cfg ПРИХОДИТЬ УЖЕ СТВОРЕНИМ, і це не стиль, а обмеження мови: `new T()`
    // всередині дженерика Enforce не компілюється -- "Bad type 'T'",
    // "Can't find class T", по парі на кожне інстанціювання. Той самий висновок
    // записаний і в ядрі, і сюди він приїхав удруге, вже своїм коштом.
    // Тому конкретний тип створює викликач, а тут лишається тільки наповнення.
    static void Load(string path, string tag, inout T cfg, bool backupOnWrite = true)
    {
        if (!cfg)
            return;

        bool fresh = false;
        string err;

        if (!FileExist(path))
        {
            // Записуємо ОДРАЗУ навмисне: адмін мусить побачити на диску файл,
            // який можна правити, а не здогадуватись, які поля бувають.
            cfg.LoadDefaults();
            fresh = true;
        }
        else if (!JsonFileLoader<T>.LoadFile(path, cfg, err))
        {
            // НЕ перезаписуємо. Зіпсований файл -- це, як правило, файл, який
            // хтось щойно правив руками: затерти його умовчаннями означає
            // знищити роботу й приховати помилку.
            OZR_Log.Error(tag + ": " + path + " does not parse (" + err + ") - using defaults, file left alone");
            cfg.LoadDefaults();
        }
        else if (cfg.Version != cfg.LatestVersion())
        {
            int from = cfg.Version;
            if (!cfg.Migrate(from))
            {
                OZR_Log.Error(tag + ": cannot migrate " + path + " from version " + from.ToString() + " - using defaults, file left alone");
                cfg.LoadDefaults();
            }
            else
            {
                string moved = tag + ": migrated " + path + " from version " + from.ToString();
                moved += " to " + cfg.Version.ToString();
                OZR_Log.Info(moved);
                fresh = true;
            }
        }

        int warnings = 0;
        cfg.Validate(warnings);
        if (warnings > 0)
            OZR_Log.Info(tag + ": " + warnings.ToString() + " thing(s) worth a look above");

        if (fresh)
            Save(path, tag, cfg);
    }

    static void Save(string path, string tag, T cfg)
    {
        string err;
        if (!JsonFileLoader<T>.SaveFile(path, cfg, err))
            OZR_Log.Error(tag + ": cannot write " + path + " (" + err + ")");
    }
}
