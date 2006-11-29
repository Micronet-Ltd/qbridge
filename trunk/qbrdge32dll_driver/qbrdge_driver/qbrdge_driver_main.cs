using System;
using System.Collections.Generic;
using System.Text;

namespace qbridge_driver
{
    class qbrdge_driver_main
    {
        static System.Windows.Forms.NotifyIcon ni;
        static void Main()
        {

            ni = new System.Windows.Forms.NotifyIcon();
            ni.Text = "RP1210A active";
            ni.Icon = System.Drawing.Icon.ExtractAssociatedIcon("c:\\io.sys");
            ni.Visible = true;
            qbrdge_driver_classlib.RP1210DllCom.MainStart();
        }
    }
}
