// Лампочка передавача.
//
// ГОРИТЬ РІВНО ПОКИ ЙДЕ ПЕРЕДАЧА -- поки тримають клавішу або поки стоїть
// защіпка. В усіх інших випадках її на екрані немає взагалі.
//
// Перша версія світилась постійно, поки при гравцеві є жива рація, і показувала
// «ефір закритий» окремим сірим станом. Це помилка того ж роду, що й індикатор,
// який ніколи не гасне: лампочка, яка горить завжди, перестає читатись саме в
// ту мить, коли починає щось означати. Те, що рація при тобі, видно в
// інвентарі; знати треба інше -- чи чути тебе просто зараз.
//
// Підпис розрізняє два випадки, які на вигляд однакові:
//   ON AIR -- говоримо, поки тримають клавішу;
//   LOCKED -- защіпка: говоритимемо й після того, як клавішу відпустили.
//
// Другий -- це те, чим видають себе цілими групами, тож він мусить читатись з
// кутка ока. Іконка в обох випадках біла: колір тут не носить значення, його
// носить сама поява іконки.
//
// Віджет живе на робочій області, поза будь-яким меню, і на нього не можна
// клікнути: див. ignorepointer у розкладці.

class OZR_PttHud
{
    static const int MODE_NONE = 0;   // ховаємось: нічого не передається
    static const int MODE_LIVE = 1;   // говоримо, поки тримають клавішу
    static const int MODE_LOCK = 2;   // защіпка: говоримо й далі

    private static const string LAYOUT = "OpenZone_Radio/gui/layouts/ozr_ptt.layout";

    // Відступ від правого краю й доля висоти екрана. Правий бік -- єдина
    // частина HUD DayZ, де ваніль нічого не малює: знизу по центру квікбар,
    // знизу зліва значки й дії, зверху зліва чат.
    private static const int   MARGIN_X = 200;
    private static const float SCREEN_Y = 0.58;

    private static Widget      s_Root;
    private static ImageWidget s_Icon;
    private static TextWidget  s_State;

    // -1, а не MODE_NONE: перший же виклик мусить пофарбувати віджет, навіть
    // якщо режим виявився нульовим.
    private static int s_Mode = -1;

    private static bool s_Failed = false;

    static void Set(int mode)
    {
        if (mode == s_Mode)
            return;

        if (!Build())
            return;

        s_Mode = mode;

        if (mode == MODE_NONE)
        {
            s_Root.Show(false);
            return;
        }

        string text = "#STR_OZR_PTT_LIVE";
        if (mode == MODE_LOCK)
            text = "#STR_OZR_PTT_LOCK";

        // Підкладка лишається темною й напівпрозорою: біле по світлому небу
        // або по снігу інакше не читається взагалі.
        int white = ARGB(255, 255, 255, 255);
        s_Root.SetColor(ARGB(150, 14, 15, 18));
        s_Icon.SetColor(white);
        s_State.SetColor(white);
        s_State.SetText(text);
        s_Root.Show(true);
    }

    // Гравець помер, місія скінчилась -- віджет мусить піти з екрана разом із
    // нею, інакше він переживе меню й повисне поверх головного.
    static void Drop()
    {
        if (!s_Root)
            return;

        s_Root.Unlink();
        s_Root  = null;
        s_Icon  = null;
        s_State = null;
        s_Mode  = -1;
    }

    private static bool Build()
    {
        if (s_Root)
            return true;

        // Одна скарга на сесію: цей метод кличеться з кожного кадру, і
        // зламана розкладка залила б лог за хвилину.
        if (s_Failed)
            return false;

        s_Root = GetGame().GetWorkspace().CreateWidgets(LAYOUT);
        if (!s_Root)
        {
            s_Failed = true;
            OZR_Log.Error("ptt icon: cannot create " + LAYOUT);
            return false;
        }

        s_Icon  = ImageWidget.Cast(s_Root.FindAnyWidget("PttIcon"));
        s_State = TextWidget.Cast(s_Root.FindAnyWidget("PttState"));

        if (!s_Icon || !s_State)
        {
            s_Failed = true;
            OZR_Log.Error("ptt icon: PttIcon or PttState missing from " + LAYOUT);
            Drop();
            return false;
        }

        Place();
        return true;
    }

    private static void Place()
    {
        // GetScreenSize -- ГЛОБАЛЬНА функція, не метод CGame: у GetGame() такої
        // немає, і виклик через нього не компілюється.
        int w, h;
        GetScreenSize(w, h);

        float fh = h;
        s_Root.SetPos(w - MARGIN_X, fh * SCREEN_Y);
    }
}
