namespace Noita.NET
{
    internal static class NativeMethods
    {
        #region kernel32
        [DllImport("kernel32.dll")]
        static extern IntPtr LoadLibrary(string dllToLoad);

        [DllImport("kernel32.dll")]
        static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

        [DllImport("kernel32.dll")]
        static extern bool FreeLibrary(IntPtr hModule);
        #endregion
    }
}
