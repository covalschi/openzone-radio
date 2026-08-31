// Профіль рації: який шматок ефіру їй доступний і через скільки ділень вона
// крокує.
//
// У 3_Game, бо профіль потрібен обом бокам: сервер ним ОБМЕЖУЄ налаштування,
// клієнт ним ПІДПИСУЄ частоту. Сама математика сітки живе в 4_World поруч із
// OZR_Bands -- вимірювати сітку можна лише там, де є світ і предмети.
//
// Чому діапазон і крок -- це політика скрипта, а не рушія. Формула рушія одна
// на всі рації: вона отримує індекс і повертає частоту, і про те, ЯКА рація
// питає, нічого не знає. Тому рушій дає одну ЧАСТУ сітку на всіх, а профіль
// каже, які її ділення цій рації дозволені. Дві рації з різними профілями
// зустрічаються там, де їхні відрізки перетинаються -- і не зустрічаються там,
// де ні. Нічого окремого для цього робити не треба.

class OZR_RadioProfile
{
    string ClassName = "";

    // Межі відрізка в МГц, включно.
    float  MinMHz    = 0;
    float  MaxMHz    = 0;

    // Через скільки МГц крокує ця рація. Мусить бути КРАТНИМ кроку сітки --
    // інакше рація стає між діленнями, і з нею не зійдеться ні з ким.
    float  StepMHz   = 0;
}

// Куди гравець перетягнув клавіатуру. Клієнтське й тільки клієнтське: сервера
// це не стосується взагалі, а от переставляти вікно щоразу після входу --
// саме та дрібниця, через яку зручним не користуються.
class OZR_KeypadPos
{
    bool  Set = false;
    float X   = 0;
    float Y   = 0;
}

// Те, що сервер розповідає клієнтові про ефір -- один раз за сесію.
//
// Клієнтові це ПОТРІБНО, і своїми силами він цього не дізнається. Сітку міряє
// OZR_Bands, ганяючи движкові нативи, але на клієнті таблиця рушія лишається
// ванільною: патч серверний. Тому GetTunedFrequency() на клієнті повертає одну
// зі старих восьми частот -- число, якого в нашому ефірі немає взагалі, -- і
// підпис доводиться рахувати самим, з індексу, який синхронізується чесно.
class OZR_GridInfo
{
    float BaseMHz = 0;
    float StepMHz = 0;
    int   Count   = 0;

    ref array<ref OZR_RadioProfile> Radios;

    void OZR_GridInfo()
    {
        Radios = new array<ref OZR_RadioProfile>();
    }
}

// Клієнтська копія. Порожня, поки сервер не відповів, і кожен, хто нею
// користується, зобов'язаний це перевірити: до відповіді малювати частоту
// нема з чого, і намалювати ванільні 87.8 було б гірше, ніж не малювати
// нічого.
class OZR_ClientGrid
{
    private static ref OZR_GridInfo s_Info;

    static void Set(OZR_GridInfo info)
    {
        s_Info = info;
    }

    static bool Ready()
    {
        return s_Info != null && s_Info.Count > 1 && s_Info.StepMHz > 0;
    }

    static float MHzAt(int index)
    {
        if (!Ready())
            return 0;
        return s_Info.BaseMHz + index * s_Info.StepMHz;
    }

    static int IndexOf(float mhz)
    {
        if (!Ready())
            return 0;
        return Math.Round((mhz - s_Info.BaseMHz) / s_Info.StepMHz);
    }

    static int Count()
    {
        if (!Ready())
            return 0;
        return s_Info.Count;
    }

    // Віддаються ПРЯМО, а не через різницю сусідніх ділень: віднімання двох
    // близьких float дає 0.01249695 замість 0.0125, і на цьому вже раз
    // погоріла математика кроку.
    static float BaseMHz()
    {
        if (!Ready())
            return 0;
        return s_Info.BaseMHz;
    }

    static float StepMHz()
    {
        if (!Ready())
            return 0;
        return s_Info.StepMHz;
    }

    static OZR_RadioProfile For(string className)
    {
        if (!Ready() || !s_Info.Radios)
            return null;

        for (int i = 0; i < s_Info.Radios.Count(); i++)
        {
            if (s_Info.Radios[i].ClassName == className)
                return s_Info.Radios[i];
        }
        return null;
    }
}
