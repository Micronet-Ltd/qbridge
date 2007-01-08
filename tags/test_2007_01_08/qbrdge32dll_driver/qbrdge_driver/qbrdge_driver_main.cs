using System;
using System.Collections.Generic;
using System.Text;

namespace qbridge_driver
{
    class qbrdge_driver_main
    {
        class IconMgr : qbrdge_driver_classlib.IconMgrBase
        {
            System.Windows.Forms.NotifyIcon ni;
            public IconMgr()
            {
                ni = new System.Windows.Forms.NotifyIcon();
                ni.Text = "RP1210A active";
                ni.Icon = System.Drawing.Icon.ExtractAssociatedIcon("q:\\dev\\software\\g70\\_qsi_opto_current_revision\\QlarityFoundry.exe");
                //ni.Icon = new System.Drawing.Icon(typeof(qbridge_driver.Properties.Resources), "Icon1");
                ni.Visible = true;
            }
            public override void HideIcon()
            {
                ni.Visible = false;
            }

            ~IconMgr()
            {
                //ni.Visible = false;
            }
        }

        static IconMgr mgr;
        static void Main()
        {
            mgr = new IconMgr();
            qbrdge_driver_classlib.RP1210DllCom.MainStart(mgr);
        }
    }
}
