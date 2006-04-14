#ifndef PROTOCOL232_H
#define PROTOCOL232_H


typedef enum _ACKCodes {
	ACK_OK						= '0',
	ACK_DUPLICATE_PACKET		= '1',
	ACK_INVALID_PACKET		= '2',
	ACK_INVALID_COMMAND		= '3',
	ACK_INVALID_DATA			= '4',
	ACK_UNABLE_TO_PROCESS	= '5',
} ACKCodes;

typedef enum _Commands {
	Init232						= '@',
	ACK							= 'A',
	InfoReq						= '*',
	RawJ1708						= '+',
	MIDFilterEnable			= 'B',
	SetMIDState					= 'C',
	SendJ1708Packet			= 'D',
	ReceiveJ1708Packet		= 'E',
	EnableJ1708TxConfirm		= 'F',
	J1708TransmitConfirm		= 'G',
	UpgradeFirmware			= 'H',
	ResetQBridge				= 'I',
} Commands;

void Initialize232Protocol();
void ProcessReceived232Data();
bool VerifyCITTCRC (UINT16 *calculatedCRC, UINT8 *buf, int leng, UINT16 crc);
void Process232Packet(UINT8 cmd, UINT8 id, UINT8* data, int dataLen);
void Send232Ack(ACKCodes code, UINT8 id, UINT8* data, UINT32 dataLen);

extern inline void QueueTx232Packet (UINT8 command, UINT8 *data, UINT32 dataLen);
void QueueTxFinal232Packet (UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen);
void TransmitFinal232Packet(UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen);
void Transmit232IfReady();
void RetryLast232();
#endif // PROTOCOL232_H
