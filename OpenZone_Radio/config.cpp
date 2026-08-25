// OpenZone Radio.
//
// The radio is not a separate box you carry next to the PDA. It is a MODULE
// that goes into one of the PDA's bays, plus the antennas that decide how far
// it reaches. That is what the bay system was built for: the device is one
// object in the pocket, and what it can do is what is plugged into it.
//
// Hard dependency on OpenZone_PDA, because the module bays, the page registry
// and the antenna contract all live there.

class CfgPatches
{
    class OpenZone_Radio
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "JM_CF_Scripts",
            "OpenZone_Core",
            "OpenZone_PDA"
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
    class OZ_Module_Base;

    // The radio board. Turns the PDA into a transceiver; without an antenna it
    // is deaf and mute, exactly like the transponder.
    class OZ_Module_Radio : OZ_Module_Base
    {
        scope = 2;
        displayName = "$STR_OZR_MOD_RADIO";
        descriptionShort = "$STR_OZR_MOD_RADIO_DESC";
    };

    // Long antenna. Same Kind as the PDA's own stub, only a bigger RangeM --
    // that is the whole extension mechanism: whichever antenna declares the
    // larger range wins.
    class OZ_Module_Antenna_Long : OZ_Module_Base
    {
        scope = 2;
        displayName = "$STR_OZR_MOD_ANT_LONG";
        descriptionShort = "$STR_OZR_MOD_ANT_LONG_DESC";
    };
};
