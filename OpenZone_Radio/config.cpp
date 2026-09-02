// OpenZone Radio -- the band system, and it stands on its own.
//
// WHAT THIS PBO IS. The ether: how many channels exist, where they sit, who
// hears whom, and push-to-talk. It needs nothing of ours -- a server that
// wants nothing but handheld radios on a wider band installs this alone.
//
// WHAT IT IS NOT. The PDA board. The radio as a MODULE in a device bay lives
// in @OpenZone_Radio_PDA, a separate pbo in this same repository, and that one
// does require the PDA. The admin tab is the same story: @OpenZone_Radio_VPP.
//
// This header used to say "hard dependency on OpenZone_PDA, because the module
// bays, the page registry and the antenna contract all live there". It was
// true once and stopped being true when the glue moved out; requiredAddons
// below has been the honest answer for a while, and a reader who trusted the
// prose over the code would have concluded the mod cannot ship alone. It can:
// booted 2026-09-01 against @CF and nothing else.

class CfgPatches
{
    class OpenZone_Radio
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        // САМОСТІЙНИЙ. Ані ядра, ані КПК тут більше немає, і це не
        // косметика: requiredAddons -- ЖОРСТКА залежність, тобто блокуюче
        // вікно ще до завантаження. Сервер, якому потрібні лише ручні
        // рації, платив би цим вікном за код, якого не використовує.
        //
        // Все, що знає про КПК, живе в @OpenZone_Radio_PDA -- окремому pbo
        // з цього ж репозиторію, який вимагає обидва боки жорстко й
        // ставиться лише тоді, коли крутять обидва. Те саме з адмінською
        // вкладкою: @OpenZone_Radio_VPP.
        //
        // CF лишається: модуль мода -- CF_ModuleWorld, і RPC ходять через
        // GetRPCManager().
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "JM_CF_Scripts",
            // Ним оголошений personalradio_staticnoise_SoundShader, від якого
            // успадковується наш сплеск. Без цього рядка базовий клас при
            // бінаризації просто не знайдеться -- і не мовчки: конфіг не
            // збереться. Це четверта ЖОРСТКА залежність, і вона нічого не
            // коштує: pbo ванільний, він є на кожному сервері.
            "DZ_Sounds_Effects"
        };
    };
};

class CfgMods
{
    class OpenZone_Radio
    {
        dir        = "OpenZone_Radio";
        name       = "OpenZone Radio";
        credits    = "Zone Protocol";
        author     = "Zone Protocol";
        type       = "mod";

        dependencies[] = {"Game", "World", "Mission"};

        // БЕЗ ЦЬОГО РЯДКА CF_ModStorage не працює й не скаржиться.
        //
        // CF реєструє хранилище лише для модів, у яких storageVersion > 0
        // (ModStructure.HasModStorage). Нуль або відсутність -- і наш блок не
        // пишеться, не читається, і жодного рядка про це в лозі не буде:
        // рація просто щоразу прокидатиметься невибраною.
        storageVersion = 1;

        // The frequency keypad's key. A mod gets exactly one inputs file --
        // a second is not read -- so everything this mod binds goes in here.
        inputs = "OpenZone_Radio/data/inputs.xml";

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"OpenZone_Radio/scripts/3_Game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"OpenZone_Radio/scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"OpenZone_Radio/scripts/5_Mission"};
            };
        };
    };
};

// Звук сплеску -- власний набір на власному семплі.
//
// Спершу він грав ванільний personalradio_staticnoise: той уже є в грі, і мод
// лишався без жодного аудіофайла. Ціна виявилась зависокою. Той семпл -- шип
// увімкненої рації (volume = 0.0501, range = 13 у ванільному шейдері), він
// створений щоб НЕ помічатись, і навіть піднятий усемеро звучав як шум, а не
// як клац передавача. Власник приніс справжній семпл рації, і це правильне
// рішення: сплеск -- подія, а не фон.
//
// Семпл: моно, 48 кГц, 0.28 с. Моно навмисне -- набір просторовий
// (spatial = 1 у baseCharacter_SoundSet), а стерео в тривимірній сцені не
// позиціонується.
//
// loop = 0, і це головна відмінність від попередньої версії: ванільний шип
// зациклений, і його доводилось обривати таймером на 140 мс. Справжній
// сплеск має власну довжину, тож обривати нічого -- рушій сам зупинить і
// прибере (SetAutodestroy у PlaySoundSet).
//
// РАДІУС ЛІСЕНКОЮ, а не одним числом, і це вибір між двома звуковими API
// гри, а не лінощі.
//
// У DayZ їх два, і вони дають РІЗНЕ. Набори (CfgSoundSets) дозволяють міняти
// гучність на льоту -- EffectSound.SetSoundVolume, -- але радіус у них
// заданий шейдером і після створення не рухається. Object.PlaySound бере
// радіус аргументом, зате повертає SoundOnVehicle, у якого з усього API одне
// GetSoundLength(): гучності там немає.
//
// Перейти на друге означало б переписати ще й модель затухання, тобто
// переробити звук, який власник щойно прийняв. Тому лишається перше, а
// налаштовуваний радіус робиться ступенями: по набору на відстань, і JSON
// вибирає найближчу. Ступені видно в лозі, щоб «поставив 30, отримав 25» не
// виглядало як несправність.
//
// Два звуки на кожній ступені: початок і кінець передачі звучать по-різному,
// бо на слух саме різниця й розрізняє «почав» і «договорив».
class CfgSoundShaders
{
    class personalradio_staticnoise_SoundShader;

    class OZR_Squelch_R5: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch", 1}
        };
        volume = 0.5;
        range  = 5;
    };

    class OZR_SquelchOff_R5: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch_off", 1}
        };
        volume = 0.5;
        range  = 5;
    };

    class OZR_Squelch_R10: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch", 1}
        };
        volume = 0.5;
        range  = 10;
    };

    class OZR_SquelchOff_R10: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch_off", 1}
        };
        volume = 0.5;
        range  = 10;
    };

    class OZR_Squelch_R15: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch", 1}
        };
        volume = 0.5;
        range  = 15;
    };

    class OZR_SquelchOff_R15: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch_off", 1}
        };
        volume = 0.5;
        range  = 15;
    };

    class OZR_Squelch_R20: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch", 1}
        };
        volume = 0.5;
        range  = 20;
    };

    class OZR_SquelchOff_R20: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch_off", 1}
        };
        volume = 0.5;
        range  = 20;
    };

    class OZR_Squelch_R25: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch", 1}
        };
        volume = 0.5;
        range  = 25;
    };

    class OZR_SquelchOff_R25: personalradio_staticnoise_SoundShader
    {
        samples[] =
        {
            {"OpenZone_Radio\sounds\ozr_squelch_off", 1}
        };
        volume = 0.5;
        range  = 25;
    };
};

class CfgSoundSets
{
    class personalradio_staticnoise_SoundSet;

    class OZR_Squelch_R5_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_Squelch_R5"};
        loop = 0;
    };

    class OZR_SquelchOff_R5_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_SquelchOff_R5"};
        loop = 0;
    };

    class OZR_Squelch_R10_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_Squelch_R10"};
        loop = 0;
    };

    class OZR_SquelchOff_R10_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_SquelchOff_R10"};
        loop = 0;
    };

    class OZR_Squelch_R15_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_Squelch_R15"};
        loop = 0;
    };

    class OZR_SquelchOff_R15_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_SquelchOff_R15"};
        loop = 0;
    };

    class OZR_Squelch_R20_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_Squelch_R20"};
        loop = 0;
    };

    class OZR_SquelchOff_R20_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_SquelchOff_R20"};
        loop = 0;
    };

    class OZR_Squelch_R25_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_Squelch_R25"};
        loop = 0;
    };

    class OZR_SquelchOff_R25_SoundSet: personalradio_staticnoise_SoundSet
    {
        soundShaders[] = {"OZR_SquelchOff_R25"};
        loop = 0;
    };
};

class CfgVehicles
{
    class PersonalRadio;

    class OZ_Radio_100m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_100";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 100;
    };

    class OZ_Radio_200m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_200";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 200;
    };

    class OZ_Radio_500m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_500";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 500;
    };

    class OZ_Radio_1000m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_1000";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 1000;
    };

    class OZ_Radio_5000m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_5000";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 5000;
    };

    class OZ_Radio_50m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_50";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 50;
    };

    class OZ_Radio_250m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_250";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 250;
    };

    class OZ_Radio_750m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_750";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 750;
    };

    class OZ_Radio_2000m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_2000";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 2000;
    };

    class OZ_Radio_10000m : PersonalRadio
    {
        scope = 2;
        displayName = "$STR_OZR_RADIO_10000";
        descriptionShort = "$STR_OZR_RADIO_DESC";
        range = 10000;
    };
};
