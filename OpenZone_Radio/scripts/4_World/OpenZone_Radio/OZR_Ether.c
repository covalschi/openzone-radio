// Серверна половина ефіру: покласти виведену сітку на диск і сказати, чи вона
// вже діє.
//
// Арифметика живе в 3_Game (OZR_Ether), бо її рахує і вкладка на клієнті. Тут
// лишається те, що має сенс лише на сервері: файл і порівняння з ВИМІРЯНОЮ
// сіткою рушія.
//
// Файл читає нативний патч при старті ПРОЦЕСУ, тому нова сітка вступає в дію
// тільки з наступним запуском сервера. Це сказано вголос -- і в лозі, і у
// вкладці: мовчазна відкладена дія гірша за відсутню.

class OZR_EtherServer
{
    private static ref OZR_EtherPlan s_Plan;

    static OZR_EtherPlan Plan()
    {
        return s_Plan;
    }

    // Порахувати й записати. Кличеться і при старті, і після кожної правки
    // профілів: файл, який відстав від профілів, -- це рівно та неузгодженість,
    // заради усунення якої все це й зроблено.
    static void Publish(OZR_Profiles cfg)
    {
        array<ref OZR_RadioProfile> radios;
        if (cfg)
            radios = cfg.Radios;

        OZR_EtherPlan plan = OZR_Ether.Derive(radios);
        s_Plan = plan;

        if (!plan.Ok)
        {
            // Файл НЕ чіпаємо. Профілі можуть бути тимчасово безглуздими --
            // адмін посеред правки, -- і затирати цим робочу сітку не можна.
            OZR_Log.Warn("ether not derived: " + plan.Why + " - the frequency file is left as it was");
            return;
        }

        // ПИШЕМО ТЕКСТ САМІ, а не через JsonFileLoader. Він серіалізує те, що
        // лишилось від числа у float32: 0.0125 виходить як
        // 0.012500000186264515, і файл, який має бути джерелом правди про крок,
        // виглядає як помилка. Виведений крок за побудовою кратний 0.0001 МГц
        // (див. UNITS_PER_MHZ), тож чотирьох знаків достатньо рівно завжди.
        string body = "{ \"base_mhz\": " + OZR_Fmt.Fixed(plan.BaseMHz, 4);
        body += ", \"step_mhz\": " + OZR_Fmt.Fixed(plan.StepMHz, 4);
        body += ", \"count\": " + plan.Count.ToString() + " }";

        FileHandle fh = OpenFile(OZR_Const.FREQUENCIES, FileMode.WRITE);
        if (fh == 0)
        {
            OZR_Log.Error("cannot open " + OZR_Const.FREQUENCIES + " for writing");
            return;
        }
        FPrintln(fh, body);
        CloseFile(fh);

        string said = "ether derived from profiles: " + OZR_Ether.Describe(plan);
        if (Matches())
            said += " (in effect)";
        else
            said += " - RESTART THE SERVER to apply; the running ether is still " + Running();

        OZR_Log.Info(said);
    }

    // Чи те, що ми вивели, збігається з тим, що рушій справді роздає зараз.
    static bool Matches()
    {
        if (!s_Plan || !s_Plan.Ok || !OZR_Grid.Ready())
            return false;

        if (OZR_Grid.Count() != s_Plan.Count)
            return false;

        // Допуск -- сота частина кроку: сітка ВИМІРЯНА, і вимір іде через
        // float32, тож вимагати побітової рівності означало б вимагати
        // неможливого.
        float tol = s_Plan.StepMHz * 0.01;
        if (Math.AbsFloat(OZR_Grid.Base() - s_Plan.BaseMHz) > tol)
            return false;
        if (Math.AbsFloat(OZR_Grid.StepMHz() - s_Plan.StepMHz) > tol)
            return false;
        return true;
    }

    static string Running()
    {
        if (!OZR_Grid.Ready())
            return "not an even grid";

        string s = OZR_Fmt.MHz(OZR_Grid.Base());
        s += " to " + OZR_Fmt.MHz(OZR_Grid.MHzAt(OZR_Grid.Count() - 1));
        s += " MHz, step " + OZR_Fmt.Step(OZR_Grid.StepMHz());
        s += ", " + OZR_Grid.Count().ToString() + " divisions";
        return s;
    }
}
