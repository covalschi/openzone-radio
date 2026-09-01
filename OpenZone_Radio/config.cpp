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
            "JM_CF_Scripts"
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
