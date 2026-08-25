// Таблиця частот рушія.
//
// НЕ ПРИПУЩЕННЯ, А ВИМІР. Скрізь пишуть «у DayZ сім частот», але число сім
// ніде в скриптах не оголошене: воно живе в рушії, і на іншій збірці могло б
// бути іншим. Тому при старті сервера мод створює одну ванільну рацію, обходить
// індекси, поки частота не почне повторюватись, записує знайдене й прибирає
// рацію за собою.
//
// Навіщо це потрібно. Віртуальні канали КПК треба на щось відображати, і
// відображати їх можна ЛИШЕ на ці частоти -- інших рушій не має. Скільки їх,
// вирішує, скільки розмов можуть звучати одночасно, не змішуючись; тому це
// число -- перше, що мод мусить про себе знати.

class OZR_Bands
{
    private static ref array<float> s_Freqs;

    static int Count()
    {
        if (!s_Freqs)
            return 0;
        return s_Freqs.Count();
    }

    static float At(int index)
    {
        if (!s_Freqs || index < 0 || index >= s_Freqs.Count())
            return 0;
        return s_Freqs[index];
    }

    // Кличеться один раз, на старті сервера.
    static void Probe()
    {
        s_Freqs = new array<float>();

        // Створюємо ПОЗА світом, у порожнечі: предмет живе кілька мілісекунд
        // і не має ні впасти комусь під ноги, ні потрапити в збереження.
        // ECE_NOLIFETIME -- щоб центральна економіка не рахувала його своїм.
        Object obj = GetGame().CreateObjectEx(OZR_Const.BAND_PROBE_CLASS, "0 0 0", ECE_NOLIFETIME | ECE_LOCAL);
        ItemTransmitter probe = ItemTransmitter.Cast(obj);

        if (!probe)
        {
            // Ванільної рації немає -- сервер зібраний якось інакше. Це не
            // падіння, але й вигадувати таблицю ми не будемо: без неї мод
            // просто не дасть каналів, і скаже про це прямо.
            OZR_Log.Warn("no " + OZR_Const.BAND_PROBE_CLASS + " to measure the band table on; radio channels stay unavailable");
            if (obj)
                GetGame().ObjectDelete(obj);
            return;
        }

        float first = 0;

        for (int i = 0; i < OZR_Const.BAND_PROBE_MAX; i++)
        {
            probe.SetFrequencyByIndex(i);

            // Читаємо НАЗАД, а не віримо, що присвоєння вдалося: саме тут
            // видно, чи рушій приймає індекс миттєво, і де він обривається.
            int got = probe.GetTunedFrequencyIndex();
            float hz = probe.GetTunedFrequency();

            if (got != i)
            {
                // Рушій не пустив далі -- межа знайдена.
                break;
            }

            if (i > 0 && hz == first)
            {
                // Пішло по колу: індекси більше не нові.
                break;
            }

            if (i == 0)
                first = hz;

            s_Freqs.Insert(hz);
        }

        GetGame().ObjectDelete(obj);

        string line = "band table measured: " + s_Freqs.Count().ToString() + " frequencies";
        for (int k = 0; k < s_Freqs.Count(); k++)
        {
            line += "  " + s_Freqs[k].ToString();
        }
        OZR_Log.Info(line);
    }
}
