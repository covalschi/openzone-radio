// Вкладка «RADIO» в адмiнському вiкнi OpenZone -- НЕОБОВ'ЯЗКОВИЙ pbo.
//
// Окремий мод, а не частина рацiї, з тiєї ж причини, що й у ядра з КПК:
// requiredAddons нижче -- ЖОРСТКА залежнiсть, тобто блокуюче вiкно ще до
// завантаження, а не тихий пропуск. Сервер без VPP мусить крутити рацiю як
// нi в чому не бувало, тому все, що знає про VPP, живе тут.
//
// DZM_VPPAdminToolsScripts -- клас CfgPatches скриптового pbo самого VPP.
// #ifdef у коді гардить на AVPPAdminTools: рушiй авто-дефайнить iмена класiв
// CfgMods, а не CfgPatches (змiряно 2026-07-31 на 1.29 diag).

class CfgPatches
{
    class OpenZone_Radio_VPP
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "OpenZone_Core",
            "OpenZone_VPP",
            "OpenZone_Radio",
            "DZM_VPPAdminToolsScripts"
        };
    };
};

class CfgMods
{
    class OpenZone_Radio_VPP
    {
        dir = "OpenZone_Radio_VPP";
        name = "OpenZone Radio VPP Tab";
        author = "Zone Protocol";
        version = "0.1.0";
        type = "mod";

        dependencies[] = {"World", "Mission"};

        class defs
        {
            // 4_World з'явився разом із розділенням: реєстрація профілів у
            // консолі ядра переїхала сюди з мода рації, бо консоль -- служба
            // ядра, а рація має працювати й без нього.
            class worldScriptModule   { value = ""; files[] = {"OpenZone_Radio_VPP/scripts/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"OpenZone_Radio_VPP/scripts/5_Mission"}; };
        };
    };
};
