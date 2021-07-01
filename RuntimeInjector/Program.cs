using System.Diagnostics;
using Noita.NET;

namespace RuntimeInjector
{
    class Program
    {
        static void Main(string[] args)
        {
            using var p = Process.GetProcessesByName("noita_dev")[0];
            var path = typeof(Main).Assembly.Location;
            var ptr = p.LoadLibrary(path);
            var lib = LoadLibrary(path);
            var addr = GetProcAddress(lib, "Initialize");
            p.Call(addr, IntPtr.Zero);
        }
    }
}
