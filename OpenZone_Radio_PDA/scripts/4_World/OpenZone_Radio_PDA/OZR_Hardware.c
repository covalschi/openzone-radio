// Залізо, яке цей мод приносить у КПК.
//
// Оголошується через ДОГОВІР КПК, а не правкою його файлів: OZ_PdaHardware
// приймає чужі модулі одним викликом, і адмін лишається головнішим -- якщо
// він уже описав цей класнейм у Hardware.json, наш опис не застосовується.
//
// Через це сервер, який хоче іншу дальність або іншу витрату, править свій
// JSON, а не чужий мод.

class OZR_Hardware
{
    private static int s_Declared = 0;

    static int Count()
    {
        return s_Declared;
    }

    static void Declare()
    {
        s_Declared = 0;

        // Плата рації. Вмикає сторінку «Рація» -- саме тому вона в
        // EnablesPages, а не в профілі пристрою: без плати сторінці нема що
        // показувати, і малювати вкладку, за якою нікого немає, гірше, ніж не
        // малювати її зовсім.
        OZ_ModuleSpec radio = new OZ_ModuleSpec();
        radio.ClassName    = "OZ_Module_Radio";
        radio.DisplayName  = "#STR_OZR_MOD_RADIO";
        radio.Kind         = OZRP_Const.MOD_RADIO;
        // Приймач шумить постійно, передавач -- лише коли говорять. Півтора
        // -- це «чутно по батареї, але не смертельно».
        radio.PowerFactor  = 1.5;
        radio.EnablesPages = new array<string>();
        radio.EnablesPages.Insert(OZRP_Const.PAGE_RADIO);
        if (OZ_PdaHardware.Declare(radio))
            s_Declared++;
    }
}
