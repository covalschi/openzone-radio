// The lag debug mode's two active parts. Both are off unless
// OZ_Radio_Settings.json says otherwise; see OZR_Meter for the passive part.
//
// 1. SelfCheck -- once at start, when `Profiler` is on: the per-call cost of
//    the paths this mod has been slow on before, against the band table THIS
//    server measured. It is a regression guard: `Ready() ~0 us` in the log is
//    the proof that the 2026-09-06 fix is in the running build, and a number
//    in the thousands is the proof that it is not.
//
// 2. SpawnRadios -- when `ProfilerRadios` is above zero: that many vanilla
//    PersonalRadio, powered and broadcasting, each on its own frequency, next
//    to the first player online. TEST STANDS ONLY. It exists because the
//    engine's transmitter pass costs (broadcasting radios x players), and a
//    stand has six people where the live server has sixty: sixty radios next
//    to six people load that pass the way ten radios among sixty do. Vanilla
//    radios on purpose -- a powered PersonalRadio broadcasts by itself, this
//    mod never touches it, and its frequency comes from the engine's table,
//    patched or not. So with the native library the sixty land on sixty
//    frequencies and without it on eight, which is exactly the comparison
//    the lag investigation needs.
//
// The spawned radios are deleted when the mission ends. A server that dies
// instead of stopping leaves them on the ground, which is one more reason
// this is for stands.
class OZR_LoadTest
{
    private static const int REPS  = 50;
    private static const int ITEMS = 20;
    private static const int TUNES = 2000000;
    private static const string CONTROL = "Rag";

    private static ref array<EntityAI> s_Spawned;

    static void SelfCheck()
    {
        int bands = OZR_Bands.Count();

        int r;
        bool sink;

        int t0 = GetGame().GetTime();
        for (r = 0; r < REPS; r++)
            sink = OZR_Grid.Ready();
        int ready = GetGame().GetTime() - t0;

        // A radio this server has a profile for, so EEInit takes the same
        // path it takes for the players' own radios. Fall back to the vanilla
        // probe class when the admin has described none.
        string radio = OZR_Const.BAND_PROBE_CLASS;
        OZR_Profiles cfg = OZR_Profiles.Get();
        if (cfg && cfg.Radios && cfg.Radios.Count() > 0 && cfg.Radios[0])
            radio = cfg.Radios[0].ClassName;

        int radios  = Create(radio);
        int control = Create(CONTROL);

        string said = "self-check: bands=" + bands.ToString();
        said += " | Ready() ~" + Per(ready, REPS).ToString() + " us";
        said += " | create " + ITEMS.ToString() + "x " + radio + " = " + radios.ToString() + " ms";
        said += ", " + ITEMS.ToString() + "x " + CONTROL + " = " + control.ToString() + " ms";
        OZR_Log.Info(said);

        Leaf();
    }

    // The engine call whose leaf the native library replaces, two million
    // times, so the answer is not lost in a millisecond clock.
    private static void Leaf()
    {
        Object obj = GetGame().CreateObjectEx(OZR_Const.BAND_PROBE_CLASS, "0 0 0", ECE_NOLIFETIME | ECE_LOCAL);
        ItemTransmitter probe = ItemTransmitter.Cast(obj);
        if (!probe)
        {
            if (obj)
                GetGame().ObjectDelete(obj);
            return;
        }

        int i;
        float sink;

        int t = GetGame().GetTime();
        for (i = 0; i < TUNES; i++)
            probe.SetFrequencyByIndex(i);
        int msSet = GetGame().GetTime() - t;

        int t2 = GetGame().GetTime();
        for (i = 0; i < TUNES; i++)
            sink = probe.GetTunedFrequency();
        int msGet = GetGame().GetTime() - t2;

        GetGame().ObjectDelete(obj);

        string said = "self-check leaf: " + TUNES.ToString() + " calls";
        said += " | SetFrequencyByIndex ~" + Nanos(msSet, TUNES).ToString() + " ns";
        said += " | GetTunedFrequency ~" + Nanos(msGet, TUNES).ToString() + " ns";
        OZR_Log.Info(said);
    }

    private static int Create(string cls)
    {
        array<Object> made = new array<Object>();

        int t = GetGame().GetTime();
        int i;
        Object o;
        for (i = 0; i < ITEMS; i++)
        {
            o = GetGame().CreateObjectEx(cls, "0 0 0", ECE_NOLIFETIME | ECE_LOCAL);
            if (o)
                made.Insert(o);
        }
        int took = GetGame().GetTime() - t;

        for (i = 0; i < made.Count(); i++)
            GetGame().ObjectDelete(made[i]);

        return took;
    }

    private static int Per(int ms, int count)
    {
        if (count <= 0 || ms < 0)
            return -1;
        return (ms * 1000) / count;
    }

    private static int Nanos(int ms, int count)
    {
        if (count <= 0 || ms < 0)
            return -1;
        return (ms * 1000000) / count;
    }

    // ------------------------------------------------------------ amplifier

    // Returns false while there is nobody to spawn next to, so the caller can
    // keep asking. Spawns once.
    static bool SpawnRadios(int n)
    {
        if (s_Spawned)
            return true;

        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);
        if (players.Count() == 0 || !players[0])
            return false;

        vector at = players[0].GetPosition();
        s_Spawned = new array<EntityAI>();

        int grid = OZR_Grid.Count();
        int lit = 0;
        int powered = 0;

        for (int i = 0; i < n; i++)
        {
            // A loose grid on the ground, eight to a row, so they neither
            // stack nor scatter out of the players' voice range. Column and
            // row are computed as ints first: `%` exists only for ints, and
            // inside Vector()'s float arguments it does not compile.
            int col = i % 8;
            int row = i / 8;
            vector pos = at + Vector(col, 0, row);
            Object o = GetGame().CreateObjectEx(OZR_Const.BAND_PROBE_CLASS, pos, ECE_PLACE_ON_SURFACE | ECE_NOLIFETIME);
            ItemTransmitter radio = ItemTransmitter.Cast(o);
            if (!radio)
            {
                if (o)
                    GetGame().ObjectDelete(o);
                continue;
            }
            s_Spawned.Insert(radio);

            // Its own division when the table is a grid; wrapped onto the
            // vanilla eight when it is not. Both are the point.
            int idx = i;
            if (grid > 0)
                idx = i % grid;
            radio.SetFrequencyByIndex(idx);

            // A battery, put in by hand: the vanilla class adds one only in
            // OnDebugSpawn (the Workbench path), and a radio spawned by script
            // arrives empty -- which players see as sixty dead radios. Measured
            // 2026-09-12 on the stand.
            radio.GetInventory().CreateAttachment("Battery9V");

            ComponentEnergyManager em = radio.GetCompEM();
            if (em)
                em.SwitchOn();
            if (em && em.IsWorking())
                powered++;

            // Said explicitly rather than trusted to OnWorkStart: a class the
            // admin listed in the profiles would have had its air shut by this
            // mod, and a silent radio loads nothing. The engine's router reads
            // this flag, not the power -- both are reported so a dead radio
            // that still counts for the engine is visible as such.
            radio.EnableBroadcast(true);
            if (radio.IsBroadcasting())
                lit++;
        }

        string said = "load: " + s_Spawned.Count().ToString() + " " + OZR_Const.BAND_PROBE_CLASS;
        said += " spawned at " + at.ToString() + ", " + powered.ToString() + " powered, " + lit.ToString() + " broadcasting";
        int distinct = n;
        if (grid > 0 && grid < distinct)
            distinct = grid;
        if (grid > 0)
            said += ", on " + distinct.ToString() + " distinct frequencies";
        else
            said += ", table not a grid - wrapped onto the vanilla eight";
        OZR_Log.Info(said);
        return true;
    }

    static void Clear()
    {
        if (!s_Spawned)
            return;

        for (int i = 0; i < s_Spawned.Count(); i++)
        {
            if (s_Spawned[i])
                GetGame().ObjectDelete(s_Spawned[i]);
        }
        OZR_Log.Info("load: " + s_Spawned.Count().ToString() + " spawned radio(s) removed");
        s_Spawned = null;
    }
}
