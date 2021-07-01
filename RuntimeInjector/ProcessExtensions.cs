using System.Diagnostics;
using System.Text;

namespace RuntimeInjector
{
    internal static class ProcessExtensions
    {
        private static IntPtr kernel32;
        private static IntPtr loadlibrary;

        static ProcessExtensions()
        {
            kernel32 = NativeMethods.LoadLibrary("kernel32");
            loadlibrary = GetProcAddress(kernel32, "LoadLibraryA");
        }

        public static IntPtr Alloc(this Process proc, int size)
        {
            return VirtualAllocEx(proc.Handle, IntPtr.Zero, (IntPtr)size,
                AllocationType.Commit | AllocationType.Reserve,
                MemoryProtection.ExecuteReadWrite);
        }

        public static bool Free(this Process proc, IntPtr ptr)
        {
            return VirtualFreeEx(proc.Handle, ptr, 0,
                FreeType.Decommit | FreeType.Release);
        }

        public static bool Write(this Process proc, IntPtr ptr, byte[] data)
        {
            IntPtr nBytes;
            return WriteProcessMemory(proc.Handle, ptr, data,
                data.Length, out nBytes);
        }

        public static byte[] Read(this Process proc, IntPtr ptr, int size)
        {
            var data = new byte[size];
            IntPtr nBytes;
            ReadProcessMemory(proc.Handle, ptr, data, size, out nBytes);
            return data;
        }

        public static IntPtr CallAsync(this Process proc, IntPtr ptr, IntPtr arg)
        {
            IntPtr hThreadId;
            var hThread = CreateRemoteThread(proc.Handle, IntPtr.Zero, 0,
                ptr, arg, 0, out hThreadId);
            return hThread;
        }

        public static int Call(this Process proc, IntPtr ptr, IntPtr arg)
        {
            IntPtr hThreadId;
            var hThread = CreateRemoteThread(proc.Handle, IntPtr.Zero, 0,
                ptr, arg, 0, out hThreadId);
            WaitForSingleObject(hThread, unchecked((uint)-1));
            uint exitCode;
            GetExitCodeThread(hThread, out exitCode);
            return (int)exitCode;
        }

        public static IntPtr LoadLibrary(this Process proc, string lib)
        {
            var libData = Encoding.UTF8.GetBytes(lib);
            var ptr = proc.Alloc(libData.Length);
            proc.Write(ptr, libData);
            var addr = proc.Call(loadlibrary, ptr);
            return (IntPtr)addr;
        }
    }

    [Flags]
    public enum FreeType
    {
        Decommit = 0x4000,
        Release = 0x8000,
    }

    [Flags]
    public enum AllocationType
    {
        Commit = 0x1000,
        Reserve = 0x2000,
        Decommit = 0x4000,
        Release = 0x8000,
        Reset = 0x80000,
        Physical = 0x400000,
        TopDown = 0x100000,
        WriteWatch = 0x200000,
        LargePages = 0x20000000
    }

    [Flags]
    public enum MemoryProtection
    {
        Execute = 0x10,
        ExecuteRead = 0x20,
        ExecuteReadWrite = 0x40,
        ExecuteWriteCopy = 0x80,
        NoAccess = 0x01,
        ReadOnly = 0x02,
        ReadWrite = 0x04,
        WriteCopy = 0x08,
        GuardModifierflag = 0x100,
        NoCacheModifierflag = 0x200,
        WriteCombineModifierflag = 0x400
    }
}
