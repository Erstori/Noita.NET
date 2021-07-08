using System.Diagnostics;

namespace Noita.NET
{
    public static class Main
    {
        [DllImport("user32.dll", CharSet = CharSet.Unicode)]
        static extern int MessageBox(IntPtr hWnd, string text, string caption, uint type);

        [UnmanagedCallersOnly]
        public static void Initialize()
        {
            
            Debugger.Launch();
            MessageBox(IntPtr.Zero, "test", "test", 0);
        }
    }
}
