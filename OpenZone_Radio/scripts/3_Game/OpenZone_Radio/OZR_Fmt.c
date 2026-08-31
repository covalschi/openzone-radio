// Як показувати частоту.
//
// float.ToString() тут не годиться: він дає то "145", то "145.125000", і
// підпис у HUD стрибав би шириною на кожному кроці настройки. Три знаки після
// коми -- рівно стільки, скільки розрізняє найдрібніший крок сітки (0.0125
// МГц), і жодного зайвого.

class OZR_Fmt
{
    static string MHz(float mhz)
    {
        if (mhz < 0)
            return "---";

        int whole = Math.Floor(mhz);
        int milli = Math.Round((mhz - whole) * 1000);

        // Округлення могло перекинути дробову частину через одиницю.
        if (milli >= 1000)
        {
            whole  = whole + 1;
            milli  = milli - 1000;
        }

        string frac = milli.ToString();
        while (frac.Length() < 3)
            frac = "0" + frac;

        return whole.ToString() + "." + frac;
    }
}
