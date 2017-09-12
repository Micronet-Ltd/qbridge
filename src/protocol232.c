#include "common.h"
#include "protocol232.h"
#include "serial.h"
#include "timers.h"
#include "J1708.h"
#include "CAN.h"
//#include "J1939.h"
#include "stdio.h"

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
1 byte: STX
1 byte: length (this includes the STX, length & checksum characters)
1 byte: command
1 byte: id (used for response and duplicate detection)
n bytes:    Data (length-MinPacketSize bytes of data)
2 bytes:    Little Endian CRC (standard QSI 16 bit CCITT CRC).  CRC is on entire packet up to this point (including the STX)
*/

const int MinPacketSize = 6;
const int CRCSize = 2;
static int lastPacketID = -1;
static UINT8 packetID = 0;
static UINT32 lastDataReceiveTime = 0;
static UINT16 crcTable[256] = {0, 0};
bool dummyBool = false;

static CircleQueue txPackets;
static bool awaiting232Ack;
static UINT32 awaiting232TxTime = 0;
static int awaiting232FailCount = 0;
static int dropped_due_to_slow_host = 0;


static void parseCANcontrol( UINT8 id, UINT8 *data, int dataLen );
static void set_CAN_filters( UINT8 id, char *data, int dataLen );
static void get_CAN_filters( UINT8 id, char *data, int dataLen );
#ifdef _DEBUG
static void dopjdebug( UINT8 id, UINT8 *data, int dataLen);
#endif

bool advRecvEnabled = false;

//GPJ... had to modify these for the new oscillator (for CAN -- 12MHz osc, 24MHz system clock
//GPJ... not sure what to do, match comment or match values.  Times based on values here
//GPJ... were half what the comment says.
//#define SERIAL_RECV_TIMEOUT 9600 // 1/2 second
//#define SERIAL_TX_TIMEOUT   38400 // 1 second
#define SERIAL_RECV_TIMEOUT Three_Quarters_of_a_Second //One_Eighth_of_a_Second
#define SERIAL_TX_TIMEOUT  (2*One_Second) //One_Half_of_a_Second
#define MAX_232_RETRIES 2

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
        if (Get_uS_TimeStamp() - lastDataReceiveTime > SERIAL_RECV_TIMEOUT ) {
            // timeout
            curPacketRecvBytes = 0;
            SERIAL_PROTOCOL_DEBUG("Serial 232 data in buffer after timeout (%d bytes)", curPacketRecvBytes );
        }
    }
    if (QueueEmpty(&hostPort->rxQueue)) {
        return;
    }
    lastDataReceiveTime = Get_uS_TimeStamp();

    // Append data from serial buffer to packet buffer
	int len2do = MAX_232PACKET-curPacketRecvBytes;
	if( len2do > 2 )
		len2do = 2;
    int len = DequeueBuf(&hostPort->rxQueue, curPacketBuf+curPacketRecvBytes, len2do);//MAX_232PACKET-curPacketRecvBytes);
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

        //GPJ What if we have only recieved one byte so far????
        //GPJ the next test would look at invalid data!!!!
        //GPJ so to avoid that, we will look to see if we have enough bytes yet
        if( curPacketRecvBytes < 2 ){
            return;
        }
        if (CurPacketSize() > curPacketRecvBytes) {
		 //GPJ -- what if we have corrupted data in front of a valid packet???  Should we validate the size somehow?
            // In this case the current packet is larger than the data we have received so far -- just quit and wait for more data
            // Ultimately, we should probably have some sort of timeout mechanism, so that if more than 200ms elapse with no new data
            // we flush the current buffer.
			//try to dequeue the rest of the packet
			len2do = CurPacketSize() - curPacketRecvBytes;
			if( len2do > 0 ){
				len = DequeueBuf(&hostPort->rxQueue, curPacketBuf+curPacketRecvBytes, len2do);//MAX_232PACKET-curPacketRecvBytes);
				curPacketRecvBytes += len;
			}
			if (CurPacketSize() > curPacketRecvBytes) { //still don't have complete packet?
				return;
			}
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
		return; //so we don't take forever in this loop, let's only do one packet per loop so we can service the transmit/receive
    }
}

/******************/
/* VerifyCITTCRC */
/****************/
bool VerifyCITTCRC(UINT16 *calculatedCRC, UINT8 *buf, int leng, UINT16 crc) {
    *calculatedCRC = calcCITT(crcTable, buf, leng);
    return  *calculatedCRC == crc;
}

//Ron wants one ID counter for both J1708 and CAN... here is where we
//get the id... it will range from 1 to 0xffffffef
//the values of 0 and 0xfffffff0 to 0xffffffff are reserved for error
//reporting by the routines that return the id as a return value
//(note, -1 and 0 have been used this way since before I started
//maintaining this code base... I just expanded the range and tried to
//make it more consistent.
UINT32 pktIDcounter = 1;

UINT32 getPktIDcounter( void ){
    UINT32 mypktIDcounter = pktIDcounter++;
    //reserve some space for error reporting (16 numbers and the 0 also)
    if( pktIDcounter >= 0xfffffff0 ) pktIDcounter = 1;
    return mypktIDcounter;
}

char *romfind( char *romstart, char *tofind, int romsize){
    int x=strlen(tofind);
    if( romsize < x ) return 0;
    romsize -= x;
    for( ; romsize >= 0; romsize-- ){
        int sz;
        for( sz = 0; sz < x; sz++ ){
            if( romstart[sz] != tofind[sz] ) break;
        }
        if( sz == x ) return romstart;
        romstart++;
    }
    return 0;
}

static int can_overflow_report = 0;
static int j1708_overflow_report = 0;
extern bool can_int_queue_overflow;
extern bool j1708RxQueueOverflowed;

/*********************/
/* Process232Packet */
/*******************/
void Process232Packet(UINT8 cmd, UINT8 id, UINT8* data, int dataLen) {
    if (cmd != ACK) {
        if (id == lastPacketID) {
            Send232Ack (ACK_DUPLICATE_PACKET, id, NULL, 0);
            return;
        }
        lastPacketID = id;
        if( (can_int_queue_overflow && can_overflow_report==0) ) can_overflow_report = 1;
        if( (j1708RxQueueOverflowed && j1708_overflow_report==0) ) j1708_overflow_report = 1;
        if( (can_overflow_report==1) || (j1708_overflow_report==1) ){
            UINT8 mydata = 0;
            mydata |= (can_overflow_report==1) ? 1 : 0;
            mydata |= (j1708_overflow_report==1) ? 2 : 0;
            if( can_overflow_report==1 ) can_overflow_report=2;
            if( j1708_overflow_report==1) j1708_overflow_report=2;
            Send232Ack( ACK_OVERFLOW_OCCURRED,id, &mydata, 1);
        }
    }

    switch (cmd) {
        case Init232:
            lastPacketID = -1;
            packetID = 0;
            J1708ResetDefaultPrefs();
            CANResetDefaultPrefs();
            if (dataLen == 0) {
                Send232Ack(ACK_OK, id, NULL, 0);
            } else {
                Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
            }
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
        case GetInfo:
            {
                extern const unsigned char BuildDateStr[];
                extern char _BootROMvars[];
                char myver[140];
                int mydl = 0;

                if (dataLen != 1) {
                    Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                    break;
                }

                if (data[0] == 0) {
                    mydl = snprintf(myver, 140,"QBridge firmware version %s. %s\n", VERSION, BuildDateStr);
                    char *sptr = romfind(_RomStartAddr, "Bootloader", 8*1024);
                    if( sptr != 0 )
                        mydl += snprintf(&myver[mydl], 140-mydl,"QBridge %s", sptr);
                    sptr = romfind(_BootROMvars, "Built", 8*1024);
                    if( sptr != 0 )
                        mydl += snprintf(&myver[mydl], 140-mydl,"Bootloader %s", sptr);
                }else if( data[0] == 1 ){ //get j1708 bus debug information
                    memset(myver,0,sizeof(myver));
                    #define cpyit(offset,var,vtype) extern vtype var; \
                                                    memcpy(myver+offset, &var, sizeof(vtype))
                    cpyit(  0, j1708RecvPacketCount, int);
                    cpyit(  4, j1708WaitForBusyBusCount, int);
                    cpyit(  8, j1708CollisionCount, int);
                    cpyit( 12, j1708DroppedRxCount, int);
                    cpyit( 16, j1708DroppedTxCnfrmCount, int);
                    cpyit( 20, j1708DroppedFromHostCount, int);
                    cpyit( 24, j1708MIDFilterEnabled, bool);
                    cpyit( 28, j1708TransmitConfirm, bool);
                    cpyit( 32, j1708RxQueueOverflowed, bool);
                    cpyit( 36, j1708BusTransitionDetected, bool);
                    cpyit( 40, j1708BusCurState, bool);
                    j1708BusTransitionDetected = false;
                    int tmp = GetFreeJ1708TxBuffers();
                    memcpy( myver+44, &tmp, sizeof(int));
                    tmp = GetFreeJ1708RxBuffers();
                    memcpy( myver+48, &tmp, sizeof(int));
                    cpyit( 52, j1708_overflow_report, int);
                    mydl = 56;
                }else if( data[0] == 2 ){ //get CAN bus debug information
                    memset(myver,0,sizeof(myver));
                    cpyit( 0,TxCnt, int);
                    cpyit( 4,RxCnt, int);
                    cpyit( 8,StuffErrCnt, int);
                    cpyit(12,FormErrCnt, int);
                    cpyit(16,AckErrCnt, int);
                    cpyit(20,Bit1ErrCnt, int);
                    cpyit(24,Bit0ErrCnt, int);
                    cpyit(28,CRCErrCnt, int);
                    cpyit(32,LostMessageCnt, int);
                    cpyit(36,can_int_queue_overflow, bool);
                    cpyit(40,can_int_queue_overflow_count, int);
                    cpyit(44,CANtransmitConfirm, bool);
                    cpyit(48,CANBusOffNotify, bool);
                    cpyit(52,CANAutoRestart, bool);
                    cpyit(56,CANautoRecoverCount, int);
                    cpyit(60,CANrxWaitForHostCount, int);
                    cpyit(64,CANrxBadValueCount, int);
                    cpyit(68,boff_int_cnt, int);
                    cpyit(72,boff_notify_cnt, int);
                    cpyit(76,boff_want_notify_cnt, int);
                    cpyit(80,CANbusErrReportCount, int);
                    cpyit(84,CANdroppedFromHostCount, int);
                    cpyit(88,CANBusTransitionDetected, bool); //memcpy( myver+44, &CANBusTransitionDetected, sizeof(bool));
                    cpyit(92,CANBusCurState, bool);//memcpy( myver+48, &CANBusCurState, sizeof(bool));
                    CANBusTransitionDetected = false;
                    int tmp = GetFreeCANtxBuffers();
                    memcpy( myver+96, &tmp, sizeof(int));
                    tmp = GetFreeCANrxBuffers();
                    memcpy( myver+100, &tmp, sizeof(int));
                    tmp = getCANBaud();
                    memcpy( myver+104, &tmp, sizeof(int));
                    tmp = getCANTestMode();
                    memcpy( myver+108, &tmp, sizeof(int));
                    tmp = getCANhwErrCnt();
                    memcpy( myver+112, &tmp, sizeof(int));
                    cpyit( 116, can_overflow_report, int);
                    mydl = 120;
                }else if( data[0] == 3 ){ //reset the rx queue overflow errors
                    can_overflow_report=0;
                    j1708_overflow_report=0;
                    can_int_queue_overflow=false;
                    j1708RxQueueOverflowed=false;
                } else {
                    DebugPrint ("Unknown GetInfo request (%d)", data[0]);
                }
                Send232Ack(ACK_OK, id, myver, mydl);
            }
            break;
        case InfoReq:
            {
#ifdef _DEBUG
                extern int allocPoolIdx;
                extern const int MaxAllocPool;
#endif
                extern const unsigned char BuildDateStr[];
                extern char _BootROMvars[];
                char myver[140];
                int mydl = 0;

                if (dataLen != 1) {
                    Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                    break;
                }

                if (data[0] == 0) {
                    mydl = snprintf(myver, 140,"QBridge firmware version %s. %s\n", VERSION, BuildDateStr);
                    char *sptr = romfind(_RomStartAddr, "Bootloader", 8*1024);
                    if( sptr != 0 )
                        mydl += snprintf(&myver[mydl], 140-mydl,"QBridge %s", sptr);
                    sptr = romfind(_BootROMvars, "Built", 8*1024);
                    if( sptr != 0 )
                        mydl += snprintf(&myver[mydl], 140-mydl,"Bootloader %s", sptr);

                    DebugPrint ("QBridge firmware version %s.  %s.  Heap in use %d / %d.", VERSION, BuildDateStr, allocPoolIdx, MaxAllocPool);
                    DebugPrint ("  J1708 Idle time=%d, TxPacketID=%d, Rx count=%d.  BusBusy=%d, Collision=%d", GetJ1708IdleTime(), pktIDcounter, j1708RecvPacketCount, j1708WaitForBusyBusCount, j1708CollisionCount);
#ifdef _DEBUG
                } else if (data[0] == 1) {
                    DebugPrint ("J1708 Event Log: ");
                    J1708PrintEventLog();
                } else if (data[0] == 2) {
                    J1708PrintMIDInfo();
#endif
                } else {
                    DebugPrint ("Unknown info request (%d), or only available in debug builds", data[0]);
                }
                Send232Ack(ACK_OK, id, myver, mydl);
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
        case MIDFilterEnable:
            if ((dataLen != 1) || (data[0] > 1)) {
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                break;
            }
            j1708MIDFilterEnabled = data[0];
            Send232Ack(ACK_OK, id, NULL, 0);
            break;
        case SetMIDState:
            if ((dataLen < 3) || (data[0] > 1)) {
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                break;
            }
            {
                int i;
                UINT32 ackCode = ACK_OK;
                for (i = 1; i < dataLen-1; i+=2) {
                    UINT16 MID = BufToUINT16(data+i);
                    if ((MID > 511) && (MID != 0xFFFF)) {
                        ackCode = ACK_INVALID_DATA;
                        break;
                    }
                    J1708SetMIDState(MID, data[0]);
                }
                Send232Ack(ackCode, id, NULL, 0);
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
            CANtransmitConfirm = data[0];
            //j1939TransmitConfirm = data[0];
            Send232Ack(ACK_OK, id, NULL, 0);
            break;
        case UpgradeFirmware:
            {
                // special case -- we need to transmit a reply, then wait for the serial port to go idle and reboot
                Send232Ack(ACK_OK, id, NULL, 0);
                while (!IsTxFifoEmpty(hostPort)) { // loop until the serial port transmission buffer is empty
                }
                Reset(BOOTFLAG_ENTER_BL);
            }
        case ResetQBridge:
                // special case -- we need to transmit a reply, then wait for the serial port to go idle and reboot
                Send232Ack(ACK_OK, id, NULL, 0);
                while (!IsTxFifoEmpty(hostPort)) { // loop until the serial port transmission buffer is empty
                }
                Reset(0);
            break;
        case SendCANPacket:
            if ( (dataLen < 3) //this option requires at least a type identifier (1) and a CAN identifier (2 for the 11 bit version)
              || (dataLen > 13)//this option can have no more than a type id(1), CAN id (4), and 8 bytes of data
              || (data[0] > 1) //identifier type can only be 0 or 1
              || ((data[0] == 1) && (dataLen < 5)) //extended CAN requires 4 byte identifier
              || ((data[0] == 0) && (dataLen > 11))//standard CAN, id=2, data max=8
              ){
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                break;
            } else {
                UINT32 identifier;
                UINT8 *dptr;
                UINT8 len;
                if( data[0] == 0 ){ //standard CAN identifier
                    identifier = BufToUINT16(&data[1]); //data[1] | (data[2] << 8);
                    if( ((identifier & ~0x000007ff) != 0)   //check for more bits than allowed in an 11 bit identifier
                     || ((identifier &  0x000007f8) == 0x7f8)){ //CAN doc says MS 8 bits can't all be recessive
                        DebugPrint("Bad STANDARD CAN identifier from 232 interface");
                        Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                        return;
                    }
                    dptr = &data[3];
                    len = dataLen - 3;
                }else{ //extended CAN identifier
                    identifier = BufToUINT32( &data[1] ); //data[1] | (data[2] << 8) | (data[3] << 16) | (data[4] << 24);
                    if( ((identifier & ~0x1fffffff) != 0)    //check for more bits than allowed in a 29 bit identifier
                     || ((identifier &  0x1fe00000) == 0x1fe00000)){  //CAN doc says MS 8 bits can't all be recessive
                        DebugPrint("Bad EXTENDED CAN identifier from 232 interface");
                        Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                        return;
                    }
                    dptr = &data[5];
                    len = dataLen - 5;
                }
                int canPacketId =CANaddTxPacket (data[0], identifier, dptr, len);
                char ackBuf[5];
                ACKCodes code = ACK_OK; //assume innocent until proven guilty
                if( canPacketId == -1 ) code = ACK_UNABLE_TO_PROCESS;
                if( canPacketId == -2 ) code = ACK_BUS_FAULT;
                ackBuf[0] = GetFreeCANtxBuffers();
                memcpy (ackBuf+1, &canPacketId, sizeof(int));
                Send232Ack(code, id, ackBuf, 5);
            }
            break;
        case CANcontrol:
            parseCANcontrol( id, data, dataLen );
            break;
        case MiscControl:
            if (dataLen != 1) {
                Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
                break;
            }
            if( data[0] != 0 ){
                Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
                break;
            }
            // special case -- we need to transmit a reply, then wait for the serial port to go idle and reboot
            Send232Ack(ACK_OK, id, NULL, 0);
            while (!IsTxFifoEmpty(hostPort)) { // loop until the serial port transmission buffer is empty
            }
            {
                PCUREGS *p = (PCUREGS *)PCU_REG_BASE;
                int kk;
                for( kk=0; kk<10; kk++ ){ //while( 1 ) {
                    p->pwrcr = 0x8040;
                }
            }
            break;
        case Change232BaudRate:
            {
                UINT32 newbaud;
                if (dataLen != 4) {
                    Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
                    break;
                }
                newbaud = BufToUINT32( &data[0] );
                if( !IsBaudSupported(newbaud) ){
                    Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
                    break;
                }
                // special case -- we need to transmit a reply, then wait for the serial port to go idle and then change the baud rate
                Send232Ack(ACK_OK, id, NULL, 0);
                while (!IsTxFifoEmpty(hostPort)) { // loop until the serial port transmission buffer is empty
                }
                {
                    CmdChangeBaud( newbaud );
                }
            }
            break;
        case MdmReset:
	  {
            if ((dataLen < 1) || ((data[0]==1) && (dataLen < 2)) || ((data[0]==2) && (dataLen < 2)) ) {
                Send232Ack(ACK_INVALID_DATA, id, NULL, 0);
		  //break;
	    }else{
            extern void StartModemReset();
	    extern void SetIgnEmerg( UINT8 val );
	    extern void ClrIgnEmerg( UINT8 val );
	    extern UINT8 GetIgnEmerg( void );
	      switch(data[0]){
	        case 0: //just do the automated wiggling of IGT and EMERG
		    StartModemReset();
                    Send232Ack(ACK_OK, id, NULL, 0);
	            break;
                case 1: //set IGN/EMERG
		    SetIgnEmerg( data[1] );
                    Send232Ack(ACK_OK, id, NULL, 0);
	            break;
                case 2: //clr IGN/EMERG
		    ClrIgnEmerg( data[1] );
                    Send232Ack(ACK_OK, id, NULL, 0);
                    break;
	        case 3: //get IGN/EMERG
		      {
		      UINT8 tmp;
                    tmp = GetIgnEmerg();
		    Send232Ack(ACK_OK, id, &tmp, sizeof(tmp));
		      }
		    break;
                default:
                    Send232Ack(ACK_INVALID_DATA,id, NULL, 0);
                    break;
              }//end switch
	    }//end else
	  }
            break;
#ifdef _DEBUG
        case PJDebug:
            dopjdebug( id, data, dataLen );
            break;
#endif
		case AdvRecvMode:
			advRecvEnabled = (data[0]!=0);
			Send232Ack (ACK_OK, id, (UINT8 *)&advRecvEnabled, sizeof(advRecvEnabled));
			break;
        case J1708TransmitConfirm:
        case ReceiveJ1708Packet:
        case ReceiveCANPacket:
        case CANbusErr:
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
bool QueueTx232Packet (UINT8 command, UINT8 *data, UINT32 dataLen) {
    packetID++;
    if (packetID == 0) {
        packetID++;
    }
    return QueueTxFinal232Packet (command, packetID, data, dataLen);
}

/**************************/
/* QueueTxFinal232Packet */
/************************/
bool QueueTxFinal232Packet (UINT8 command, UINT8 packetID, UINT8 *data, UINT32 dataLen) {
    if (dataLen >= MAX_232PACKET - MinPacketSize) {
        return FALSE;
    }
    AssertPrint (dataLen < 64, "Serial transmission request too long");

    UINT8 len = dataLen;
    if( (len + 3) <= QueueSpaceAvailableCount(&txPackets) ){
        Enqueue(&txPackets, &len, 1);
        Enqueue(&txPackets, &command, 1);
        Enqueue(&txPackets, &packetID, 1);
        int addedLen = Enqueue(&txPackets, data, dataLen);
        AssertPrint (addedLen == dataLen, "Error adding data to 232 tx buffer");
        addedLen++; //remove the compiler warning when _DEBUG not defined
    }else{
        dropped_due_to_slow_host++;
        DebugPrint("Not enough room to queue another packet, dropping cmd %x id %d", command, packetID);
        return FALSE;
    }

    Transmit232IfReady();
    return TRUE;
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
        if ( (Get_uS_TimeStamp() - awaiting232TxTime) > SERIAL_TX_TIMEOUT ) {
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
    if ( (QueueValidBytesCount(&txPackets) < 4)) {
        return;
    }

    last232DataLen  = DequeueOne(&txPackets);
    last232Command = DequeueOne(&txPackets);
    last232PacketID = DequeueOne(&txPackets);
    int retreivedLen = DequeueBuf(&txPackets, last232Data, last232DataLen);
    //GPJ!!! what if len > 64???
    //GPJ!!! what if amount in que is less than data len??
    AssertPrint (retreivedLen == last232DataLen, "Error retreiving data from buffer");
    retreivedLen++; //remove the compiler warning when _DEBUG not defined

    RetryLast232();
    awaiting232FailCount = 0;
}

/*****************/
/* RetryLast232 */
/***************/
void RetryLast232() {
//DebugPrint ("RetryLast232 %02X, retrycount=%d", last232Command, awaiting232FailCount);
    TransmitFinal232Packet (last232Command, last232PacketID, last232Data, last232DataLen);

	/*awaiting232Ack = true;
	*/
	if(((last232Command == ReceiveJ1708Packet)||(last232Command == ReceiveCANPacket))
		&& advRecvEnabled)
	{
		awaiting232Ack = false;
		return;
	}else{
		awaiting232Ack = true;
	}
	awaiting232TxTime = Get_uS_TimeStamp();
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
    //GPJ!! what if transmit fails???
}

//#############################################################################
//#############################################################################
//#############################################################################
//##### Further 232 command protocol handling... broken out to keep
//#####    upper level readable
//#####
//#############################################################################
//#############################################################################
//#############################################################################
static void parseCANcontrol( UINT8 id, UINT8 *data, int dataLen ){
    if( dataLen < 1 ) {
        Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
        return;
    }
    switch( data[0] ) {
        case 'd':   //set the debug mode
            if( dataLen != 2 ) {
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            switch( data[1] ){
                case 'L':   //loopback
                    setCANTestMode(Test_Loop_Back);
                    break;
                case 'S':   //silent
                    setCANTestMode(Test_Silent);
                    break;
                case 'H':   //hot self test
                    setCANTestMode(Test_Hot_SelfTest);
                    break;
                case 'N':   //no debug mode
                    setCANTestMode(Test_No_Test_Mode);
                    break;
                default:
                    Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                    return;
            }
            break;
        case 'b':   //set the baud rate
            if( dataLen != 2 ) {
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            switch( data[1] ) {
                case '0': //default baud
                    setCANBaud( DEFAULT_CAN_BAUD_RATE );
                    break;
                case '1': //1 meg
                    setCANBaud( 1000000l );
                    break;
                case '2': //512k
                    setCANBaud(  500000l );
                    break;
                case '3': //250k
                    setCANBaud(  250000l );
                    break;
                case '4': //125k
                    setCANBaud(  125000l );
                    break;
                default:
                    Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                    return;
            }
            break;
        case 'r':   //restart CAN after bus fault
            if( dataLen != 2 ) {
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            if( data[1] & 0x01) CANRestart();
            if( data[1] & 0x02) CANResetDefaultPrefs();
            if( data[1] & 0x04) InitializeCAN();
            if( data[1] & 0x08) ClearCANTxQueue();
            if( data[1] & 0x10) DisableCANTxIP();
            if( data[1] & 0x20) ClearCANRxQueue();
            //if( data[1] & 0x40) CANAutoRestart = FALSE;
            //if( data[1] & 0x80) CANAutoRestart = TRUE;
            break;
        case 'n':   //enable/disable 'bus-off' notification
            if( dataLen != 2 ){
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            if( data[1] == 0 ) CANBusOffNotify = FALSE; else CANBusOffNotify = TRUE;
            break;
        case 'a':
            if( dataLen != 2 ){
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            if( data[1] == 0 ) CANAutoRestart = FALSE; else CANAutoRestart = TRUE;
            break;
        case 'f':   //set CAN filter
            set_CAN_filters( id, &data[1], dataLen-1 );  //what if filters are disabled?
            return;
        case 'g':   //get CAN filter
            get_CAN_filters(id, &data[1], dataLen-1 );
            return;
        case 'e':   //enable/disable CAN filter
            if( dataLen != 2 ){
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            if( data[1] == 0 ) { //filters disabled... need to enable one filter that enables all messages
                EnableCANReceiveALL();
            }else{ //filters are enabled.... but what if none have been setup, then they will get nothing?
                DisableCANReceiveALL();
            }
            break;
        case 'i':   //get CAN info (status, counters, etc)
            break;
        default:
            Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
            return;
    }
    Send232Ack (ACK_OK, id, NULL, 0);
}

//************************************************************
// Process a list of CAN filters from the RS232 interface
//************************************************************
static void set_CAN_filters( UINT8 id, char *data, int dataLen ){ //what if filters are disabled?
    //validate data received
    if( dataLen < 2 ){  //need 2 byte id and a 2 byte mask
        DebugPrint("Not enough data to interpret CAN filter list");
        Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
        return;
    }
    bool myenable = *data++;
    dataLen--;
    int dl = dataLen;
    char *myd = data;
    //first step is to validate them all before acting on any
    while( dl ){
        if( myd[0] == 0 ) { //standard CAN identifier
            if( dl < 5 ){  //need 2 byte id and a 2 byte mask (plus type specifier)
                DebugPrint("Bad STANDARD CAN filter length");
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            UINT32 identifier = BufToUINT16(&myd[1]);
            UINT32 mask       = BufToUINT16(&myd[3]);
            if( mask == 0xffff ) mask &= 0x7ff; //special value allow all bits
            if( (mask & ~0x000007ff) != 0){  //check for more bits than allowed in an 11 bit identifier
                DebugPrint("Bad STANDARD CAN identifier filter from 232 interface");
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            if( ((identifier & ~0x000007ff) != 0)   //check for more bits than allowed in an 11 bit identifier
             || ((identifier &  0x000007f8) == 0x7f8)   //CAN doc says MS 8 bits can't all be recessive
             || ((identifier & ~mask) != 0) ){//can't really check for bits that are to be masked off
                DebugPrint("Bad STANDARD CAN identifier filter from 232 interface");
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            myd += 5;
            dl -= 5;
        }else { //extended CAN identifier
            if( dl < 9 ){ //need 4 byte id and a 4 byte mask (plus type specifier)
                DebugPrint("Bad Extended CAN filter length");
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            UINT32 identifier = BufToUINT32(&myd[1]);
            UINT32 mask       = BufToUINT32(&myd[5]);
            if( mask == 0xffffffff ) mask &= 0x1fffffff; //special value allow all bits
            if( ((identifier & ~0x1fffffff) != 0)    //check for more bits than allowed in a 29 bit identifier
             || ((identifier &  0x1fe00000) == 0x1fe00000)    //CAN doc says MS 8 bits can't all be recessive
             || ((identifier & ~mask) != 0) ){ //can't really check for bits that are to be masked off
                DebugPrint("Bad EXTENDED CAN identifier filter from 232 interface");
                Send232Ack (ACK_INVALID_DATA, id, NULL, 0);
                return;
            }
            myd += 9;
            dl -= 9;
        }
    }
    //set each filter
    dl = dataLen;
    myd = data;
    while( dl ){
        UINT32 identifier;
        UINT32 mask;
        if( myd[0] == 0 ) { //standard
            identifier = BufToUINT16(&myd[1]) | STANDARD_CAN_FLAG;
            mask       = BufToUINT16(&myd[3]);
            myd += 5;
            dl -= 5;
        }else { //extended
            identifier = BufToUINT32(&myd[1]);
            mask       = BufToUINT32(&myd[5]);
            if( mask == 0xffffffff ) mask &= 0x1fffffff; //special value allow all bits
            myd += 9;
            dl -= 9;
        }
        if( myenable ) { //turning a filter ON
            if( findCANfilter( mask, identifier ) == 0 ){    //if not already enabled, then it is okay to add, if it is already enabled, quietly ignore this request
                if( setCANfilter( mask, identifier ) == 0 ){ //couldn't set.... must be out of space
                    Send232Ack(ACK_UNABLE_TO_PROCESS,id, NULL, 0);
                }
            } //filter was already enabled... just quietly ignore this request?
        }else{ //turning a filter OFF
            int fp;
            fp = findCANfilter( mask, identifier );
            if( fp != 0 ){ //if this filter is on, let's turn it off... if not enabled... just quietly ignore this request
                unsetCANfilter( fp );
            }//filter was not enabled, so we can not turn it off... just quietly ignore this request?
        }
    }
    Send232Ack(ACK_OK, id, NULL, 0);
}

//************************************************************
// Build a list of CAN filters & return it via the RS232 interface
//************************************************************
static void get_CAN_filters( UINT8 id, char *data, int dataLen ){
    int i;
    UINT32 identifier;
    UINT32 mask;
    char myretd[9*NUM_USER_CAN_FILTERS];
    char *list = myretd;
    for( i=1; i<=NUM_USER_CAN_FILTERS; i++ ){
        int t = read_CAN_filter(i, &mask, &identifier);
        if( t ){
            if( identifier & STANDARD_CAN_FLAG ) {
                identifier &= ~STANDARD_CAN_FLAG;
                *list++ = 0;
                *list++ = identifier & 0xff;
                *list++ = (identifier >> 8) & 0xff;
                *list++ = mask & 0xff;
                *list++ = (mask >> 8) & 0xff;
            }else{
                *list++ = 1;
                *list++ = identifier & 0xff;
                *list++ = (identifier >> 8) & 0xff;
                *list++ = (identifier >> 16) & 0xff;
                *list++ = (identifier >> 24) & 0xff;
                *list++ = mask & 0xff;
                *list++ = (mask >> 8) & 0xff;
                *list++ = (mask >> 16) & 0xff;
                *list++ = (mask >> 24) & 0xff;
            }
        }
    }
    Send232Ack( ACK_OK, id, myretd, list-myretd );
}

#ifdef _DEBUG
//#############################################################################
//#############################################################################
//#############################################################################
//##### Stuff for development since I don't have a debugger
//#####    This gives me the ability to read/write anywhere in memory
//#####    and the ability to execut any function
//#############################################################################
//#############################################################################
//#############################################################################
static void dopjreadbytes(char *src, char *dest, int num){
    int x;
    for( x=0; x < num; x++ )
        *dest++ = *src++;
}

static void dopjreadhalfs(UINT16 *src, UINT16 *dest, int num){
    int x;
    for( x=0; x < num; x++ )
        *dest++ = *src++;
}

static void dopjreadwords(UINT32 *src, UINT32 *dest, int num){
    int x;
    for( x=0; x < num; x++ )
        *dest++ = *src++;
}

static void dopjreadstuff(void *src, void *dest, int num, UINT8 type){
    if( type == 4 ) { dopjreadwords((UINT32 *)src, (UINT32 *)dest, num); return; }
    if( type == 2 ) { dopjreadhalfs((UINT16 *)src, (UINT16 *)dest, num); return; }
    if( type == 1 ) { dopjreadbytes((UINT8  *)src, (UINT8  *)dest, num); return; }
}


static void dopjwritebytes(char *where, char *vals, int num){
    int x;
    for( x=0; x < num; x++ )
        *where++ = *vals++;
}

static void dopjwritehalfs(UINT16 *where, UINT8 *vals, int num){
    int x;
    UINT16 tmp;
    for( x=0; x < num; x++ ){
        //because the words we are going to write are contained in
        //the array used to received serial data, and because of the
        //header and stuff, our data is likely not word aligned, so
        //here we have to build the word first before we write it
        tmp = vals[0] | (vals[1]<<8);
        *where++ = tmp;
        vals += 2;
    }
}

static void dopjwritewords(UINT32 *where, UINT8 *vals, int num){
    int x;
    UINT32 tmp;
    for( x=0; x < num; x++ ){
        //because the words we are going to write are contained in
        //the array used to received serial data, and because of the
        //header and stuff, our data is likely not word aligned, so
        //here we have to build the word first before we write it
        tmp = vals[0] | (vals[1]<<8) | (vals[2]<<16) | (vals[3]<<24);
        *where++ = tmp;
        vals += 4;
   }
}

static void dopjwritestuff( void *where, void *vals, int num, UINT8 type ) {
    if( type == 4 ) { dopjwritewords((UINT32 *)where, (UINT8  *)vals, num); return; }
    if( type == 2 ) { dopjwritehalfs((UINT16 *)where, (UINT8  *)vals, num); return; }
    if( type == 1 ) { dopjwritebytes((UINT8  *)where, (UINT8  *)vals, num); return; }
}

UINT32 measure_get_on_bus_time;

static void dopjdebug( UINT8 id, UINT8 *data, int dataLen ){
    UINT8 size;
    UINT8 listtype;
    char *p;
    int x;
    int len;
    void (*fn)(void);
    UINT8 outdata[256];

    if( dataLen == 0 ) { Send232Ack(ACK_OK, id, NULL, 0); return; }
    if( (dataLen % 4) != 0 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
    dataLen -= 4;   //skip the descriptor for everything else
    switch( data[0] ) {
        case 'r':   //read
            if( (dataLen % 4) != 0 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
            size = data[1]; //'w', 'h', 'b', 's' (word, half word, byte, string)
            switch( size ){
                case 'w': size = 4; break;
                case 'h': size = 2; break;
                case 'b': size = 1; break;
                case 's': size = 1; break;
                default:
                    Send232Ack(ACK_INVALID_PACKET, id, NULL, 0);
                    return;
            }
            listtype = data[2]; //0 list of memory locations to read, 1=memory loc & len
            //now loop thru parameters (pointers to what to read)
            switch( listtype ){
                case 0:{ //List of locations to read
                    data += 4;
                    dataLen = dataLen/4;
                    for( x = 0; x < dataLen; x++ ) {
                        p = (char *)(data[4] | (data[5] << 8) | (data[6] << 16)| (data[7] << 24));
                        dopjreadstuff( p, (void *)&outdata[x*size], 1, size);
                    }
                    Send232Ack(ACK_OK,id, outdata, dataLen*size);
                    break;}
                case 1:{ //location and length
                    data += 4;
                    dataLen = dataLen/4;
                    if( dataLen != 2 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
                    p = (char *)(data[0] | (data[1] << 8) | (data[2] << 16)| (data[3] << 24));
                    data += 4;
                    len = data[0] | (data[1] << 8) | (data[2] << 16)| (data[3] << 24);
                    dopjreadstuff( p, (void *)&outdata[0], len, size);
                    Send232Ack(ACK_OK,id, outdata, len*size);
                    break;}
                default:
                    Send232Ack(ACK_INVALID_PACKET, id, NULL, 0);
                    return;
            }
            break;
        case 'w':   //write
            size = data[1]; //'w', 'h', 'b', 's' (word, half word, byte, string)
            switch( size ){
                case 'w': size = 4; break;
                case 'h': size = 2; break;
                case 'b': size = 1; break;
                case 's': size = 1; break;
                default:
                    Send232Ack(ACK_INVALID_PACKET, id, NULL, 0);
                    return;
            }
            listtype = data[2]; //0 list of memory locations to write, 1=memory loc & len
            //now loop thru parameters (pointers to what address & value to write)
            switch( listtype ){
                case 0:{ //?
                    Send232Ack(ACK_INVALID_PACKET,id, outdata, dataLen*size);
                    break;}
                case 1:{ //location and list of values (so we write incrementing addresses)
                    data += 4;
                    if( dataLen < 4 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
                    p = (char *)(data[0] | (data[1] << 8) | (data[2] << 16)| (data[3] << 24));
                    data += 4;
                    dataLen -= 4;
                    if( dataLen < 0 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
                    if( dataLen % size ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
                    dataLen /= size;
                    dopjwritestuff( p, (void *)&data[0], dataLen, size);
                    Send232Ack(ACK_OK,id, NULL, 0);
                    break;}
                default:
                    Send232Ack(ACK_INVALID_PACKET, id, NULL, 0);
                    return;
            }
            break;
        case 'j':   //j1708 test command
            {
            extern void GPIO_Config(IOPortRegisterMap *ioport, UINT16 pin, Gpio_PinModes md );
            if( data[1] == '0' ){
                //J1708_BUS_DISABLE_XMIT_AND_DEASSERT();
                GPIO_Config((IOPortRegisterMap *)IOPORT0_REG_BASE, UART1_Tx_Pin, GPIO_OUT_PP ); \
                GPIO_CLR( 0,11 ); /* UART1_Tx_Pin ....deasserted is high */ \
            }
            if( data[1] == '1' ){
                measure_get_on_bus_time = 0;
                GPIO_SET( 0,11 ); /* UART1_Tx_Pin ....deasserted is high */ \
                //J1708_BUS_ENABLE_XMIT();
                GPIO_Config((IOPortRegisterMap *)IOPORT0_REG_BASE, UART1_Tx_Pin, GPIO_AF_PP );
            }
            if( data[1] == '2' ){
            }
            Send232Ack(ACK_OK,id, (char *)&measure_get_on_bus_time, 4);
            }
            break;
        case 'x':   //execute
            if( dataLen != 4 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; }
            fn = (void (*)(void))(data[4] | (data[5] << 8) | (data[6] << 16)| (data[7] << 24));
            if( (UINT32)fn & 0x3 ) { Send232Ack(ACK_INVALID_PACKET, id, NULL, 0); return; } //can't execute odd addresses (and we don't do thumb)
            (*fn)();
            Send232Ack(ACK_OK,id, NULL, 0);
            break;
        case 'y': //execute (index based list of functions)
        default:
            Send232Ack (ACK_INVALID_COMMAND, id, NULL, 0);
            break;
    }
}

#endif
