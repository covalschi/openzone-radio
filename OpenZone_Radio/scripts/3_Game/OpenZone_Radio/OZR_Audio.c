// Гучність того, що рація видає -- сплеску й голосу.
//
// ДВІ РІЗНІ РЕЧІ, і плутати їх не можна. Сплеск -- це наш власний звук на
// предметі, і його гучність ми множимо самі. Голос у рації -- це окремий
// канал мікшера рушія, у якого є свій регулятор, і ним ваніль користується
// рівно двічі: глушить на смерті й повертає на респавні
// (dayzplayerimplement.c:865, missiongameplay.c:1630).
//
// ЧОМУ ЦЕ ВЗАГАЛІ НАШ КЛОПІТ. Регулятора радіо в налаштуваннях гри НЕМАЄ:
// у вкладці звуку п'ять повзунків -- Master, Effects, Music, VoIP output,
// VoIP threshold, -- і жодного для радіо. Тип AT_OPTIONS_RADIO у переліку
// оголошений, але не використаний ніде. Тобто гравець не може зробити рацію
// гучнішою, навіть якщо дуже хоче; єдиний, хто може, -- мод.
//
// Числа приходять із сервера, бо це рішення про гру, а не про вуха: на
// рольовому сервері те, наскільки добре чути ефір, -- частина балансу, і
// вирішувати його мусить той, хто той баланс тримає.

class OZR_Audio
{
    // 1.0 -- «нічого не чіпати». Саме тому одиниця, а не нуль: поле, яке
    // забули заповнити, мусить лишати гру такою, якою її зробила ваніль.
    private static float s_SquelchGain = 1.0;
    private static float s_VoiceGain   = 1.0;

    // Гучність радіоканалу такою, якою її дав рушій, до нашого втручання.
    //
    // Знімається ОДИН раз і потрібна тому, що підсилення -- множник. Без
    // збереженої основи кожне повторне застосування множило б уже помножене,
    // і рація ставала б голоснішою з кожним респавном.
    private static float s_BaseVoice = -1;

    static float SquelchGain()
    {
        return s_SquelchGain;
    }

    static void SetGains(float squelch, float voice)
    {
        if (squelch > 0)
            s_SquelchGain = squelch;
        if (voice > 0)
            s_VoiceGain = voice;

        ApplyVoice();
    }

    // Покласти підсилення голосу на мікшер.
    //
    // Пишемо і в сам мікшер, і в g_Game.m_volume_radio. Друге не надмірність:
    // ваніль на респавні відновлює гучність САМЕ з цього поля, тож без запису
    // туди підсилення жило б до першої смерті й тихо зникало -- а «мод працює,
    // поки не помреш» це найгірший різновид несправності, бо його ніхто не
    // пов'яже з причиною.
    static void ApplyVoice()
    {
        if (!GetGame() || GetGame().IsDedicatedServer())
            return;

        // AbstractSoundScene, а не SoundScene: другого типу в грі немає
        // взагалі (game.c:734 повертає саме цей).
        AbstractSoundScene scene = GetGame().GetSoundScene();
        if (!scene)
            return;

        if (s_BaseVoice < 0)
            s_BaseVoice = scene.GetRadioVolume();

        float want = s_BaseVoice * s_VoiceGain;

        scene.SetRadioVolume(want, 1);
        g_Game.m_volume_radio = want;

        string said = "radio voice: base " + s_BaseVoice.ToString();
        said += " x " + s_VoiceGain.ToString();
        said += " = " + want.ToString();
        OZR_Log.Dbg(said);
    }
}
