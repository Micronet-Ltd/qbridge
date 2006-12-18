using System;
using System.Text;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;

namespace qbrdge_driver_classlib
{
    public class J1939Transaction
    {
        public J1939Transaction()
        {
            PendingCAN = new List<byte[]>();
        }

        public bool useRTSCTS = false;
        private bool useBAM = false;
        private bool isDone = false;
        private bool isComplete = false;
        public bool isAddressClaim = false;

        public byte SA = 0x00;
        public byte DA = 0x00;
        private byte how_priority = 0x00;

        public void UpdateJ1939Data(string data) {
            isComplete = false;
            isDone = false;

            PendingCAN.Clear();
            PendingIDX = 0;
            useRTSCTS = false;
            useBAM = false;
            byte PS_GE = (byte)data[0];
            byte PF = (byte)data[1];
            byte R_DP = (byte)data[2];
            how_priority = (byte)data[3];
            DA = (byte)data[5]; 
            SA = (byte)data[4];
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
            UInt16 msgDataLen = (UInt16) (data.Length - 6);
            if (msgDataLen <= 8)
            {
                useRTSCTS = false;
                useBAM = false;
                byte[] can_pkt = new byte[5 + msgDataLen];
                //set priority, reserv, and dp bits
                can_pkt[4] = (byte)(can_pkt[4] | R_DP);
                can_pkt[4] = (byte)(can_pkt[4] | (byte)((how_priority & 0x07) * 2 * 2));
                //set PDU Format (PF)
                can_pkt[3] = PF;
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
                can_pkt[1] = SA;
                //set type extended CAN
                can_pkt[0] = 0x01;
                Support.StringToByteArray(data.Substring(6, data.Length-6)).CopyTo(can_pkt, 5);
                PendingCAN.Add(can_pkt);
            }
            else if (useBAM)
            {
                //make TP.CM_BAM packet
                //Data Length: 8bytes, DataPage: 0, PDU Format: 236, PDU Specific: Destination Address
                //Default priority: 7, PGN: 00EC00
                //Control byte = 32, Broadcast Announce Message
                byte tot_pkts = (byte)((msgDataLen + 6) / 7);
                AddTPCMCanPacket(32, DA, SA, data, msgDataLen, tot_pkts);
                AddTPDTCanPackets(DA, SA, data, tot_pkts);
            }
            else if (useRTSCTS)
            {
                //make TP.CM_RTS packet
                //Data Length: 8bytes, DataPage: 0, PDU Format: 236, PDU Specific: Destination Address
                //Default priority: 7, PGN: 00EC00
                //Control Byte = 16, request to send
                byte tot_pkts = (byte)((msgDataLen + 6) / 7);
                AddTPCMCanPacket(16, DA, SA, data, msgDataLen, tot_pkts);
                AddTPDTCanPackets(DA, SA, data, tot_pkts);
                IDXMax = 0;
            }
        }

        private void AddTPCMCanPacket(byte controlByte, byte DA, byte SA, string data,
            UInt16 msgDataLen, byte tot_pkts)
        {
            byte[] can_pkt = new byte[5 + 8];
            can_pkt[0] = 0x01; //can extended
            //set priority, reserv, and dp bits
            can_pkt[4] = 0x00;
            can_pkt[4] = (byte)(can_pkt[4] | (byte)((how_priority & 0x07) * 2 * 2));
            //set PDU Format (PF)
            can_pkt[3] = 0xEC;
            //set PDU Specific (Dest. Address)
            can_pkt[2] = DA;
            //set source address
            can_pkt[1] = SA;
            //data 8 bytes
            //Control byte
            can_pkt[5] = controlByte;
            //total message size, number of bytes
            byte[] msg_size = Support.UInt16ToBytes(msgDataLen, false);
            can_pkt[6] = msg_size[0];
            can_pkt[7] = msg_size[1];
            //total number of packets
            can_pkt[8] = tot_pkts;
            //Max num of packets that can be sent in response to CTS
            can_pkt[9] = 0xFF;
            //parameter group number of the packeted msg
            can_pkt[10] = (byte)data[0];
            can_pkt[11] = (byte)data[1];
            can_pkt[12] = (byte)data[2];
            PendingCAN.Add(can_pkt);
        }

        private void AddTPDTCanPackets(byte DA, byte SA, string data, byte tot_pkts)
        {
            for (int i = 0; i < tot_pkts; i++)
            {
                //send TP.DT pkts
                //Data Length: 8, Data Page: 0, PDU Format: 235, PDU Specific: Destinatin Address
                //Default priority: 7, PGN: 00EB00
                byte[] tp_pkt = new byte[5 + 8];
                tp_pkt[0] = 0x01; //can extended
                //set priority, reserv and dp bits
                tp_pkt[4] = 0x00;
                tp_pkt[4] = (byte)(tp_pkt[4] | (byte)((how_priority & 0x07) * 2 * 2));
                //set pdu format (PF)
                tp_pkt[3] = 0xEB;
                //set pdu specific (DA)
                tp_pkt[2] = DA;
                //set source address
                tp_pkt[1] = SA;
                //sequence number
                tp_pkt[5] = (byte)(i + 1);
                //data 7 bytes
                Debug.WriteLine("length: " + data.Length.ToString());
                for (int j = 0; j < 7; j++)
                {                        
                    int idx = 6 + (i * 7) + j;
                    if (idx < data.Length)
                    {
                        tp_pkt[6 + j] = (byte)data[idx];
                    }
                    else
                    {
                        tp_pkt[6 + j] = 0xFF;
                    }
                }
                PendingCAN.Add(tp_pkt);
            }
        }

        List<byte[]> PendingCAN;
        int PendingIDX = 0;

        int IDXMax = 0; //used for RTS/CTS only

        public byte[] GetCANPacket() {
            if (isDone)
            {
                return new byte[0];
            }
            if (useRTSCTS == false)
            {
                if (PendingIDX >= PendingCAN.Count)
                {
                    return new byte[0];
                }
                return PendingCAN[PendingIDX++];
            }
            else if (useRTSCTS == true)
            {
                if (PendingIDX <= IDXMax && PendingIDX < PendingCAN.Count)
                {
                    StartTimer();
                    return PendingCAN[PendingIDX++];
                }
            }
            return new byte[0];
        }

        //call this function to notify class that last
        //transmit was successful
        public void TransmitConfirm() {
            if (useRTSCTS == false)
            {
                if (PendingIDX >= PendingCAN.Count)
                {
                    isDone = true;
                    isComplete = true;
                }
            }
        }

        public bool IsDone()
        {
            return isDone;
        }

        public bool IsComplete()
        {
            return isComplete;
        }

        //call this function when incoming CAN is recieved
        //it will parse it if needed for RTS/CTS
        //data is the last 7 bytes of the CAN packet data
        public void ProcessCAN(byte sourceAddr, byte destAddr, byte controlByte, byte[] data) {
            if (isDone)
            {
                return;
            }
            if (useRTSCTS == false)
            {
                return;
            }
            if (controlByte == 17 && sourceAddr == DA && destAddr == SA)
            {
                PendingIDX = data[1];
                IDXMax = PendingIDX + data[0];
                Debug.WriteLine("RECV CTS: " + PendingIDX.ToString() + " -> " + IDXMax.ToString());
                if (isDone == false)
                    StartTimer();
            }
            else if (controlByte == 19 && sourceAddr == DA && destAddr == SA)
            {
                isDone = true;
                isComplete = true;
                StopTimer();
            }
            return;       
        }

        //abort RTS CTS
        public void AddressAbort()
        {
            isDone = true;
            isComplete = false;
            PendingCAN.Clear();
            addrLost = true;
        }
        public bool addrLost = false;

        //this function is called by the timer class when
        //the bampacket recieve has timed out
        public void TimeOut(Object state)
        {
            if (isDone == false)
            {
                isDone = true;
                isComplete = false;
                if (useRTSCTS == false)
                {
                    isComplete = true;
                }
            }
            QBSerial.CheckForCompleteJ1939();
        }

        Timer myTimer;

        //timers only used for RTS/CTS
        private void StartTimer()
        {
            StopTimer();
            TimerCallback timerDelegate = new TimerCallback(TimeOut);
            if (useRTSCTS)
            {
                myTimer = new Timer(timerDelegate, this, 120000, Timeout.Infinite);
            }
            else
            {
                myTimer = new Timer(timerDelegate, this, 10000, Timeout.Infinite);
            }
        }
        private void StopTimer()
        {
            if (myTimer != null)
            {
                myTimer.Dispose();
            }
        }
    }

    //keep track of incoming RTS/CTS packet dat, timeout and send CTS if data not recieved
    //within time limit.   Clear packets after # retry.  Create function to determine if
    //complete.  Send end of msg when last data packet received.
    class J1939RTSCTSReceiver
    {
        public J1939RTSCTSReceiver(SerialPortInfo portInfo, byte[] pgn, byte howPriority, byte sourceAddress,
            byte destAddress, UInt16 msgSize, byte numPkts, byte maxPktSend, int clientidx)
        {
            StartTimer();
            if (pgn.Length != 3)
            {
                isvalid = false;
            }
            param_group_num = pgn;
            how_priority = howPriority;
            source_addr = sourceAddress;
            dest_addr = destAddress;
            tot_msg_size = msgSize;
            tot_num_pkts = numPkts;
            max_pkt_send = maxPktSend;
            port_info = portInfo;
            num_retries = 125;
            nextSeq = 1;
            client_idx = clientidx;
            SendCTSPacket(false);

            //keep track of what packets have been 
            //received.
            main_data = new byte[numPkts * 7];
            main_status = new bool[numPkts];
        }

        public int client_idx; //client number that has claimed DA

        private int PktTimeout = 1000; //milliseconds

        private int nextSeq = 1; //next expected Seq number in DP packet.
        private bool isvalid = true;
        private bool iscomplete = false;
        private int nextCTS = 0; //when nextSeq equals nextCTS then send a new CTS

        //rp1210 variables
        private byte[] param_group_num = new byte[3];
        private byte how_priority = 0x00;
        private byte source_addr = 0x00; //source address of originator of connection
        public byte dest_addr = 0x00; //target address of the originator of connection (my addr)
        private UInt16 tot_msg_size;
        private byte tot_num_pkts;
        private byte max_pkt_send;

        private SerialPortInfo port_info;
        private byte[] main_data = new byte[0];
        private bool[] main_status = new bool[0];
        private byte num_retries;

        private void SendCTSPacket(bool isResend)
        {
            byte[] can_pkt = new byte[5 + 8];
            byte R_DP = 0x00;
            //set priority, reserv, and dp bits
            can_pkt[4] = (byte)(can_pkt[4] | R_DP);
            can_pkt[4] = (byte)(can_pkt[4] | (byte)((how_priority & 0x03) * 2 * 2));
            //set PDU Format (PF)
            can_pkt[3] = 0xEC; //TP.CM
            //set PDU Specific (PS), DA or GE
            can_pkt[2] = source_addr;
            //set source address
            can_pkt[1] = dest_addr;
            //set type extended CAN
            can_pkt[0] = 0x01;
            //control byte
            can_pkt[5+0] = 17;
            //max number of packets that can be sent
            if (isResend)
            {
                //find next seq
                for (int i = 0; i < main_status.Length; i++)
                {
                    if (main_status[i] == false)
                    {
                        nextSeq = i+1;
                        break;
                    }
                }
                //total to request
                byte tot = 0;
                for (int j = nextSeq-1; j < main_status.Length; j++)
                {
                    if (main_status[j] == false)
                    {
                        tot++;
                        if (tot >= max_pkt_send)
                        {
                            break;
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                can_pkt[5 + 1] = tot;
            }
            else
            {
                can_pkt[5 + 1] = max_pkt_send;
                nextCTS = nextSeq + max_pkt_send;
            }
            //next packet
            can_pkt[5 + 2] = (byte)nextSeq;
            can_pkt[5+3] = 0x00; //reserved
            can_pkt[5+4] = 0x00; //reserved
            can_pkt[5+5] = param_group_num[0]; //parameter group number
            can_pkt[5 + 6] = param_group_num[1];
            can_pkt[5 + 7] = param_group_num[2];
            //send CTS don't wait for ack
            byte pktId = 0;
            byte[] outData = QBSerial.MakeQBridgePacket(PacketCmdCodes.PKT_CMD_SEND_CAN, 
                can_pkt, ref pktId);

            Debug.Write("OUT CTS " + port_info.com.PortName + ": ");
            for (int j = 0; j < outData.Length; j++)
            {
                byte b = (byte)outData[j];
                Debug.Write(b.ToString() + ",");
            }
            Debug.WriteLine("");

            try
            {
                port_info.com.Write(outData, 0, outData.Length);
            }
            catch (Exception exp)
            {
                Debug.WriteLine("SendCTSPacket: "+exp.ToString());
            }
        }
        private void SendEndOfMsgAck()
        {
            byte[] can_pkt = new byte[5 + 8];
            byte R_DP = 0x00;
            //set priority, reserv, and dp bits
            can_pkt[4] = (byte)(can_pkt[4] | R_DP);
            can_pkt[4] = (byte)(can_pkt[4] | (byte)((how_priority & 0x03) * 2 * 2));
            //set PDU Format (PF)
            can_pkt[3] = 0xEC;
            //set PDU Specific (PS), DA or GE
            can_pkt[2] = source_addr;
            //set source address
            can_pkt[1] = dest_addr;
            //set type extended CAN
            can_pkt[0] = 0x01;
            //control byte, 19, end of message acknowledge
            can_pkt[5 + 0] = 19;
            //total message size, number of bytes
            byte[] msg_size = Support.UInt16ToBytes(tot_msg_size, false);
            can_pkt[5 + 1] = msg_size[0];
            can_pkt[5 + 2] = msg_size[1];
            //total num of packets
            can_pkt[5 + 3] = tot_num_pkts;
            //reserve
            can_pkt[5 + 4] = 0;
            //pgn
            can_pkt[5 + 5] = param_group_num[0]; //parameter group number
            can_pkt[5 + 6] = param_group_num[1];
            can_pkt[5 + 7] = param_group_num[2];
            //send End of Msg Ack don't wait for ack
            byte pktId = 0;
            byte[] outData = QBSerial.MakeQBridgePacket(PacketCmdCodes.PKT_CMD_SEND_CAN,
                can_pkt, ref pktId);

            Debug.Write("OUT EMA " + port_info.com.PortName + ": ");
            for (int j = 0; j < outData.Length; j++)
            {
                byte b = (byte)outData[j];
                Debug.Write(b.ToString() + ",");
            }
            Debug.WriteLine("");

            try
            {
                port_info.com.Write(outData, 0, outData.Length);
            }
            catch (Exception exp)
            {
                Debug.WriteLine("SendCTSPacket: " + exp.ToString());
            }
        }

        //if the class has timed out, then it is marked as invalid
        //and should be removed from the Queue
        public bool IsValid()
        {
            return isvalid;
        }

        //when an entire packet has been formed, mark as complete
        public bool IsComplete()
        {
            return iscomplete;
        }

        //call this function to stop recieving and abort
        public void Abort()
        {
            isvalid = false;
            iscomplete = false;
        }

        //call this whenever a new potential TD packet is
        //recieved by the QBridge
        //data is the last 7 bytes of the CAN packet data
        public bool UpdateData(string portName, byte sourceAddr, byte seqNum, byte[] data)
        {
            if (sourceAddr != source_addr || portName != port_info.com.PortName)
            {
                return iscomplete;
            }

            Debug.WriteLine("CURRSEQ: " + seqNum.ToString() + " HX: "+seqNum.ToString("X2"));
            Debug.Write("SEQDATA: ");
            for (int i = 0; i < data.Length; i++)
            {
                Debug.Write(data[i].ToString() + ",");
            }
            Debug.WriteLine("");

            if (seqNum > main_status.Length || data.Length != 7)
            {
                return iscomplete;
            }

            //add data to main_data
            main_status[seqNum-1] = true;
            Array.Copy(data, 0, main_data, 7 * (seqNum-1), 7);

            //is finish
            bool isFinish = true;
            for (int i = 0; i < main_status.Length; i++)
            {
                if (main_status[i] == false)
                {
                    isFinish = false;
                }
            }

            if (isFinish)
            {
                iscomplete = true;
                StopTimer();
                SendEndOfMsgAck();
                return iscomplete;
            }
            nextSeq = seqNum + 1;
            if (nextSeq == nextCTS && iscomplete == false)
            {
                SendCTSPacket(true);
            }
            if (iscomplete == false)
            {
                StartTimer();
            }
            return iscomplete;
        }

        //this function is called by the timer class when
        //the packet recieve has timed out
        public void TimeOut(Object state)
        {
            if (iscomplete)
            {
                return;
            }
            if (num_retries > 0)
            {
                num_retries--;
                Debug.WriteLine("TIMEOUT RESEND CTS "+port_info.com.PortName);
                SendCTSPacket(true);
                StartTimer();
            }
            else
            {
                isvalid = false;
            }
        }

        Timer myTimer;

        //timer functions
        private void StartTimer()
        {
            StopTimer();
            TimerCallback timerDelegate = new TimerCallback(TimeOut);
            myTimer = new Timer(timerDelegate, this, PktTimeout, Timeout.Infinite);
        }
        private void StopTimer()
        {
            if (myTimer != null)
            {
                myTimer.Dispose();
            }
        }

        //call this function when iscomplete to return
        //the bytes for rp1210a readmessage
        public byte[] GetRP1210ReadMessage()
        {
            //add timestamp to J1939 packet
            byte[] timestamp = Support.Int32ToBytes(Environment.TickCount, true);

            if (main_data.Length > tot_msg_size)
            {
                byte[] tmp = new byte[tot_msg_size];
                for (int i = 0; i < tmp.Length; i++)
                {
                    tmp[i] = main_data[i];
                }
                main_data = tmp;
            }

            byte[] readMsg = new byte[10 + main_data.Length];
            timestamp.CopyTo(readMsg, 0);
            param_group_num.CopyTo(readMsg, 4);
            readMsg[7] = how_priority;
            readMsg[8] = source_addr;
            readMsg[9] = dest_addr;
            main_data.CopyTo(readMsg, 10);
            return readMsg;
        }
    }

    //keep track of an incoming BAM packet data, timeout if data not recieved within
    //BAM 750ms time limit, and clear packet.  Create function to determine if
    //BAM packet complete.
    class J1939BAMReceiver 
    {
        public J1939BAMReceiver(string portName, byte[] pgn, byte howPriority, byte sourceAddress,
            byte destAddress, UInt16 msgSize, byte numPkts)
        {
            StartTimer();
            if (pgn.Length != 3) {
                isvalid = false;
            }
            param_group_num = pgn;
            how_priority = howPriority;
            source_addr = sourceAddress;
            dest_addr = destAddress;
            tot_msg_size = msgSize;
            tot_num_pkts = numPkts;
            port_name = portName;
        }

        private const int BAMPktTimeout = 8000; //milliseconds

        private int nextSeq = 1; //next expected Seq number in DP packet.
        private bool isvalid = true;
        private bool iscomplete = false;
        
        //rp1210 variables
        private byte[] param_group_num = new byte[3];
        private byte how_priority = 0x00;
        private byte source_addr = 0x00;
        public byte dest_addr = 0x00;
        private UInt16 tot_msg_size;
        private byte tot_num_pkts;

        private string port_name;
        private byte[] main_data = new byte[0];


        //if the class has timed out, then it is marked as invalid
        //and should be removed from the Queue
        public bool IsValid()
        {
            return isvalid;
        }

        //when an entire packet has been formed, mark as complete
        public bool IsComplete()
        {
            return iscomplete;
        }

        //call this whenever a new potential BAM packet is
        //recieved by the QBridge
        //data is the last 7 bytes of the CAN packet data
        public bool UpdateData(string portName, byte sourceAddr, byte seqNum, byte[] data)
        {
            if (sourceAddr != source_addr || seqNum != nextSeq || 
                portName != port_name)
            {
                return iscomplete;
            }

            Debug.WriteLine("SEQNUM: " + seqNum.ToString());
            Debug.Write("SEQDATA: ");
            for (int i = 0; i < data.Length; i++)
            {
                Debug.Write(data[i].ToString() + ",");
            }
            Debug.WriteLine("");

            //add data to main_data
            byte[] new_data = new byte[main_data.Length + data.Length];
            main_data.CopyTo(new_data, 0);
            data.CopyTo(new_data, main_data.Length);
            main_data = new_data;

            if (tot_num_pkts == nextSeq)
            {
                iscomplete = true;
                StopTimer();
            }
            nextSeq++;
            if (iscomplete == false)
            {
                StartTimer();
            }
            return iscomplete;
        }

        //this function is called by the timer class when
        //the bampacket recieve has timed out
        public void TimeOut(Object state) {
            if (iscomplete)
            {
                return;
            }
            isvalid = false;
        }

        Timer myTimer;

        //timer functions
        private void StartTimer()
        {
            StopTimer();
            TimerCallback timerDelegate = new TimerCallback(TimeOut);
            myTimer = new Timer(timerDelegate, this, BAMPktTimeout, Timeout.Infinite);       
        }
        private void StopTimer() {
            if (myTimer != null)
            {
                myTimer.Dispose();
            }
        }

        //call this function when iscomplete to return
        //the bytes for rp1210a readmessage
        public byte[] GetRP1210ReadMessage()
        {
            //add timestamp to J1939 packet
            byte[] timestamp = Support.Int32ToBytes(Environment.TickCount, true);

            if (main_data.Length > tot_msg_size)
            {
                byte[] tmp = new byte[tot_msg_size];
                for (int i = 0; i < tmp.Length; i++)
                {
                    tmp[i] = main_data[i];
                }
                main_data = tmp;
            }

            byte[] readMsg = new byte[10 + main_data.Length];
            timestamp.CopyTo(readMsg, 0);
            param_group_num.CopyTo(readMsg, 4);
            readMsg[7] = how_priority;
            readMsg[8] = source_addr;
            readMsg[9] = dest_addr;
            main_data.CopyTo(readMsg, 10);
            return readMsg;
        }
    }
}
