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
        return Fixed(mhz, 3);
    }

    // Крок буває дрібнішим за підпис частоти: 0.0125 у трьох знаках стає
    // 0.013, тобто НЕ тим числом, яке доведеться ввести назад. Там, де число
    // редагують, а не читають, знаків треба стільки, скільки в ньому є.
    static string Step(float mhz)
    {
        return Fixed(mhz, 4);
    }

    static string Fixed(float v, int places)
    {
        if (v < 0)
            return "---";

        int scale = 1;
        int i;
        for (i = 0; i < places; i++)
            scale = scale * 10;

        int whole = Math.Floor(v);
        int frac  = Math.Round((v - whole) * scale);

        // Округлення могло перекинути дробову частину через одиницю.
        if (frac >= scale)
        {
            whole = whole + 1;
            frac  = frac - scale;
        }

        string tail = frac.ToString();
        while (tail.Length() < places)
            tail = "0" + tail;

        return whole.ToString() + "." + tail;
    }
}
