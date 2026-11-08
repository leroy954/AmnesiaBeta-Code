# Lists the visible top level windows of the Unreal editor processes
Add-Type @'
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;
public static class Wnd {
    public delegate bool EnumProc(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc cb, IntPtr l);
    [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint pid);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    public static List<string> List(uint[] pids) {
        var r = new List<string>();
        EnumWindows((h, l) => {
            uint pid; GetWindowThreadProcessId(h, out pid);
            if (Array.IndexOf(pids, pid) >= 0 && IsWindowVisible(h)) {
                var sb = new StringBuilder(256); GetWindowText(h, sb, 256);
                r.Add(pid + " " + h + " '" + sb + "'");
            }
            return true;
        }, IntPtr.Zero);
        return r;
    }
}
'@
$ids = [uint32[]]@(Get-Process UnrealEditor -ErrorAction SilentlyContinue | ForEach-Object { $_.Id })
[Wnd]::List($ids)
