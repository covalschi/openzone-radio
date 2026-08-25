// Сторінка «Рація»: канали, налаштування, PTT.
//
// Тут немає нічого про звук, і це не пропуск. Голос веде рушій: він дивиться,
// який передавач у гравця й на що той налаштований. Наша справа -- поставити
// смугу й у потрібну мить увімкнути передачу; далі рушій робить усе сам.

class OZ_PdaHandlerRadio : OZ_PageHandler
{
    override string Handle(string op, string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok    = false;
        error = "STR_OZ_ERR_UNKNOWN_OP";

        if (op == "list")
            return List(sender, ok, error);

        if (op == "tune")
            return Tune(json, sender, ok, error);

        if (op == "ptt")
            return Ptt(json, sender, ok, error);

        return "";
    }

    // ------------------------------------------------------------- перелік

    private string List(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        string uid = sender.GetPlainId();
        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return "";
        }

        OZ_Module_Radio board = OZR_Set.BoardIn(pda);

        OZR_RadioState st = new OZR_RadioState();
        st.HasBoard = (board != null);
        st.Powered  = pda.OZ_IsOn();
        st.RangeM   = OZR_Set.AntennaRange(pda);
        st.Live     = board && board.OZR_IsLive();

        if (board)
        {
            OZR_Channel here = OZR_Settings.ByBand(board.GetTunedFrequencyIndex());
            if (here)
                st.Current = here.Id;
        }

        OZR_Settings cfg = OZR_Settings.Get();
        for (int i = 0; cfg && cfg.Channels && i < cfg.Channels.Count(); i++)
        {
            OZR_Channel c = cfg.Channels[i];

            OZR_ChannelInfo info = new OZR_ChannelInfo();
            info.Id      = c.Id;
            info.Name    = c.Name;
            info.Band    = c.Band;
            info.Freq    = OZR_Bands.At(c.Band);
            info.Allowed = OZR_Set.Allowed(uid, c);
            info.Current = (c.Id == st.Current);
            st.Items.Insert(info);
        }

        string outJson;
        string err;
        if (!JsonFileLoader<OZR_RadioState>.MakeData(st, outJson, err, false))
        {
            OZR_Log.Error("radio state serialise failed: " + err);
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return outJson;
    }

    // ---------------------------------------------------------- настройка

    private string Tune(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZR_TuneRef r;
        string err;
        if (!JsonFileLoader<OZR_TuneRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_Module_Radio board;
        if (!Ready(sender, board, error))
            return "";

        OZR_Channel c = OZR_Settings.ById(r.Id);
        if (!c)
        {
            error = "STR_OZR_ERR_NO_CHANNEL";
            return "";
        }

        // Фракцію звіряє СЕРВЕР, і тільки він. Клієнт малює заборонений канал
        // сірим, але сіре в розмітці нікого ні від чого не тримає.
        if (!OZR_Set.Allowed(sender.GetPlainId(), c))
        {
            error = "STR_OZR_ERR_NOT_YOURS";
            return "";
        }

        board.SetFrequencyByIndex(c.Band);

        // Перечитуємо НАЗАД: присвоєння індексу -- прохання до рушія, а не
        // факт, і саме тут видно, чи він його прийняв.
        if (board.GetTunedFrequencyIndex() != c.Band)
        {
            string miss = "engine refused band " + c.Band.ToString();
            miss += " for channel \"" + c.Id + "\"";
            OZR_Log.Warn(miss);
            error = "STR_OZR_ERR_NO_CHANNEL";
            return "";
        }

        ok = true;
        error = "";
        return "";
    }

    // ---------------------------------------------------------------- PTT

    private string Ptt(string json, PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

        OZR_PttRef r;
        string err;
        if (!JsonFileLoader<OZR_PttRef>.LoadData(json, r, err) || !r)
        {
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        OZ_Module_Radio board;
        if (!Ready(sender, board, error))
        {
            // Відпустити кнопку можна й тоді, коли говорити вже нічим --
            // наприклад, батарея сіла посеред фрази. Мовчки погоджуємось.
            if (!r.On)
            {
                ok = true;
                error = "";
            }
            return "";
        }

        board.OZR_Speak(r.On);

        ok = true;
        error = "";
        return "";
    }

    // ------------------------------------------------------------- спільне

    // Пристрій у руках, живлення є, антена є, плата є. Порядок перевірок
    // такий, щоб гравець побачив ПЕРШУ справжню причину, а не останню.
    private bool Ready(PlayerIdentity sender, out OZ_Module_Radio board, out string error)
    {
        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return false;
        }

        if (!pda.OZ_IsOn())
        {
            error = "STR_OZ_ERR_NO_POWER";
            return false;
        }

        board = OZR_Set.BoardIn(pda);
        if (!board)
        {
            error = "STR_OZR_ERR_NO_BOARD";
            return false;
        }

        if (OZR_Set.AntennaRange(pda) <= 0)
        {
            error = "STR_OZR_ERR_NO_ANTENNA";
            return false;
        }

        error = "";
        return true;
    }
}
