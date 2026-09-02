// Ванільний голос і наша гашетка на ОДНІЙ клавіші.
//
// Задача власника, дослівно: стандартна клавіша -- лише проксиміті, а друга
// клавіша -- і проксиміті, і передача в рацію. Одним натисканням, без
// утримання двох кнопок.
//
// ЧОМУ ЦЕ НЕ РОБИТЬСЯ КОДОМ НАПРЯМУ. Почати передачу голосу скрипт не може:
// рушій дає лише EnableVoN (дозвіл, яким ваніль затикає мертвого) і
// SetVoiceLevel (шепіт/мова/крик). Сама передача -- нативний ввід
// UAVoiceOverNet, і виклику «говорити» не існує.
//
// Зате існує друге: прив'язки правляться зі скрипта. UAInput має
// AddAlternative / SelectAlternative / BindComboByHash, а UAInputAPI --
// UpdateControls і Export. Тобто клавішу нашого PTT можна ДОДАТИ ванільному
// голосу другою прив'язкою, лишивши першу (CapsLock) недоторканою. Тоді
// стандартна клавіша говорить лише вголос, а наша -- і вголос, і в ефір.
//
// Раніше тут стояла порада зробити це руками в екрані керування. Вона була
// дана перевіреною наполовину: що спрацьовує НАШ ввід -- виміряно, що при
// цьому спрацьовує ванільний -- ні. Тому нижче ще й спостереження: кожна
// зміна стану голосу пише рядок, і збіг двох половин однієї клавіші більше
// не питання віри.
//
// ТІЛЬКИ ДОДАЄМО. Жодного ClearBinding, ClearAlternative чи перепризначення:
// це чужий профіль керування, і мод, який переставляє гравцеві клавіші, --
// це мод, який їх колись зітре.

class OZR_Von
{
    private static bool s_Watching = false;
    private static bool s_Mirrored = false;

    // --------------------------------------------------------- observation

    static void Watch()
    {
        if (s_Watching)
            return;

        VONManagerBase von = VONManager.GetInstance();
        if (!von || !von.m_OnVonStateEvent)
        {
            OZR_Log.Warn("von: no VONManager to watch - the vanilla voice key stays invisible in this log");
            return;
        }

        von.m_OnVonStateEvent.Insert(OnVonChanged);
        s_Watching = true;
    }

    // Кожен Insert має свій Remove: статик переживає перезапуск місії, а
    // підписка, зроблена двічі, писала б кожен рядок двічі.
    static void Unwatch()
    {
        if (!s_Watching)
            return;

        VONManagerBase von = VONManager.GetInstance();
        if (von && von.m_OnVonStateEvent)
            von.m_OnVonStateEvent.Remove(OnVonChanged);

        s_Watching = false;
    }

    static void OnVonChanged()
    {
        Mission m = GetGame().GetMission();
        if (!m)
            return;

        string said = "von: vanilla voice ";
        if (m.IsVoNActive())
            said += "ON";
        else
            said += "off";

        OZR_Log.Info(said);
    }

    // ------------------------------------------------------------ the wiring

    // Повісити клавішу нашого PTT другою прив'язкою на ванільний голос.
    //
    // Робиться один раз за сесію й ІДЕМПОТЕНТНО: якщо така прив'язка вже є --
    // нічого не чіпаємо. Інакше кожен запуск додавав би ще одну альтернативу,
    // і за тиждень у гравця в налаштуваннях був би стовпчик однакових рядків.
    static void Mirror()
    {
        if (s_Mirrored)
            return;

        // Поки сервер не сказав своє, умовчання -- не підстава чіпати чужі
        // клавіші.
        if (!OZR_Audio.Got())
            return;

        if (!OZR_Audio.MirrorPtt())
        {
            OZR_Log.Info("von: mirroring is off in the server settings - the radio key opens the air only");
            s_Mirrored = true;
            return;
        }

        UAInput ptt = GetUApi().GetInputByName(OZR_Const.INPUT_PTT);
        UAInput von = GetUApi().GetInputByName(OZR_Const.INPUT_VOICE);

        if (!ptt || !von)
        {
            OZR_Log.Warn("von: no " + OZR_Const.INPUT_PTT + " or no " + OZR_Const.INPUT_VOICE + " - cannot mirror");
            return;
        }

        // Що саме прив'язано до нашої гашетки. Нуль означає, що гравець її
        // зняв -- тоді дзеркалити нічого, і це не помилка.
        if (ptt.BindKeyCount() < 1)
        {
            OZR_Log.Info("von: " + OZR_Const.INPUT_PTT + " has no key bound - nothing to mirror");
            s_Mirrored = true;
            return;
        }

        int key = ptt.GetBindKey(0);

        string tell = "von: mirroring key " + key.ToString();
        tell += " (" + GetUApi().GetButtonName(key) + ")";
        tell += " onto " + OZR_Const.INPUT_VOICE;
        OZR_Log.Info(tell);

        int keep = von.AlternativeIndex();

        if (HasKey(von, key))
        {
            OZR_Log.Info("von: already there - left alone");
            von.SelectAlternative(keep);
            s_Mirrored = true;
            return;
        }

        von.AddAlternative();
        von.SelectAlternative(von.AlternativeCount() - 1);
        von.BindComboByHash(key);

        // UpdateControls -- щоб зміна подіяла зараз, Export -- щоб пережила
        // перезапуск гри. Без другого гравець отримував би мовчазне
        // повернення до старого стану при кожному наступному вході.
        GetUApi().UpdateControls();
        GetUApi().Export();

        bool ok = HasKey(von, key);
        von.SelectAlternative(keep);

        if (ok)
            OZR_Log.Info("von: added as alternative " + (von.AlternativeCount() - 1).ToString() + " - one key now speaks and transmits");
        else
            OZR_Log.Warn("von: the binding did not take - the engine kept its own answer; bind it by hand in Controls");

        s_Mirrored = true;
    }

    // Чи є ця клавіша серед прив'язок вводу -- у БУДЬ-ЯКІЙ альтернативі.
    //
    // Перебирати доводиться саме так: BindKeyCount і GetBindKey відповідають
    // лише про ВИБРАНУ альтернативу, тож питання «чи є вона взагалі» без
    // перемикання не має відповіді.
    private static bool HasKey(UAInput input, int key)
    {
        int alts = input.AlternativeCount();
        for (int a = 0; a < alts; a++)
        {
            input.SelectAlternative(a);

            int keys = input.BindKeyCount();
            for (int k = 0; k < keys; k++)
            {
                if (input.GetBindKey(k) == key)
                    return true;
            }
        }
        return false;
    }
}
