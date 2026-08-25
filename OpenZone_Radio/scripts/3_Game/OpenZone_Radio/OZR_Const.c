// Константи мода рації.

class OZR_Const
{
    static const string LOG_PREFIX = "[OpenZone/Radio] ";

    // Сторінка «Рація» в КПК. Її вмикає модуль рації, а не профіль: без
    // плати рації сторінці нема що показувати.
    static const string PAGE_RADIO = "radio";

    // Вид модуля в договорі OZ_PdaHardware.
    static const string MOD_RADIO = "radio";

    // Скільки індексів частот перебирати, шукаючи межу рушія. Число завідомо
    // більше за очікуване: межу шукаємо, а не припускаємо.
    static const int BAND_PROBE_MAX = 32;

    // Клас предмета, на якому міряємо таблицю частот. Ванільна рація --
    // єдиний ItemTransmitter, який точно є на будь-якому сервері.
    static const string BAND_PROBE_CLASS = "PersonalRadio";
}
