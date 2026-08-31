// Серверна половина VPP-субмода: одна реєстрація й нічого більше.
//
// Модуль потрібен рівно тому, що реєструвати конфіг у консолі ядра треба на
// СЕРВЕРІ й після того, як рація свої профілі прочитала. Вкладка на клієнті
// про це не знає й знати не мусить: вона лише просить конфіг на ім'я.

[CF_RegisterModule(OZR_VppModule)]
class OZR_VppModule : CF_ModuleWorld
{
    override void OnInit()
    {
        super.OnInit();
        EnableMissionStart();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        if (!GetGame().IsServer())
            return;

        OZR_VppAdminCfg.Declare();
    }
}
