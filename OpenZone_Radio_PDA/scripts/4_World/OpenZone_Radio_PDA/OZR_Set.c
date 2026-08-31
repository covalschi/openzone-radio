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
            if (!spec || spec.Kind != OZRP_Const.MOD_RADIO)
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
    // Дальність плати -- з ЇЇ ВЛАСНОГО конфіга, як у будь-якої рації.
    //
    // Окремого модуля антени більше немає, і зникнення це не втрата, а
    // виправлення поняття: плата -- це рація, а антена в рації не окрема річ,
    // яку носять у сусідньому відсіку. Хто хоче дістати далі -- ставить іншу
    // плату, рівно як інший гравець бере іншу ручну рацію.
    static float RangeOf(OZ_Module_Radio board)
    {
        if (!board)
            return 0;

        string path = "CfgVehicles " + board.GetType() + " range";
        if (!GetGame().ConfigIsExisting(path))
            return 0;

        return GetGame().ConfigGetFloat(path);
    }

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

            // Живлення плати -- живлення КПК, і більше нічого: своєї батареї
            // в неї немає, а дальність тепер її власна й від відсіків не
            // залежить.
            bool live = pda.OZ_IsOn();
            board.OZR_Wake(live);
        }
    }
}
