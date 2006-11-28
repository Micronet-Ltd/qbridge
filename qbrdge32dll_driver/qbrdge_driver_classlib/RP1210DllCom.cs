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

        public static void MainStart()
        {
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
                    //_DbgTrace("Udp Listen Receive");
                    data = udpListener.Receive(ref iep);
                    //                    Debug.WriteLine("Lock udplisten parseudpp");
                    lock (Support.lockThis)
                    {
                        //                        Debug.WriteLine("Lock2 udplisten parseudpp");
                        ParseUdpPacket(data, iep);
                        //                        Debug.WriteLine("UnLock udplisten parseudpp");

                    }

                    QBSerial.checkCloseComports();

                    //                    Debug.WriteLine("Lock udplisten endprog");
                    lock (Support.lockThis)
                    {
                        //                        Debug.WriteLine("Lock2 udplisten endprog");
                        if (dllPorts.Count == 0)
                        {
                            EndProgram();
                        }

                        QBSerial.CheckSendMsgQ();
                        //                        Debug.WriteLine("UnLock udplisten endprog");
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
            if (sdata != "ack" & sdata != "hello")
            {
                //Debug.WriteLine("parseudp: " + sdata);
            }
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
            else if (cmd == "newclientid")
            {
                // Assign Client ID: port, comNum
                SerialPortInfo sinfo = QBSerial.ComNumToSerialPortInfo(intNum1);
                if (sinfo != null)
                {
                    if (sinfo.requestRawMode)
                    {
                        UdpSend("-1", iep);
                        return;
                    }
                }
                ClientIDManager.AddNewClientID(iep, intNum1);
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
                            if (BitConverter.ToUInt32(tmp, 0) > (UInt32)131071) //0x01FFFF
                            {   //invalid PGN
                                break;
                            }
                            if (jf.priority > 7)
                            {   //invalid priority
                                break;
                            }
                            ClientIDManager.clientIds[clientId].J1939Filter = true;
                            ClientIDManager.clientIds[clientId].J1939FilterList.Add(jf);
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
                    ClientIDManager.ClientIDInfo client = ClientIDManager.clientIds[clientId];

                    if (client.claimAddress >= 0)
                    {
                        QBSerial.AbortClientRTSCTS(client.serialInfo, (byte)client.claimAddress);
                    }

                    client.claimAddress = cmdDataBytes[0];
                    byte[] addrName = new byte[8];
                    for (int i = 0; i < addrName.Length; i++)
                    {
                        addrName[i] = cmdDataBytes[i + 1];
                    }
                    client.claimAddressName = addrName;
                    client.claimAddrDelayTimer();

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
            //Debug.WriteLine("dllhellotimeout");
            IPEndPoint iep = new IPEndPoint(0, 0);
            dllPortInfo dllPort = new dllPortInfo();
            //            Debug.WriteLine("Lock DllHelloTimeOut");
            lock (Support.lockThis)
            {
                //                Debug.WriteLine("Lock2 DllHelloTimeOut");
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
                //                Debug.WriteLine("UnLock DllHelloTimeOut");
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
            //            Debug.WriteLine("Lock DllHelloReplyTimeOut");
            lock (Support.lockThis)
            {
                //                Debug.WriteLine("Lock2 DllHelloReplyTimeOut");
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
                    EndProgram();
                }
                else
                {
                    StartHelloTimer();
                }
                //                Debug.WriteLine("UnLock DllHelloReplyTimeOut");
            }
        }
    }
}