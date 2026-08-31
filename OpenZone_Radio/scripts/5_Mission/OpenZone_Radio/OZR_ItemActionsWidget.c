// Підпис частоти в HUD.
//
// Ванільний віджет питає GetTunedFrequency() -- рушій, і на КЛІЄНТІ той
// повертає table[index & 7] зі СВОЄЇ таблиці, яка лишилась ванільною: патч
// серверний. Тому в руках у гравця рація, налаштована, скажімо, на 145.125, а
// в HUD світиться 87.8 -- одна з восьми старих частот, якої в нашому ефірі
// немає взагалі.
//
// Індекс при цьому синхронізується чесно, і сітку клієнт отримав від сервера.
// Значить підпис треба рахувати, а не питати.
//
// Рація без профілю лишається на ванільному підписі: чужі й ванільні рації
// працюють у ванільній сітці, і рахувати їх нашою формулою було б брехнею в
// інший бік.

modded class ItemActionsWidget
{
    override protected string GetRadioFrequency()
    {
        TransmitterBase trans;
        if (!Class.CastTo(trans, m_EntityInHands))
            return super.GetRadioFrequency();

        if (!OZR_ClientGrid.Ready())
            return super.GetRadioFrequency();

        if (!OZR_ClientGrid.For(trans.GetType()))
            return super.GetRadioFrequency();

        return OZR_Fmt.MHz(OZR_ClientGrid.MHzAt(trans.GetTunedFrequencyIndex()));
    }
}
