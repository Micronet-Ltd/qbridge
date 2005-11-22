#include "common.h"
#include "protocol232.h"
#include "serial.h"
#include "timers.h"
#include "J1708.h"

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
static int lastPacketID = -1;
static UINT8 packetID = 0;
static UINT64 lastDataReceiveTime = 0;
static UINT16 crcTable[256] = {0, 0};
bool dummyBool = false;

static CircleQueue txPackets;
static bool awaiting232Ack;
static UINT64 awaiting232TxTime = 0; 
static int awaiting232FailCount = 0;

#define SERIAL_RECV_TIMEOUT 9600 // 1/2 second
#define SERIAL_TX_TIMEOUT	38400 // 1 second
#define MAX_232_RETRIES 3

/**************************/
/* Initialize232Protocol */
/************************/
void Initialize232Protocol() {
	createCITTTable(crcTable);
	lastPacketID = -1;
	packetID = 0;
	InitializeQueue(&txPackets);
}

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
	if (curPacketRecvBytes > 0) {
		// See if we timed out on data receive.
		if (lastDataReceiveTime + SERIAL_RECV_TIMEOUT < GetTimerTime(&MainTimer)) {
			// timeout
			curPacketRecvBytes = 0;
			SERIAL_PROTOCOL_DEBUG("Serial 232 data in buffer after timeout");
		}
	}
	if (QueueEmpty(&hostPort->rxQueue)) {
		return;
	}
	lastDataReceiveTime = GetTimerTime(&MainTimer);

	// Append data from serial buffer to packet buffer
	int len = DequeueBuf(&hostPort->rxQueue, curPacketBuf+curPacketRecvBytes, MAX_232PACKET-curPacketRecvBytes);
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
	*calculatedCRC = calcCITT(crcTable, buf, leng);
	return  *calculatedCRC == crc;
}

/*********************/
/* Process232Packet */
/*******************/
void Process232Packet(UINT8 cmd, UINT8 id, UINT8* data, int dataLen) {
	if ((id == lastPacketID) && (cmd != ACK)) {
		Send232Ack (ACK_DUPLICATE_PACKET, id, NULL, 0);
		return;
	}
	lastPacketID = id;

	switch (cmd) {
		case Init232:
			lastPacketID = -1;
			packetID = 0;
			J1708ResetDefaultPrefs();
			Send232Ack(ACK_OK, id, NULL, 0);
			break;
		case ACK:
			if (dataLen >= 1) {
				switch (data[0]) {
				case ACK_INVALID_PACKET:
					awaiting232FailCount++;
					SERIAL_PROTOCOL_DEBUG("Received ACK_INVALID_PACKET %d response from host", awaiting232FailCount);
					if (awaiting232FailCount > MAX_232_RETRIES) {
						SERIAL_PROTOCOL_DEBUG("Max etries exceeded.  Giving up");
						awaiting232Ack = false;
					} else {
//DebugPrint ("Calling Retry (ack error)");
						RetryLast232();
					}
					break;
				case ACK_INVALID_DATA:
				case ACK_INVALID_COMMAND:
					SERIAL_PROTOCOL_DEBUG("Received ACK_INVALID_COMMAND or ACK_INVALID_DATA (%d) response from host", data[0]);
					awaiting232Ack = false;
					break;
				case ACK_DUPLICATE_PACKET:
					SERIAL_PROTOCOL_DEBUG("Received duplicate packet notification from host");
					// fall through
				case ACK_OK:
					awaiting232Ack = false;
					break;
				default:
					SERIAL_PROTOCOL_DEBUG("Received unknown ACK code from host");
					awaiting232Ack = false;
					break;
				}
			}
			break;
		case InfoReq:
			{
				extern int allocPoolIdx;
				extern const int MaxAllocPool;
				extern const unsigned char BuildDateStr[];

				if (dataLen != 1) {
					Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
					break;
				}

				if (data[0] == 0) {
					DebugPrint ("QBridge firmware version %s.  %s.  Heap in use %d / %d.", VERSION, BuildDateStr, allocPoolIdx, MaxAllocPool);
					DebugPrint ("  J1708 Idle time=%d, TxPacketID=%d, Rx count=%d.  BusBusy=%d, Collision=%d", GetJ1708IdleTime(), j1708IDCounter, j1708RecvPacketCount, j1708WaitForBusyBusCount, j1708CollisionCount);
#ifdef _DEBUG
				} else if (data[0] == 1) {
					DebugPrint ("J1708 Event Log: ");
					J1708PrintEventLog();
				} else if (data[0] == 2) {
					J1708PrintPIDInfo();
#endif
				} else {
					DebugPrint ("Unknown info request (%d), or only available in debug builds", data[0]);
				}
				Send232Ack(ACK_OK, id, NULL, 0);
			}
			break;
		case RawJ1708:
			{
				int j1708PacketId =J1708AddFormattedTxPacket (J1708_DEFAULT_PRIORITY, data, dataLen);
				char ackBuf[5];
				ACKCodes code = (j1708PacketId == -1) ? ACK_UNABLE_TO_PROCESS : ACK_OK;
				ackBuf[0] = GetFreeJ1708TxBuffers();
				memcpy (ackBuf+1, &j1708PacketId, sizeof(int));
				Send232Ack(code, id, ackBuf, 5);
			}
			break;
		case PIDFilterEnable:
			if ((dataLen != 1) || (data[0] > 1)) {
				Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
				break;
			}
			j1708PIDFilterEnabled = data[0];
			Send232Ack(ACK_OK, id, NULL, 0);
			break;
		case SetPIDState:
			if ((dataLen != 3) || (data[2] > 1)) {
				Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
				break;
			}
			{ 
				UINT16 pid = BufToUINT16(data);
				if (pid > 511) {
					Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
					break;
				}
				J1708SetPIDState(pid, data[2]);
				Send232Ack(ACK_OK, id, NULL, 0);
			}
			break;
		case SendJ1708Packet:
			if ((dataLen < 2) || (dataLen > 21) || (data[0] == 0) || (data[0] > 8)) {
				Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
				break;
			} else {
				int j1708PacketId =J1708AddUnFormattedTxPacket (data[0], data+1, dataLen-1);
				char ackBuf[5];
				ACKCodes code = (j1708PacketId == -1) ? ACK_UNABLE_TO_PROCESS : ACK_OK;
				ackBuf[0] = GetFreeJ1708TxBuffers();
				memcpy (ackBuf+1, &j1708PacketId, sizeof(int));
				Send232Ack(code, id, ackBuf, 5);
			}
			break;
		case EnableJ1708TxConfirm:
			if (dataLen != 1) {
				Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
				break;
			}
			j1708TransmitConfirm = data[0];
			Send232Ack(ACK_OK, id, NULL, 0);
			break;
		case UpgradeFirmware:
			{
				// special case -- we need to transmit a reply, then wait for the serial port to go idle and reboot
				Send232Ack(ACK_OK, id, NULL, 0);
				while (!IsTxFifoEmpty(hostPort)) { // loop until the serial port transmission buffer is empty
				}
				extern void Reset(void);
				Reset();
			}		
		case J1708TransmitConfirm:
		case ReceiveJ1708Packet:
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
//DebugPrint ("Sending Ack to packet %d from host",id);
	TransmitFinal232Packet(ACK, id, buf, dataLen+1);
}

/*********************/
/* QueueTx232Packet */
/*******************/
void QueueTx232Packet (UINT8 command, UINT8 *data, UINT32 dataLen) {
	packetID++;
	if (packetID == 0) {
		packetID++;
	}
	QueueTxFinal232Packet (command, packetID, data, dataLen);
}

/**************************/
/* QueueTxFinal232Packet */
/************************/
void QueueTxFinal232Packet (UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen) {
	if (dataLen >= MAX_232PACKET - MinPacketSize) {
		return;
	}
	AssertPrint (dataLen < 64, "Serial transmission request too long");

	UINT8 len = dataLen;
	Enqueue(&txPackets, &len, 1);
	Enqueue(&txPackets, &command, 1);
	Enqueue(&txPackets, &packetID, 1);
	int addedLen = Enqueue(&txPackets, data, dataLen);
	AssertPrint (addedLen == dataLen, "Error adding data to 232 tx buffer");

	Transmit232IfReady();
}

static UINT8 last232DataLen = 0;
static UINT8 last232Command = 0;
static UINT8 last232PacketID = 0;
static UINT8 last232Data[64];

/***********************/
/* Transmit232IfReady */
/*********************/
void Transmit232IfReady() {
	if (awaiting232Ack) {
		if (GetTimerTime(&MainTimer) > awaiting232TxTime + SERIAL_TX_TIMEOUT) {
			awaiting232FailCount++;
			if (awaiting232FailCount > MAX_232_RETRIES) {
				SERIAL_PROTOCOL_DEBUG("Host device failed to respond to request.  Giving up");
				awaiting232Ack = false;
			} else {
				SERIAL_PROTOCOL_DEBUG("Host device timeout. Retry %d", awaiting232FailCount);
//DebugPrint ("Calling Retry (timeout)");
				RetryLast232();
			}
		}
		return;
	}
	if (QueueEmpty(&txPackets)) {
		return;
	}

	last232DataLen  = DequeueOne(&txPackets);
	last232Command = DequeueOne(&txPackets);
	last232PacketID = DequeueOne(&txPackets);
	int retreivedLen = DequeueBuf(&txPackets, last232Data, last232DataLen);
	AssertPrint (retreivedLen == last232DataLen, "Error retreiving data from buffer");

	RetryLast232();
	awaiting232FailCount = 0;
}

/*****************/
/* RetryLast232 */
/***************/
void RetryLast232() {
//DebugPrint ("RetryLast232 %02X, retrycount=%d", last232Command, awaiting232FailCount);
	TransmitFinal232Packet (last232Command, last232PacketID, last232Data, last232DataLen);
	awaiting232Ack = true;
	awaiting232TxTime = GetTimerTime(&MainTimer);
}

/***************************/
/* TransmitFinal232Packet */
/*************************/
void TransmitFinal232Packet(UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen) {
//DebugPrint ("Transmitting final packet %02X", command);
	UINT8 *buf = (UINT8 *)alloca(dataLen + MinPacketSize);
	buf[0] = STX;
	buf[1] = dataLen + MinPacketSize;
	buf[2] = command;
	buf[3] = packetID;
	memcpy (buf+4, data, dataLen);

	// CRC here
	UINT16 crc = calcCITT(crcTable, buf, dataLen+MinPacketSize-CRCSize);
	buf[dataLen+4] = (crc & 0xFF);
	buf[dataLen+5] = (crc & 0xFF00) >> 8;

	Transmit (hostPort, buf, buf[1]);
}

