// Панель «RADIO» в адмiнському вiкнi OpenZone: профiлi ручних рацiй формою.
// Чiпляється вкладкою до вiкна ядра через modded class -- субмод субмода, як
// це вже зроблено в КПК: ядро про рацiю не знає й знати не мусить.
//
// Гарди: NO_GUI -- сервер компiлює Mission без UI; AVPPAdminTools i
// OpenZone_VPP -- iмена класiв CfgMods (їх авто-дефайнить рушiй).
//
// ЧОМУ ЦЕ НЕ РЕДАКТОР JSON. Ефiр НЕ задається окремо -- вiн ВИВОДИТЬСЯ з цих
// самих профiлiв (OZR_Ether.Derive): низ -- найнижча межа, верх -- найвища,
// крок -- найбiльший спiльний дiльник усiх крокiв i всiх змiщень меж. Тому
// профiль не може «не влiзти в сiтку»: сiтка йде за ним.
//
// Але два наслiдки цього в текстi JSON не видно взагалi, i саме заради них
// панель iснує:
//
//   1. ЕФIР МIНЯЄТЬСЯ ВIД ОДНОГО ЧИСЛА. Дрiбнiший крок в однiй рацiї подрiбнює
//      крок УСЬОГО ефiру, бо НСД падає; ширша смуга розсуває його межi. Панель
//      показує, яким ефiр СТАНЕ, поки ти ще друкуєш.
//   2. Є СТЕЛЯ, i вона не наша: налаштований iндекс їде до гравця
//      синхрозмiнною на OZR_Const.INDEX_MAX. Ефiр, якому треба бiльше дiлень,
//      сервер вiдхилить -- тож панель каже про це ДО збереження, з числом,
//      якого не вистачило.
//
// I третє, вже про час: файл сiтки читає нативний патч при старті ПРОЦЕСУ, тож
// новий ефiр дiє з наступного запуску сервера. Панель каже, що зараз у ефiрi й
// що стане пiсля рестарту -- мовчазна вiдкладена дiя гiрша за вiдсутню.
//
// Дальнiсть НЕ РЕДАГУЄТЬСЯ, i це не лiнощi. `range` живе в CfgVehicles класу,
// його читає сам рушiй, а скрипту доступнi лише ConfigGetFloat i рiдня --
// жодного ConfigSet у грi немає. Змiнити дальнiсть на живому серверi
// неможливо; вона змiнюється в config.cpp i переупаковкою pbo. Тому поле
// показує справжнє число з конфiгу класу i мовчить: поле вводу, яке нiчого не
// робить, гiрше за його вiдсутнiсть.

#ifdef AVPPAdminTools
#ifdef OpenZone_VPP
#ifndef NO_GUI

modded class OZ_VppAdminMenu
{
    private ref OZR_Profiles m_RadCfg;
    private int  m_RadPicked   = -1;
    private bool m_RadDelArmed = false;

    // Перемальовування списку кличе SelectRow, а рушiй вiдповiдає на нього
    // OnItemSelected -- вийшла б рекурсiя. Прапорець рве це коло (так само,
    // як у ядрi).
    private bool m_RadRepaint = false;

    override void OnCreate(Widget RootW)
    {
        super.OnCreate(RootW);

        if (!M_SUB_WIDGET)
            return;

        Widget pane = GetGame().GetWorkspace().CreateWidgets("OpenZone_Radio_VPP/gui/layouts/ozr_vpp_pane.layout", M_SUB_WIDGET);
        if (!pane)
        {
            OZR_Log.Error("radio vpp pane: layout failed to load");
            return;
        }

        RegisterPane("radio", "RADIO", pane);
    }

    override void OnPaneShown(string id)
    {
        super.OnPaneShown(id);

        if (id == "radio")
        {
            AskCfg(OZR_AdminCfg.CFG_PROFILES);
            PaintGrid();
        }
    }

    override void OnCfgText(string name, string body)
    {
        if (name == OZR_AdminCfg.CFG_PROFILES)
        {
            OZR_Profiles c;
            string err;
            if (JsonFileLoader<OZR_Profiles>.LoadData(body, c, err) && c)
            {
                m_RadCfg = c;
                if (!m_RadCfg.Radios)
                    m_RadCfg.Radios = new array<ref OZR_RadioProfile>();
                m_RadPicked = -1;
                RebuildRadList();
                PaintGrid();
            }
            else
            {
                Hint("RadioProfiles.json does not parse: " + err);
            }
            return;
        }

        super.OnCfgText(name, body);
    }

    override void OnCfgApplied()
    {
        super.OnCfgApplied();
        if (CurrentPane() == "radio")
            AskCfg(OZR_AdminCfg.CFG_PROFILES);
    }

    override bool OnItemSelected(Widget w, int x, int y, int row, int column, int oldRow, int oldColumn)
    {
        if (w && w.GetName() == "RadList" && !m_RadRepaint)
        {
            PickRad(row);
            return true;
        }
        return super.OnItemSelected(w, x, y, row, column, oldRow, oldColumn);
    }

    override bool OnClick(Widget w, int x, int y, int button)
    {
        if (w && M_SUB_WIDGET)
        {
            string nm = w.GetName();

            if (nm == "BtnRadNew")
            {
                NewRad();
                return true;
            }
            if (nm == "BtnRadDel")
            {
                DeleteRad();
                return true;
            }
            if (nm == "BtnRadCheck")
            {
                PaintChecks();
                return true;
            }
            if (nm == "BtnRadSave")
            {
                SaveRad();
                return true;
            }
            if (nm == "BtnRadReload")
            {
                m_RadDelArmed = false;
                AskCfg(OZR_AdminCfg.CFG_PROFILES);
                Hint("reloaded from the server");
                return true;
            }
        }

        return super.OnClick(w, x, y, button);
    }

    // ------------------------------------------------------------ сiтка

    private void PaintGrid()
    {
        Say("RadGrid", "ether now: " + Running());
        PaintEther();
    }

    private string Running()
    {
        if (!OZR_ClientGrid.Ready())
            return "the server has not sent it yet";

        int n = OZR_ClientGrid.Count();
        string line = OZR_Fmt.MHz(OZR_ClientGrid.BaseMHz());
        line += " to " + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(n - 1));
        line += " MHz, step " + OZR_Fmt.Step(OZR_ClientGrid.StepMHz());
        line += ", " + n.ToString() + " divisions";
        return line;
    }

    // Ефiр, який вийде з ПОТОЧНОГО стану форми. Рахується тiєю ж функцiєю, що
    // й на серверi, i з тих самих профiлiв -- iнакше попередження тут i
    // вiдмова там могли б розiйтись.
    private OZR_EtherPlan Planned()
    {
        if (!m_RadCfg || !m_RadCfg.Radios)
            return null;

        // Поля вибраного рядка ще не збереженi, але саме вони й цiкавлять:
        // сенс попередження -- побачити наслiдок ДО збереження. Тому рахуємо по
        // копiї, у якiй вибраний рядок узятий з форми.
        array<ref OZR_RadioProfile> what = new array<ref OZR_RadioProfile>();
        for (int i = 0; i < m_RadCfg.Radios.Count(); i++)
        {
            OZR_RadioProfile src = m_RadCfg.Radios[i];
            OZR_RadioProfile cp = new OZR_RadioProfile();
            cp.ClassName = src.ClassName;
            cp.MinMHz    = src.MinMHz;
            cp.MaxMHz    = src.MaxMHz;
            cp.StepMHz   = src.StepMHz;

            if (i == m_RadPicked)
            {
                cp.ClassName = GetEdit("RadF_Class");
                cp.MinMHz    = GetEdit("RadF_Min").ToFloat();
                cp.MaxMHz    = GetEdit("RadF_Max").ToFloat();
                cp.StepMHz   = GetEdit("RadF_Step").ToFloat();
            }
            what.Insert(cp);
        }
        return OZR_Ether.Derive(what);
    }

    private void PaintEther()
    {
        OZR_EtherPlan plan = Planned();
        if (!plan)
        {
            Say("RadEther", "");
            return;
        }

        if (!plan.Ok)
        {
            Say("RadEther", "after restart: WILL BE REFUSED - " + plan.Why);
            return;
        }

        string line = "after restart: " + OZR_Ether.Describe(plan);
        if (SameAsRunning(plan))
            line += "   (unchanged)";
        else
            line += "   <<< RESTART THE SERVER to apply";
        Say("RadEther", line);
    }

    private bool SameAsRunning(OZR_EtherPlan plan)
    {
        if (!OZR_ClientGrid.Ready() || !plan || !plan.Ok)
            return false;
        if (OZR_ClientGrid.Count() != plan.Count)
            return false;

        // Допуск -- сота частина кроку: жива сiтка ВИМIРЯНА, i вимiр iде через
        // float32.
        float tol = plan.StepMHz * 0.01;
        if (Math.AbsFloat(OZR_ClientGrid.BaseMHz() - plan.BaseMHz) > tol)
            return false;
        if (Math.AbsFloat(OZR_ClientGrid.StepMHz() - plan.StepMHz) > tol)
            return false;
        return true;
    }

    // ------------------------------------------------------------ список

    private void RebuildRadList()
    {
        TextListboxWidget lb = TextListboxWidget.Cast(M_SUB_WIDGET.FindAnyWidget("RadList"));
        if (!lb || !m_RadCfg || !m_RadCfg.Radios)
            return;

        m_RadRepaint = true;
        lb.ClearItems();

        for (int i = 0; i < m_RadCfg.Radios.Count(); i++)
        {
            OZR_RadioProfile p = m_RadCfg.Radios[i];
            string row = p.ClassName;
            row += "   " + OZR_Fmt.MHz(p.MinMHz) + "-" + OZR_Fmt.MHz(p.MaxMHz);
            row += " /" + OZR_Fmt.Step(p.StepMHz);
            lb.AddItem(row, NULL, 0);
        }

        if (m_RadPicked >= 0 && m_RadPicked < m_RadCfg.Radios.Count())
            lb.SelectRow(m_RadPicked);

        m_RadRepaint = false;
    }

    private void PickRad(int row)
    {
        if (!m_RadCfg || !m_RadCfg.Radios)
            return;
        if (row < 0 || row >= m_RadCfg.Radios.Count())
            return;

        m_RadPicked   = row;
        m_RadDelArmed = false;

        OZR_RadioProfile p = m_RadCfg.Radios[row];
        SetEdit("RadF_Class", p.ClassName);
        SetEdit("RadF_Min",   OZR_Fmt.MHz(p.MinMHz));
        SetEdit("RadF_Max",   OZR_Fmt.MHz(p.MaxMHz));
        SetEdit("RadF_Step",  OZR_Fmt.Step(p.StepMHz));

        PaintRange(p.ClassName);
        PaintChecks();
    }

    // Справжнє число з конфiгу класу, а не копiя в JSON: копiя розiйшлася б з
    // рушiєм, а розбiжнiсть у ЦЬОМУ числi нiде не видно -- рацiя просто чути
    // не так далеко, як написано у вкладцi.
    private void PaintRange(string cls)
    {
        TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("RadF_Range"));
        if (!t)
            return;

        if (cls == "")
        {
            t.SetText("-");
            return;
        }

        string path = "CfgVehicles " + cls + " range";
        if (!GetGame().ConfigIsExisting(path))
        {
            t.SetText("no such item class, or it declares no range");
            return;
        }

        float r = GetGame().ConfigGetFloat(path);
        t.SetText(Math.Round(r).ToString() + " m");
    }

    // ------------------------------------------------------------ перевiрки
    //
    // Рахується з ПОЛIВ, а не зi збереженого профiлю: сенс кнопки в тому, щоб
    // побачити наслiдок ДО збереження.

    private void PaintChecks()
    {
        PaintRange(GetEdit("RadF_Class"));
        PaintEther();

        float lo   = GetEdit("RadF_Min").ToFloat();
        float hi   = GetEdit("RadF_Max").ToFloat();
        float step = GetEdit("RadF_Step").ToFloat();

        // Порожнє поле -- це не помилка, це «ще не заповнено». Нуль справдi
        // лежить нижче будь-якого ефiру, але нуля нiхто не вводив.
        if (lo <= 0 || hi <= 0 || step <= 0)
        {
            Say("RadF_Chan", "channels: -");
            Say("RadF_Fit1", "nothing to check yet - pick a profile on the left, or fill the three fields");
            Say("RadF_Fit2", "");
            Say("RadF_Fit3", "");
            return;
        }

        if (hi <= lo)
        {
            Say("RadF_Chan", "channels: none");
            Say("RadF_Fit1", "the upper bound must be above the lower one");
            Say("RadF_Fit2", "");
            Say("RadF_Fit3", "");
            return;
        }

        OZR_EtherPlan plan = Planned();

        // Ефiр вiдхилять цiлком -- рахувати канали цього профiлю нема сенсу,
        // бо сiтки, в якiй їх рахувати, не буде.
        if (!plan || !plan.Ok)
        {
            Say("RadF_Chan", "channels: -");
            Say("RadF_Fit1", "the ether these profiles ask for cannot be built - see the line at the top");
            Say("RadF_Fit2", "");
            Say("RadF_Fit3", "");
            return;
        }

        int stride = Math.Round(step / plan.StepMHz);
        if (stride < 1)
            stride = 1;

        int count = Math.Round((hi - lo) / step) + 1;

        string chan = "channels: " + count.ToString();
        chan += "   from " + OZR_Fmt.MHz(lo) + " to " + OZR_Fmt.MHz(hi);
        Say("RadF_Chan", chan);

        string every = "step: every " + stride.ToString();
        if (stride == 1)
            every += " division of the new ether";
        else
            every += " divisions of the new ether";
        Say("RadF_Fit1", every);

        // Друге питання, i єдине, яке ще може мати вiдповiдь «нi»: чи працює
        // цей профiль ВЖЕ, чи тiльки пiсля рестарту. Ефiр пiде за ним у будь-
        // якому разi, але не цiєї ж секунди.
        string now = "in effect now: ";
        if (!OZR_ClientGrid.Ready())
        {
            now += "unknown - no ether from the server yet";
        }
        else
        {
            float gLo = OZR_ClientGrid.MHzAt(0);
            float gHi = OZR_ClientGrid.MHzAt(OZR_ClientGrid.Count() - 1);
            float gTol = OZR_ClientGrid.StepMHz() * 0.5;

            if (lo < gLo - gTol || hi > gHi + gTol)
            {
                now += "NO - outside the running ether, the server clamps it to ";
                float cLo = lo;
                float cHi = hi;
                if (cLo < gLo)
                    cLo = gLo;
                if (cHi > gHi)
                    cHi = gHi;
                now += OZR_Fmt.MHz(cLo) + " to " + OZR_Fmt.MHz(cHi) + " until the restart";
            }
            else
            {
                now += "yes";
            }
        }
        Say("RadF_Fit2", now);

        Say("RadF_Fit3", "");
    }

    private void Say(string widget, string text)
    {
        TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget(widget));
        if (t)
            t.SetText(text);
    }

    // ------------------------------------------------------------ правки

    private void NewRad()
    {
        if (!m_RadCfg)
        {
            Hint("nothing loaded yet");
            return;
        }
        if (!m_RadCfg.Radios)
            m_RadCfg.Radios = new array<ref OZR_RadioProfile>();

        OZR_RadioProfile p = new OZR_RadioProfile();
        p.ClassName = "";

        // Заготовка лягає на сiтку -- нову рацiю можна зберегти одразу пiсля
        // того, як їй дали iм'я, i вона працюватиме.
        if (OZR_ClientGrid.Ready())
        {
            p.MinMHz  = OZR_ClientGrid.MHzAt(0);
            p.MaxMHz  = OZR_ClientGrid.MHzAt(OZR_ClientGrid.Count() - 1);
            p.StepMHz = OZR_ClientGrid.StepMHz();
        }

        m_RadCfg.Radios.Insert(p);
        m_RadPicked = m_RadCfg.Radios.Count() - 1;
        RebuildRadList();
        PickRad(m_RadPicked);
        Hint("new profile added - give it an item class and press SAVE ALL");
    }

    // Друге натискання пiдтверджує. Модальнi вiкна у VPP -- пастка, тому
    // пiдтвердження скрiзь у цiй вкладцi робиться саме так.
    private void DeleteRad()
    {
        if (!m_RadCfg || !m_RadCfg.Radios || m_RadPicked < 0)
        {
            Hint("pick a profile first");
            return;
        }

        if (!m_RadDelArmed)
        {
            m_RadDelArmed = true;
            Hint("press DELETE again to remove " + m_RadCfg.Radios[m_RadPicked].ClassName);
            return;
        }

        string gone = m_RadCfg.Radios[m_RadPicked].ClassName;
        m_RadCfg.Radios.Remove(m_RadPicked);
        m_RadPicked   = -1;
        m_RadDelArmed = false;
        RebuildRadList();
        Hint("removed " + gone + " - press SAVE ALL to write it");
    }

    private void SaveRad()
    {
        if (!m_RadCfg || !m_RadCfg.Radios)
        {
            Hint("nothing loaded yet");
            return;
        }

        // Поля належать ВИБРАНОМУ рядку, i лише йому: решта вже в об'єктi.
        if (m_RadPicked >= 0 && m_RadPicked < m_RadCfg.Radios.Count())
        {
            string cls = GetEdit("RadF_Class");
            if (cls == "")
            {
                Hint("the item class cannot be empty");
                return;
            }

            OZR_RadioProfile p = m_RadCfg.Radios[m_RadPicked];
            p.ClassName = cls;
            p.MinMHz    = GetEdit("RadF_Min").ToFloat();
            p.MaxMHz    = GetEdit("RadF_Max").ToFloat();
            p.StepMHz   = GetEdit("RadF_Step").ToFloat();

            if (p.MaxMHz <= p.MinMHz)
            {
                Hint("the upper bound must be above the lower one");
                return;
            }
            if (p.StepMHz <= 0)
            {
                Hint("the step must be above zero");
                return;
            }
        }

        string body;
        string err;
        if (!JsonFileLoader<OZR_Profiles>.MakeData(m_RadCfg, body, err, false))
        {
            Hint("cannot serialise: " + err);
            return;
        }

        SendCfg(OZR_AdminCfg.CFG_PROFILES, body);
        m_RadDelArmed = false;
        RebuildRadList();
        Hint("sent to the server");
    }
}

#endif
#endif
#endif
