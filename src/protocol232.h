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
	ACK							= 'A',
	InfoReq						= '*',
	RawJ1708						= 'r',
} Commands;

void ProcessReceived232Data();
bool VerifyCITTCRC (UINT16 *calculatedCRC, UINT8 *buf, int leng, UINT16 crc);
void Process232Packet(UINT8 cmd, UINT8 id, UINT8* data, int dataLen);
void Send232Ack(ACKCodes code, UINT8 id, UINT8* data, UINT32 dataLen);

extern inline void Transmit232Packet (UINT8 command, UINT8 *data, UINT32 dataLen);
void TransmitFinal232Packet (UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen);

#endif // PROTOCOL232_H