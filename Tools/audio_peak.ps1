# Prints the output peak (0..1) of a process's Windows audio session, one line per window: "<unix ms> <max peak>".
# Reads the same meter as the Volume Mixer, so it shows what the game really sends to the speakers.
# Usage: powershell -NoProfile -File audio_peak.ps1 -Seconds 20 -Window 0.5 -Process UnrealEditor
param([double]$Seconds = 10, [double]$Window = 0.5, [string]$Process = "UnrealEditor")

$code = @"
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;

namespace AudioMeter
{
	[ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")]
	class MMDeviceEnumeratorCom { }

	[ComImport, Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	interface IMMDeviceEnumerator
	{
		int EnumAudioEndpoints(int dataFlow, int stateMask, out IntPtr devices);
		int GetDefaultAudioEndpoint(int dataFlow, int role, out IMMDevice device);
	}

	[ComImport, Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	interface IMMDevice
	{
		int Activate(ref Guid iid, int clsCtx, IntPtr activationParams, [MarshalAs(UnmanagedType.IUnknown)] out object iface);
	}

	[ComImport, Guid("77AA99A0-1BD6-484F-8BC7-2C654C9A9B6F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	interface IAudioSessionManager2
	{
		int GetAudioSessionControl(IntPtr groupingParam, int streamFlags, out IntPtr sessionControl);
		int GetSimpleAudioVolume(IntPtr groupingParam, int streamFlags, out IntPtr audioVolume);
		int GetSessionEnumerator(out IAudioSessionEnumerator sessionEnum);
	}

	[ComImport, Guid("E2F5BB11-0570-40CA-ACDD-3AA01277DEE8"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	interface IAudioSessionEnumerator
	{
		int GetCount(out int count);
		int GetSession(int index, [MarshalAs(UnmanagedType.IUnknown)] out object session);
	}

	[ComImport, Guid("BFB7FF88-7239-4FC9-8FA2-07C950BE9C6D"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	interface IAudioSessionControl2
	{
		int GetState(out int state);
		int GetDisplayName([MarshalAs(UnmanagedType.LPWStr)] out string name);
		int SetDisplayName([MarshalAs(UnmanagedType.LPWStr)] string value, ref Guid context);
		int GetIconPath([MarshalAs(UnmanagedType.LPWStr)] out string path);
		int SetIconPath([MarshalAs(UnmanagedType.LPWStr)] string value, ref Guid context);
		int GetGroupingParam(out Guid param);
		int SetGroupingParam(ref Guid param, ref Guid context);
		int RegisterAudioSessionNotification(IntPtr client);
		int UnregisterAudioSessionNotification(IntPtr client);
		int GetSessionIdentifier([MarshalAs(UnmanagedType.LPWStr)] out string id);
		int GetSessionInstanceIdentifier([MarshalAs(UnmanagedType.LPWStr)] out string id);
		int GetProcessId(out uint pid);
		int IsSystemSoundsSession();
		int SetDuckingPreference([MarshalAs(UnmanagedType.Bool)] bool optOut);
	}

	[ComImport, Guid("C02216F6-8C67-4B5B-9D00-D008E73E0064"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
	interface IAudioMeterInformation
	{
		int GetPeakValue(out float peak);
	}

	public static class Meter
	{
		static float Peak(HashSet<uint> pids)
		{
			float best = 0.0f;
			IMMDeviceEnumerator enumerator = (IMMDeviceEnumerator)new MMDeviceEnumeratorCom();
			IMMDevice device;
			enumerator.GetDefaultAudioEndpoint(0, 1, out device);
			Guid iid = typeof(IAudioSessionManager2).GUID;
			object managerObject;
			device.Activate(ref iid, 23, IntPtr.Zero, out managerObject);
			IAudioSessionManager2 manager = (IAudioSessionManager2)managerObject;
			IAudioSessionEnumerator sessions;
			manager.GetSessionEnumerator(out sessions);
			int count;
			sessions.GetCount(out count);

			for (int i = 0; i < count; i++)
			{
				object sessionObject;
				sessions.GetSession(i, out sessionObject);
				uint pid;
				((IAudioSessionControl2)sessionObject).GetProcessId(out pid);

				if (pids.Contains(pid))
				{
					float peak;
					((IAudioMeterInformation)sessionObject).GetPeakValue(out peak);
					best = Math.Max(best, peak);
				}

				Marshal.ReleaseComObject(sessionObject);
			}

			Marshal.ReleaseComObject(sessions);
			Marshal.ReleaseComObject(manager);
			Marshal.ReleaseComObject(device);
			Marshal.ReleaseComObject(enumerator);
			return best;
		}

		public static void Run(string processName, double seconds, double window)
		{
			DateTime end = DateTime.UtcNow.AddSeconds(seconds);

			while (DateTime.UtcNow < end)
			{
				HashSet<uint> pids = new HashSet<uint>();

				foreach (Process p in Process.GetProcessesByName(processName))
				{
					pids.Add((uint)p.Id);
				}

				DateTime windowEnd = DateTime.UtcNow.AddSeconds(window);
				float max = 0.0f;

				while (DateTime.UtcNow < windowEnd)
				{
					max = Math.Max(max, Peak(pids));
					Thread.Sleep(20);
				}

				long ms = (long)(DateTime.UtcNow - new DateTime(1970, 1, 1)).TotalMilliseconds;
				Console.WriteLine(ms + " " + max.ToString("F4", System.Globalization.CultureInfo.InvariantCulture));
				Console.Out.Flush();
			}
		}
	}
}
"@

Add-Type -TypeDefinition $code -Language CSharp
[AudioMeter.Meter]::Run($Process, $Seconds, $Window)
