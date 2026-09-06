// Гучність і чутність сплеску, як їх бачить клієнт.
//
// ТУТ БІЛЬШЕ НЕМАЄ ГОЛОСУ, і це висновок з виміру, а не спрощення.
//
// Було: множник на SoundScene.SetRadioVolume -- єдиний регулятор радіоканалу,
// який рушій узагалі дає скрипту, і якого немає в налаштуваннях гри (у
// вкладці звуку п'ять повзунків, і радіо серед них нема). Виглядало як
// точний інструмент.
//
// Виміряно на живому клієнті 2026-09-02 з множником 10:
//
//     radio voice: base 1 x 10 = 10; engine now reports 10
//
// Рушій число ПРИЙНЯВ і повернув недоторканим -- не обрізав, не відкинув, --
// а гучності голосу в рації це не змінило нічого. Тобто канал існує, значення
// зберігається, і на те, що чути з рації, воно не впливає. Ручку прибрано:
// код, який обіцяє те, чого не робить, гірший за відсутній, бо наступного
// разу на нього покладуться.
//
// Лишається сплеск -- звук НАШ, і з ним обидва виміри чесні: гучність
// множиться тут, а чутність вибирається ступенем набору (див. config.cpp).

class OZR_Audio
{
    // 1.0 -- «нічого не чіпати». Саме тому одиниця, а не нуль: поле, яке
    // забули заповнити, мусить лишати гру такою, якою її зробила ваніль.
    private static float s_SquelchGain = 1.0;

    // BOOL, а не float. Прапорець їхав числом і порівнювався з нулем назад,
    // хоч сусідній пакет того ж модуля вже возить Param2<bool, bool>: дві
    // мови для одного типу в одному моді -- це запрошення переплутати.
    private static bool s_MirrorPtt = true;

    // Чи сервер уже сказав своє. До цього моменту значення вище -- лише
    // умовчання, і діяти за ними означало б, наприклад, переписати гравцеві
    // клавіші на сервері, який цього не просив.
    private static bool s_Got = false;

    // Ступінь лісенки, а не просимі метри: перерахунок робиться один раз при
    // отриманні, щоб кожен сплеск не питав те саме.
    private static int s_SquelchRung = OZR_Const.SQUELCH_RANGE_DEFAULT;

    // Не про звук, і живе тут лише тому, що їде тим самим пакетом. Клієнту
    // воно потрібне рівно для іконки: та мусить світитись тоді й тільки
    // тоді, коли натискання щось відкриє.
    private static bool s_PttFromCargo = false;

    static float SquelchGain()
    {
        return s_SquelchGain;
    }

    static bool MirrorPtt()
    {
        return s_MirrorPtt;
    }

    static bool Got()
    {
        return s_Got;
    }

    static int SquelchRung()
    {
        return s_SquelchRung;
    }

    static bool PttFromCargo()
    {
        return s_PttFromCargo;
    }

    static void SetGains(float squelch, bool mirror, int range, bool cargo)
    {
        s_PttFromCargo = cargo;

        int rung = OZR_Const.SquelchRung(range);
        if (rung != range)
        {
            string step = "squelch range: asked for " + range.ToString() + " m, ";
            step += "using the nearest step " + rung.ToString() + " m";
            OZR_Log.Info(step);
        }
        s_SquelchRung = rung;

        if (squelch > 0)
            s_SquelchGain = squelch;

        s_MirrorPtt = mirror;
        s_Got       = true;
    }
}
