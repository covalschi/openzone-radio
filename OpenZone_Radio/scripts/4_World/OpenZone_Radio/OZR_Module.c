// Серверна половина мода рації.
//
// Мод НЕ несе власного пристрою: рація -- це плата в відсіку КПК. Тому все,
// що тут робиться при старті, -- це три речі:
//
//   1. виміряти таблицю частот рушія (див. OZR_Bands: число «сім» ніде не
//      оголошене, і вигадувати його не можна);
//   2. оголосити своє залізо в договорі КПК -- плату рації й довгу антену;
//   3. сказати одним рядком, що з цього вийшло, щоб вердикт стенда мав за що
//      зачепитись.

[CF_RegisterModule(OZR_Module)]
class OZR_Module : CF_ModuleWorld
{
    override void OnInit()
    {
        super.OnInit();

        // Порядок такий самий, як у решті модів OpenZone: спершу super, потім
        // підписки. Інакше CF не встигає зареєструвати модуль, і подія
        // приходить у порожнечу.
        EnableMissionStart();
    }

    override void OnMissionStart(Class sender, CF_EventArgs args)
    {
        super.OnMissionStart(sender, args);

        if (!GetGame().IsServer())
            return;

        OZR_Bands.Probe();
        OZR_Hardware.Declare();

        string summary = "radio loaded: bands=" + OZR_Bands.Count().ToString();
        summary += " modules=" + OZR_Hardware.Count().ToString();
        OZR_Log.Info(summary);
    }
}
