// Логер мода рації.
//
// Свій префікс, а не ядерний: у лозі сервера з тридцятьма модами треба одразу
// бачити, ЧИЙ це рядок, і вердикт MCP шукає саме префікс.
//
// WARNING пишеться повним словом навмисно: вердикт шукає \bWARNING\b, і
// скорочене WARN під це не підпадає -- бюджет попереджень тоді перестав би
// стежити за нашими ж перевірками.

class OZR_Log
{
    static void Info(string msg)
    {
        Print(OZR_Const.LOG_PREFIX + msg);
    }

    static void Warn(string msg)
    {
        Print(OZR_Const.LOG_PREFIX + "WARNING: " + msg);
    }

    static void Error(string msg)
    {
        Print(OZR_Const.LOG_PREFIX + "ERROR: " + msg);
    }

    // Тихий рівень: вмикається тим самим Settings.Debug ядра, бо два окремі
    // вимикачі для однієї збірки -- це два місця, які розійдуться.
    static void Dbg(string msg)
    {
        if (!OZ_Log.IsDebug())
            return;
        Print(OZR_Const.LOG_PREFIX + "dbg: " + msg);
    }
}
