using System;
using System.Collections.Generic;
using System.Text;

namespace qbridge_driver
{
    class qbrdge_driver_wince_main
    {
        static void Main()
        {
            qbrdge_driver_classlib.RP1210DllCom.MainStart();
        }
    }
}
