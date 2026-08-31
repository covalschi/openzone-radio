// Канали КПК в адмінській консолі ядра.
//
// Живуть тут, а не в моді рації, з тієї ж причини, що й самі канали: канал --
// це поняття стику з КПК, а не ручної рації. Ядро цей pbo вимагає жорстко,
// тому реєстр консолі тут доступний без жодних умов.

class OZRP_ChannelsApplier : OZ_AdminCfgApplier
{
    override bool Apply(string json)
    {
        OZR_ChannelCfg tmp;
        string err;
        if (!JsonFileLoader<OZR_ChannelCfg>.LoadData(json, tmp, err) || !tmp)
        {
            OZR_Log.Warn("admin: RadioChannels.json rejected: " + err);
            return false;
        }

        OZ_ConfigLoader<OZR_ChannelCfg>.Save(OZRP_Const.CHANNELS, "RadioChannels", tmp);
        OZR_ChannelCfg.ServerLoad();
        return true;
    }
}

class OZRP_AdminCfg
{
    static const string CFG_CHANNELS = "RadioChannels";

    static void Declare()
    {
        OZ_AdminCfg.Register(CFG_CHANNELS, OZRP_Const.CHANNELS, new OZRP_ChannelsApplier(), "radio");
    }
}
