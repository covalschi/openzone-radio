// Панель «RADIO» в адмiнському вiкнi OpenZone: профiлi ручних рацiй формою.
// Чiпляється вкладкою до вiкна ядра через modded class -- субмод субмода, як
// це вже зроблено в КПК: ядро про рацiю не знає й знати не мусить.
//
// Гарди: NO_GUI -- сервер компiлює Mission без UI; AVPPAdminTools i
// OpenZone_VPP -- iмена класiв CfgMods (їх авто-дефайнить рушiй).
//
// ЧОМУ ЦЕ НЕ РЕДАКТОР JSON. Профiль ламається не синтаксисом, а числами, i
// рiвно двома способами, обидва з яких у текстi не видно:
//
//   1. межа лягла МIЖ дiленнями сiтки рушiя -- рацiя стане не туди, куди
//      написано;
//   2. крок не кратний кроку сiтки -- рацiя крокує повз дiлення й не
//      зустрiне нiкого взагалi.
//
// Обидва ловляться лише проти ЖИВОЇ сiтки, яку сервер прислав цьому клiєнту
// (OZR_ClientGrid). Тому панель не просто складає JSON, а рахує, що з цих
// чисел вийде: скiльки каналiв, чи лягли межi, чи кратний крок. Це i є те, за
// чим сюди приходять.
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
        TextWidget t = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget("RadGrid"));
        if (!t)
            return;

        if (!OZR_ClientGrid.Ready())
        {
            t.SetText("grid: the server has not sent it yet - checks are unavailable");
            return;
        }

        int n = OZR_ClientGrid.Count();
        string line = "grid: " + OZR_Fmt.MHz(OZR_ClientGrid.BaseMHz());
        line += " to " + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(n - 1));
        line += " MHz, step " + OZR_Fmt.Step(OZR_ClientGrid.StepMHz());
        line += ", " + n.ToString() + " divisions";
        t.SetText(line);
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
        float lo   = GetEdit("RadF_Min").ToFloat();
        float hi   = GetEdit("RadF_Max").ToFloat();
        float step = GetEdit("RadF_Step").ToFloat();

        PaintRange(GetEdit("RadF_Class"));

        if (!OZR_ClientGrid.Ready())
        {
            Say("RadF_Chan", "channels: unknown - no grid from the server");
            Say("RadF_Fit1", "");
            Say("RadF_Fit2", "");
            Say("RadF_Fit3", "");
            return;
        }

        float gs = OZR_ClientGrid.StepMHz();

        // Пiв кроку сiтки: все, що ближче, -- те саме дiлення, а не сусiднє.
        float eps = gs * 0.5;

        // 1. Межi лягли на дiлення?
        int loIdx = OZR_ClientGrid.IndexOf(lo);
        int hiIdx = OZR_ClientGrid.IndexOf(hi);
        float loOn = Math.AbsFloat(OZR_ClientGrid.MHzAt(loIdx) - lo);
        float hiOn = Math.AbsFloat(OZR_ClientGrid.MHzAt(hiIdx) - hi);

        string fit1 = "bounds: ";
        if (loOn < eps && hiOn < eps)
        {
            fit1 += "on the grid";
        }
        else
        {
            fit1 += "OFF the grid - nearest are ";
            fit1 += OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(loIdx));
            fit1 += " and " + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(hiIdx));
        }
        Say("RadF_Fit1", fit1);

        // 2. Межi всерединi сiтки?
        string fit2 = "span: ";
        if (loIdx < 0 || hiIdx > OZR_ClientGrid.Count() - 1)
            fit2 += "OUTSIDE the grid - the engine has no such divisions";
        else if (hiIdx <= loIdx)
            fit2 += "EMPTY - the upper bound must be above the lower one";
        else
            fit2 += "inside the grid";
        Say("RadF_Fit2", fit2);

        // 3. Крок кратний кроку сiтки?
        int stride = Math.Round(step / gs);
        if (stride < 1)
            stride = 1;
        float want = stride * gs;

        string fit3 = "step: ";
        if (Math.AbsFloat(want - step) < gs * 0.02)
        {
            fit3 += "every " + stride.ToString() + " divisions";
        }
        else
        {
            fit3 += "NOT a multiple of the grid step - the radio would sit between divisions; ";
            fit3 += "nearest usable is " + OZR_Fmt.Step(want);
        }
        Say("RadF_Fit3", fit3);

        // Скiльки каналiв з цього вийде.
        string chan = "channels: ";
        if (hiIdx > loIdx && stride > 0)
        {
            int count = ((hiIdx - loIdx) / stride) + 1;
            chan += count.ToString();
            chan += "   from " + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(loIdx));
            chan += " to " + OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(loIdx + (count - 1) * stride));
        }
        else
        {
            chan += "none";
        }
        Say("RadF_Chan", chan);
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
