using System;
using System.Runtime.InteropServices;

namespace Noita.NET
{
    public static class Main
    {
        [UnmanagedCallersOnly]
        public static void Initialize()
        {

            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine("hello world");
        }
    }
}
