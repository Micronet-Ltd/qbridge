using System;
using System.Collections.Generic;
using System.Text;

namespace qbrdge_driver_classlib
{
    class J1939Transaction
    {
        public J1939Transaction()
        {
            PendingCAN = new List<byte[]>();
        }

        private bool useRTSCTS = false;
        private bool useBAM = false;
        private bool isDone = false;

        public void UpdateJ1939Data(string data) {
            PendingCAN.Clear();
            PendingIDX = 0;
            useRTSCTS = false;
            useBAM = false;
            byte PS_GE = (byte)data[0];
            byte PF = (byte)data[1];
            byte R_DP = (byte)data[2];
            byte how_priority = (byte)data[3];
            byte SA = (byte)data[4];
            byte DA = (byte)data[5];
            if ((how_priority & 0x80) != 0)
            {
                useBAM = true; // BAM
            }
            else
            {
                useRTSCTS = true; // RTS/CTS
            }
            if (DA == 0xFF) // global address 
            {
                useBAM = true;
                useRTSCTS = false;
            }
            int msgDataLen = data.Length - 6;
            if (msgDataLen <= 8)
            {
                useRTSCTS = false;
                useBAM = false;
                byte[] can_pkt = new byte[4 + msgDataLen];
                //set priority, reserv, and dp bits
                can_pkt[0] = (byte)(can_pkt[0] | R_DP);
                can_pkt[0] = (byte)(can_pkt[0] | (how_priority & 0x03));
                //set PDU Format (PF)
                can_pkt[1] = PF;
                //set PDU Specific (PS), DA or GE
                if (PF >= 240)
                {
                    can_pkt[2] = PS_GE;
                }
                else
                {
                    can_pkt[2] = DA;
                }
                //set source address
                can_pkt[3] = SA;
                Support.StringToByteArray(data).CopyTo(can_pkt, 4);
                PendingCAN.Add(can_pkt);
            }
            //UNFINISHED                    
        }

        List<byte[]> PendingCAN;
        int PendingIDX = 0;

        public byte[] GetCANPacket() {
            if (useRTSCTS == false && useBAM == false)
            {
                if (PendingIDX >= PendingCAN.Count)
                {
                    return new byte[0];
                }
                return PendingCAN[PendingIDX++];
            }
            return new byte[0];
        }

        //call this function to notify class that last
        //transmit was successful
        public void TransmitConfirm() {
            if (useRTSCTS == false && useBAM == false)
            {
                if (PendingIDX >= PendingCAN.Count)
                {
                    isDone = true;
                }
            }
        }

        public bool IsDone()
        {
            return isDone;
        }
    }
}
