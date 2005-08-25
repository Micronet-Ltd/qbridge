#include "common.h"
#include "protocol232.h"
#include "serial.h"

#define CRC_CITT_ONLY
#define INC_CRC_FUNCS
#include "crc.h"

#include <string.h>
#include <alloca.h>

#define STX 2
#define MAX_232PACKET 256
static UINT8 curPacketBuf[MAX_232PACKET];
static int curPacketRecvBytes = 0;
#define CurPacketSize() curPacketBuf[1]

#ifdef _DEBUG
#define SERIAL_PROTOCOL_DEBUG(...) DebugPrint("(Ser Dbg): " __VA_ARGS__)
#else
#define SERIAL_PROTOCOL_DEBUG(fmt, ...)
#endif

/* A packet has the following format:
1 byte:	STX
1 byte:	length (this includes the STX, length & checksum characters)
1 byte:	command
1 byte:	id (used for response and duplicate detection)
n bytes:	Data (length-MinPacketSize bytes of data)
2 bytes:	Little Endian CRC (standard QSI 16 bit CCITT CRC).  CRC is on entire packet up to this point (including the STX)
*/

const int MinPacketSize = 6;
const int CRCSize = 2;
static UINT8 packetID = 0;
static UINT16 crcTable[256] = {0};
static bool crcTableCreated = false;

#define CREATE_TABLE_IF_NECESSARY { if (!crcTableCreated) { \
													createCITTTable(crcTable); \
													crcTableCreated = true; } }


/*******************/
/* ShiftCurPacket */
/*****************/
static inline void ShiftCurPacket(int numChars) {
	memmove(curPacketBuf, curPacketBuf+numChars, curPacketRecvBytes-numChars);
	curPacketRecvBytes -= numChars;
}

/***************************/
/* ProcessReceived232Data */
/*************************/
void ProcessReceived232Data() {
	if (QueueEmpty(&com1.rxQueue)) {
		return;
	}

	// Append data from serial buffer to packet buffer
	int len = DequeueBuf(&com1.rxQueue, curPacketBuf+curPacketRecvBytes, MAX_232PACKET-curPacketRecvBytes);
	curPacketRecvBytes += len;

	while (curPacketRecvBytes > 0) {
		// See if the first byte if the buffer is an STX character
		if (curPacketBuf[0] != STX) {
			// search for an STX character and shift it to the front of the buffer.
			// This should occur rarely so the shift won't be a big deal in terms of CPU usage
			int pos;
			for (pos = 0; pos < curPacketRecvBytes; pos++) {
				if (curPacketBuf[pos] == STX) {
					// shift the STX to the front.  Discard data prior to the STX
					ShiftCurPacket (pos);
					break;
				}
			}

			// No STX found -- reset the buffer and exit
			if (pos >= curPacketRecvBytes) {
				SERIAL_PROTOCOL_DEBUG("Invalid data in serial buffer recieved: No STX character found");
				curPacketRecvBytes = 0;
				return;
			}
		}

		if (CurPacketSize() > curPacketRecvBytes) {
			// In this case the current packet is larger than the data we have received so far -- just quit and wait for more data
			// Ultimately, we should probably have some sort of timeout mechanism, so that if more than 200ms elapse with no new data
			// we flush the current buffer.
			return;
		}

		// At this point we have a potential packet starting with an STX character
		if (CurPacketSize() < MinPacketSize) {
			// Invalid packet -- must be at least 6 bytes to be valid.  Remove initial STX character
			// and loop back to find next STX char
			SERIAL_PROTOCOL_DEBUG("Invalid packet size received: %d", curPacketRecvBytes);
			curPacketBuf[0] = '\0';
			Send232Ack (ACK_INVALID_PACKET, 0, NULL, 0);
			continue;
		}


		UINT16 expectedCRC;
		if (!VerifyCITTCRC(&expectedCRC, curPacketBuf, CurPacketSize()-CRCSize, BufToUINT16(curPacketBuf + CurPacketSize()-CRCSize))) {
			// invalid checksum was received, look for next STX character and resume parsing
			SERIAL_PROTOCOL_DEBUG("Invalid crc on packet. (Received %04X, expected %04X)", BufToUINT16(curPacketBuf + CurPacketSize()-CRCSize), expectedCRC);
			curPacketBuf[0] = '\0';
			Send232Ack (ACK_INVALID_PACKET, 0, NULL, 0);
			continue;
		}
	
		// At this point we have a valid packet or an INCREDIBLY unlucky stream of bad bytes (STX, length & CRC are valid).
		Process232Packet (curPacketBuf[2], curPacketBuf[3], curPacketBuf+MinPacketSize-CRCSize, CurPacketSize()-MinPacketSize);

		// If any contents of the packet remain, shift and repeat
		if (CurPacketSize() == curPacketRecvBytes) {
			// Nothing remained in the buffer -- clear it out and exit the function
			curPacketBuf[0] = '\0';
			curPacketRecvBytes = 0;
			return;
		}
		ShiftCurPacket(CurPacketSize());

	}
}

/******************/
/* VerifyCITTCRC */
/****************/
bool VerifyCITTCRC(UINT16 *calculatedCRC, UINT8 *buf, int leng, UINT16 crc) {
	CREATE_TABLE_IF_NECESSARY;
	*calculatedCRC = calcCITT(crcTable, buf, leng);
	return  *calculatedCRC == crc;
}

/*********************/
/* Process232Packet */
/*******************/
void Process232Packet(UINT8 cmd, UINT8 id, UINT8* data, int dataLen) {
	static int lastPacketID = -1;
	if (id == lastPacketID) {
		Send232Ack (ACK_DUPLICATE_PACKET, id, NULL, 0);
		return;
	}
	lastPacketID = id;

	switch (cmd) {
		case InfoReq:
			{
				extern int allocPoolIdx;
				extern const int MaxAllocPool;
				extern const unsigned char BuildDateStr[];

				DebugPrint ("QBridge firmware version %s.  %s.  Heap in use %d / %d.", VERSION, BuildDateStr, allocPoolIdx, MaxAllocPool);
				Send232Ack(ACK_OK, id, NULL, 0);
			}
			break;
		default:
			SERIAL_PROTOCOL_DEBUG ("Received unknown packet.  Command=%c.  DataLen=%d.  ID=%d", cmd, dataLen, id);
			Send232Ack (ACK_INVALID_COMMAND, id, NULL, 0);
			break;
	}

}

/***************/
/* Send232Ack */
/*************/
void Send232Ack(ACKCodes code, UINT8 id, UINT8* data, UINT32 dataLen) {
	UINT8 * buf = (UINT8 *)alloca(dataLen+1);
	memcpy (buf+1, data, dataLen);
	buf[0] = code;
	TransmitFinal232Packet(ACK, id, buf, dataLen+1);
}

/**********************/
/* Transmit232Packet */
/********************/
void Transmit232Packet (UINT8 command, UINT8 *data, UINT32 dataLen) {
	packetID++;
	if (packetID == 0) {
		packetID++;
	}
	TransmitFinal232Packet (command, packetID, data, dataLen);
}

/***************************/
/* TransmitFinal232Packet */
/*************************/
void TransmitFinal232Packet (UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen) {
	if (dataLen >= MAX_232PACKET - MinPacketSize) {
		return;
	}

	UINT8 *buf = (UINT8 *)alloca(dataLen + MinPacketSize);
	buf[0] = STX;
	buf[1] = dataLen + MinPacketSize;
	buf[2] = command;
	buf[3] = packetID;
	memcpy (buf+4, data, dataLen);

	// CRC here
	CREATE_TABLE_IF_NECESSARY;
	UINT16 crc = calcCITT(crcTable, buf, dataLen+MinPacketSize-CRCSize);
	buf[dataLen+4] = (crc & 0xFF);
	buf[dataLen+5] = (crc & 0xFF00) >> 8;

	Transmit (&com1, buf, buf[1]);
}
