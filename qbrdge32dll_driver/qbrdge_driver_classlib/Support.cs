using System;
using System.Collections.Generic;
using System.Text;
using System.IO.Ports;

namespace qbrdge_driver_classlib
{
    class Support
    {
        //keep track of reply from comports, if no reply received handle appropriately
        public const int ackReplyLimit = 10000;
        public const int j1708ConfirmLimit = 2000; // milliseconds
        public const int portLostLimit = 2000; // milliseconds

        public static System.Object lockThis = new Object();

        public static byte[] StringToByteArray(string source)
        {
            byte[] dest = new byte[source.Length];
            for (int i = 0; i < dest.Length; i++)
            {
                dest[i] = (byte)source[i];
            }
            return dest;
        }

        public static string ByteArrayToString(byte[] source)
        {
            char[] tmp = new char[source.Length];
            for (int i = 0; i < tmp.Length; i++)
            {
                tmp[i] = (char)source[i];
            }
            string dest = new String(tmp);
            return dest;
        }
        // returns true if same
        public static bool ArrayCompare(byte[] a, byte[] b)
        {
            if (a == null && b == null)
            {
                return true;
            }
            if (a == null || b == null)
            {
                return false;
            }
            if (a.Length != b.Length)
            {
                return false;
            }
            for (int i = 0; i < a.Length; i++)
            {
                if (a[i] != b[i])
                {
                    return false;
                }
            }
            return true;
        }
        public static Int32 BytesToInt32(byte[] fourByteArr)
        {
            return System.BitConverter.ToInt32(fourByteArr, 0);
        }
        public static UInt64 BytesToUInt64(byte[] eightByteArr)
        {
            return System.BitConverter.ToUInt64(eightByteArr, 0);
        }
        public static byte[] Int32ToBytes(Int32 int32Input, bool bigendien)
        {
            if (bigendien == false)
            {
                return System.BitConverter.GetBytes(int32Input);
            }
            else
            {
                byte[] a = new byte[4];
                byte[] b = System.BitConverter.GetBytes(int32Input);
                int j = 0;
                for (int i = a.Length-1; i >= 0; i--)
                {
                    a[j] = b[i];
                    j++;
                }
                return a;
            }
        }
        public static UInt16 BytesToUInt16(byte[] twoByteArr, bool bigendien)
        {
            if (bigendien == false)
            {
                return System.BitConverter.ToUInt16(twoByteArr, 0);
            }
            else
            {
                byte[] a = new byte[2];
                a[0] = twoByteArr[1];
                a[1] = twoByteArr[0];
                return System.BitConverter.ToUInt16(a, 0);
            }
        }
        public static byte[] UInt16ToBytes(UInt16 input, bool bigendien)
        {
            if (bigendien == false)
            {
                return System.BitConverter.GetBytes(input);
            }
            else
            {
                byte[] a = new byte[2];
                byte[] b = System.BitConverter.GetBytes(input);
                int j = 0;
                for (int i = a.Length - 1; i >= 0; i--)
                {
                    a[j] = b[i];
                    j++;
                }
                return a;
            }
        }

        public static void SendClientDataPacket(string pktType, QBTransaction qbt)
        {
            uint notify;
            if (qbt.isNotify)
            {
                notify = 1;
            }
            else
            {
                notify = 0;
            }

            //<pkt type>, <client id>, <trans id>, <is notify, 0 or 1>, <data len>;<data>
            ClientIDManager.ClientUDPSend(pktType + "," + qbt.clientId.ToString() + "," +
                qbt.msgId.ToString() + "," + notify.ToString(), qbt.clientId);
        }

        public static void SendClientDataPacket(string pktType, int clientId, byte[] pktData)
        {
            //<pkt type>, <client id>, <trans id>, <is notify, 0 or 1>, <data len>;<data>
            ClientIDManager.ClientUDPSend(pktType + "," + clientId.ToString() + "," +
                "0" + "," + "0" + "," + pktData.Length.ToString() + ";" + Support.ByteArrayToString(pktData), clientId);
        }

        public static SerialPort ClientToSerialPort(int clientId)
        {
            return ClientIDManager.clientIds[clientId].serialInfo.com;
        }
        public static SerialPortInfo ClientToSerialPortInfo(int clientId)
        {
            return ClientIDManager.clientIds[clientId].serialInfo;
        }

        //send message to debug port
        private static SerialPort dbgSPort = null;
        public static void _DbgTrace(string outString)
        {            
            /*UdpClient uc = new UdpClient();
            byte[] outb = Support.StringToByteArray(outString);
            IPEndPoint iep = new IPEndPoint(IPAddress.Loopback, UDP_DEBUG_PORT);
            uc.Send(outb, outb.Length, iep);*/            
            /*
            try {
                if (dbgSPort == null)
                {
                    dbgSPort = new SerialPort("COM2");
                    dbgSPort.BaudRate = 115200;
                    dbgSPort.Open();
                }
                dbgSPort.Write(outString);
            }
            catch (Exception) {}
             */             
            return;
        }
    }

    class UDPReplyType
    {
        public const string sendJ1708commerr = "sendJ1708commerr";
        public const string sendJ1708replytimeout = "sendJ1708replytimeout";
        public const string sendJ1708confirmfail = "sendJ1708confirmfail";
        public const string sendJ1708success = "sendJ1708success";
        public const string readmessage = "readmessage";
        public const string sendJ1939commerr = "sendJ1708commerr";
        public const string sendJ1939replytimeout = "sendJ1708replytimeout";
        public const string sendJ1939confirmfail = "sendJ1708confirmfail";
        public const string sendJ1939success = "sendJ1708success";
        public const string sendJ1939addresslost = "sendJ1939addresslost";
        public const string sendJ1939RTSCTStimeout = "sendJ1939RTSCTStimeout";
        public const string sendJ1939invalidpacket = "sendJ1939invalidpacket"; // sent to DLL when send called with invalid packet format
    }

    enum PacketRecvType
    {
        PKT_INCOMPLETE,
        PKT_INVALID,
        PKT_VALID
    }
    enum PacketAckCodes : byte
    {
        PKT_ACK_OK = 0x30,
        PKT_ACK_DUPLICATE_PACKET = 0x31,
        PKT_ACK_INVALID_PACKET = 0x32,
        PKT_ACK_INVALID_COMMAND = 0x33,
        PKT_ACK_INVALID_DATA = 0x34,
        PKT_ACK_UNABLE_TO_PROCESS = 0x35
    }
    public enum PacketCmdCodes : byte
    {
        PKT_CMD_INIT = 0x40,
        PKT_CMD_ACK = 0x41,
        PKT_CMD_MID_FILTER = 0x42,
        PKT_CMD_SET_MID = 0x43,
        PKT_CMD_SEND_J1708 = 0x44,
        PKT_CMD_RECV_J1708 = 0x45,
        PKT_CMD_ENABLE_J1708_CONFIRM = 0x46,
        PKT_CMD_J1708_CAN_CONFIRM = 0x47,
        PKT_CMD_UPGRADE_FIRMWARE = 0x48,
        PKT_CMD_RESET_QBRIDGE = 0x49,
        PKT_CMD_INFO_REQ = 0x2A,
        PKT_CMD_RAW_J1708 = 0x2B,
        PKT_CMD_REQUEST_RAW = 0x2C,
        PKT_CMD_J1708_ECHO = 0x2D,
        PKT_CMD_SEND_CAN = 0x4A,
        PKT_CMD_RECV_CAN = 0x4B,
        PKT_CMD_CAN_CONTROL = 0x4C,
        PKT_CMD_ENABLE_ADV_RCV = 0x50
    }

    enum RP1210ErrorCodes : int
    {
        ERR_ADDRESS_CLAIM_FAILED = 146,
        ERR_ADDRESS_LOST = 153,
        ERR_ADDRESS_NEVER_CLAIMED = 157,
        ERR_INVALID_CLIENT_ID = 129,
        ERR_HARDWARE_NOT_RESPONDING = 142,
        ERR_INVALID_COMMAND = 144,
        ERR_MULTIPLE_CLIENTS_CONNECTED = 156,
        ERR_CHANGE_MODE_FAILED = 150,
        ERR_FW_UPGRADE = 193,
        ERR_INVALID_MSG_PACKET = 199
    }

    public enum RP1210SendCommandType : int
    {
        SC_UNDEFINED = -1,
        SC_RESET_DEVICE = 0,
        SC_SET_ALL_FILTER_PASS = 3,
        SC_SET_MSG_FILTER_J1939 = 4,
        SC_SET_MSG_FILTER_CAN = 5,
        SC_SET_MSG_FILTER_J1708 = 7,
        SC_GENERIC_DRIVER_CMD = 14,
        SC_SET_J1708_MODE = 15,
        SC_SET_ECHO_TRANS_MSGS = 16,
        SC_SET_ALL_FILTER_DISCARD = 17,
        SC_SET_MSG_RECEIVE = 18,
        SC_PROTECT_J1939_ADDRESS = 19,
        SC_UPGRADE_FIRMWARE = 256
    }
}
