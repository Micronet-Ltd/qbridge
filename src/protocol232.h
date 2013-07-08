#ifndef PROTOCOL232_H
#define PROTOCOL232_H


typedef enum _ACKCodes {
    ACK_OK                      = '0', //0x30
    ACK_DUPLICATE_PACKET        = '1', //0x31
    ACK_INVALID_PACKET          = '2', //0x32
    ACK_INVALID_COMMAND         = '3', //0x33
    ACK_INVALID_DATA            = '4', //0x34
    ACK_UNABLE_TO_PROCESS       = '5', //0x35
    ACK_BUS_FAULT               = '6', //0x36
    ACK_OVERFLOW_OCCURRED       = '7', //0x37
} ACKCodes;

typedef enum _Commands {
    Init232                     = '@', //0x40
    ACK                         = 'A', //0x41
    MIDFilterEnable             = 'B', //0x42
    SetMIDState                 = 'C', //0x43
    SendJ1708Packet             = 'D', //0x44
    ReceiveJ1708Packet          = 'E', //0x45
    EnableTxConfirm             = 'F', //0x46  //for now, one command to enable all
    EnableJ1708TxConfirm        = 'F', //0x46  // |
    EnableJ1939TxConfirm        = 'F', //0x46  // |
    EnableCANTxConfirm          = 'F', //0x46  // |
    TransmitConfirm             = 'G', //0x47  //for now, one message back to host to confirm any
    J1708TransmitConfirm        = 'G', //0x47  // |
    J1939TransmitConfirm        = 'G', //0x47  // |
    CANTransmitConfirm          = 'G', //0x47  // |
    UpgradeFirmware             = 'H', //0x48
    ResetQBridge                = 'I', //0x49
    SendCANPacket               = 'J', //0x4A
    ReceiveCANPacket            = 'K', //0x4B
    CANcontrol                  = 'L', //0x4C
    GetInfo                     = 'M', //0x4D
    CANbusErr                   = 'N', //0x4E
    MiscControl                 = 'O', //0x4f
    AdvRecvMode                 = 'P', //0x50
    MdmReset                    = 'Q', //0x51
    Change232BaudRate           = 'R', //0x52
	InfoReq                     = '*', //0x2A
    RawJ1708                    = '+', //0x2B
    ReqRawPackets               = ',', //0x2C
    ReqEcho                     = '-', //0x2D
    PJDebug                     = 'p', //0x70
} Commands;

void Initialize232Protocol();
void ProcessReceived232Data();
bool VerifyCITTCRC (UINT16 *calculatedCRC, UINT8 *buf, int leng, UINT16 crc);
void Process232Packet(UINT8 cmd, UINT8 id, UINT8* data, int dataLen);
void Send232Ack(ACKCodes code, UINT8 id, UINT8* data, UINT32 dataLen);

extern inline bool QueueTx232Packet (UINT8 command, UINT8 *data, UINT32 dataLen);
bool QueueTxFinal232Packet (UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen);
void TransmitFinal232Packet(UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen);
void Transmit232IfReady();
void RetryLast232();

extern UINT32 getPktIDcounter( void );

extern bool advRecvEnabled;

#endif // PROTOCOL232_H
