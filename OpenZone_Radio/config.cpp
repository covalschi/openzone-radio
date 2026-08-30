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
    //
    // simulation="itemTransmitter" is the ONE line that matters, and it was
    // expensive to find. The engine picks an entity's native type from this
    // field -- not from the config parent, and not from the script class. A
    // board without it comes out a plain ItemBase however faithfully its
    // script class extends TransmitterBase: measured on the stand, where the
    // cast to a transmitter kept failing while everything else looked right,
    // and a board the engine does not consider a transmitter carries no voice
    // at all.
    class OZ_Module_Radio : OZ_Module_Base
    {
        scope = 2;
        simulation = "itemTransmitter";
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

    // ------------------------------------------------------------------
    // Handheld radios that differ in one number and nothing else.
    //
    // `range` is the engine's OWN per-class transmission distance, in metres.
    // Not invented here: read out of the vanilla configs, where PersonalRadio
    // declares 5000, BaseRadio 50000 and the megaphone 200. The itemTransmitter
    // simulation reads it, and these inherit that simulation from PersonalRadio
    // along with the model, the battery slot and the energy manager -- so one
    // line per class is genuinely all that changes.
    //
    // `inputRange[]` is deliberately left alone. That is how close you must be
    // for the radio to pick your voice up, per voice level {whisper, talk,
    // shout}, and moving it together with `range` would make two things change
    // at once for no reason a player could name or notice.
    //
    // The reach is in the display name on purpose. Five radios on one model are
    // otherwise indistinguishable in the hand, and a tier the player cannot see
    // is a tier they will assume does not exist.
    //
    // None of these spawn as loot by themselves: natural spawning needs entries
    // in the server's types.xml. Until then they are admin- and trader-placed.
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
};
