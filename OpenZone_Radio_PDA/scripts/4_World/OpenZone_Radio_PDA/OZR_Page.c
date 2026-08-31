// Сторінка «Рація»: стан плати й настройка на ділення.
//
// Тут немає нічого про звук, і це не пропуск. Голос веде рушій: він дивиться,
// який передавач у гравця й на що той налаштований. Наша справа -- поставити
// частоту; далі рушій робить усе сам.
//
// PTT тут теж НЕМАЄ, і це не пропуск удруге. Плата -- звичайна профільна
// рація, а спільний обхід інвентаря в моді рації відкриває ВСІ профільні
// рації гравця, хоч у руках, хоч усередині КПК. Окрема операція «ptt» на цій
// сторінці була б другим шляхом до того самого, і два шляхи рано чи пізно
// розійшлися б у тому, кого вважати увімкненим.

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

        return "";
    }

    // --------------------------------------------------------------- стан

    private string List(PlayerIdentity sender, out bool ok, out string error)
    {
        ok = false;

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

        if (board)
        {
            st.RangeM = OZR_Set.RangeOf(board);
            st.Live   = board.OZR_IsLive();
            st.Index  = board.GetTunedFrequencyIndex();

            if (OZR_Grid.Ready())
                st.FreqMHz = OZR_Grid.MHzAt(st.Index);

            OZR_RadioProfile p = OZR_Profiles.For(board.GetType());
            if (p)
            {
                st.MinMHz  = p.MinMHz;
                st.MaxMHz  = p.MaxMHz;
                st.StepMHz = p.StepMHz;
            }
        }

        string body;
        string err;
        if (!JsonFileLoader<OZR_RadioState>.MakeData(st, body, err, false))
        {
            OZR_Log.Error("radio page: cannot serialise state: " + err);
            error = "STR_OZ_ERR_INTERNAL";
            return "";
        }

        ok = true;
        error = "";
        return body;
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

        // Ті самі перевірки, що й для ручної рації, і навмисно ті самі: плата
        // -- така сама профільна рація, і межі їй ставить її ж профіль.
        OZR_RadioProfile prof = OZR_Profiles.For(board.GetType());
        int lo;
        int hi;
        int stride;
        if (!prof || !OZR_Grid.Window(prof, lo, hi, stride))
        {
            error = "STR_OZR_ERR_NO_PROFILE";
            return "";
        }

        if (r.Index < lo || r.Index > hi)
        {
            error = "STR_OZR_ERR_OUT_OF_BAND";
            return "";
        }

        if (((r.Index - lo) % stride) != 0)
        {
            error = "STR_OZR_ERR_OFF_STEP";
            return "";
        }

        // Через OZR_TuneTo, а не SetFrequencyByIndex: він же й розкаже про нову
        // частоту клієнтові.
        board.OZR_TuneTo(r.Index);

        // Перечитуємо НАЗАД: присвоєння індексу -- прохання до рушія, а не
        // факт, і саме тут видно, чи він його прийняв.
        if (board.GetTunedFrequencyIndex() != r.Index)
        {
            OZR_Log.Warn("radio page: engine refused index " + r.Index.ToString());
            error = "STR_OZR_ERR_OUT_OF_BAND";
            return "";
        }

        ok = true;
        error = "";
        return "";
    }

    private bool Ready(PlayerIdentity sender, out OZ_Module_Radio board, out string error)
    {
        board = null;

        OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(sender);
        if (!pda)
        {
            error = "STR_OZ_ERR_NO_DEVICE";
            return false;
        }

        if (!pda.OZ_IsOn())
        {
            error = "STR_OZ_ERR_OFF";
            return false;
        }

        board = OZR_Set.BoardIn(pda);
        if (!board)
        {
            error = "STR_OZR_ERR_NO_BOARD";
            return false;
        }

        error = "";
        return true;
    }
}
