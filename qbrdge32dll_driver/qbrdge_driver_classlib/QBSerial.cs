#define IS_STOP_WAIT

using System;
using System.IO;
using System.Net;
using System.Text;
using System.IO.Ports;
using System.Threading;
using System.Diagnostics;
using System.Net.Sockets;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace qbrdge_driver_classlib
{
    class SerialPortInfo
    {
        public SerialPortInfo()
        {
        }

        public SerialPort com = null;
        public void StartRead()
        {
            com.ReadTimeout = 200;
            readThread = new Thread(new ThreadStart(ThreadRead));
            readThread.IsBackground = true;
            readThread.Priority = RP1210DllCom.sysThreadPriority;
            readThread.Start();
        }

        public int lastRecvPktId = 0;

        private const int MAX_BUFFSIZE = 10000;
        public byte[] inBuff = new byte[MAX_BUFFSIZE];
        public int inBuffLen = 0;

        public List<QBTransaction> QBTransactionNew = new List<QBTransaction>();
        public List<QBTransaction> QBTransactionSent = new List<QBTransaction>();
        public List<QBTransaction> QBTransJ1939Pending = new List<QBTransaction>();

        public bool qbInitNeeded = true;
        public bool requestRawMode = false;
        public Thread readThread = null;

        public void ThreadRead()
        {
            int slpMS = 1;
            //Support._DbgTrace("ReadThread Priority: " + Thread.CurrentThread.Priority.ToString());
            while (true)
            {
                //wait 1 ms seconds between reading packets
                Thread.Sleep(slpMS);
                try
                {
                    if (com.IsOpen)
                    {
                        int inDataLen = com.BytesToRead;
                        if (inDataLen <= 0)
                        {
                            //no data in wait longer
                            slpMS = 100;
                            continue;
                        }
                        else
                        {
                            slpMS = 1;
                        }

                        int newDataLen = Math.Min(inDataLen,inBuff.Length - inBuffLen-2);
                        com.Read(inBuff, inBuffLen, newDataLen);
                        //Support._DbgTrace("R");
                        lock (Support.lockThis)
                        {
                            inBuffLen += newDataLen;
                            if (inBuffLen > 0)
                            {
                                QBSerial.DataReceived(this);

                                QBSerial.CheckSendMsgQ();
                            }
                            if (inBuffLen > (MAX_BUFFSIZE - 1000)) { inBuffLen = 0; }
                        }
                    }
                }

                catch (InvalidOperationException)
                   {
                       //the specified port is not open
                       //return;
                       continue;
                   }
                   catch (TimeoutException)
                   {
                       continue;
                   }
	            catch (Exception)
	               {
	                   try
	                   {
	                       if (com != null)
	                       {
	                           com.Close();
	                           Thread.Sleep(100);
	                           com.Open();
	                           continue;
	                       }
	                   }
	                   catch (Exception) { 
	                       //return;
	                       continue;
	                   }
	                   continue;
	               }

            }
        }

        public bool GetReplyPending()
        {
#if IS_STOP_WAIT
            if (QBTransactionSent.Count > 0)
            {
                return true;
            }
            return false;
#else
            return false;
#endif
        }

        //special for firmware upgrade
        public bool fwUpgrade = false;

        public void RestartTimer()
        {
            try
            {
                StopTimer();
                Thread.Sleep(1);
                StartTimer();
            }
            catch (Exception) {/*RestartTimer()*/ }
        }

        //this timer will be used to detect a lost QBridge
        private Timer recvTimer = null;

        public void TimeOut(Object stateInfo)
        {
            Thread.CurrentThread.Priority = RP1210DllCom.sysThreadPriority;
            lock (Support.lockThis) 
            {
                if (QBTransactionSent.Count == 0) {
                    //add query transaction
                    QBTransaction qbt = new QBTransaction();
                    // comport being opened for the first time, send an Init command
                    // to verify that a QBridge is attached.
                    qbt.cmdType = PacketCmdCodes.PKT_CMD_INFO_REQ;
                    byte[] pktData = new byte[1];
                    pktData[0] = 0;
                    qbt.pktData = pktData;
                    qbt.timePeriod = Support.portLostLimit;
                    qbt.timeoutReply = UDPReplyType.sendJ1939replytimeout;
                    QBTransactionNew.Add(qbt);

                    QBSerial.CheckSendMsgQ();
                }
            }
        }

        private void StartTimer()
        {
            if (recvTimer == null)
            {
                TimerCallback timerDelegate = new TimerCallback(TimeOut);
                recvTimer = new Timer(timerDelegate, this, Timeout.Infinite, Timeout.Infinite);
            }

            recvTimer.Change(Convert.ToInt32(Support.portLostLimit), Timeout.Infinite);
        }

        public void StopTimer()
        {
            try
            {
                // Stop timer if it is running
                if (this.recvTimer != null)
                {
                    this.recvTimer.Change(Timeout.Infinite, Timeout.Infinite);
                }
            }
            catch (Exception) { /*StopTimer()*/}
        }       
    }

    class QBSerial
    {
        private static int msgBlockId = 1;
        public static int NewMsgBlockId()
        {
            msgBlockId++;
            if (msgBlockId > 100000)
            {
                msgBlockId = 1;
            }
            return msgBlockId;
        }

        static int msgIdIdx = 0;
        static bool[] msgIdAvail = new bool[127];

        private static List<SerialPortInfo> comPorts = new List<SerialPortInfo>();

        public static void InitQBSerial()
        {
            for (int i = 0; i < msgIdAvail.Length; i++)
            {
                msgIdAvail[i] = true;
            }
        }

        //a new packetId should be inserted in index 3 for
        //each transmitted j1708 packet
        private static byte packetId = 0;
        public static byte NewPacketID()
        {
            packetId++;
            if (packetId == 0)
            {
                packetId = 1;
            }
            return packetId;
        }

        public static bool GetSerialPortInfo(string comName, ref SerialPortInfo newcom)
        {
            for (int i = 0; i < comPorts.Count; i++)
            {
                if (comPorts[i].com.PortName == comName)
                {
                    newcom = comPorts[i];
                    return true;
                }
            }
            return false;
        }
        
        public static void RegisterSerialPort(string comNum, int clientId, IPEndPoint iep)
        {
            SerialPortInfo newcom = new SerialPortInfo();

            SerialPort com = null;
            for (int i = 0; i < closeComports.Count; i++)
            {
                if (closeComports[i].PortName == comNum)
                {
                    com = closeComports[i];
                    closeComports.RemoveAt(i);
                    break;
                }
            }
            if (com == null)
            {
                com = new SerialPort(comNum, 115200, Parity.None, 8, StopBits.One);
                com.ReadBufferSize = 40000;// 4096;

                try
                {
                    com.Open();
                }
                catch (Exception exp /*RegisterSerialPort com.open()*/)
                {
                    //Debug.WriteLine(exp.ToString());
                    RP1210DllCom.UdpSend("-4", iep);
                    return;
                }

                //send init to stop QB from sending data and empty serial buffer
                try
                {
                    byte[] pktData = new byte[0];
                    PacketCmdCodes cmdType = PacketCmdCodes.PKT_CMD_INIT;
                    byte pktId = 100;
                    byte[] outPkt = QBSerial.MakeQBridgePacket(cmdType, pktData, ref pktId);
                    com.Write(outPkt, 0, outPkt.Length);
                    com.ReadExisting();
                }
                catch (Exception) { /*RegisterSerialPort com.write*/}
            }

            //com is opened, ready to use, attach to serialinfo where reading will take place
            newcom.com = com;
            newcom.StartRead();

            ReInitSerialPort(ref newcom, clientId);

            //add to comportinfo to list
            comPorts.Add(newcom);

            ClientIDManager.clientIds[clientId].serialInfo = comPorts[comPorts.Count-1];
        }

        public static void ReInitSerialPort(ref SerialPortInfo sinfo, int clientId)
        {
            //Support._DbgTrace("ReInitSerialPort");
            //no udp reply send, create msg packet
            QBTransaction qbt = new QBTransaction();
            qbt.clientId = clientId;
            qbt.dllInPort = ClientIDManager.clientIds[clientId].dllInPort;
            // comport being opened for the first time, send an Init command
            // to verify that a QBridge is attached.
            qbt.cmdType = PacketCmdCodes.PKT_CMD_INIT;
            byte[] pktData = new byte[0];
            qbt.pktData = pktData;
            qbt.timePeriod = Support.ackReplyLimit;
            qbt.timeoutReply = UDPReplyType.sendJ1939replytimeout;
            sinfo.QBTransactionNew.Add(qbt);
        }

        private static bool WaitForValidComReply(SerialPort com)
        {
            com.ReadTimeout = 500; //500 ms
            try
            {
                int inDataLen = 0;
                byte[] inData = new byte[300];
                PacketCmdCodes cmdType = 0;
                PacketAckCodes ackCode = 0;
                byte[] pdata = new byte[0];
                byte pid = 0;
                for (int i = 0; i < 3; i++)
                {
                    int len = com.Read(inData, inDataLen, 300 - inDataLen);
                    inDataLen = inDataLen + len;
                    PacketRecvType ptype =
                        ProcessQBridgePacket(ref inData, ref inDataLen, ref cmdType,
                            ref pid, ref ackCode, ref pdata);
                    if (ptype == PacketRecvType.PKT_VALID)
                    {
                        return true;
                    }
                    else if (ptype == PacketRecvType.PKT_INVALID)
                    {
                        //Support._DbgTrace("WaitForValidComReply func, PKT_INVALID");
                        return false;
                    }
                }
            }
            catch (TimeoutException /*WaitForValidComReply return false*/)
            {
                return false;
            }
            return true;
        }

        public void Close()
        {
            for (int i = 0; i < comPorts.Count; i++)
            {               
                comPorts[i].com.Close();
            }
        }

        public static int GetMsgId()
        {
            msgIdIdx++;
            if (msgIdIdx >= msgIdAvail.Length)
            {
                msgIdIdx = 0;
            }
            if (msgIdAvail[msgIdIdx] == false)
            {
                msgIdIdx--;
                if (msgIdIdx < 0)
                {
                    msgIdIdx = msgIdAvail.Length - 1;
                }
                return 0;
            }
            else
            {
                msgIdAvail[msgIdIdx] = false;
                return (msgIdIdx + 1);
            }
        }

        public static void FreeMsgId(int msgId)
        {
            msgIdAvail[msgId - 1] = true;
        }

        public static void RemovePort(SerialPortInfo sinfo)
        {
            for (int i = 0; i < comPorts.Count; i++)
            {
                if (comPorts[i].com.PortName == sinfo.com.PortName)
                {
                    comPorts.RemoveAt(i);
                    try
                    {
                        closeComports.Insert(closeComports.Count, sinfo.com);
                    }
                    catch (Exception e /*RemovePort*/)
                    {
                        //Debug.WriteLine(e.ToString());
                    }
                    return;
                }
            }
        }

        private static PacketRecvType ProcessQBridgePacket(ref byte[] inData, ref int inDataLen,
            ref PacketCmdCodes cmdType, ref byte pktId, ref PacketAckCodes ackCode,
            ref byte[] pktData)
        {
            cmdType = 0;
            ackCode = 0;
            pktId = 0;
            int idx1 = 0;
            int pktLen = 0;
            for (int i = 0; i < inDataLen; i++)
            {
                if (inData[i] == 0x02)
                { //pkt start found
                    idx1 = i;
                    i++;
                    if (i >= inDataLen)
                    { //incomplete
                        return PacketRecvType.PKT_INCOMPLETE;
                    }
                    pktLen = (int)inData[i];
                    if ((idx1 + pktLen) > inDataLen)
                    {
                        return PacketRecvType.PKT_INCOMPLETE;
                    }
                    break;
                }
            }
            if (pktLen < 6)
            {
                RemoveLeftData(ref inData, ref inDataLen, idx1 + pktLen);
                return PacketRecvType.PKT_INVALID;
            }

            // verify crc, copy data into pkt array
            byte[] pkt = new byte[pktLen - 2];
            byte[] pktCrc = new byte[2];
            byte[] verCrc = new byte[2];
            Array.Copy(inData, idx1, pkt, 0, pkt.Length);
            Array.Copy(inData, idx1 + pktLen - 2, pktCrc, 0, 2);
            verCrc = GetCRC16(pkt);

            //copy data section
            pktData = new byte[pktLen - 6];
            Array.Copy(pkt, 4, pktData, 0, pktLen - 6);

            if (Support.ArrayCompare(verCrc, pktCrc) == false)
            {
                RemoveLeftData(ref inData, ref inDataLen, idx1 + 1);
                return PacketRecvType.PKT_INCOMPLETE;
            }

            cmdType = (PacketCmdCodes)pkt[2];
            if (cmdType == PacketCmdCodes.PKT_CMD_ACK && pktLen > 6)
            {
                ackCode = (PacketAckCodes)pkt[4];
            }
            pktId = pkt[3];

            RemoveLeftData(ref inData, ref inDataLen, idx1 + pktLen);
            return PacketRecvType.PKT_VALID;
        }

        private static void RemoveLeftData(ref byte[] inData, ref int inDataLen, int len)
        {
            inDataLen = inDataLen - len;
            Array.Copy(inData, len, inData, 0, inDataLen);
        }

        private static byte[] GetCRC16(byte[] data)
        {
            return calcCrc16(data);
        }

        public static UInt16[] crc16Tbl = new UInt16[256] {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
    };
        /***************************
        * calcCRC16
        ***************************/
        /* Calculates a 16 bit CRC
        * buf points to data (a bunch of bytes)
        * len indicates number of bytes pointed to by buf
        * This should be the CCITT standard CRC */
        public static byte[] calcCrc16(byte[] data)
        {
            byte p1;
            long i;
            ushort crc16;
            crc16 = 0xffff;
            for (i = 0; i < data.Length; i++)
            {
                p1 = data[i];
                ushort tmp1 = (ushort)((uint)crc16 >> 8);
                tmp1 = (ushort)((uint)tmp1 ^ (uint)p1);
                tmp1 = (ushort)((uint)tmp1 & 0x00ff);
                tmp1 = crc16Tbl[tmp1];
                ushort tmp2 = (ushort)((uint)crc16 << 8);
                crc16 = (ushort)((uint)tmp1 ^ (uint)tmp2);
            }
            byte[] crcData = new byte[2];
            crcData[0] = (byte) crc16;
            crcData[1] = (byte)((uint)crc16 >> 8);
            return (crcData);
        } /* end calcCrc16 */

        private static List<SerialPort> closeComports = new List<SerialPort>();

        //call this function outside of locked sections
        //to close comports
        public static void checkCloseComports()
        {
            while (closeComports.Count > 0)
            {
                SerialPort sport = new SerialPort();
                lock (Support.lockThis)
                {
                    if (closeComports.Count < 1)
                    {
                        break;
                    }
                    sport = closeComports[0];
                    closeComports.RemoveAt(0);
                }
                sport.Close();
            }
        }

        public static void DataReceived(SerialPortInfo portInfo)
        {
            if (portInfo.QBTransactionNew.Count == 0)
            {
                portInfo.RestartTimer();
            }
            else
            {
                portInfo.StopTimer();
            }

            if (portInfo.fwUpgrade)
            {
                ParseFWUpgrade(portInfo);
                return;
            }

            PacketCmdCodes cmdType = 0;
            PacketAckCodes ackCode = 0;
            byte[] pktData = new byte[0];
            byte pktId = 0;
            while (true)
            {
                if (portInfo.inBuffLen == 0 || portInfo.fwUpgrade)
                {
                    portInfo.inBuffLen = 0;
                    break;
                }

                PacketRecvType recvType = ProcessQBridgePacket(ref portInfo.inBuff, ref portInfo.inBuffLen,
                    ref cmdType, ref pktId, ref ackCode, ref pktData);

                if (recvType == PacketRecvType.PKT_INCOMPLETE)
                {
                    break;
                }
                else if (recvType == PacketRecvType.PKT_INVALID) {
                    //Support._DbgTrace("Invalid Packet, sending NACK");
                    SendACKPacket(PacketAckCodes.PKT_ACK_INVALID_PACKET, pktId, portInfo.com);
                    break;
                }
                else {                    
                    ProcessValidQBPacket(cmdType, pktId, ackCode, pktData, portInfo);
                }
            }
        }

        private static void ProcessValidQBPacket(PacketCmdCodes cmdType, byte pktId,
            PacketAckCodes ackCode, byte[] pktData, SerialPortInfo portInfo)
        {           
            //find matching transaction, if exists
            QBTransaction qbt = new QBTransaction();
            int qbtIdx = -1;
            int idx = 0;
            foreach (QBTransaction tmp in portInfo.QBTransactionSent)
            {
                if (tmp.pktId == pktId)
                {
                    qbt = tmp;
                    qbtIdx = idx;
                    break;
                }
                idx++;
            }
            
            if (cmdType == PacketCmdCodes.PKT_CMD_ACK &&
                ackCode == PacketAckCodes.PKT_ACK_OK &&
                pktData.Length == 6 &&
                qbt != null && qbtIdx > -1)
            {
                //reply from sendJ1708 pkt
                //get J1708PacketID
                byte[] j1708pktIdB = new byte[4];
                Array.Copy(pktData, 2, j1708pktIdB, 0, 4);
                int j1708pktId = Support.BytesToInt32(j1708pktIdB);

                if (qbt.cmdType == PacketCmdCodes.PKT_CMD_RAW_J1708)
                {
                    Support.SendClientDataPacket(UDPReplyType.sendJ1708success, qbt);
                    portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                    CheckJ1939Pending(portInfo);
                }
                else
                {
                    qbt.timePeriod = Support.j1708ConfirmLimit;
                    qbt.RestartTimer();
                    qbt.pktId = 0;
                    qbt.confirmId = j1708pktId;
                    qbt.timeoutReply = UDPReplyType.sendJ1708confirmfail;
                }
            }
            else if (cmdType == PacketCmdCodes.PKT_CMD_ACK &&
                (ackCode == PacketAckCodes.PKT_ACK_UNABLE_TO_PROCESS ||
                ackCode == PacketAckCodes.PKT_ACK_INVALID_DATA) &&
                qbt != null && qbtIdx > -1)
            {
                //error reply from j1708 packet
                Support.SendClientDataPacket(UDPReplyType.sendJ1708confirmfail, qbt);
                portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                CheckJ1939Pending(portInfo);
            }
            else if (cmdType == PacketCmdCodes.PKT_CMD_J1708_CAN_CONFIRM &&
                pktData.Length == 5)
            {                
                //reply from sendJ1708, or CAN pkt
                byte[] conIDB = new byte[4];
                Array.Copy(pktData, 1, conIDB, 0, 4);
                int conID = Support.BytesToInt32(conIDB);
                for (int i = 0; i < portInfo.QBTransactionSent.Count; i++)
                {
                    QBTransaction qt = portInfo.QBTransactionSent[i];
                    if (qt.confirmId == conID)
                    {
                        qt.StopTimer();
                        if (qt.isJ1939)
                        {
                            if (pktData[0] == 0x00)
                            {
                                //CAN transmit confirm failed, unable to be transmitted, fail
                                if (qt.j1939transaction.isAddressClaim == false)
                                {
                                    Support.SendClientDataPacket(UDPReplyType.sendJ1939confirmfail, qt);
                                }
                                else 
                                {
                                    ClientIDManager.clientIds[qt.clientId].AbortTimer(UDPReplyType.sendJ1939confirmfail);
                                }
                                portInfo.QBTransactionSent.RemoveAt(i);
                                CheckJ1939Pending(portInfo);
                                break;
                            }
                            else if (pktData[0] == 0x01)
                            {
                                //CAN transmit confirm, success
                                qt.j1939transaction.TransmitConfirm();
                                if (qt.j1939transaction.IsDone() && qt.j1939transaction.IsComplete())
                                {
                                    if (qt.j1939transaction.isAddressClaim == false)
                                    {
                                        Support.SendClientDataPacket(UDPReplyType.sendJ1939success, qt);
                                    }
                                    portInfo.QBTransactionSent.RemoveAt(i);
                                }
                                else if (qt.j1939transaction.IsDone() && !qt.j1939transaction.IsComplete())
                                {
                                    if (qt.j1939transaction.isAddressClaim == false)
                                    {
                                        Support.SendClientDataPacket(UDPReplyType.sendJ1939replytimeout, qt);
                                    }
                                    else
                                    {
                                        ClientIDManager.clientIds[qt.clientId].AbortTimer(UDPReplyType.sendJ1939replytimeout);
                                    }
                                    portInfo.QBTransactionSent.RemoveAt(i);          
                                }
                                else
                                {   //get next j1939 packet, and send... or wait..
                                    qt.pktData = qt.j1939transaction.GetCANPacket(); //get packet for current state.
                                    qt.StopTimer();
                                    portInfo.QBTransactionNew.Add(qt);
                                    portInfo.QBTransactionSent.RemoveAt(i);
                                }
                                CheckJ1939Pending(portInfo);
                                break;
                            }
                        }
                        else
                        {
                            if (pktData[0] == 0x00)
                            { 
                                //j1708 transmit confirm failed, unable to be transmitted, fail
                                Support.SendClientDataPacket(UDPReplyType.sendJ1708confirmfail, qt);
                            }
                            else if (pktData[0] == 0x01)
                            {
                                //j1708 transmit confirm, success
                                Support.SendClientDataPacket(UDPReplyType.sendJ1708success, qt);
                            }
                            portInfo.QBTransactionSent.RemoveAt(i);
                            CheckJ1939Pending(portInfo);
                            break;
                        }
                    }
                }
                // send ACK ok
                SendACKPacket(PacketAckCodes.PKT_ACK_OK, pktId, portInfo.com);
            }
            else if (cmdType == PacketCmdCodes.PKT_CMD_RECV_J1708)
            {
                //received j1708 data
                if (pktId != portInfo.lastRecvPktId)
                {
                    J1708PktRecv(portInfo.com.PortName, pktData, -1);
                    SendACKPacket(PacketAckCodes.PKT_ACK_OK, pktId, portInfo.com);
                    //Support._DbgTrace("J");
                }
                else
                {
                    SendACKPacket(PacketAckCodes.PKT_ACK_DUPLICATE_PACKET, pktId, portInfo.com);
                    //Support._DbgTrace("JD");
                }
                portInfo.lastRecvPktId = pktId;
            }
            else if (cmdType == PacketCmdCodes.PKT_CMD_RECV_CAN)
            {
                //received CAN data
                if (pktId != portInfo.lastRecvPktId)
                {
                    J1939PktRecv(portInfo, pktData, -1);
                    SendACKPacket(PacketAckCodes.PKT_ACK_OK, pktId, portInfo.com);
                    //Support._DbgTrace("C");
                }
                else
                {
                    SendACKPacket(PacketAckCodes.PKT_ACK_DUPLICATE_PACKET, pktId, portInfo.com);
                    //Support._DbgTrace("CD");
                }
                portInfo.lastRecvPktId = pktId;
            }
            else if (cmdType == PacketCmdCodes.PKT_CMD_ACK &&
                ackCode == PacketAckCodes.PKT_ACK_OK &&
                pktData.Length == 1)
            {
                // ACK Recieved
                if (qbt.pktId == pktId)
                {
                    if (qbt.cmdType == PacketCmdCodes.PKT_CMD_RESET_QBRIDGE)
                    {
                        //Reset QB acked, send init
                        qbt.cmdType = PacketCmdCodes.PKT_CMD_INIT;
                        qbt.RestartTimer();
                        ClearQBTransactionNew(qbt);
                        portInfo.QBTransactionNew.Add(qbt);
                        portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_INIT)
                    {   //Init acked, send Enable Transmit Confirm
                        byte[] pData = new byte[1];
                        pData[0] = 0x01;
                        qbt.pktData = pData;
                        qbt.cmdType = PacketCmdCodes.PKT_CMD_ENABLE_J1708_CONFIRM;
                        qbt.timePeriod = Support.ackReplyLimit;
                        qbt.timeoutReply = UDPReplyType.sendJ1708replytimeout;
                        qbt.RestartTimer();
                        ClearQBTransactionNew(qbt);
                        portInfo.QBTransactionNew.Add(qbt);
                        portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_ENABLE_J1708_CONFIRM)
                    {
                        //send can filter off
                        byte[] pData = new byte[2];
                        pData[0] = 0x65;
                        pData[1] = 0x00;
                        qbt.pktData = pData;
                        qbt.cmdType = PacketCmdCodes.PKT_CMD_CAN_CONTROL;
                        qbt.timePeriod = Support.ackReplyLimit;
                        qbt.timeoutReply = UDPReplyType.sendJ1939replytimeout;
                        qbt.RestartTimer();
                        ClearQBTransactionNew(qbt);
                        portInfo.QBTransactionNew.Add(qbt);
                        portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_CAN_CONTROL)
                    {
                        //Enable Confirm Ack, send mid filter off
                        byte[] pData = new byte[1];
                        pData[0] = 0x00;
                        qbt.pktData = pData;
                        qbt.cmdType = PacketCmdCodes.PKT_CMD_MID_FILTER;
                        qbt.timePeriod = Support.ackReplyLimit;
                        qbt.timeoutReply = UDPReplyType.sendJ1708replytimeout;
                        qbt.RestartTimer();
                        ClearQBTransactionNew(qbt);
                        portInfo.QBTransactionNew.Add(qbt);
                        portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_MID_FILTER)
                    {
                        qbt.StopTimer();
                        portInfo.qbInitNeeded = false;
                        if (qbt.sendCmdType == RP1210SendCommandType.SC_RESET_DEVICE)
                        {
                            ClientIDManager.RemoveClientID(qbt.clientId, qbt.dllInPort);
                            RP1210DllCom.UdpSend("0", qbt.dllInPort);
                        }
                        else
                        {
                            RP1210DllCom.UdpSend(qbt.clientId.ToString(), qbt.dllInPort);
                        }
                        portInfo.QBTransactionSent.RemoveAt(qbtIdx);

                        //update CAN filters on QBridge
                        RP1210DllCom.UpdateQBridgeCANFilters(qbt.clientId);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_UPGRADE_FIRMWARE)
                    {
                        qbt.StopTimer();
                        Thread.Sleep(3000);
                        string fileName = qbt.extraData;
                        //open firmware file and transmit
                        try
                        {
                            qbt.extraData = "";
                            FileStream fs = new FileStream(fileName, FileMode.Open, FileAccess.Read);
                            BinaryReader r = new BinaryReader(fs);
                            byte[] newData = new byte[0];
                            do
                            {
                                newData = r.ReadBytes(1000);
                                qbt.extraData = qbt.extraData + Support.ByteArrayToString(newData);
                            }
                            while (newData.Length > 0);

                            fs.Close();

                            qbt.timePeriod = 15000;
                            qbt.timeoutReply = UDPReplyType.sendJ1708replytimeout;

                            int j = qbt.extraData.IndexOf("\n", 0);
                            if (j < 0)
                            {
                                RP1210DllCom.UdpSend(
                                    ((int)RP1210ErrorCodes.ERR_FW_UPGRADE).ToString(), qbt.dllInPort);
                                portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                            }
                            else
                            {
                                qbt.fwUpgrade = true;
                                portInfo.fwUpgrade = true;
                                portInfo.com.Write("A\r\n");
                                qbt.lastSentPkt = Support.StringToByteArray("A\r\n");
                                qbt.extraIdx = 0;
                                qbt.StartTimer();
                                return;
                            }
                        }
                        catch (Exception /*ProcessValidQBPacket PKT_CMD_UPGRADE_FIRMWARE*/)
                        {
                            RP1210DllCom.UdpSend(
                                ((int)RP1210ErrorCodes.ERR_FW_UPGRADE).ToString(), qbt.dllInPort);
                            portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                        }
                    }
                }
            }
            else if (cmdType == PacketCmdCodes.PKT_CMD_ACK && qbt != null && qbtIdx > -1) 
            {
                if (qbt.cmdType == PacketCmdCodes.PKT_CMD_INFO_REQ) {
                    qbt.StopTimer();
                    portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                }
            }
            else {
                //Support._DbgTrace("Undefined Data Received");
                //Debug.Write("Undefined Data Received: ");
            }
        }

        //check for complete J1939 transactions and remove from lists
        public static void CheckForCompleteJ1939()
        {
            for (int j = 0; j < comPorts.Count; j++)
            {
                SerialPortInfo portInfo = comPorts[j];
                for (int i = 0; i < (portInfo.QBTransactionNew.Count + portInfo.QBTransactionSent.Count); i++)
                {
                    QBTransaction qt;
                    if (i < portInfo.QBTransactionNew.Count)
                    {
                        qt = portInfo.QBTransactionNew[i];
                    }
                    else
                    {
                        qt = portInfo.QBTransactionSent[i - portInfo.QBTransactionNew.Count];
                    }
                    if (qt.j1939transaction.IsDone())
                    {
                        if (qt.j1939transaction.IsComplete())
                        {
                            Support.SendClientDataPacket(UDPReplyType.sendJ1939success, qt);
                        }
                        else
                        {
                            if (qt.j1939transaction.useRTSCTS)
                            {
                                Support.SendClientDataPacket(UDPReplyType.sendJ1939RTSCTStimeout, qt);
                            }
                            else
                            {
                                Support.SendClientDataPacket(UDPReplyType.sendJ1939replytimeout, qt);
                            }
                        }
                        if (i < portInfo.QBTransactionNew.Count)
                        {
                            portInfo.QBTransactionNew.RemoveAt(i);
                        }
                        else
                        {
                            portInfo.QBTransactionSent.RemoveAt(i - portInfo.QBTransactionNew.Count);
                        }
                        i--;
                        CheckJ1939Pending(portInfo);
                    }
                }
            }
        }

        private static void ParseFWUpgrade(SerialPortInfo portInfo)
        {
            if (portInfo.inBuffLen < 1)
            {
                return;
            }
            if (portInfo.inBuff[portInfo.inBuffLen-1] != 0x15 && 
                portInfo.inBuff[portInfo.inBuffLen-1] != 0x06)
            {
                return;
            }
            portInfo.inBuffLen = 0;

            QBTransaction qbt = new QBTransaction();
            int qbtIdx = 0;

            for (int i = 0; i < portInfo.QBTransactionSent.Count; i++)
            {
                qbt = portInfo.QBTransactionSent[i];
                if (qbt.fwUpgrade)
                {
                    qbtIdx = i;
                    break;
                }
            }

            qbt.StopTimer();
            try
            {
                int idx = qbt.extraData.IndexOf("\n", qbt.extraIdx);
                if (idx < 0)
                {
                    qbt.fwUpgrade = false;
                    portInfo.fwUpgrade = false;
                    RP1210DllCom.UdpSend("0", qbt.dllInPort);
                    portInfo.QBTransactionSent.RemoveAt(qbtIdx);
                    
                    //add init packet
                    qbt.extraData = "";
                    qbt.extraIdx = 0;
                    qbt.fwUpgrade = false;
                    qbt.isNotify = false;
                    qbt.lastSentPkt = new byte[0];
                    qbt.pktData = new byte[0];
                    qbt.sendCmdType = RP1210SendCommandType.SC_UNDEFINED;
                    qbt.cmdType = PacketCmdCodes.PKT_CMD_INIT;
                    portInfo.QBTransactionNew.Insert(0, qbt);
                }
                else
                {
                    string tmp = qbt.extraData.Substring(qbt.extraIdx, idx - qbt.extraIdx + 1);
                    byte[] outData = Support.StringToByteArray(tmp);
                    qbt.extraIdx = idx+1;
                    portInfo.com.Write(outData, 0, outData.Length);
                    qbt.lastSentPkt = outData;
                    qbt.timePeriod = 60000;
                    qbt.timeoutReply = UDPReplyType.sendJ1708replytimeout;
                    qbt.StartTimer();
                }
            }
            catch (Exception /*ParseFWUpgrade*/)
            {
                RP1210DllCom.UdpSend(
                    ((int)RP1210ErrorCodes.ERR_FW_UPGRADE).ToString(), qbt.dllInPort);
                qbt.fwUpgrade = false;
                portInfo.fwUpgrade = false;
                portInfo.QBTransactionSent.RemoveAt(qbtIdx);
            }
            return;
        }

        public static void J1708PktRecv(string portName, byte[] pktData, int ignoreClientId)
        {
            //add timestamp to j1708 packet
            byte[] tstamp = Support.Int32ToBytes(Environment.TickCount, true);
            byte[] ptmp = new byte[4 + pktData.Length];
            tstamp.CopyTo(ptmp, 0);
            pktData.CopyTo(ptmp, 4);
            pktData = ptmp;
            byte mid = 0;
            if (pktData.Length > 4) {
                mid = pktData[4];
            }
            else {
                return;
            }

            for (int i = 0; i < ClientIDManager.clientIds.Length; i++)
            {
                ClientIDManager.ClientIDInfo clientInfo = ClientIDManager.clientIds[i];
                if (clientInfo.available == false &&
                    clientInfo.serialInfo.com.PortName == portName &&
                    clientInfo.allowReceive &&
                    clientInfo.isJ1939Client == false &&
                    i != ignoreClientId)
                {
                    if (clientInfo.J1708MIDFilter == false) {
                        Support.SendClientDataPacket(UDPReplyType.readmessage,
                            i, pktData);
                    }
                    else {
                        for (int j = 0; j < clientInfo.J1708MIDList.Length; j++) {
                            if (mid == clientInfo.J1708MIDList[j]) {
                                //if client is registered to recieve MID then send message
                                Support.SendClientDataPacket(UDPReplyType.readmessage,
                                    i, pktData);
                                break;
                            }
                        }
                    }
                }
            }
        }

        private static List<J1939BAMReceiver> BAMRecvList = new List<J1939BAMReceiver>();
        private static List<J1939RTSCTSReceiver> RTSCTSRecvList = new List<J1939RTSCTSReceiver>();

        private static int GetClientIdxFromClaimAddress(byte DA, SerialPortInfo portInfo)
        {
            //verify claimed address of a client matches destination address (DA)
            int clientIdx = -1;
            for (int i = 0; i < ClientIDManager.clientIds.Length; i++) {
                ClientIDManager.ClientIDInfo client = ClientIDManager.clientIds[i];
                if (client != null) {
                    if (client.serialInfo != null) {
                        if (portInfo.com == client.serialInfo.com) {
                            if (client.claimAddress == DA && client.claimAddrAvailable) {
                                clientIdx = i;
                            }
                        }
                    }
                }
            }
            return clientIdx;
        }

        //pktData in format:
        //[1 byte 0x01][4 bytes CAN Id][<= 8 bytes Msg Data]
        //see EIA-232 serial protocol to QBridge
        public static void J1939PktRecv(SerialPortInfo portInfo, byte[] pktData, int ignoreClientId)
        {
            if (pktData[0] == 0x00)
            {
                //standard CAN 
                return;
            }
            if (portInfo == null)
            {
                return;
            }
            string portName = portInfo.com.PortName;

            //add timestamp to J1939 packet
            byte[] timestamp = Support.Int32ToBytes(Environment.TickCount, true);

            byte[] pgn = new byte[3];
            byte how_priority, SA, DA;

            byte PS, PF, P_R_DP;
            SA = pktData[1];
            PS = pktData[2];
            PF = pktData[3];
            P_R_DP = pktData[4];

            pgn[1] = PF;
            if (PF >= 240)
            {
                pgn[0] = PS;
                DA = 0x00;
            }
            else
            {
                pgn[0] = 0x00;
                DA = PS;
            }
            pgn[2] = (byte)(P_R_DP & 0x03);
            how_priority = (byte)(((byte)P_R_DP / 2 / 2) & 0x07);
            //Check if it is a new BAM or RTS/CTStransfer
            if ((pktData.Length - 5) == 8) {
                J1939TPCtrlBytes TPCtrlByte = (J1939TPCtrlBytes)pktData[5];
                if (pgn[1] == 0xEC && pgn[2] == 0x00 && TPCtrlByte == J1939TPCtrlBytes.TP_CM_BAM)
                {
                    //TP.CAM_BAM packet
                    //Debug.WriteLine("BAM PKT "+portInfo.com.PortName);
                    byte[] data_pgn = new byte[3];
                    data_pgn[0] = pktData[5+5];
                    data_pgn[1] = pktData[5+6];
                    data_pgn[2] = pktData[5+7];
                    byte[] msg_size = new byte[2];
                    msg_size[0] = pktData[5 + 1];
                    msg_size[1] = pktData[5 + 2];
                    J1939BAMReceiver bamRecv = new J1939BAMReceiver(portName, data_pgn, how_priority, SA, DA,
                        Support.BytesToUInt16(msg_size, false), pktData[5 + 3]);
                    if (bamRecv.IsValid())
                    {
                        BAMRecvList.Add(bamRecv);
                    }
                    return;
                }
                else if (pgn[1] == 0xEC && TPCtrlByte == J1939TPCtrlBytes.TP_CM_RTS)
                {
                    //TP.CM_RTS
                    int clientIdx = GetClientIdxFromClaimAddress(DA, portInfo);
                    if (clientIdx < 0)
                    {
                        //claimed address not found
                        return;
                    }

                    byte[] data_pgn = new byte[3];
                    data_pgn[0] = pktData[5+5];
                    data_pgn[1] = pktData[5+6];
                    data_pgn[2] = pktData[5+7]; //data_pgn in little endien format
                    byte[] msg_size = new byte[2];
                    msg_size[0] = pktData[5 + 1];
                    msg_size[1] = pktData[5 + 2];
                    J1939RTSCTSReceiver recv = new J1939RTSCTSReceiver(portInfo, data_pgn, how_priority, SA, DA,
                        Support.BytesToUInt16(msg_size, false), pktData[5 + 3], pktData[5 + 4], clientIdx);
                    if (recv.IsValid())
                    {
                        RTSCTSRecvList.Add(recv);
                    }
                    return;                    
                }
                else if (pgn[1] == 0xEB && pgn[2] == 0x00)
                {
                    //TP.DT packet

                    //check each BAM receiver
                    for (int i = 0; i < BAMRecvList.Count; i++)
                    {
                        J1939BAMReceiver br = BAMRecvList[i];                        
                        byte[] temp = new byte[7];
                        for (int j = 0; j < 7; j++) {
                            temp[j] = pktData[6+j];
                        }
                        //notify bam receiver of new data
                        br.UpdateData(portName, SA, pktData[5], temp);
                        if (br.IsComplete())
                        {
                            //remove from list and send to rp1210 app.
                            BAMRecvList.RemoveAt(i);
                            i--;
                            byte[] rmsg = br.GetRP1210ReadMessage();
                            NewClientsJ1939ReadMessage(rmsg, portName, ignoreClientId);
                        }
                        else if (br.IsValid() == false)
                        {
                            BAMRecvList.RemoveAt(i);
                            i--;
                        }
                    }
                    //check each RTSCTS receiver
                    for (int i = 0; i < RTSCTSRecvList.Count; i++)
                    {
                        J1939RTSCTSReceiver br = RTSCTSRecvList[i];
                        byte[] temp = new byte[7];
                        for (int j = 0; j < 7; j++)
                        {
                            temp[j] = pktData[6 + j];
                        }

                        //notify BAM receiver of new data
                        br.UpdateData(portName, SA, pktData[5], temp);
                        if (br.IsComplete())
                        {
                            //remove from list and send to rp1210 app.
                            RTSCTSRecvList.RemoveAt(i);
                            i--;
                            byte[] rmsg = br.GetRP1210ReadMessage();
                            ClientJ1939ReadMsg(rmsg, portName, br.client_idx);
                        }
                        else if (br.IsValid() == false)
                        {
                            RTSCTSRecvList.RemoveAt(i);
                            i--;
                        }
                    }

                    return;
                }
                else if (pgn[1] == 0xEC && pgn[2] == 0x00 && (TPCtrlByte == J1939TPCtrlBytes.TP_CM_CTS || TPCtrlByte == J1939TPCtrlBytes.TP_CM_EndOfMsgACK
                    || TPCtrlByte == J1939TPCtrlBytes.TP_Conn_Abort)) 
                {
                    //TP.CM_CTS, TP.CM_EndOfMsgACK OR TP.Conn_Abort 

                    if (TPCtrlByte.Equals(J1939TPCtrlBytes.TP_Conn_Abort)) {
                        //TP.Conn_Abort
                        int clientIdx = GetClientIdxFromClaimAddress(DA, portInfo);
                        if (clientIdx < 0) {
                            //claimed address not found, ignore
                            return;
                        }
                        ClientIDManager.ClientIDInfo client = ClientIDManager.clientIds[clientIdx];
                        //abort receiving RTS/CTS connections
                        foreach (J1939RTSCTSReceiver recv in RTSCTSRecvList) {
                            if (recv.client_idx == clientIdx) {
                                recv.Abort();
                            }
                        }
                        for (int i = BAMRecvList.Count - 1; i >= 0; i--) {
                            J1939BAMReceiver br = BAMRecvList[i];
                            if (SA == br.source_addr && portName == br.port_name) {
                                BAMRecvList.RemoveAt(i);
                            }
                        }
                    }

                    byte[] temp = new byte[7];
                    for (int j = 0; j < 7; j++) {
                        temp[j] = pktData[6 + j];
                    }

                    //check each RTS/CTS pending transaction, traverse all QBTransactions
                    for (int i = 0; i < (portInfo.QBTransactionNew.Count + portInfo.QBTransactionSent.Count); i++) {
                        QBTransaction qt;
                        if (i < portInfo.QBTransactionNew.Count) {
                            qt = portInfo.QBTransactionNew[i];
                        }
                        else {
                            qt = portInfo.QBTransactionSent[i - portInfo.QBTransactionNew.Count];
                        }
                        if (qt.isJ1939) {
                            if (qt.j1939transaction.IsDone() == false) 
                            {
                                qt.j1939transaction.ProcessCAN(SA, DA, TPCtrlByte, temp);
                            }
                        }
                        if (qt.j1939transaction.IsDone()) {
                            if (TPCtrlByte == J1939TPCtrlBytes.TP_Conn_Abort) {
                                Support.SendClientDataPacket(UDPReplyType.sendJ1939TPAbort, qt);
                            }
                            else if (qt.j1939transaction.IsComplete()) {
                                Support.SendClientDataPacket(UDPReplyType.sendJ1939success, qt);
                            }
                            else {
                                Support.SendClientDataPacket(UDPReplyType.sendJ1939replytimeout, qt);
                            }
                            if (i < portInfo.QBTransactionNew.Count) {
                                portInfo.QBTransactionNew.RemoveAt(i);
                            }
                            else {
                                portInfo.QBTransactionSent.RemoveAt(i);
                            }
                            i--;
                            CheckJ1939Pending(portInfo);
                            if (TPCtrlByte == J1939TPCtrlBytes.TP_CM_EndOfMsgACK) 
                            {
                                //EndOfMsgAck... only process one
                                return;
                            }
                        }
                        else {
                            if (qt.pktData.Length == 0) {
                                qt.pktData = qt.j1939transaction.GetCANPacket();
                            }
                        }
                    }
                    return;
                }
                else if (pgn[1] == 0xEE && pgn[2] == 0x00)
                {
                    //Address Claimed Message
                    for (int i = 0; i < ClientIDManager.clientIds.Length; i++)
                    {
                        ClientIDManager.ClientIDInfo client = ClientIDManager.clientIds[i];
                        if (client == null)
                        {
                            //do nothing
                        }
                        else if (client.serialInfo == null)
                        {
                            //do nothing
                        }
                        else if (i != ignoreClientId && portInfo.com.PortName == client.serialInfo.com.PortName)
                        {
                            if ((int)SA == (int)client.claimAddress)
                            {
                                //address claim conflict
                                byte[] claimName = new byte[8];
                                for (int j = 0; j < 8; j++)
                                {
                                    claimName[j] = pktData[5 + j];
                                }
                                UInt64 extNameVal = Support.BytesToUInt64(claimName);
                                UInt64 nameVal = Support.BytesToUInt64(client.claimAddressName);
                                if (nameVal < extNameVal)
                                {
                                    //don't lose claim, send address claim for own address
                                    //create address claim message
                                    QBTransaction qbt = null;
                                    RP1210DllCom.AddAddressClaimMsg(SA, client.claimAddressName, i,
                                        false, -1, ref qbt);
                                }
                                else
                                {
                                    //loose claim on address
                                    AbortClientRTSCTS(client.serialInfo, (byte)client.claimAddress);
                                    client.lostClaimAddress = client.claimAddress;
                                    client.claimAddress = -1;                                    
                                }
                            }
                        }
                    }
                    return;
                }
            }

            byte[] newpkt = new byte[10 + pktData.Length-5];
            pktData.CopyTo(newpkt, 5); //copy data
            timestamp.CopyTo(newpkt, 0);
            pgn.CopyTo(newpkt, 4);
            newpkt[7] = how_priority;
            newpkt[8] = SA;
            newpkt[9] = DA;

            NewClientsJ1939ReadMessage(newpkt, portName, ignoreClientId);
        }

        private static void NewClientsJ1939ReadMessage(byte[] readMsg, string portName, int ignoreClientId)
        {
            for (int i = 0; i < ClientIDManager.clientIds.Length; i++)
            {
                if (i != ignoreClientId)
                {
                    ClientJ1939ReadMsg(readMsg, portName, i);
                }
            }
        }
        private static void ClientJ1939ReadMsg(byte[] readMsg, string portName, int client)
        {
            ClientIDManager.ClientIDInfo clientInfo = ClientIDManager.clientIds[client];
            if (clientInfo.available == false &&
                clientInfo.serialInfo.com.PortName == portName &&
                clientInfo.allowReceive &&
                clientInfo.isJ1939Client) 
            {
                if (clientInfo.J1939Filter == false)
                {
                    //if client is registered to receiving port then send message
                    Support.SendClientDataPacket(UDPReplyType.readmessage,
                        client, readMsg);
                }
                else {
                    //if no filters set, then return
                    if (clientInfo.J1939FilterList.Count == 0) {
                        return;
                    }

                    //Uncomment next 2 lines when testing filtering moved to QBridge
                    //Support.SendClientDataPacket(UDPReplyType.readmessage, client, readMsg);
                    //return;

                    byte[] msg_pgn = new byte[3];
                    msg_pgn[0] = readMsg[4];
                    msg_pgn[1] = readMsg[5];
                    msg_pgn[2] = readMsg[6];
                    byte msg_source = readMsg[8];
                    byte msg_dest = readMsg[9];
                    byte msg_priority = (byte)((byte)(readMsg[7]*2*2*2*2*2)/2/2/2/2/2);
                    bool use_pgn, use_source, use_dest, use_priority;
                    for (int i = 0; i < clientInfo.J1939FilterList.Count; i++)
                    {
                        ClientIDManager.J1939Filter jf = clientInfo.J1939FilterList[i];
                        if ((jf.flag & 0x01) != 0)
                            use_pgn = true;
                        else
                            use_pgn = false;
                        if ((jf.flag & 0x02) != 0)
                            use_priority = true;
                        else
                            use_priority = false;
                        if ((jf.flag & 0x04) != 0)
                            use_source = true;
                        else
                            use_source = false;
                        if ((jf.flag & 0x08) != 0)
                            use_dest = true;
                        else
                            use_dest = false;
                        if ( ((use_pgn && jf.pgn[0] == msg_pgn[0] && jf.pgn[1] == msg_pgn[1] && jf.pgn[2] == msg_pgn[2]) ||
                            (use_pgn == false))
                            &&
                            ((use_priority && jf.priority == msg_priority) || (use_priority == false))
                            &&
                            ((use_source && jf.sourceAddr == msg_source) || (use_source == false))
                            &&
                            ((use_dest && jf.destAddr == msg_dest) || (use_dest == false)) )
                        {
                            Support.SendClientDataPacket(UDPReplyType.readmessage, client, readMsg);
                            return;
                        }
                    }
                }
            }
        }

        private static void SendACKPacket(PacketAckCodes ackCode, byte pktId, SerialPort com)
        {
            // to verify that a QBridge is attached.
            byte[] pktData = new byte[1];
            pktData[0] = (byte)ackCode;
            byte[] newpkt = MakeQBridgePacket(PacketCmdCodes.PKT_CMD_ACK, pktData, ref pktId);
            try {
                com.Write(newpkt, 0, newpkt.Length);
            }
            catch (Exception) {/*SendACKPacket*/ }
        }

        public static byte[] MakeQBridgePacket(PacketCmdCodes cmdType, byte[] data, ref byte pktId)
        {
            byte[] pkt = new byte[4 + data.Length];
            pkt[0] = 0x02;
            pkt[1] = (byte)(pkt.Length + 2);
            pkt[2] = (byte)cmdType; // Init command see QBridge docs
            if (cmdType == PacketCmdCodes.PKT_CMD_ACK)
            {
                pkt[3] = pktId;
            }
            else
            {
                pkt[3] = NewPacketID();
                pktId = pkt[3];
            }
            data.CopyTo(pkt, 4);

            //calc and add crc
            byte[] crc = GetCRC16(pkt);
            byte[] newpkt = new byte[pkt.Length + 2];
            pkt.CopyTo(newpkt, 0);
            if (crc != null)
            {
                newpkt[newpkt.Length - 2] = crc[0];
                newpkt[newpkt.Length - 1] = crc[1];
            }
            return newpkt;
        }

        // call this function to check the send msg Q and send messages if possible
        // this function should always be called in a synchronized thread
        public static void CheckSendMsgQ()
        {
            foreach(SerialPortInfo serialInfo in comPorts)
            {
                for (int i = 0; i < serialInfo.QBTransactionNew.Count; i++) 
                {
                    if (serialInfo.GetReplyPending())
                    {
                        break;
                    }
                    QBTransaction qbt = serialInfo.QBTransactionNew[i];

                    //skip empty send CAN packets, pkData will be updated later by CheckJ1939Pending
                    while (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_CAN &&
                        qbt.pktData.Length == 0)
                    {
                        i++;
                        if (i >= serialInfo.QBTransactionNew.Count)
                            break;

                        qbt = serialInfo.QBTransactionNew[i];
                    }
                    if (i >= serialInfo.QBTransactionNew.Count)
                        break;
                    
                    qbt.lastSentPkt = MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);

                    try
                    {
                        if (qbt.isJ1939) {
                            if (qbt.j1939transaction.IsDone() == false)
                            {
                                serialInfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);
                                qbt.RestartTimer();                                
                            }
                        }
                        else {                            
                            serialInfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);
                            qbt.RestartTimer();
                        }
                        serialInfo.QBTransactionSent.Add(qbt);
                    }
                    catch (Exception exp /*CheckSendMsgQ*/)
                    {
                        SendClientErrorPacket(qbt);
                    }
                    serialInfo.QBTransactionNew.RemoveAt(i);
                    i--;
                    CheckJ1939Pending(serialInfo);
                }
            }
        }

        public static void SendClientErrorPacketAlt(QBTransaction qbt)
        {
            if (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_J1708 ||
                qbt.cmdType == PacketCmdCodes.PKT_CMD_RAW_J1708 ||
                qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_CAN)
            {
                Support.SendClientDataPacket(qbt.timeoutReply, qbt);
            }
            else
            {
                QBSerial.SendClientErrorPacket(qbt);
            }
        }

        public static void SendClientErrorPacket(QBTransaction qbt)
        {
            //Support._DbgTrace("SendClientErrorPacket, " + qbt.cmdType.ToString());
            if (qbt.sendCmdType != RP1210SendCommandType.SC_UNDEFINED) {
                RP1210DllCom.UdpSend(
                    RP1210ErrorCodes.ERR_HARDWARE_NOT_RESPONDING.ToString(), qbt.dllInPort);
            }
            else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_INIT) {
                RP1210DllCom.UdpSend("-5", qbt.dllInPort);
            }
            else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_J1708 ||
                qbt.cmdType == PacketCmdCodes.PKT_CMD_RAW_J1708) {
                Support.SendClientDataPacket(UDPReplyType.sendJ1708commerr, qbt);
            }
            else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_CAN) {
                Support.SendClientDataPacket(UDPReplyType.sendJ1939commerr, qbt);
            }
        }

        // call this function to clear the message Q when a serial
        //port is not responding and notifiy the DLL
        public static void ClearQBTransactionNew(SerialPortInfo serialInfo)
        {
            for (int i = 0; i < serialInfo.QBTransactionNew.Count; i++) {
                QBTransaction qbt = serialInfo.QBTransactionNew[i];

                //send appropriate error response to DLL that QB is not responding
                QBSerial.SendClientErrorPacketAlt(qbt);

                serialInfo.QBTransactionNew.RemoveAt(i);
                i--;
                CheckJ1939Pending(serialInfo);
            }
        }

        public static void ClearQBTransactionNew(QBTransaction qbt)
        {
            try {
                SerialPortInfo sInfo = Support.ClientToSerialPortInfo(qbt.clientId);
                ClearQBTransactionNew(sInfo);
            }
            catch (Exception) { /*ClearQBTransactionNew*/}
        }

        //returns msgQID
        public static int AddSendJ1708Msg(int clientId, string j1708msg, bool isNotify, bool isBlocking)
        {
            int msgId;
            if (isNotify)
            {
                msgId = GetMsgId();
                if (msgId <= 0)
                {
                    return msgId;
                }
            }
            else if (isBlocking)
            {
                msgId = NewMsgBlockId();
            }
            else
            {
                msgId = 0;
            }
            SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
            QBTransaction qbt = new QBTransaction();
            qbt.clientId = clientId;
            qbt.isNotify = isNotify;
            qbt.msgId = msgId;
            if (sinfo.requestRawMode)
            {
                qbt.cmdType = PacketCmdCodes.PKT_CMD_RAW_J1708;
            }
            else
            {
                qbt.cmdType = PacketCmdCodes.PKT_CMD_SEND_J1708;
            }
            qbt.pktData = Support.StringToByteArray(j1708msg);
            qbt.timePeriod = Support.ackReplyLimit;
            qbt.timeoutReply = UDPReplyType.sendJ1708replytimeout;
            ClientIdToSerialInfo(clientId).QBTransactionNew.Add(qbt);

            //send message to all clients on com, except sender
            ClientIDManager.ClientIDInfo sendClientInfo = ClientIDManager.clientIds[clientId];
            if (j1708msg.Length > 0)
            {
                j1708msg = j1708msg.Remove(0, 1);
            }
            J1708PktRecv(sendClientInfo.serialInfo.com.PortName, Support.StringToByteArray(j1708msg), clientId);
            return msgId;
        }

        //returns msgQID
        public static int AddSendJ1939Msg(int clientId, string msg, bool isNotify, bool isBlocking)
        {            
            int msgId;
            if (isNotify)
            {
                msgId = GetMsgId();
                if (msgId <= 0)
                {
                    return msgId;
                }
            }
            else if (isBlocking)
            {
                msgId = NewMsgBlockId();
            }
            else
            {
                msgId = 0;
            }

            //verify if PGN is valid
            byte[] tmp = new byte[4];
            tmp[0] = (byte)msg[0];
            tmp[1] = (byte)msg[1];
            tmp[2] = (byte)msg[2];
            tmp[3] = 0;
            if (BitConverter.ToUInt32(tmp, 0) > (UInt32)131071) //0x01FFFF
            {   //invalid PGN
                return -(int)RP1210ErrorCodes.ERR_INVALID_MSG_PACKET;
            }

            SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
            QBTransaction qbt = new QBTransaction();
            qbt.clientId = clientId;
            qbt.isNotify = isNotify;
            qbt.msgId = msgId;
            qbt.isJ1939 = true;
            qbt.j1939transaction = new J1939Transaction();
            qbt.numRetries = 3;
            qbt.cmdType = PacketCmdCodes.PKT_CMD_SEND_CAN;

            qbt.j1939transaction.UpdateJ1939Data(msg); //add message, process

            ClientIDManager.ClientIDInfo clientInfo = ClientIDManager.clientIds[clientId];
            if (qbt.j1939transaction.useRTSCTS && 
                (clientInfo.claimAddress < 0 || qbt.j1939transaction.SA != clientInfo.claimAddress ||
                clientInfo.claimAddrAvailable == false))
            {
                //need claimed address for RTS/CTS
                return -(int)RP1210ErrorCodes.ERR_ADDRESS_NEVER_CLAIMED;
            }

            if (qbt.j1939transaction.SA != clientInfo.claimAddress
                && qbt.j1939transaction.SA == clientInfo.lostClaimAddress)
            {
                clientInfo.lostClaimAddress = -1;
                return -(int)RP1210ErrorCodes.ERR_ADDRESS_LOST;
            }

            if (qbt.j1939transaction.useRTSCTS && msg[1] >= 240)
            {
                return -(int)RP1210ErrorCodes.ERR_INVALID_MSG_PACKET;
            }

            qbt.timePeriod = Support.j1708ConfirmLimit;
            qbt.timeoutReply = UDPReplyType.sendJ1939replytimeout;

            //add timestamp to J1939 packet
            byte[] timestamp = Support.Int32ToBytes(Environment.TickCount, true);
            byte[] newpkt = new byte[4 + msg.Length];
            timestamp.CopyTo(newpkt, 0);
            Support.StringToByteArray(msg).CopyTo(newpkt, 4);
            NewClientsJ1939ReadMessage(newpkt, sinfo.com.PortName, clientId);

            //delay 1939 so that packets to same addr aren't going
            //at the same time
            if (IsQBTransJ1939Pending(qbt, ClientIdToSerialInfo(clientId)) == true)
            {
                ClientIdToSerialInfo(clientId).QBTransJ1939Pending.Add(qbt);
                return msgId;
            }
            qbt.pktData = qbt.j1939transaction.GetCANPacket(); //get packet for current state.
            ClientIdToSerialInfo(clientId).QBTransactionNew.Add(qbt);

            return msgId;
        }

        public static void CheckJ1939Pending(SerialPortInfo sinfo) {
            for (int i = 0; i < sinfo.QBTransJ1939Pending.Count; i++)
            {
                QBTransaction qbt = sinfo.QBTransJ1939Pending[i];
                if (IsQBTransJ1939Pending(qbt, sinfo) == false)
                {
                    sinfo.QBTransJ1939Pending.RemoveAt(i);
                    i--;
                    qbt.pktData = qbt.j1939transaction.GetCANPacket(); //get packet for current state.
                    sinfo.QBTransactionNew.Add(qbt);
                }
            }
        }

        public static bool IsQBTransJ1939Pending(QBTransaction qbt, SerialPortInfo portInfo)
        {
            if (qbt.isJ1939)
            {
                for (int i = 0; i < (portInfo.QBTransactionNew.Count + portInfo.QBTransactionSent.Count); i++)
                {
                    QBTransaction qt;
                    if (i < portInfo.QBTransactionNew.Count)
                    {
                        qt = portInfo.QBTransactionNew[i];
                    }
                    else
                    {
                        qt = portInfo.QBTransactionSent[i - portInfo.QBTransactionNew.Count];
                    }
                    if (qt.isJ1939)
                    {
                        if (qt.clientId == qbt.clientId && qt.j1939transaction.SA == qbt.j1939transaction.SA &&
                            qt.j1939transaction.useRTSCTS == qbt.j1939transaction.useRTSCTS &&
                            qt.j1939transaction.DA == qbt.j1939transaction.DA)
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public static SerialPortInfo ClientIdToSerialInfo(int clientId)
        {
            return ClientIDManager.clientIds[clientId].serialInfo;
        }

        private void ClientUDPSend(string data, int cid)
        {
            ClientIDManager.ClientUDPSend(data, cid);
        }

        public static void ComReplyTimeOut(Object stateInfo)
        {
            Thread.CurrentThread.Priority = RP1210DllCom.sysThreadPriority;
            lock (Support.lockThis) 
            {
                QBTransaction qbt = (QBTransaction)stateInfo;

                if (qbt.numRetries > 0 && ClientIDManager.clientIds[qbt.clientId].available == false)
                {
                    //Support._DbgTrace("ComReplyTimeout, " + qbt.cmdType.ToString());
                    if (qbt.isJ1939)
                    {
                        if (qbt.j1939transaction.IsDone())
                        {
                            qbt.StopTimer();
                            return;
                        }
                    }
                    SerialPort com = Support.ClientToSerialPort(qbt.clientId);

                    if (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_CAN)
                    {
                        //QBridge, seems to get in a state where it's not sending a confirm on send
                        //try fixing by send enable transmit confirm and try again
                        //this seems to fix the state that QB gets in where it stops sending the confirm
                        byte[] pktData = new byte[1];
                        pktData[0] = 0x01;
                        PacketCmdCodes cmdType = PacketCmdCodes.PKT_CMD_ENABLE_J1708_CONFIRM;
                        byte pktId = 0;
                        byte[] outPkt = QBSerial.MakeQBridgePacket(cmdType, pktData, ref pktId);
                        com.Write(outPkt, 0, outPkt.Length);                        
                    }

                    qbt.lastSentPkt = MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);
                    com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);
                    qbt.numRetries--;
                    qbt.RestartTimer();
                }
                else
                {
                    //Support._DbgTrace("Comm Timeout, Re-Intitialize Sequence Started");
                    //timeout, assume QB removed an re-initialize
                    SerialPortInfo sInfo = Support.ClientToSerialPortInfo(qbt.clientId);
                    qbt.StopTimer();

                    if (qbt.sendCmdType != RP1210SendCommandType.SC_UNDEFINED)
                    {
                        RP1210DllCom.UdpSend(
                           ((int)RP1210ErrorCodes.ERR_HARDWARE_NOT_RESPONDING).ToString(), qbt.dllInPort);
                        RemoveSentQBTransaction(qbt);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_J1708 ||
                        qbt.cmdType == PacketCmdCodes.PKT_CMD_RAW_J1708)
                    {
                        Support.SendClientDataPacket(qbt.timeoutReply, qbt);
                        RemoveSentQBTransaction(qbt);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_SEND_CAN)
                    {
                        Support.SendClientDataPacket(qbt.timeoutReply, qbt);
                        RemoveSentQBTransaction(qbt);
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_INIT ||
                        qbt.cmdType == PacketCmdCodes.PKT_CMD_ENABLE_J1708_CONFIRM ||
                        qbt.cmdType == PacketCmdCodes.PKT_CMD_MID_FILTER)
                    {
                        RP1210DllCom.UdpSend("-3", qbt.dllInPort);
                        RemoveSentQBTransaction(qbt);
                        if (sInfo != null && sInfo.qbInitNeeded)
                        {
                            ClientIDManager.RemoveClientID(qbt.clientId);
                        }
                    }
                    else if (qbt.cmdType == PacketCmdCodes.PKT_CMD_INFO_REQ)
                    {
                        RemoveSentQBTransaction(qbt);
                    }

                    //QB not responding. Clear all pending transactions for clients 
                    //using that serial prort and re-initialize.
                    if (sInfo != null)
                    {
                        ClearQBTransactionNew(sInfo);
                        ReInitSerialPort(ref sInfo, qbt.clientId);
                    }
                    CheckSendMsgQ();
                }
            }
        }

        public static void RemoveSentQBTransaction(QBTransaction qbt)
        {
            SerialPortInfo sinfo = ClientIDManager.clientIds[qbt.clientId].serialInfo;
            if (sinfo == null)
            {
                return;
            }
            for (int i = 0; i < sinfo.QBTransactionSent.Count; i++)
            {
                QBTransaction qbts = sinfo.QBTransactionSent[i];
                if (qbts.pktId == qbt.pktId && qbts.cmdType == qbt.cmdType)
                {
                    sinfo.QBTransactionSent.RemoveAt(i);
                    CheckJ1939Pending(sinfo);
                    return;
                }
            }
        }

        public static SerialPortInfo ComNumToSerialPortInfo(int comNum)
        {
            string comStr = "COM" + comNum.ToString();
            for (int i = 0; i < comPorts.Count; i++)
            {
                if (comPorts[i].com.PortName == comStr)
                {
                    return comPorts[i];
                }
            }
            return null;
        }

        [DllImport("coredll.dll", SetLastError = true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool SystemParametersInfo(uint uiAction, uint uiParam, String pvParam, uint fWinIni);
        const uint SPI_GETPLATFORMNAME = 260;
        private static string GetTermName()
        {
            String termName = "";
            try {
                termName = termName.PadRight(64);
                SystemParametersInfo(SPI_GETPLATFORMNAME, (uint)termName.Length, termName, 0);
            }
            catch (Exception exp) {//GetTermName
                //Debug.WriteLine(exp.StackTrace);
            }
            return termName;
        }

        public static bool UpgradeFirmware()
        {   //used as blocking upgrade.
            bool usingComports = false;
            SerialPort com = null;
            try
            {
                if (GetTermName().IndexOf("VM") >= 0) {
                    com = new SerialPort("COM7", 115200, Parity.None, 8, StopBits.One);
                }
                else {
                    com = new SerialPort("COM3", 115200, Parity.None, 8, StopBits.One);
                }
            }
            catch (Exception exp)//UpgradeFirmware
            {
                exp.ToString();
                if (comPorts.Count <= 0)
                {
                    return false;
                }
                com = comPorts[0].com;
                usingComports = true;
            }
            com.Open();
            com.ReadTimeout = 15000;

            //read file data
            FileStream fs = new FileStream("qbridge.srec", FileMode.Open, FileAccess.Read);
            BinaryReader r = new BinaryReader(fs);
            string fileData = "";
            byte[] newData = new byte[0];
            do
            {
                newData = new byte[0];
                newData = r.ReadBytes(1000);
                fileData = fileData + Support.ByteArrayToString(newData);
            }
            while (newData.Length > 0);
            fs.Close();

            //loop and send data
            int idx1 = 0;
            int idx2 = 0;
            byte[] outData = new byte[0];
            byte[] inData = new byte[300];
            int inDataLen = 0;
            //fileData = "A\r\n" + fileData;

            do
            {
                idx2 = fileData.IndexOf('\n', idx1);
                if (idx2 == -1)
                {
                    break;
                }
                outData = Support.StringToByteArray(fileData.Substring(idx1, idx2 - idx1 + 1));
                com.Write(outData, 0, outData.Length);
                try
                {
                    inDataLen = com.Read(inData, 0, 300);
                }
                catch (Exception exp)//UpgradeFirmware com.read
                {
                    //Debug.WriteLine("FIRMWARE UPGRADE FAILED "+exp.ToString());
                    com.Close();
                    return false;
                }
                if (inData[inDataLen - 1] == 0x06)
                {
                    idx1 = idx2 + 1;
                }
            }
            while (true);

            if (usingComports == false)
            {
                com.Close();
            }
            return true;
        }

        public static void AbortClientRTSCTS(SerialPortInfo portInfo, byte oldAddr) {
            //stop receiving on the address
            for (int i = 0; i < RTSCTSRecvList.Count; i++)
            {
                if (RTSCTSRecvList[i].dest_addr == oldAddr)
                {
                    RTSCTSRecvList[i].Abort();
                }
            }
            //stop sending on that address
            for (int i = 0; i < (portInfo.QBTransactionNew.Count + portInfo.QBTransactionSent.Count); i++)
            {
                QBTransaction qt;
                if (i < portInfo.QBTransactionNew.Count)
                {
                    qt = portInfo.QBTransactionNew[i];
                }
                else
                {
                    qt = portInfo.QBTransactionSent[i - portInfo.QBTransactionNew.Count];
                }
                if (qt.isJ1939)
                {
                    if (qt.j1939transaction.SA == oldAddr)
                    {
                        qt.j1939transaction.Abort();
                        Support.SendClientDataPacket(UDPReplyType.sendJ1939addresslost, qt);                   
                        if (i < portInfo.QBTransactionNew.Count)
                        {
                            portInfo.QBTransactionNew.RemoveAt(i);
                            i--;
                        }
                        else
                        {
                            portInfo.QBTransactionSent.RemoveAt(i - portInfo.QBTransactionNew.Count);
                            i--;
                        }
                        CheckJ1939Pending(portInfo);
                    }
                }
            }
        }
    }

    public class QBTransaction
    {
        public QBTransaction()
        {
            sendCmdType = RP1210SendCommandType.SC_UNDEFINED;
            timeoutReply = UDPReplyType.sendJ1939replytimeout;
        }
        private Timer replyTimer;

        public int clientId;
        public int dllInPort;
        public RP1210SendCommandType sendCmdType;

        //for j1708 send msg
        public bool isNotify;
        public int msgId;
        //info. for reply
        public byte pktId;
        public int confirmId;

        //for initializing qbridge sequence
        public PacketCmdCodes cmdType;
        public byte[] pktData;

        public byte[] lastSentPkt;

        //time vars
        public int timePeriod;
        public string timeoutReply;
        public int numRetries = 0;

        public string extraData;
        public int extraIdx;
       
        //special for firmware upgrade
        public bool fwUpgrade = false;

        //j1939 info.
        public bool isJ1939 = false;
        public J1939Transaction j1939transaction = new J1939Transaction();

        public void StartTimer()
        {
            try
            {
                if (replyTimer == null)
                {
                    TimerCallback timerDelegate = new TimerCallback(QBSerial.ComReplyTimeOut);
                    replyTimer = new Timer(timerDelegate, this, Timeout.Infinite, Timeout.Infinite);
                }
                replyTimer.Change(timePeriod, Timeout.Infinite);
            }
            catch (Exception) { }//StartTimer()
        }
        public void StopTimer() 
        {
            // Stop timer if it is running
            try
            {
                if (replyTimer != null)
                {
                    replyTimer.Change(Timeout.Infinite, Timeout.Infinite);
                }
            }
            catch (Exception) { }// StopTimer() 
        }
        public void RestartTimer() {
            StopTimer();
            Thread.Sleep(10);
            StartTimer();
        }
    }
}
