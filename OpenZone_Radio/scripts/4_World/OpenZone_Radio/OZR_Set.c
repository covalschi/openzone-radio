// Хто де налаштований -- і чому тут немає таблиці.
//
// Поточний канал НІДЕ не зберігається окремо. Його носить сам пристрій:
// TransmitterBase пише індекс частоти у своє збереження, і після перезапуску
// плата прокидається там, де стояла. Ім'я каналу знаходимо назад по смузі.
// Друга таблиця «хто на чому» неминуче розійшлася б із першою -- а розійшовшись,
// показувала б гравцеві не той ефір, у якому він насправді сидить.

class OZR_Set
{
    // Плата рації в цьому КПК, або порожньо.
    //
    // Шукаємо за ВИДОМ, а не за класом, і лише потім зводимо до передавача.
    // Причина -- у тому, що ці дві речі можуть розійтись: договір каже, що в
    // відсіку рація, а сутність виявляється звичайним предметом, який рушій
    // передавачем не вважає. Мовчазний null тоді виглядав би як «плати
    // немає», тобто збрехав би про причину.
    static OZ_Module_Radio BoardIn(OZ_PDA_Base pda)
    {
        if (!pda)
            return null;

        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            EntityAI att = pda.OZ_Attached(OZ_PdaConst.ModuleSlot(i));
            if (!att)
                continue;

            OZ_ModuleSpec spec = OZ_PdaHardware.ModuleFor(att.GetType());
            if (!spec || spec.Kind != OZR_Const.MOD_RADIO)
                continue;

            OZ_Module_Radio board = OZ_Module_Radio.Cast(att);
            if (board)
                return board;

            string wrong = att.GetType() + " sits in a bay as a radio board, but the engine did not make it a transmitter";
            wrong += " (it is " + att.ClassName() + ")";
            OZR_Log.Warn(wrong);
            return null;
        }
        return null;
    }

    // Найдальша антена з усіх вставлених. Те саме правило, що й у карти:
    // перемагає та, що оголосила більший RangeM.
    static float AntennaRange(OZ_PDA_Base pda)
    {
        if (!pda)
            return 0;

        float best = 0;

        for (int i = 0; i < OZ_PdaConst.MODULE_SLOTS_MAX; i++)
        {
            string cls = pda.OZ_ModuleClass(i);
            if (cls == "")
                continue;

            OZ_ModuleSpec spec = OZ_PdaHardware.ModuleFor(cls);
            if (!spec || spec.Kind != OZ_PdaConst.MOD_ANTENNA)
                continue;

            if (spec.RangeM > best)
                best = spec.RangeM;
        }
        return best;
    }

    // Чи пустить фракція. Канал без фракції -- для всіх.
    static bool Allowed(string uid, OZR_Channel c)
    {
        if (!c)
            return false;
        if (c.Faction == "")
            return true;

        OZ_PlayerData d = OZ_PlayerStore.Load(uid);
        return d && d.Faction == c.Faction;
    }

    // Живлення й прийом тримаємо в тон КПК, і робимо це ПОЗА сторінкою.
    //
    // Інакше рація починала б чути лише після того, як хтось відкрив вкладку
    // -- а рація, яка мовчить, поки на неї не подивишся, це не рація.
    static void Sync()
    {
        array<Man> players = new array<Man>();
        GetGame().GetPlayers(players);

        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase p = PlayerBase.Cast(players[i]);
            if (!p || !p.GetIdentity())
                continue;

            OZ_PDA_Base pda = OZ_PdaLookup.HeldBy(p.GetIdentity());
            if (!pda)
                continue;

            OZ_Module_Radio board = BoardIn(pda);
            if (!board)
                continue;

            // Без антени плата не чує нікого: те саме правило, що й у
            // транспондера, і воно ж робить довгу антену вартою відсіку.
            bool live = pda.OZ_IsOn() && AntennaRange(pda) > 0;
            board.OZR_Wake(live);
        }
    }
}
