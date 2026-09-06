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

            // ОДИН предикат на всіх -- див. OZR_RadioProfile.Problem. Тут він
            // ВІДМОВЛЯЄ цілком: ефір виводиться з усього набору одразу, і
            // порахувати його «без одного профілю» означало б віддати адмінові
            // сітку, якої він не просив.
            string bad = p.Problem();
            if (bad != "")
            {
                plan.Why = p.Named() + " " + bad;
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

        // Стеля -- РІШЕННЯ ВЛАСНИКА, а не властивість рушія (2026-09-01,
        // ТЗ-5 R-E4). Обґрунтування через синхрозмінну тут стояло й більше не
        // діє: RegisterNetSyncVariableInt оголошений БЕЗ меж (OZR_Tuning), тож
        // ділення возить будь-яке. Лишається саме число -- 65536 ділень ефіру,
        // і крапка; єдине місце, де воно тепер щось значить, -- ця відмова.
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

    // ОПИС СІТКИ ЧИСЛАМИ -- одне формулювання на всіх.
    //
    // Той самий рядок «низ to верх MHz, step X, N divisions» був написаний
    // тричі: тут, у OZR_EtherServer.Running (з виміряної сітки) і у вкладці
    // VPP (з клієнтської копії). Власний коментар при цьому вимагав, щоб
    // сервер і панель казали ОДНЕ Й ТЕ САМЕ -- вимога, яку три копії
    // виконують лише доти, доки їх не правили нарізно.
    static string DescribeGrid(float baseMHz, float step, int count)
    {
        if (count < 1)
            return "-";

        string s = OZR_Fmt.MHz(baseMHz);
        s += " to " + OZR_Fmt.MHz(baseMHz + (count - 1) * step);
        s += " MHz, step " + OZR_Fmt.Step(step);
        s += ", " + count.ToString() + " divisions";
        return s;
    }

    // Один рядок про те, що вийде. Однаковий у лозі сервера й у вкладці --
    // навмисне: два різні описи одного числа читаються як два різні числа.
    static string Describe(OZR_EtherPlan plan)
    {
        if (!plan)
            return "-";
        if (!plan.Ok)
            return plan.Why;

        return DescribeGrid(plan.BaseMHz, plan.StepMHz, plan.Count);
    }

    // Чи план збігається з описаною трьома числами сіткою.
    //
    // Допуск -- сота частина кроку: жива сітка ВИМІРЯНА, і вимір іде через
    // float32, тож вимагати побітової рівності означало б вимагати
    // неможливого. Порівняння теж було написане двічі -- на сервері й у
    // вкладці, -- з тим самим ризиком розійтись.
    static bool Same(OZR_EtherPlan plan, float baseMHz, float step, int count)
    {
        if (!plan || !plan.Ok || count < 1)
            return false;

        if (count != plan.Count)
            return false;

        float tol = plan.StepMHz * 0.01;
        if (Math.AbsFloat(baseMHz - plan.BaseMHz) > tol)
            return false;
        if (Math.AbsFloat(step - plan.StepMHz) > tol)
            return false;

        return true;
    }
}
