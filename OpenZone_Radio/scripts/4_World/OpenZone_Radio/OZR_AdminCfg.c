// Конфіги рації в адмінській консолі ядра.
//
// Реєструє їх ВЛАСНИК, тобто цей мод: ядро не знає типів рації й знати не
// мусить -- воно вміє лише спитати текст і віддати новий тому, хто вміє його
// розібрати. Обидва аплікатори роблять одне й те саме в тому самому порядку:
// РОЗІБРАТИ, перевірити, і лише потім писати на диск. Текст, який не
// розбирається, до файлу не доходить, і чинна версія лишається чинною.
//
// Перевірка тут -- не дублікат клієнтської. Клієнт у вкладці VPP не дає
// набрати дурницю, але аплікатор -- це межа: сюди приходить те, що надіслали,
// а не те, що показували. Клієнтові вірити не можна навіть адмінському.
//
// Чого тут НЕМАЄ -- дальності. `range` живе в CfgVehicles класу, його читає
// рушій, і скрипт має рівно `ConfigGetFloat` -- жодного `ConfigSet`. Дальність
// не змінюється на живому сервері взагалі, тому вкладка її ПОКАЗУЄ (з конфігу
// класу) і не дає редагувати: поле, яке нічого не робить, гірше за його
// відсутність.

class OZR_ProfilesApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZR_Profiles tmp;
        string err;
        if (!JsonFileLoader<OZR_Profiles>.LoadData(json, tmp, err) || !tmp)
        {
            OZR_Log.Warn("admin: RadioProfiles.json rejected: " + err);
            return false;
        }

        // Те, від чого профіль стає непрацездатним, а не просто дивним.
        // Кривий крок або межі поза сіткою -- це попередження Validate;
        // порожній класнейм і перевернутий відрізок -- це поломка.
        if (tmp.Radios)
        {
            for (int i = 0; i < tmp.Radios.Count(); i++)
            {
                OZR_RadioProfile p = tmp.Radios[i];
                if (!p)
                    continue;

                if (p.ClassName == "")
                {
                    OZR_Log.Warn("admin: RadioProfiles.json rejected: profile " + i.ToString() + " has no class name");
                    return false;
                }
                if (p.MaxMHz <= p.MinMHz)
                {
                    OZR_Log.Warn("admin: RadioProfiles.json rejected: " + p.ClassName + " has an empty or inverted band");
                    return false;
                }
                if (p.StepMHz <= 0)
                {
                    OZR_Log.Warn("admin: RadioProfiles.json rejected: " + p.ClassName + " has a step of zero");
                    return false;
                }
            }
        }

        OZ_ConfigLoader<OZR_Profiles>.Save(OZR_Const.PROFILES, "RadioProfiles", tmp);
        OZR_Profiles.ServerLoad();

        // Ефір іде слідом за профілями, завжди. Інакше на диску лишилась би
        // сітка, виведена з ПОПЕРЕДНІХ чисел, і наступний старт сервера підняв
        // би ефір, якого вже ніхто не просив.
        OZR_EtherServer.Publish(OZR_Profiles.Get());
        return true;
    }
}

class OZR_SettingsApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZR_Settings tmp;
        string err;
        if (!JsonFileLoader<OZR_Settings>.LoadData(json, tmp, err) || !tmp)
        {
            OZR_Log.Warn("admin: Radio.json rejected: " + err);
            return false;
        }

        OZ_ConfigLoader<OZR_Settings>.Save(OZR_Const.SETTINGS, "Radio", tmp);
        OZR_Settings.ServerLoad();
        return true;
    }
}

class OZR_AdminCfg
{
    // Імена глобальні на всю консоль: у КПК уже є свій «Profiles», і другий
    // такий склеївся б із ним мовчки.
    static const string CFG_PROFILES = "RadioProfiles";
    static const string CFG_CHANNELS = "RadioChannels";

    static void Declare()
    {
        OZ_AdminCfg.Register(CFG_PROFILES, OZR_Const.PROFILES, new OZR_ProfilesApplier(), "radio");
        OZ_AdminCfg.Register(CFG_CHANNELS, OZR_Const.SETTINGS, new OZR_SettingsApplier(), "radio");
    }
}
