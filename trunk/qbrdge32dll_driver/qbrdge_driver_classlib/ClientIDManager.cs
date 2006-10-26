using System;
using System.Net;
using System.Text;
using System.IO.Ports;
using System.Diagnostics;
using System.Collections.Generic;

namespace qbrdge_driver_classlib
{
    class ClientIDManager
    {
        public class ClientIDInfo
        {
            public int dllInPort = 0; // udp port of dll app.
            public bool available = true;
            public SerialPortInfo serialInfo = null; // serial port assigned to client id

            //MID filter enable/disable
            public bool J1708MIDFilter = false;
            public byte[] J1708MIDList = new byte[0];

            public bool allowReceive = true;
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
        public static void AddNewClientID(IPEndPoint iep, int comNum)
        {
            string comNumStr = "COM";
            comNumStr = comNumStr.Insert(comNumStr.Length, comNum.ToString());

            SerialPortInfo sinfo = new SerialPortInfo();
            if (QBSerial.GetSerialPortInfo(comNumStr, ref sinfo))
            {

                for (int i = 0; i < clientIds.Length; i++)
                {
                    if (clientIds[i].available)
                    {                 
                        //serial port and client id assigned immed. send reply
                        Debug.WriteLine("Reg Serial TRUE");
                        clientIds[i].available = false;
                        Debug.WriteLine("cid: iepport: " + iep.Port.ToString());
                        clientIds[i].dllInPort = iep.Port;
                        clientIds[i].serialInfo = sinfo;
                        if (sinfo.qbInitNeeded)
                        {
                            QBSerial.ReInitSerialPort(ref clientIds[i].serialInfo, i);
                        }
                        else
                        {
                            RP1210DllCom.UdpSend(i.ToString(), iep);
                        }
                        return;
                    }
                }
                //no clients available
                RP1210DllCom.UdpSend("-1", iep);
            }

            int clientId = -1;
            for (int i = 0; i < clientIds.Length; i++)
            {
                if (clientIds[i].available)
                {
                    clientIds[i].available = false;
                    clientId = i;
                    break;
                }
            }
            if (clientId == -1)
            {
                RP1210DllCom.UdpSend("-1", iep);
            }

            clientIds[clientId].dllInPort = iep.Port;
            Debug.WriteLine("cid: iepport2: " + iep.Port.ToString());
            // attempt to open serial port, start sequence of msgs to qbridge
            QBSerial.RegisterSerialPort(comNumStr, clientId, iep);
        }

        //make available slots from removed port
        public static void FreeClientIDs(int port)
        {
            for (int i = 0; i < clientIds.Length; i++)
            {
                if (clientIds[i].dllInPort == port)
                {
                    RemoveClientID(i, port);
                }
            }
        }

        public static void RemoveClientID(int cid)
        {
            Debug.WriteLine("RemoveClientID " + cid.ToString());
            int port = clientIds[cid].dllInPort;
            RemoveClientID(cid, port);
        }

        public static void RemoveClientID(int cid, int port)
        {
            Debug.Write("RemoveClientID "+cid.ToString() + " " +port.ToString());
            Debug.WriteLine(cid);
            if (clientIds[cid].dllInPort == port)
            {
                SerialPortInfo sinfo = clientIds[cid].serialInfo;
                clientIds[cid].available = true;
                clientIds[cid].dllInPort = -1;
                clientIds[cid].serialInfo = null;

                // remove j1708 message for that client id

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
