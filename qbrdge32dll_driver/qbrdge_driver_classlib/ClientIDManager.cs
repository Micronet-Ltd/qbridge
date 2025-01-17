using System;
using System.Net;
using System.Text;
using System.IO.Ports;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;

namespace qbrdge_driver_classlib
{
    class ClientIDManager
    {
        public class J1939Filter
        {
            public byte flag = 0;
            public byte[] pgn = { 0, 0, 0 };
            public byte priority = 0;
            public byte sourceAddr = 0;
            public byte destAddr = 0;
            public byte[] qbCtrlPktData1 = null; //see "CAN Control Packet (command code 'L')
            public byte[] qbCtrlPktData2 = null; //[1 byte, extended][4 bytes, ext id][4 bytes mask]
        }

        public class ClientIDInfo
        {
            public int dllInPort = 0; // udp port of dll app.
            public bool available = true;
            public SerialPortInfo serialInfo = null; // serial port assigned to client id

            public bool isJ1939Client = false;

            //MID filter enable/disable
            public bool J1708MIDFilter = true;
            public byte[] J1708MIDList = new byte[0];         
            //J1939 msg filtering
            public bool J1939Filter = true;
            public List<J1939Filter> J1939FilterList = new List<J1939Filter>();

            //used for RP1210 SC_SET_MSG_RECEIVE command
            public bool allowReceive = true;

            //see j1939-81 for info. on address claiming
            public int claimAddress = -1; // -1 for not address claimed
            public int lostClaimAddress = -1; //last last claimed address
            public byte[] claimAddressName = new byte[8];
            public bool claimAddrAvailable = true;
            public QBTransaction claimQBT = null;
            //needed for multi-frame RTS/CTS recieve or send messages

            public void ResetDefaults()
            {
                dllInPort = 0;
                available = true;
                serialInfo = null; // serial port assigned to client id
                isJ1939Client = false;
                J1708MIDFilter = true;
                J1708MIDList = new byte[0];         
                J1939Filter = true;
                J1939FilterList = new List<J1939Filter>();
                allowReceive = true;
                claimAddress = -1;
                lostClaimAddress = -1;
                claimAddressName = new byte[8];
                claimAddrAvailable = true;
                if (claimQBT != null)
                {
                    claimQBT.StopTimer();
                    claimQBT = null;
                }
            }

            //delay availability of claimAddress to allow
            //responses from other devices
            public void claimAddrDelayTimer(QBTransaction qt)
            {
                //claim addr. in progress
                if (claimQBT != null)
                {
                    claimAddress = -1;
                    AbortTimer(UDPReplyType.sendJ1939addresslost);
                }
                claimAddrAvailable = false;
                claimQBT = qt;
                if (myTimer != null)
                {
                    myTimer.Dispose();
                }
                //send address claim fail msg.
                TimerCallback timerDelegate = new TimerCallback(TimeOut);
                myTimer = new Timer(timerDelegate, UDPReplyType.sendJ1939addresslost, 1000, Timeout.Infinite);
            }
            private Timer myTimer;
            //this function is called by the timer class
            private void TimeOut(Object errorStr)
            {
                myTimer.Dispose();
                myTimer = null;
                claimAddrAvailable = true;

                if (claimAddress != -1)
                {
                    //send addr claim ok msg.
                    Support.SendClientDataPacket(UDPReplyType.sendJ1939success, claimQBT);                                       
                }
                else
                {
                    //send addr claim fail
                    Support.SendClientDataPacket((string)errorStr, claimQBT);
                }
                SerialPortInfo sinfo = Support.ClientToSerialPortInfo(claimQBT.clientId);
                for (int i = 0; i < sinfo.QBTransactionSent.Count; i++)
                {
                    if (claimQBT.Equals(sinfo.QBTransactionSent[i]))
                    {
                        sinfo.QBTransactionSent.RemoveAt(i);
                        i--;
                    }
                }
                claimQBT.StopTimer();
                claimQBT = null;
            }

            public void AbortTimer(string errorStr)
            {
                claimAddress = -1;
                TimeOut(errorStr);
            }

        }
        
        public static ClientIDInfo[] clientIds = new ClientIDInfo[128];

        public static void Init()
        {
            //initialize clientIds list
            for (int i = 0; i < clientIds.Length; i++)
            {
                clientIds[i] = new ClientIDInfo();
            }
        }

        //add new client id, return new client id or error code < 0
        public static void AddNewClientID(IPEndPoint iep, int comNum, bool isJ1939Client)
        {
            int clientId = -1;
            for (int i = 0; i < clientIds.Length; i++)
            {
                if (clientIds[i].available)
                {
                    clientIds[i].available = false;
                    clientIds[i].dllInPort = iep.Port;
                    clientIds[i].claimAddress = -1;
                    clientIds[i].lostClaimAddress = -1;
                    clientIds[i].claimAddressName = new byte[8];
                    clientIds[i].J1708MIDFilter = true;
                    clientIds[i].J1708MIDList = new byte[0];
                    clientIds[i].J1939Filter = true;
                    clientIds[i].J1939FilterList.Clear();
                    clientIds[i].allowReceive = true;
                    clientIds[i].isJ1939Client = isJ1939Client;
                    clientId = i;
                    break;
                }
            }
            if (clientId == -1)
            {
                //no clients available return error code
                RP1210DllCom.UdpSend("-1", iep);
                return;
            }

            //creating new client
            string comNumStr = "COM";
            comNumStr = comNumStr.Insert(comNumStr.Length, comNum.ToString());

            SerialPortInfo sinfo = new SerialPortInfo();
            if (QBSerial.GetSerialPortInfo(comNumStr, ref sinfo))
            {
                //Debug.WriteLine("GetSerialPortInfo OK");
                clientIds[clientId].serialInfo = sinfo;

                if (sinfo.qbInitNeeded)
                {
                    QBSerial.ReInitSerialPort(ref clientIds[clientId].serialInfo, clientId);
                }
                else
                {
                    RP1210DllCom.UdpSend(clientId.ToString(), iep);
                }
                return;
            }
            else
            {
                clientIds[clientId].dllInPort = iep.Port;
                // attempt to open serial port, start sequence of msgs to qbridge
                QBSerial.RegisterSerialPort(comNumStr, clientId, iep);
            }
        }

        //make available slots from removed port
        public static void FreeClientIDs(int port)
        {
            for (int i = 0; i < clientIds.Length; i++)
            {
                if (clientIds[i].dllInPort == port)
                {
                    try
                    {
                        RemoveClientID(i, port);
                    }
                    catch (Exception) {/*continue for loop in FreeClientIDs(int port)*/}
                }
            }
        }

        public static void RemoveClientID(int cid)
        {
            int port = clientIds[cid].dllInPort;
            RemoveClientID(cid, port);
        }

        public static void RemoveClientID(int cid, int port)
        {
            if (clientIds[cid].dllInPort == port)
            {
                SerialPortInfo sinfo = clientIds[cid].serialInfo;

                //clean up pending QB messages from QBTransactionNew and QBTransactionSent
                for (int i = 0; i < sinfo.QBTransactionNew.Count; i++)
                {
                    QBTransaction qbt = sinfo.QBTransactionNew[i];
                    if (qbt.clientId == cid)
                    {
                        QBSerial.SendClientErrorPacketAlt(qbt);
                        sinfo.QBTransactionNew.RemoveAt(i);
                        i--;
                        QBSerial.CheckJ1939Pending(sinfo);
                    }
                }
                //clean up pending QB messages from QBTransactionNew and QBTransactionSent
                for (int i = 0; i < sinfo.QBTransactionSent.Count; i++)
                {
                    QBTransaction qbt = sinfo.QBTransactionSent[i];
                    if (qbt.clientId == cid)
                    {
                        QBSerial.SendClientErrorPacketAlt(qbt);
                        sinfo.QBTransactionSent.RemoveAt(i);
                        i--;
                    }
                }
                //clean up J1939 pending
                for (int i = 0; i < sinfo.QBTransJ1939Pending.Count; i++)
                {
                    QBTransaction qbt = sinfo.QBTransJ1939Pending[i];
                    if (qbt.clientId == cid)
                    {
                        QBSerial.SendClientErrorPacketAlt(qbt);
                        sinfo.QBTransactionSent.RemoveAt(i);
                        i--;
                    }
                }

                clientIds[cid].ResetDefaults();

                //check if serial port used by other client ids
                for (int i = 0; i < clientIds.Length; i++)
                {
                    if (clientIds[i].available == false && clientIds[i].serialInfo != null)
                    {
                        SerialPortInfo temp = clientIds[i].serialInfo;
                        if (temp.com.PortName == sinfo.com.PortName)
                        {
                            sinfo = null;
                            break;
                        }
                    }
                }
                //if no more client ids connect to serial port then close conn.
                if (sinfo != null)
                {
                    QBSerial.RemovePort(sinfo);
                }
            }
        }

        public static void ClientUDPSend(string data, int clientId)
        {
            IPEndPoint iep = new IPEndPoint(0, clientIds[clientId].dllInPort+1);
            RP1210DllCom.UdpSend(data, iep);
        }

        public static List<ClientIDInfo> PortToClients(SerialPort com)
        {
            List<ClientIDInfo> clientList = new List<ClientIDInfo>();
            if (com == null)
            {
                return clientList;
            }
            for (int i = 0; i < clientIds.Length; i++)
            {
                if (clientIds[i].available == false) {
                    if (clientIds[i].serialInfo.com != null)
                    {
                        if (clientIds[i].serialInfo.com.PortName == com.PortName)
                        {
                            clientList.Add(clientIds[i]);
                        }
                    }
                }
            }
            return clientList;
        }
    }
}
