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

// Клієнтська копія ефіру. Порожня, поки сервер не відповів, і кожен, хто нею
// користується, зобов'язаний це перевірити: до відповіді малювати частоту нема
// з чого, і намалювати ванільні 87.8 було б гірше, ніж не малювати нічого.
//
// НАПОВНЮЄТЬСЯ ЧИСЛАМИ, А НЕ JSON-ОМ, і це не смак. Раніше сюди їхав один
// рядок із усім одразу -- сітка й усі профілі, -- і на одинадцятому профілі він
// переріс межу рушія: рядок-значення ріжеться на 1023 байтах, а обробник падає
// з «String CORRUPTED - FIX OnStoreLoad()». Причому падає ТИХО з точки зору
// гравця: сітка просто не приїжджає, і сторінка чесно пише «сервер ще не
// сказав».
//
// Тепер сітка -- три числа, профіль -- чотири поля, кожен своїм пакетом. Межі
// довжини тут немає взагалі, бо немає рядка, який можна переростити.
class OZR_ClientGrid
{
    private static float s_Base  = 0;
    private static float s_Step  = 0;
    private static int   s_Count = 0;

    private static ref array<ref OZR_RadioProfile> s_Radios;

    // Сітка приходить ПЕРШОЮ й скидає перелік профілів: пакети йдуть
    // гарантованими й по порядку, тож усе, що приїде після неї, належить їй.
    static void SetGrid(float base, float step, int count)
    {
        s_Base   = base;
        s_Step   = step;
        s_Count  = count;
        s_Radios = new array<ref OZR_RadioProfile>();
    }

    static void AddProfile(string className, float lo, float hi, float step)
    {
        if (!s_Radios)
            s_Radios = new array<ref OZR_RadioProfile>();

        OZR_RadioProfile p = new OZR_RadioProfile();
        p.ClassName = className;
        p.MinMHz    = lo;
        p.MaxMHz    = hi;
        p.StepMHz   = step;
        s_Radios.Insert(p);
    }

    // Дзеркало серверного OZR_Grid.Ready(), наскільки клієнт узагалі здатен:
    // рівномірність звідси не видно (їдуть три числа, а не таблиця), тому
    // сервер більше й не надсилає нерівну -- див. OZR_Module.OZR_GridReq.
    // Тут лишається те, що перевіряється: сітка з двох ділень -- це пряма
    // через дві точки, і рахувати по ній частоти так само безпідставно, як
    // по ванільній вісімці. Поріг той самий, що на сервері: менше трьох --
    // не сітка.
    static bool Ready()
    {
        return s_Count > 2 && s_Step > 0;
    }

    static float MHzAt(int index)
    {
        if (!Ready())
            return 0;
        return s_Base + index * s_Step;
    }

    static int IndexOf(float mhz)
    {
        if (!Ready())
            return 0;
        return Math.Round((mhz - s_Base) / s_Step);
    }

    static int Count()
    {
        if (!Ready())
            return 0;
        return s_Count;
    }

    // Віддаються ПРЯМО, а не через різницю сусідніх ділень: віднімання двох
    // близьких float дає 0.01249695 замість 0.0125, і на цьому вже раз
    // погоріла математика кроку.
    static float BaseMHz()
    {
        if (!Ready())
            return 0;
        return s_Base;
    }

    static float StepMHz()
    {
        if (!Ready())
            return 0;
        return s_Step;
    }

    // Скільки профілів доїхало. Потрібне рівно для однієї відповіді -- «нуль»
    // проти «є, але не для цієї рації», -- і без нього ці два випадки в
    // діагностиці нерозрізненні.
    static int ProfileCount()
    {
        if (!s_Radios)
            return 0;
        return s_Radios.Count();
    }

    static OZR_RadioProfile For(string className)
    {
        if (!s_Radios)
            return null;

        for (int i = 0; i < s_Radios.Count(); i++)
        {
            if (s_Radios[i].ClassName == className)
                return s_Radios[i];
        }
        return null;
    }

    static void All(out array<ref OZR_RadioProfile> outList)
    {
        outList = new array<ref OZR_RadioProfile>();
        if (!s_Radios)
            return;
        for (int i = 0; i < s_Radios.Count(); i++)
            outList.Insert(s_Radios[i]);
    }
}
