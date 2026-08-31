// Ефір ВИВОДИТЬСЯ з профілів, а не задається окремо.
//
// Раніше це були дві незалежні речі: сітка рушія жила у своєму файлі поруч із
// нативним патчем, а профілі рацій -- у своєму, і узгоджувати їх доводилось
// руками. Узгодження при цьому не мало жодного зовнішнього прояву: профіль, що
// не вліз у сітку, не ламався голосно -- він мовчки обрізався. Тобто адмін мав
// тримати в голові інваріант, про порушення якого ніхто не питав.
//
// Тепер питання «яким має бути ефір» не ставиться взагалі. Ефір -- це рівно те,
// що просять рації:
//
//   низ  -- найнижча межа серед усіх профілів;
//   верх -- найвища;
//   крок -- найбільший спільний дільник УСІХ кроків І УСІХ зміщень меж від низу.
//
// Останнє -- не педантизм. Крок мусить ділити не тільки інші кроки, а й
// відстань від низу до кожної межі: інакше межа лягає між діленнями, і рація
// стає не туди, куди написано. НСД по обох множинах одразу дає найбільший крок,
// при якому кожне число з профілів лягає точно.
//
// РАХУЄМО В ЦІЛИХ. НСД по float -- спосіб отримати сміття: 0.0125 у float32
// насправді 0.012500000186, і будь-яка перевірка «націло» на ньому бреше. Тому
// все переводиться в десятитисячні мегагерца (0.1 кГц), і далі це звичайна
// цілочисельна арифметика, точна за побудовою. Верх 152 МГц -- це 1 520 000
// одиниць, що вкладається і в int, і у float32 без втрат.
//
// ЖИВЕ В 3_Game НАВМИСНЕ. Рахує це і сервер (щоб записати файл), і адмінська
// вкладка на клієнті (щоб попередити ДО збереження, а не після рестарту). Дві
// копії цієї арифметики розійшлися б, і розійшлися б тихо.

// Що вийшло з профілів -- разом із причиною, коли не вийшло нічого.
class OZR_EtherPlan
{
    bool   Ok        = false;
    float  BaseMHz   = 0;
    float  StepMHz   = 0;
    int    Count     = 0;

    // Скільки ділень знадобилось би насправді. Заповнюється й тоді, коли
    // Ok == false через стелю: число, якого не вистачило, -- це і є відповідь
    // на питання «наскільки я промахнувся».
    int    Needed    = 0;
    string Why       = "";

    float TopMHz()
    {
        if (!Ok)
            return 0;
        return BaseMHz + (Count - 1) * StepMHz;
    }
}

class OZR_Ether
{
    // Одиниця обліку: 0.1 кГц. Дрібніше за будь-який осмислений крок рації і
    // достатньо грубо, щоб float32 переводився в неї без похибки.
    private static const float UNITS_PER_MHZ = 10000.0;

    private static int Units(float mhz)
    {
        return Math.Round(mhz * UNITS_PER_MHZ);
    }

    private static int Gcd(int a, int b)
    {
        if (a < 0)
            a = -a;
        if (b < 0)
            b = -b;

        while (b != 0)
        {
            int t = a % b;
            a = b;
            b = t;
        }
        return a;
    }

    // Порахувати ефір за набором профілів. Ніколи не повертає null.
    static OZR_EtherPlan Derive(array<ref OZR_RadioProfile> radios)
    {
        OZR_EtherPlan plan = new OZR_EtherPlan();

        if (!radios || radios.Count() == 0)
        {
            plan.Why = "there are no radio profiles to derive an ether from";
            return plan;
        }

        int i;
        int loU   = 0;
        int hiU   = 0;
        bool any  = false;

        for (i = 0; i < radios.Count(); i++)
        {
            OZR_RadioProfile p = radios[i];
            if (!p)
                continue;

            if (p.MinMHz <= 0)
            {
                plan.Why = p.ClassName + " starts at or below zero MHz";
                return plan;
            }
            if (p.MaxMHz <= p.MinMHz)
            {
                plan.Why = p.ClassName + " has an empty or inverted band";
                return plan;
            }
            if (p.StepMHz <= 0)
            {
                plan.Why = p.ClassName + " has a step of zero";
                return plan;
            }

            int a = Units(p.MinMHz);
            int b = Units(p.MaxMHz);

            if (!any)
            {
                loU = a;
                hiU = b;
                any = true;
            }
            else
            {
                if (a < loU)
                    loU = a;
                if (b > hiU)
                    hiU = b;
            }
        }

        if (!any)
        {
            plan.Why = "there are no usable radio profiles";
            return plan;
        }

        int stepU = 0;
        for (i = 0; i < radios.Count(); i++)
        {
            OZR_RadioProfile q = radios[i];
            if (!q)
                continue;

            stepU = Gcd(stepU, Units(q.StepMHz));
            stepU = Gcd(stepU, Units(q.MinMHz) - loU);
            stepU = Gcd(stepU, Units(q.MaxMHz) - loU);
        }

        if (stepU <= 0)
        {
            plan.Why = "every profile asks for one and the same frequency";
            return plan;
        }

        plan.Needed = ((hiU - loU) / stepU) + 1;

        // Стеля НЕ вигадана. Налаштований індекс їде до клієнта синхрозмінною,
        // оголошеною на OZR_Const.INDEX_MAX (див. OZR_Tuning) -- це і є межа
        // рушія в наших руках. Ділення понад неї існувало б на сервері й було б
        // невидиме гравцеві: рація стояла б на частоті, якої її власник не
        // бачить. Краще відмовити тут.
        if (plan.Needed > OZR_Const.INDEX_MAX + 1)
        {
            string big = "these bands and steps need " + plan.Needed.ToString();
            big += " divisions, and the highest index that can reach a player is ";
            big += OZR_Const.INDEX_MAX.ToString();
            big += " - use a coarser step, or a narrower spread of bands";
            plan.Why = big;
            return plan;
        }

        plan.Ok      = true;
        plan.BaseMHz = loU / UNITS_PER_MHZ;
        plan.StepMHz = stepU / UNITS_PER_MHZ;
        plan.Count   = plan.Needed;
        return plan;
    }

    // Один рядок про те, що вийде. Однаковий у лозі сервера й у вкладці --
    // навмисне: два різні описи одного числа читаються як два різні числа.
    static string Describe(OZR_EtherPlan plan)
    {
        if (!plan)
            return "-";
        if (!plan.Ok)
            return plan.Why;

        string s = OZR_Fmt.MHz(plan.BaseMHz);
        s += " to " + OZR_Fmt.MHz(plan.TopMHz());
        s += " MHz, step " + OZR_Fmt.Step(plan.StepMHz);
        s += ", " + plan.Count.ToString() + " divisions";
        return s;
    }
}
