using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace qbridge_driver
{
	class qbrdge_driver_wince_main
	{
		[DllImport("coredll.dll", SetLastError = true)]
		public static extern Int32 WaitForSingleObject(IntPtr Handle, Int32 Wait);
		[DllImport("coredll.dll", SetLastError = true)]
		public static extern IntPtr CreateMutex(IntPtr lpMutexAttributes, bool bInitiaOwner, string lpName);
		[DllImport("coredll.dll", SetLastError = true)]
		public static extern bool ReleaseMutex(IntPtr hMutex);
		[DllImport("coredll.dll", SetLastError = true)]
		public static extern bool CloseHandle(IntPtr hObject);

		public const int WAIT_TIMEOUT = 102;
		public const int WAIT_OBJECT_0 = 0;
		const int ERROR_ALREADY_EXISTS = 183;

		static void Main()
		{
			var hMutex = CreateMutex(IntPtr.Zero, false, "QBridgeWinCEDriver");
			if (hMutex == null) return;

			if (WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0)
			{
				//We have the mutex
				//if (Marshal.GetLastWin32Error() != ERROR_ALREADY_EXISTS)
				//{
					qbrdge_driver_classlib.RP1210DllCom.MainStart(new qbrdge_driver_classlib.IconMgrBase());
				//}
			}

            // Note:  The MainStart function actually does exit, so we don't have a clean place for us to clean up the mutex.  Instead, 
            // rely on the OS to do the cleanup.

            //ReleaseMutex(hMutex);
			//CloseHandle(hMutex);
		}
	}
}
