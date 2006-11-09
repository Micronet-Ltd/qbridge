using System;
using System.Collections.Generic;
using System.Text;

namespace qbrdge_driver_classlib
{
    class J1939Transaction
    {
        public J1939Transaction()
        {
        }

        private bool useRTSCTS = false;
        private bool useBAM = false;

        public void UpdateJ1939Data(string data) {
            useRTSCTS = false;
            useBAM = false;
            byte how_priority = (byte)data[3];
            if ((how_priority & 0x80) != 0)
            {
                useBAM = true; // BAM
            }
            else
            {
                useRTSCTS = true; // RTS/CTS
            }
            byte da = (byte)data[5];
            if (da == 0xFF) // global address 
            {
                useBAM = true;
                useRTSCTS = false;
            }
            int msgDataLen = data.Length - 6;
            if (msgDataLen <= 8)
            {
                useRTSCTS = false;
                useBAM = false;
            }
            //UNFINISHED
                    
        }

        public byte[] GetCANPacket() {
            byte[] b = new byte[0];
            return b;
        }
    }
}
