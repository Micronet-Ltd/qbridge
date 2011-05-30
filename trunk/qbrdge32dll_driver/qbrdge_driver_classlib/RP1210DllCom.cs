using System;
using System.Net;
using System.IO.Ports;
using System.Threading;
using System.Diagnostics;
using System.Collections;
using System.Net.Sockets;
using System.Collections.Generic;

namespace qbrdge_driver_classlib
{
    public class IconMgrBase
    {
        public IconMgrBase() { i = 5; }
        public virtual void HideIcon()
        {
            i = i + i;
        }
        int i;
    }

    struct dllPortInfo
    {
        public int dllInPort; //port that dll will initiate communication from
        public int dllOutPort; //port that either C++ or C# app. can exchange packets with
        public bool dllOutPortReady;
    }

    //Class will manage communcation to RP1210 DLL
   public class RP1210DllCom
    {
        private static Thread aliveThread; //thread to keep application alive
        private static Thread udpListenThread; //thread that's going to listen for UDP data
        private static UdpClient udpListener;
        public const int udpCorePort = 11234; //core UDP port

        public const int UDP_DEBUG_PORT = 23233;

        private static Timer dllHelloTimer; //timer for sending hello packet periodically to dll
        private static Timer dllHelloReplyTimer; //timer for timing out if no response from dll
        const int dllHelloTimePeriod = 1000; //how often to send hello to dll
        const int dllHelloReplyTimeLimit = 10000; //time limit for response from dll
        //const int dllHelloReplyTimeLimit = 1000; 

        const int maxClients = 128; //maximum number of clients allowed from dll's

        private static List<dllPortInfo> dllPorts = new List<dllPortInfo>();
        private static bool[] dllPortAvailable = new bool[maxClients];

        private int dllHelloPortIdx = 0;

        private static RP1210DllCom mainRP1210Com;

        static IconMgrBase icoMgr;
        public static void MainStart(IconMgrBase mgr)
        {
            icoMgr = mgr;
            mainRP1210Com = new RP1210DllCom();
        }

        public RP1210DllCom()
        {
            //QBSerial.UpgradeFirmware();
            mainRP1210Com = this;

            aliveThread = new Thread(new ThreadStart(KeepProgramAlive));
            aliveThread.Priority = ThreadPriority.BelowNormal;
            aliveThread.Start();
            _DbgTrace("aliveThread started");

            udpListenThread = new Thread(new ThreadStart(UdpListen));
            udpListenThread.Start();
            _DbgTrace("udpListenThread started");

            for (int i = 0; i < dllPortAvailable.Length; i++)
            {
                dllPortAvailable[i] = true;
            }

            ClientIDManager.Init();
            QBSerial.InitQBSerial();
        }

        private static dllPortInfo GetNewDllPortInfo()
        {
            for (int i = 0; i < dllPortAvailable.Length; i++)
            {
                if (dllPortAvailable[i])
                {
                    dllPortAvailable[i] = false;
                    dllPortInfo dllInfo = new dllPortInfo();
                    dllInfo.dllOutPort = ((i + 1) * 2) + RP1210DllCom.udpCorePort;
                    dllInfo.dllInPort = dllInfo.dllOutPort - 1;
                    return dllInfo;
                }
            }
            return new dllPortInfo();
        }

        private void MakeDllPortAvailable(dllPortInfo dllInfo)
        {
            int i = ((dllInfo.dllOutPort - RP1210DllCom.udpCorePort) / 2) - 1;
            dllPortAvailable[i] = true;
        }

        private static void KeepProgramAlive()
        {
            while (true)
            {
                _DbgTrace("AliveThread Loop");
                Thread.CurrentThread.Join();
            }
        }

       public static void EndProgram()
       {
           _DbgTrace("end program");
           Debug.WriteLine("end program");
           udpListener.Close();
           try
           {
               aliveThread.Abort();
           }
           catch (Exception exp)
           {
               Debug.WriteLine(exp.ToString());
           }
           try
           {
               udpListenThread.Abort();
           }
           catch (Exception exp)
           {
               Debug.WriteLine(exp.ToString());
           }

           icoMgr.HideIcon();
       } 
 
        public static void UdpListen()
        {
            // setup udp listener
            udpListener = new UdpClient(udpCorePort);
            udpListener.Client.Blocking = true;
            IPEndPoint iep = new IPEndPoint(IPAddress.Any, 0);

            //periodically send hello to udp connections.
            TimerCallback timerDelegate = new TimerCallback(mainRP1210Com.DllHelloReplyTimeOut);
            dllHelloReplyTimer = new Timer(timerDelegate, iep, Timeout.Infinite, 0);
            timerDelegate = new TimerCallback(mainRP1210Com.DllHelloTimeOut);
            dllHelloTimer = new Timer(timerDelegate, iep, dllHelloReplyTimeLimit, Timeout.Infinite);

            byte[] data;
            while (true)
            {
                try
                {
                    data = udpListener.Receive(ref iep);
                    lock (Support.lockThis)
                    {
                        ParseUdpPacket(data, iep);
                    }

                    QBSerial.checkCloseComports();

                    lock (Support.lockThis)
                    {
                        if (dllPorts.Count == 0)
                        {
                            EndProgram();
                        }

                        QBSerial.CheckSendMsgQ();
                    }
                }
                catch (Exception exp)
                {
                    Debug.WriteLine("udplisten "+exp.ToString());
                }
            }
        }

        public static void ParseUdpPacket(byte[] data, IPEndPoint iep)
        {
            string sdata = Support.ByteArrayToString(data);

            //check if packet from listenPort-1 and send back
            //an assigned port# if it is
            if (iep.Port == (udpCorePort - 1))
            {
                if (sdata == "init")
                {
                    dllPortInfo newDllConn = GetNewDllPortInfo();
                    UdpSend(newDllConn.dllInPort.ToString(), iep);
                    dllPorts.Add(newDllConn);
                }
            }
            else
            {
                if (sdata == "close")
                {
                    // remove from port list
                    for (int i = 0; i < dllPorts.Count; i++)
                    {
                        if (dllPorts[i].dllInPort == iep.Port)
                        {
                            dllPorts.RemoveAt(i);
                            i--;
                        }
                    }
                    ClientIDManager.FreeClientIDs(iep.Port);
                }
                else if (sdata == "procid")
                {
                    // Get Process ID
                    int procid = Process.GetCurrentProcess().Id;
                    UdpSend(procid.ToString(), iep);
                }
                else if (sdata == "ack")
                {
                    dllHelloReplyTimer.Dispose();
                    StartHelloTimer();
                }
                else if (sdata == "hello")
                {
                    UdpSend("ack", iep);
                    for (int i = 0; i < dllPorts.Count; i++)
                    {
                        if (iep.Port == dllPorts[i].dllOutPort)
                        {
                            dllPortInfo dinfo = dllPorts[i];
                            dinfo.dllOutPortReady = true;
                            dllPorts[i] = dinfo;
                        }
                    }
                }
                else
                {
                    // packet in format <client id>,<command>;
                    ParseUdpDataPacket(sdata, iep);
                    return;
                }
            }
        }

        private static void ParseUdpDataPacket(string sdata, IPEndPoint iep)
        {
            // packet in format <number>,<command>;<data> OR
            // <number>|<number>,<command>;<data>
            int commaIdx = sdata.IndexOf(",", 0);
            int idx1 = sdata.IndexOf("|", 0, commaIdx);
            string num = "";
            int intNum1 = 0, intNum2 = 0;
            if (idx1 == -1)
            {
                idx1 = sdata.IndexOf(",", 0);
                num = sdata.Substring(0, idx1);
                intNum1 = Convert.ToInt32(num);
            }
            else
            {
                num = sdata.Substring(0, idx1);
                intNum1 = Convert.ToInt32(num);
                int tmp = idx1;
                idx1 = sdata.IndexOf(",", 0);
                num = sdata.Substring(tmp + 1, idx1 - tmp - 1);
                intNum2 = Convert.ToInt32(num);
            }
            int idx2 = sdata.IndexOf(";", idx1);
            string cmd = sdata.Substring(idx1 + 1, idx2 - idx1 - 1);

            if (cmd == "disconnect")
            {
                Debug.WriteLine("DISCONNECT CLIENT: " + intNum1.ToString());
                ClientIDManager.RemoveClientID(intNum1, iep.Port);
                SerialPortInfo sinfo = Support.ClientToSerialPortInfo(intNum1);
                if (sinfo != null)
                {
                    sinfo.requestRawMode = false;
                }
                UdpSend("ok", iep);
            }
            else if (cmd == "j1708notifymsg")
            {
                // add to msg queue return msg q#, return 0 if full, <0 if error
                string j1708msg = sdata.Substring(idx2 + 1);
                int msgQID = QBSerial.AddSendJ1708Msg(intNum1, j1708msg, true, false);
                UdpSend(msgQID.ToString(), iep);
            }
            else if (cmd == "j1708blockmsg")
            {
                // add to blocking queue
                string j1708msg = sdata.Substring(idx2 + 1);
                int blockID = QBSerial.AddSendJ1708Msg(intNum1, j1708msg, false, true);
                UdpSend(blockID.ToString(), iep);
            }
            else if (cmd == "j1708msg")
            {   
                //send non-blocking non-notify msg
                string j1708msg = sdata.Substring(idx2 + 1);
                QBSerial.AddSendJ1708Msg(intNum1, j1708msg, false, false);
                UdpSend("0", iep);
            }
            else if (cmd == "j1939notifymsg")
            {
                //add to msg queue return msg q#, return 0 if full, <0 if error
                string msg = sdata.Substring(idx2 + 1);
                int msgQID = QBSerial.AddSendJ1939Msg(intNum1, msg, true, false);
                UdpSend(msgQID.ToString(), iep);
            }
            else if (cmd == "j1939blockmsg")
            {
                //add to blocking queue
                string msg = sdata.Substring(idx2 + 1);
                int blockID = QBSerial.AddSendJ1939Msg(intNum1, msg, false, true);
                UdpSend(blockID.ToString(), iep);
            }
            else if (cmd == "j1939msg")
            {
                //send non-notify, non-block msg
                string msg = sdata.Substring(idx2 + 1);
                QBSerial.AddSendJ1939Msg(intNum1, msg, false, false);
                UdpSend("0", iep);
            }
            else if (cmd == "newJ1708clientid" || cmd == "newJ1939clientid")
            {
                // Assign Client ID: port, comNum
                Debug.WriteLine("NEWCLIENT: "+cmd + " com" + intNum1.ToString());
                SerialPortInfo sinfo = QBSerial.ComNumToSerialPortInfo(intNum1);
                if (sinfo != null)
                {
                    if (sinfo.requestRawMode)
                    {
                        UdpSend("-1", iep);
                        return;
                    }
                }                    
                ClientIDManager.AddNewClientID(iep, intNum1, (cmd == "newJ1939clientid"));
            }
            else if (cmd == "queryj1708clients")
            {
                int numClients = 0;
                int clientId = intNum1;
                SerialPortInfo sinfo = Support.ClientToSerialPortInfo(intNum1);
                if (sinfo != null)
                {
                    List<ClientIDManager.ClientIDInfo> clients = ClientIDManager.PortToClients(sinfo.com);
                    numClients = clients.Count;
                }
                UdpSend(numClients.ToString(), iep);
                return;
            }
            else if (cmd == "freeMsgId")
            {
                //_DbgTrace("freeMsgId Recv: " + intNum1.ToString() + "\n");
                //<msgId>,freeMsgId;<blank>
                QBSerial.FreeMsgId(intNum1);
                return;
            }
            else if (cmd == "sendcommand")
            {
                // process RP1210_SendCommand function return result
                int clientId = intNum1;
                int cmdNum = intNum2;
                string cmdData = sdata.Substring(idx2 + 1);
                int returnCode = 0;
                byte[] cmdDataBytes = Support.StringToByteArray(cmdData);
                if (cmdNum == (int)RP1210SendCommandType.SC_RESET_DEVICE)
                {
                    //Reset Device
                    SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
                    List<ClientIDManager.ClientIDInfo> clientIds = ClientIDManager.PortToClients(sinfo.com);
                    if (clientIds.Count <= 0)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_CLIENT_ID;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    else if (clientIds.Count > 1)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_MULTIPLE_CLIENTS_CONNECTED;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }

                    //send Reset Device cmd to qbridge
                    QBTransaction qbt = new QBTransaction();
                    qbt.clientId = clientId;
                    qbt.cmdType = PacketCmdCodes.PKT_CMD_RESET_QBRIDGE;
                    byte[] pktData = new byte[0];
                    qbt.pktData = pktData;
                    qbt.dllInPort = iep.Port;
                    qbt.numRetries = 2;
                    qbt.timePeriod = Support.ackReplyLimit;
                    qbt.sendCmdType = RP1210SendCommandType.SC_RESET_DEVICE;
                    sinfo.QBTransactionNew.Add(qbt);
                    //Debug.WriteLine("RESET DEVICE COMMAND ADDED");
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_ALL_FILTER_PASS)
                {
                    //Set All Filter States to Pass
                    ClientIDManager.clientIds[clientId].J1708MIDFilter = false;
                    ClientIDManager.clientIds[clientId].J1708MIDList = new byte[0];
                    ClientIDManager.clientIds[clientId].J1939Filter = false;
                    ClientIDManager.clientIds[clientId].J1939FilterList.Clear();

                    SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
                                        
                    //set QBridge CAN filters to Off
                    UpdateQBridgeCANFilters(iep, clientId);

                    if (cmdData.Length != 0)
                        UdpSend(((int)RP1210ErrorCodes.ERR_INVALID_COMMAND).ToString(), iep);
                    else
                        UdpSend("0", iep);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_MSG_FILTER_J1939)
                {
                    //Set Message Filtering for J1939
                    returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                    if ((cmdData.Length % 7) != 0 || cmdData.Length == 0)
                    {
                        UdpSend(returnCode.ToString(), iep);
                    }
                    else
                    {
                        byte[] filterData = new byte[7];
                        for (int i = 0; i < (cmdData.Length / 7); i++)
                        {
                            Array.Copy(cmdDataBytes, i * 7, filterData, 0, 7);
                            ClientIDManager.J1939Filter jf = new ClientIDManager.J1939Filter();
                            jf.flag = filterData[0];
                            jf.pgn[0] = filterData[1];
                            jf.pgn[1] = filterData[2];
                            jf.pgn[2] = filterData[3];
                            jf.priority = filterData[4];
                            jf.sourceAddr = filterData[5];
                            jf.destAddr = filterData[6];
                            if (jf.flag > 15)
                            {   //invalid Filter Flag
                                break;
                            }
                            byte[] tmp = new byte[4];
                            Array.Copy(jf.pgn, 0, tmp, 0, 3);
                            tmp[3] = 0;
                            if (BitConverter.ToUInt32(tmp, 0) > (UInt32)131071 && (jf.flag & 0x01) != 0) //0x01FFFF
                            {   //invalid PGN
                                break;
                            }
                            if (jf.priority > 7 && (jf.flag & 0x02) != 0)
                            {   //invalid priority
                                break;
                            }
                            if (jf.flag > 0) {
                                //create data to be sent to the QBridge CAN filter
                                jf.qbCtrlPktData1 = new byte[9]; //[0x01][id CAN ext][mask]
                                jf.qbCtrlPktData1[0] = 0x01;
                                if ((jf.flag & 0x04) != 0) {
                                    //source address
                                    jf.qbCtrlPktData1[1] = jf.sourceAddr;
                                    jf.qbCtrlPktData1[1 + 4] = 0xFF;
                                }
                                if ((jf.flag & 0x02) != 0) {
                                    //priority
                                    jf.qbCtrlPktData1[4] = (byte)(jf.priority * 2 * 2);
                                    jf.qbCtrlPktData1[4 + 4] = 0x07 * 2 * 2;
                                }
                                if ((jf.flag & 0x08) != 0) {
                                    //destination address
                                    jf.qbCtrlPktData2 = new byte[jf.qbCtrlPktData1.Length];
                                    jf.qbCtrlPktData1.CopyTo(jf.qbCtrlPktData2, 0);
                                    jf.qbCtrlPktData2[2] = jf.destAddr;
                                    jf.qbCtrlPktData2[2 + 4] = 0xFF;
                                }
                                if ((jf.flag & 0x08) != 0 && (jf.flag & 0x01) == 0) {
                                    //dest. address, No pgn
                                    jf.qbCtrlPktData1 = null;
                                }
                                if ((jf.flag & 0x01) != 0) {
                                    //pgn
                                    jf.qbCtrlPktData1[2] = jf.pgn[0];
                                    jf.qbCtrlPktData1[3] = jf.pgn[1];
                                    jf.qbCtrlPktData1[4] = (byte)((jf.qbCtrlPktData1[4] & 0xFE) | (jf.pgn[2] & 0x01));
                                    jf.qbCtrlPktData1[2 + 4] = 0xFF;
                                    jf.qbCtrlPktData1[3 + 4] = 0xFF;
                                    jf.qbCtrlPktData1[4 + 4] = (byte)(jf.qbCtrlPktData1[4 + 4] | 0x01);
                                }

                            }
                            ClientIDManager.clientIds[clientId].J1939Filter = true;
                            
                            //check for duplicates
                            bool isFilterDuplicate = false;
                            foreach(ClientIDManager.J1939Filter f in
                                ClientIDManager.clientIds[clientId].J1939FilterList) 
                            {
                                if (f.flag.Equals(jf.flag) &&
                                    f.priority.Equals(jf.priority) &&
                                    f.sourceAddr.Equals(jf.sourceAddr) &&
                                    f.destAddr.Equals(jf.destAddr) &&
                                    f.pgn[0].Equals(jf.pgn[0]) &&
                                    f.pgn[1].Equals(jf.pgn[1]) &&
                                    f.pgn[2].Equals(jf.pgn[2]))
                                {
                                    isFilterDuplicate = true;
                                }
                            }

                            if (isFilterDuplicate == false) {
                                ClientIDManager.clientIds[clientId].J1939FilterList.Add(jf);

                                UpdateQBridgeCANFilters(iep, clientId);
                            }
                            returnCode = 0;
                        }
                        UdpSend(returnCode.ToString(), iep);
                    }
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_MSG_FILTER_CAN)
                {
                    //Set Message Filtering for CAN
                    //NOT IMPLEMENTED YET
                    returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                    UdpSend(returnCode.ToString(), iep);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_MSG_FILTER_J1708)
                {
                    //Set Message Filtering for J1708/J1587
                    ClientIDManager.clientIds[clientId].J1708MIDFilter = true;
                    //add mid codes to midlist
                    string newFilter = cmdData;
                    string oldFilter = Support.ByteArrayToString(
                        ClientIDManager.clientIds[clientId].J1708MIDList);
                    for (int a = 0; a < oldFilter.Length; a++)
                    {
                        bool dup = false;
                        for (int b = 0; b < newFilter.Length; b++)
                        {
                            if (newFilter[b] == oldFilter[a])
                            {
                                dup = true;
                            }
                        }
                        if (dup == false)
                        {
                            newFilter = newFilter.Insert(0, oldFilter.Substring(a, 1));
                        }
                    }
                    ClientIDManager.clientIds[clientId].J1708MIDList = Support.StringToByteArray(newFilter);
                    UdpSend("0", iep);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_GENERIC_DRIVER_CMD)
                {
                    //Generic Drive Command
                    if (cmdDataBytes.Length < 2)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                    }
                    else
                    {
                        SerialPortInfo sinfo = ClientIDManager.clientIds[clientId].serialInfo;
                        try
                        {
                            byte[] pktData = new byte[cmdDataBytes.Length - 2];
                            Array.Copy(cmdDataBytes, 2, pktData, 0, pktData.Length);
                            byte[] outData = QBSerial.MakeQBridgePacket((PacketCmdCodes)cmdDataBytes[0], pktData, ref cmdDataBytes[1]);
                            sinfo.com.Write(outData, 0, outData.Length);
                            returnCode = 0;
                        }
                        catch (Exception exp)
                        {
                            exp.ToString();
                            returnCode = (int)RP1210ErrorCodes.ERR_HARDWARE_NOT_RESPONDING;
                        }
                    }
                    UdpSend(returnCode.ToString(), iep);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_J1708_MODE)
                {
                    SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
                    List<ClientIDManager.ClientIDInfo> clientIds = ClientIDManager.PortToClients(sinfo.com);
                    if (clientIds.Count <= 0)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_CLIENT_ID;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    else if (clientIds.Count > 1)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_CHANGE_MODE_FAILED;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    if (cmdDataBytes.Length < 1)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    if (cmdDataBytes[0] == '0' || cmdDataBytes[0] == 0x00)
                    {
                        sinfo.requestRawMode = false;
                    }
                    else if (cmdDataBytes[0] == '1' || cmdDataBytes[0] == 0x01)
                    {
                        sinfo.requestRawMode = true;
                    }
                    else
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    QBTransaction qbt = new QBTransaction();
                    qbt.clientId = clientId;
                    qbt.cmdType = PacketCmdCodes.PKT_CMD_REQUEST_RAW;
                    byte[] pktData = new byte[1];
                    if (sinfo.requestRawMode)
                    {
                        pktData[0] = 0x01;
                    }
                    else
                    {
                        pktData[0] = 0x00;
                    }
                    qbt.dllInPort = iep.Port;
                    qbt.numRetries = 2;
                    qbt.timePeriod = Support.ackReplyLimit;
                    qbt.sendCmdType = RP1210SendCommandType.SC_SET_J1708_MODE;
                    sinfo.QBTransactionNew.Add(qbt);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_ECHO_TRANS_MSGS)
                {
                    // Set Echo Transmitted Messages, not implemented in QB yet
                    returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                    UdpSend(returnCode.ToString(), iep);
                    return;
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_ALL_FILTER_DISCARD)
                {
                    //Set All Filter States To Discard
                    ClientIDManager.clientIds[clientId].J1708MIDFilter = true;
                    ClientIDManager.clientIds[clientId].J1708MIDList = new byte[0];
                    ClientIDManager.clientIds[clientId].J1939Filter = true;
                    ClientIDManager.clientIds[clientId].J1939FilterList.Clear();

                    // If all J1939 filters are on and all the filter lists are empty, then 
                    //enable QBridge CAN filtering
                    bool allListsEmpty = true;
                    bool allFiltersOn = true;
                    foreach (ClientIDManager.ClientIDInfo cInfo in ClientIDManager.clientIds) {
                        if (cInfo.J1939Filter == false) {
                            allFiltersOn = false;
                            break;
                        }
                        if (cInfo.J1939FilterList.Count > 0) {
                            allListsEmpty = false;
                            break;
                        }
                    }
                    if (allListsEmpty && allFiltersOn) {
                        //set QBridge CAN filters to On
                        UpdateQBridgeCANFilters(iep, clientId);
                    }

                    if (cmdData.Length != 0)
                        UdpSend(((int)RP1210ErrorCodes.ERR_INVALID_COMMAND).ToString(), iep);
                    else
                        UdpSend("0", iep);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_SET_MSG_RECEIVE)
                {
                    //Set Message Recieve
                    if (cmdDataBytes.Length != 1)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    ClientIDManager.ClientIDInfo client = ClientIDManager.clientIds[clientId];
                    returnCode = 0;
                    if (cmdDataBytes[0] == '1' || cmdDataBytes[0] == 0x01)
                    {
                        client.allowReceive = true;
                    }
                    else if (cmdDataBytes[0] == '0' || cmdDataBytes[0] == 0x00)
                    {
                        client.allowReceive = false;
                    }
                    else
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                    }
                    UdpSend(returnCode.ToString(), iep);
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_PROTECT_J1939_ADDRESS)
                {
                    //Protect J1939 Address, not implemented in QB yet
                    if (cmdDataBytes.Length != 10)
                    {
                        returnCode = -(int)RP1210ErrorCodes.ERR_INVALID_COMMAND;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    if (cmdDataBytes[0] == 254) {
                        //null address, invalid
                        returnCode = -(int)RP1210ErrorCodes.ERR_ADDRESS_CLAIM_FAILED;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    ClientIDManager.ClientIDInfo client = ClientIDManager.clientIds[clientId];

                    if (client.claimAddress >= 0)
                    {
                        QBSerial.AbortClientRTSCTS(client.serialInfo, (byte)client.claimAddress);
                    }

                    if (cmdDataBytes[0] == 255)
                    {
                        //global address, clear
                        client.claimAddress = -1;
                        UdpSend("-4", iep);
                        return;
                    }

                    client.claimAddress = cmdDataBytes[0];
                    byte[] addrName = new byte[8];
                    for (int i = 0; i < addrName.Length; i++)
                    {
                        addrName[i] = cmdDataBytes[i + 1];
                    }
                    client.claimAddressName = addrName;

                    //send address claim cmd to qbridge, send return code when confirmed
                    int msgId;
                    bool isNotify = false;

                    if (cmdDataBytes[9] == 1)
                        isNotify = true;

                    if (isNotify)
                    {
                        msgId = QBSerial.GetMsgId();
                        if (msgId <= 0)
                        {
                            UdpSend(msgId.ToString(), iep);
                            return;
                        }
                    }
                    else
                    {
                        msgId = QBSerial.NewMsgBlockId();
                    }

                    //create address claim message
                    QBTransaction qbt = null;
                    AddAddressClaimMsg(cmdDataBytes[0], addrName, clientId, isNotify, msgId, ref qbt);
                    client.claimAddrDelayTimer(qbt);

                    //send message to all clients on com, except sender
                    ClientIDManager.ClientIDInfo sendClientInfo = ClientIDManager.clientIds[clientId];
                    QBSerial.J1939PktRecv(sendClientInfo.serialInfo, qbt.pktData, clientId);

                    UdpSend(msgId.ToString(), iep);
                    return;
                }
                else if (cmdNum == (int)RP1210SendCommandType.SC_UPGRADE_FIRMWARE)
                {
                    //custom cmd, upgrade firmware
                    SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
                    List<ClientIDManager.ClientIDInfo> clientIds = ClientIDManager.PortToClients(sinfo.com);
                    if (clientIds.Count <= 0)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_INVALID_CLIENT_ID;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    else if (clientIds.Count > 1)
                    {
                        returnCode = (int)RP1210ErrorCodes.ERR_MULTIPLE_CLIENTS_CONNECTED;
                        UdpSend(returnCode.ToString(), iep);
                        return;
                    }
                    //send Upgrade FW cmd to qbridge
                    QBTransaction qbt = new QBTransaction();
                    qbt.clientId = clientId;
                    qbt.cmdType = PacketCmdCodes.PKT_CMD_UPGRADE_FIRMWARE;
                    byte[] pktData = new byte[0];
                    qbt.pktData = pktData;
                    qbt.dllInPort = iep.Port;
                    qbt.numRetries = 2;
                    qbt.timePeriod = Support.ackReplyLimit;
                    qbt.sendCmdType = RP1210SendCommandType.SC_UPGRADE_FIRMWARE;
                    qbt.extraData = cmdData;
                    sinfo.QBTransactionNew.Add(qbt);
                }
                else
                {
                    UdpSend(((int)RP1210ErrorCodes.ERR_INVALID_COMMAND).ToString(), iep);
                }
            }
        }

        private static void UpdateQBridgeCANFilters(IPEndPoint iep, int clientId)
        {
            try {

                //count CAN filters for QBridge, if > 25, then disable filters & return
                int qbFilterCount = 0;
                bool clientJ1939FilterOff = false;
                foreach (ClientIDManager.ClientIDInfo cInfo in ClientIDManager.clientIds) {
                    if (cInfo.J1939Filter == false) {
                        clientJ1939FilterOff = true;
                    }
                    else {
                        foreach (ClientIDManager.J1939Filter filter in cInfo.J1939FilterList) {
                            if (filter.qbCtrlPktData1 != null) {
                                qbFilterCount++;
                            }
                            if (filter.qbCtrlPktData2 != null) {
                                qbFilterCount++;
                            }
                        }
                    }
                }

                SerialPortInfo sinfo = ClientIDManager.clientIds[clientId].serialInfo;
                QBTransaction qbt = new QBTransaction();
                byte[] pktData = new byte[2];

                if (qbFilterCount > 25 || clientJ1939FilterOff) {
                    //send CAN filter off
                    qbt.clientId = clientId;
                    pktData[0] = 0x65; //'e'
                    pktData[1] = 0x00;
                    qbt.pktData = pktData;
                    qbt.dllInPort = iep.Port;
                    qbt.cmdType = PacketCmdCodes.PKT_CMD_CAN_CONTROL;
                    qbt.numRetries = 2;
                    qbt.timePeriod = 100;
                    qbt.isNotify = true;
                    qbt.timeoutReply = "";
                    qbt.lastSentPkt = QBSerial.MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);
                    sinfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);
                    return;
                }

                //transmit filters to QBridge

                //send CAN filter on
                qbt.clientId = clientId;
                pktData[0] = 0x65; //'e'
                pktData[1] = 0x01;
                qbt.pktData = pktData;
                qbt.dllInPort = iep.Port;
                qbt.cmdType = PacketCmdCodes.PKT_CMD_CAN_CONTROL;
                qbt.numRetries = 2;
                qbt.timePeriod = 100;
                qbt.isNotify = true;
                qbt.timeoutReply = "";
                qbt.lastSentPkt = QBSerial.MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);                
                sinfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);
                
                //send CAN filter reset to defaults, disables transmit confirm
                qbt = new QBTransaction();
                qbt.clientId = clientId;
                pktData = new byte[2];
                pktData[0] = 0x72; //'r'
                pktData[1] = 0x02; //reset all defaults
                qbt.pktData = pktData;
                qbt.dllInPort = iep.Port;
                qbt.cmdType = PacketCmdCodes.PKT_CMD_CAN_CONTROL;
                qbt.numRetries = 2;
                qbt.timePeriod = 100;
                qbt.isNotify = true;
                qbt.timeoutReply = "";
                qbt.lastSentPkt = QBSerial.MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);                
                sinfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);

                //send CAN filter setup
                qbt = new QBTransaction();
                qbt.clientId = clientId;
                pktData = new byte[300];
                int idx = 0;
                pktData[0] = 0x66; //'f'
                pktData[1] = 0x01; //enable filters
                idx += 2;
                foreach (ClientIDManager.ClientIDInfo cInfo in ClientIDManager.clientIds) {
                    foreach (ClientIDManager.J1939Filter filter in cInfo.J1939FilterList) {
                        if (filter.qbCtrlPktData1 != null) {
                            filter.qbCtrlPktData1.CopyTo(pktData, idx);
                            idx += filter.qbCtrlPktData1.Length;
                        }
                        if (filter.qbCtrlPktData2 != null) {
                            filter.qbCtrlPktData2.CopyTo(pktData, idx);
                            idx += filter.qbCtrlPktData2.Length; 
                        }
                    }
                }
                byte[] tmp = new byte[idx];
                Array.Copy(pktData, tmp, tmp.Length);
                pktData = tmp;
                qbt.pktData = pktData;
                qbt.dllInPort = iep.Port;
                qbt.cmdType = PacketCmdCodes.PKT_CMD_CAN_CONTROL;
                qbt.numRetries = 2;
                qbt.timePeriod = 100;
                qbt.isNotify = true;
                qbt.timeoutReply = "";
                qbt.lastSentPkt = QBSerial.MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);
                sinfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);

                //*very important* Re-Enable Transmit Confirm
                byte[] pData = new byte[1];
                pData[0] = 0x01;
                qbt.pktData = pData;
                qbt.cmdType = PacketCmdCodes.PKT_CMD_ENABLE_J1708_CONFIRM;
                qbt.numRetries = 2;
                qbt.timePeriod = 100;
                qbt.timeoutReply = "";
                qbt.lastSentPkt = QBSerial.MakeQBridgePacket(qbt.cmdType, qbt.pktData, ref qbt.pktId);
                sinfo.com.Write(qbt.lastSentPkt, 0, qbt.lastSentPkt.Length);
            }
            catch (Exception) { }
        }

        public static void AddAddressClaimMsg(byte claimAddr, byte[] addrName, int clientId, bool isNotify,
            int msgId, ref QBTransaction qbt)
        {
            byte[] msg = new byte[6+8];
            //set pgn
            msg[1] = 0xEE;
            //set how priority
            msg[3] = 0x06;
            //source address
            msg[4] = claimAddr;
            //dest address
            msg[5] = 255;
            addrName.CopyTo(msg, 6);
            
            SerialPortInfo sinfo = Support.ClientToSerialPortInfo(clientId);
            qbt = new QBTransaction();
            qbt.clientId = clientId;
            qbt.isNotify = isNotify;
            qbt.msgId = msgId;
            qbt.isJ1939 = true;
            qbt.j1939transaction = new J1939Transaction();
            qbt.j1939transaction.isAddressClaim = true;
            qbt.cmdType = PacketCmdCodes.PKT_CMD_SEND_CAN;

            qbt.j1939transaction.UpdateJ1939Data(Support.ByteArrayToString(msg)); //add message, process
            qbt.pktData = qbt.j1939transaction.GetCANPacket(); //get packet for current state.

            qbt.numRetries = 2;
            qbt.timePeriod = Support.ackReplyLimit;
            qbt.timeoutReply = UDPReplyType.sendJ1708replytimeout;
            QBSerial.ClientIdToSerialInfo(clientId).QBTransactionNew.Add(qbt);     
        }

        //send message to debug port
        public static void _DbgTrace(string outString)
        {
            UdpClient uc = new UdpClient();
            byte[] outb = Support.StringToByteArray(outString);
            IPEndPoint iep = new IPEndPoint(IPAddress.Loopback, UDP_DEBUG_PORT);
            uc.Send(outb, outb.Length, iep);
        }

        public void DllHelloTimeOut(Object stateInfo)
        {
            IPEndPoint iep = new IPEndPoint(0, 0);
            dllPortInfo dllPort = new dllPortInfo();

            lock (Support.lockThis)
            {
                if (dllPorts.Count > 0)
                {
                    dllHelloPortIdx++;
                    if (dllHelloPortIdx >= dllPorts.Count)
                    {
                        dllHelloPortIdx = 0;
                    }
                    dllPort = dllPorts[dllHelloPortIdx];
                    iep = new IPEndPoint(0, dllPort.dllOutPort);
                }
                if (iep.Port > 0 && dllPort.dllOutPortReady)
                {
                    UdpSend("hello", iep); //send hello
                    StartHelloReplyTimer(dllPort);
                    return;
                }
                StartHelloTimer();
            }
        }

        public static void UdpSend(string text, IPEndPoint iep)
        {
            byte[] outData = Support.StringToByteArray(text);
            IPEndPoint outIep = new IPEndPoint(IPAddress.Loopback, iep.Port);
            udpListener.Send(outData, outData.Length, outIep);
            //udpListener.Send(outData, outData.Length, "127.0.0.1", iep.Port);
        }

        public static void UdpSend(string text, int port)
        {
            byte[] outData = Support.StringToByteArray(text);
            IPEndPoint outIep = new IPEndPoint(IPAddress.Loopback, port);
            udpListener.Send(outData, outData.Length, outIep);
            //udpListener.Send(outData, outData.Length, "127.0.0.1", port);
        }

        private static void StartHelloTimer()
        {
            dllHelloTimer.Dispose();
            TimerCallback timerDelegate = new TimerCallback(mainRP1210Com.DllHelloTimeOut);
            dllHelloTimer = new Timer(timerDelegate, 0, dllHelloTimePeriod, Timeout.Infinite);
        }

        private static void StartHelloReplyTimer(dllPortInfo dllPort)
        {
            dllHelloReplyTimer.Dispose();
            TimerCallback timerDelegate = new TimerCallback(mainRP1210Com.DllHelloReplyTimeOut);
            dllHelloReplyTimer = new Timer(timerDelegate, dllPort, dllHelloReplyTimeLimit, Timeout.Infinite);
        }

        private void DllHelloReplyTimeOut(Object obj)
        {
            lock (Support.lockThis)
            {
                dllPortInfo dllPort = (dllPortInfo)obj;

                for (int i = 0; i < dllPorts.Count; i++)
                {
                    dllPortInfo dllPort2 = dllPorts[i];
                    if (dllPort.dllOutPort == dllPort2.dllOutPort)
                    {
                        ClientIDManager.FreeClientIDs(dllPort.dllInPort);
                        dllPorts.RemoveAt(i);
                        i--;
                    }
                }
                if (dllPorts.Count == 0)
                {
                    QBSerial.checkCloseComports();
                    EndProgram();
                }
                else
                {
                    StartHelloTimer();
                }
            }
        }
    }
}