// Профілі рацій в адмінській консолі ядра.
//
// ЧОМУ НЕ В САМІЙ РАЦІЇ. Консоль -- служба ядра: транспорт, гейт прав і реєстр
// конфігів живуть там. Рація ж має працювати й без ядра, а посилання на клас
// із незавантаженого мода в Enforce не існує навіть у мертвій гілці. Тому
// реєстрація переїхала сюди -- у pbo, який ядро й так вимагає жорстко.
//
// Без цього pbo рація не втрачає нічого, крім живого редагування: профілі
// читаються з того ж файлу при старті, і правити його можна руками.
//
// Перевірка тут -- не дублікат клієнтської. Вкладка не дасть набрати дурницю,
// але аплікатор -- це межа: сюди приходить те, що надіслали, а не те, що
// показували. Клієнтові вірити не можна навіть адмінському.

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

        OZR_ConfigLoader<OZR_Profiles>.Save(OZR_Const.PROFILES, "RadioProfiles", tmp);
        OZR_Profiles.ServerLoad();

        // Ефір іде слідом за профілями, завжди. Інакше на диску лишилась би
        // сітка, виведена з ПОПЕРЕДНІХ чисел.
        OZR_EtherServer.Publish(OZR_Profiles.Get());
        return true;
    }
}

class OZR_VppAdminCfg
{
    static const string CFG_PROFILES = "RadioProfiles";

    static void Declare()
    {
        OZ_AdminCfg.Register(CFG_PROFILES, OZR_Const.PROFILES, new OZR_ProfilesApplier(), "radio");
    }
}
