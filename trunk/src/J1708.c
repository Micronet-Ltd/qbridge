#include <string.h>
#include <alloca.h>

#include "common.h"
#include "J1708.h"
#include "serial.h"
#include "timers.h"
#include "stdio.h"
#include "protocol232.h"

J1708Queue j1708Queue;

J1708Message j1708CurTxMessage;
bool j1708PacketReady;
int j1708RecvPacketLen;
char j1708RecvPacket[21];
int j1708RecvPacketCount;
int j1708WaitForBusyBusCount;
int j1708CollisionCount;
enum J1708State j1708State;
int j1708CurCollisionCount;
bool j1708RetransmitNeeded;
UINT32 j1708RetransmitIdleTime;
int  j1708CheckingMIDCharForCollision = -1;

bool j1708MIDFilterEnabled = true;
UINT8 j1708EnabledMIDs[64] = {0};
bool j1708TransmitConfirm = false;

#ifdef _J1708DEBUG
static char msgIdx;
J1708EventLog j1708EventLog[256];
UINT8 j1708EventLogIndex;
#endif

/********************/
/* InitializeJ1708 */
/******************/
void InitializeJ1708() {
    SetPortSettings(j1708Port, 9600, 8, 'N', 1, FALSE); //important not to start baud rate generator until a timer can be started with it at the same time
    j1708Port->port->timeout = 10; // the idle time for a J1708 bus

    j1708Queue.head = 0;
    j1708Queue.tail = 0;
    j1708PacketReady = false;
    j1708RecvPacketCount = true;
    j1708WaitForBusyBusCount = 0;
    j1708CollisionCount = 0;
    j1708RecvPacketLen = 0;
    j1708State = JST_Passive;
    j1708CurCollisionCount = 0;
    j1708RetransmitNeeded = 0;
    j1708RetransmitIdleTime = 0;
    j1708CheckingMIDCharForCollision = -1;

    //j1708MIDFilterEnabled = true;
    //memset (j1708EnabledMIDs, 0, sizeof(j1708EnabledMIDs));

#ifdef _J1708DEBUG
    msgIdx = 0;
    int i;
    j1708EventLogIndex = 0;
    for (i = 0; i < 256; i++) {
        j1708EventLog[i].event = JEV_None;
    }
#endif

    StartJ1708IdleTimer();
}

/************************/
/* GetNextJ1708Message */
/**********************/
J1708Message * GetNextJ1708Message() {
    if (j1708Queue.head == j1708Queue.tail) {
        return NULL;
    }

    return &(j1708Queue.msgs[j1708Queue.tail]);
}

#ifdef _J1708DEBUG
/********************************/
/* J1708DebugPrintRxPacketInfo */
/******************************/
void J1708DebugPrintRxPacketInfo(char *msg) {
    char tbuf[256];
    int idx = 0;
    int i = 0;
    UINT8 sum = 0;
    msgIdx++;
    J1708LogEvent (JEV_Msg, msgIdx);
    snprintf (tbuf, 256, "%02d Rx J1708 packet. (%s) %d bytes.  State=%d.  Packet=", msgIdx, msg, j1708RecvPacketLen, j1708State);
    idx = strlen(tbuf);
    for (i = 0; i < j1708RecvPacketLen; i++) {
        sum += j1708RecvPacket[i];
        snprintf (tbuf+idx, 4, "%02X.", j1708RecvPacket[i]);
        idx += 3;
    }
    snprintf (tbuf+idx, 256-idx, " Checksum=%d", (int)sum);
    idx = strlen(tbuf);
    J1708DebugPrint (tbuf);
}
#else
#define J1708DebugPrintRxPacketInfo(msg)
#endif

/***************************/
/* ProcessJ1708RecvPacket */
/*************************/
void ProcessJ1708RecvPacket() {
    if (!j1708PacketReady) {
        return;
    }

    J1708LogEvent(JEV_RecvFromBusProc, j1708RecvPacketLen);
    j1708PacketReady = false;

    if (j1708State == JST_Transmitting) {
        if (memcmp(j1708RecvPacket, j1708CurTxMessage.data, j1708RecvPacketLen) != 0) {
            J1708DebugPrint ("Full Compare Collision");
            J1708EnterCollisionState(JCR_FullComp);
            J1708DebugPrintRxPacketInfo("col");
            j1708RetransmitNeeded = true;
        } else {
            if (j1708TransmitConfirm) {
                UINT8 buf[5];
                buf[0] = 1;
                memcpy(buf+1, &j1708CurTxMessage.id, 4);
                QueueTx232Packet(J1708TransmitConfirm, buf, 5);
            }
            J1708DebugPrintRxPacketInfo("tx ");
            j1708CurCollisionCount = 0;
            j1708RetransmitNeeded = false;
        }
        j1708State = JST_Passive;
    } else {
        UINT8 sum = 0;
        int i;
        for (i = 0; i < j1708RecvPacketLen; i++) {
            sum += j1708RecvPacket[i];
        }
        if (sum != 0) {
            J1708LogEvent(JEV_ChecksumErr, 0);
            J1708DebugPrint ("Invalid checksum received on inbound packet");
        } else {
            AssertPrint (j1708State != JST_IgnoreRxData, "Error -- received packet while in collision state");
            j1708RecvPacketCount++;
            J1708DebugPrintRxPacketInfo("rx ");
            if (MIDPassesFilter()) {
                QueueTx232Packet (ReceiveJ1708Packet, j1708RecvPacket, j1708RecvPacketLen - 1); // Omit checksum byte
            }
        }
    }

}

/********************/
/* MIDPassesFilter */
/******************/
bool MIDPassesFilter () {
    if (!j1708MIDFilterEnabled) {
        return true;
    }

    int MID = j1708RecvPacket[0];
    if (MID == 255) {
        MID = 256 + j1708RecvPacket[1];
    }

    return (j1708EnabledMIDs[MID/8] & BIT(MID%8)) != 0;
}

extern void GPIO_Config(IOPortRegisterMap *ioport, UINT16 pin, Gpio_PinModes md );

#define J1708_BUS_DISABLE_XMIT_AND_DEASSERT() do{ \
            GPIO_Config((IOPortRegisterMap *)IOPORT0_REG_BASE, UART1_Tx_Pin, GPIO_OUT_PP ); \
            GPIO_SET( 0,11 ); /* UART1_Tx_Pin ....deasserted is high */ \
            }while(0)
#define J1708_BUS_ENABLE_XMIT() GPIO_Config((IOPortRegisterMap *)IOPORT0_REG_BASE, UART1_Tx_Pin, GPIO_AF_PP )


/******************************/
/* ProcessJ1708TransmitQueue */
/****************************/
void ProcessJ1708TransmitQueue() {
    extern bool StartJ1708Transmit( char data_to_send );
    // Is there anything to transmit?
    if ((j1708Queue.head == j1708Queue.tail) && !j1708RetransmitNeeded) {
        return;
    }

    // Is the transmission queue empty?
    if (!QueueEmpty(&j1708Port->txQueue)) {
        return;
    }

    // Is transmission fifo empty? (can only send J1708 when we have completed all other packets)
    if (!IsTxFifoEmpty(j1708Port)) {
        return;
    }

    J1708_BUS_ENABLE_XMIT();

    if (j1708RetransmitNeeded) {
        UINT32 idleTime = GetJ1708IdleTime();
        if (idleTime < j1708RetransmitIdleTime) {
            return;
        }
        if( !StartJ1708Transmit( j1708CurTxMessage.data[0] ) ){
            return;
        }
        j1708State = JST_Transmitting;
        j1708CheckingMIDCharForCollision = 0;
        j1708Port->port->intEnable |= RxBufNotEmptyIE;
        Transmit (j1708Port, &j1708CurTxMessage.data[1], j1708CurTxMessage.len-1);
        j1708RetransmitNeeded = false;
        IRQ_ENABLE();
        J1708LogEventIdle(JEV_Retry, j1708CurCollisionCount, idleTime);
        J1708DebugPrint ("Retransmit");
    } else {
        J1708Message *msg = GetNextJ1708Message();
        if (msg == NULL) {
            DebugPrint ("Unexpected null message retreived");
            return;
        }

        // Not trying to transmit anything, now let us see if we have been idle long enough
        UINT32 busAccessTime = ConvertJ1708IdleCountToTimerTicks(J1708_IDLE_TIME + 2 * msg->priority);

        j1708CurCollisionCount = 0;
        memcpy(&j1708CurTxMessage, msg, sizeof(j1708CurTxMessage));

        UINT32 idleTime = GetJ1708IdleTime();
        if (busAccessTime > idleTime) {
            j1708WaitForBusyBusCount++;
            return; // bus has not been idle long enough
        }
        if( !StartJ1708Transmit( msg->data[0] ) ){
            return;
        }
        j1708State= JST_Transmitting;
        j1708CheckingMIDCharForCollision = 0;
        j1708Port->port->intEnable |= RxBufNotEmptyIE;
        Transmit (j1708Port, &msg->data[1], msg->len-1);
        IRQ_ENABLE();
        J1708LogEventIdle(JEV_Transmit, 0, idleTime);
        j1708Queue.tail = (j1708Queue.tail + 1) % J1708_QUEUE_SIZE;
    }
}

/******************************/
/* J1708AddFormattedTxPacket */
/****************************/
int J1708AddFormattedTxPacket (UINT8 priority, UINT8 *data, UINT8 len) {
    J1708LogEvent(JEV_RecvFromHost, len);
    if (len > 21) {
        return -1;
    }

    int nextHead = (j1708Queue.head + 1) % J1708_QUEUE_SIZE;
    if (nextHead == j1708Queue.tail) {
        // this means that the queue was full
        return -1;
    }

    int curHead = j1708Queue.head;
    j1708Queue.msgs[curHead].priority = priority;
    j1708Queue.msgs[curHead].len = len;
    memcpy(j1708Queue.msgs[curHead].data, data, len);
    j1708Queue.msgs[curHead].id = getPktIDcounter();
    j1708Queue.head = nextHead;
    return j1708Queue.msgs[curHead].id;
}

/********************************/
/* J1708AddUnFormattedTxPacket */
/******************************/
int J1708AddUnFormattedTxPacket(UINT8 priority, UINT8 *data, UINT8 len) {
    UINT8 *buf = (UINT8*)alloca(len+1);
    int i;
    UINT8 sum = 0;
    for (i = 0; i < len; i++) {
        sum += data[i];
        buf[i] = data[i];
    }

    buf[len] = 256-sum;
    return J1708AddFormattedTxPacket(priority, buf, len+1);
}


/**************************/
/* GetFreeJ1708TxBuffers */
/************************/
int GetFreeJ1708TxBuffers() {
    int numInUse;
    if (j1708Queue.head > j1708Queue.tail) {
        numInUse = j1708Queue.head - j1708Queue.tail;
    } else {
        numInUse = (j1708Queue.head + J1708_QUEUE_SIZE) - j1708Queue.tail;
    }
    return (J1708_QUEUE_SIZE - 1) - numInUse;
}

/**********************/
/* J1708ComIRQHandle */
/********************/
// There is an implicit assumption here that a framing error == a bus collision
void J1708ComIRQHandle() {
//#ifdef _J1708DEBUG
//    if (j1708Port->port->status & ~(TxFull | TxHalfEmpty | TxEmpty)) {
//        J1708LogEvent(JEV_SerIRQ, j1708Port->port->status);
//    }
//    AssertPrint (!j1708PacketReady || !(j1708Port->port->status & RxBufNotEmtpy), "Warning -- J1708 data received before previous packet was processed");
//#endif

    // If we received data, we want to get an idle timeout interrupt (this is so we can measure the 10 baud ticks)
    // deliniating a packet boundary, in case this is the final byte of the packet
    if (j1708Port->port->status & RxBufNotEmtpy) {
        j1708Port->port->intEnable |= TimeoutIdleIE;
    }

    // If a packet was ready, we need to pull it out of the RX buffer in the PostHandle, and also turn off the
    // interrupt on idle timeout
    if (j1708Port->port->status & (TimeoutIdle | TimeoutNotEmtpy)) {
        j1708PacketReady = true;
        j1708Port->port->intEnable &= ~TimeoutIdleIE;
    }

    // If we are transmitting, we want to interrupt on the first couple of characters only and check it for a collision
    while ((j1708CheckingMIDCharForCollision >= 0) && (j1708Port->port->status & RxBufNotEmtpy) && !(j1708Port->port->status & FrameError)) {
        UINT8 rxChar;
        // we were double checking the first character of a transmission for a collision
        rxChar = j1708Port->port->rxBuffer;
        Enqueue(&j1708Port->rxQueue, &rxChar, 1);
        if (rxChar != j1708CurTxMessage.data[j1708CheckingMIDCharForCollision]) {
//GPIO_SET(1,6);
            //as per the J1708 spec, get off the bus at end of this char or sooner, then take this as the mid
            J1708_BUS_DISABLE_XMIT_AND_DEASSERT();
            j1708Port->port->txReset = 0;
            j1708Port->port->intEnable &= ~TxHalfEmptyIE;

            J1708DebugPrint ("Collision (mismatch at byte %d -- was %02X, expecting %02X)", j1708CheckingMIDCharForCollision, rxChar, j1708CurTxMessage.data[j1708CheckingMIDCharForCollision]);
            J1708EnterCollisionState(JCR_FirstByteMismatch);
            ClearQueue(&j1708Port->rxQueue);
            j1708Port->port->rxReset = 0; // Clear out the RxFifo
            ClearQueue (&j1708Port->txQueue);
            j1708Port->port->txReset = 0;
            j1708Port->port->intEnable &= ~TxHalfEmptyIE;
            break;
        }
        j1708CheckingMIDCharForCollision++;
        if (j1708CheckingMIDCharForCollision > 1) {
            j1708CheckingMIDCharForCollision = -1;
            j1708Port->port->intEnable &= ~RxBufNotEmptyIE; // first of all turn this interrupt off, we don't need it anymore
        }
    }

    // if we got a framing error, then treat as a collision, otherwise normal processing
    if (j1708Port->port->status & FrameError) {
        if (j1708State == JST_Passive) {
            // 2 other devices collided -- nothing for me to worry about
        } else if (j1708State == JST_Transmitting) {
            // A packet I was transmitting was involved in a collision
//GPIO_SET(1,6);
            //it's not clear to me what we should do in this case, but let's get off the bus asap
            J1708_BUS_DISABLE_XMIT_AND_DEASSERT();
            j1708Port->port->txReset = 0;
            j1708Port->port->intEnable &= ~TxHalfEmptyIE;
            J1708DebugPrint ("Framing Collision");
            J1708EnterCollisionState(JCR_Framing);
        } else {
            // already in a collision state -- we really shouldn't be transmitting anything at this point, but just in case...
            ClearQueue (&j1708Port->txQueue);
            j1708Port->port->txReset = 0;
            j1708Port->port->intEnable &= ~TxHalfEmptyIE;
        }
        ClearQueue(&j1708Port->rxQueue);
        j1708Port->port->rxReset = 0; // Clear out the RxFifo

    } else if (j1708Port->port->status != 0) {
        HandleComIRQ(j1708Port);
    }

    if (j1708PacketReady) {
        if (QueueEmpty(&j1708Port->rxQueue) || (j1708State == JST_IgnoreRxData)) {
            j1708PacketReady = false;
            j1708State = JST_Passive;
            ClearQueue(&j1708Port->rxQueue);
        } else {
            J1708LogEvent(JEV_RecvFromBus, 0);
            j1708RecvPacketLen = DequeueBuf(&j1708Port->rxQueue, j1708RecvPacket, 21);
        }

        AssertPrint (QueueEmpty(&j1708Port->rxQueue), "Warning -- J1708 recv buffer had extra bytes");
    }
//GPIO_CLR(1,6);
}


#ifdef _J1708DEBUG
/**********************/
/* J1708LogEventIdle */
/********************/
void J1708LogEventIdle (UINT8 event, UINT16 flags, UINT32 idleTime) {
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    j1708EventLog[j1708EventLogIndex].event = event;
    j1708EventLog[j1708EventLogIndex].flags = flags;
    j1708EventLog[j1708EventLogIndex].time = GetMainTimeInBaudTicks();
    j1708EventLog[j1708EventLogIndex].state = j1708State;
    j1708EventLog[j1708EventLogIndex].idle = idleTime;
    j1708EventLogIndex++;
    RESTORE_IRQ(saveState);
}

/***********************/
/* J1708PrintEventLog */
/*********************/
void J1708PrintEventLog() {
    char buf[256];
    char *msgs[] = { "full compare", "framing error", "mismatch header" };
    char *states[] = { "Pasv", "Tx  ", "Ignr" };
    UINT8 i = j1708EventLogIndex;
    int baseTime, lastTime=0;
    bool first = true;

    Transmit (debugPort, "Rel Time   Idle State Message\r\n", 31);
    do {
        if (j1708EventLog[i].event != JEV_None) {
            if (first) {
                baseTime = j1708EventLog[i].time;
                lastTime = j1708EventLog[i].time;
                first = false;
            }
            snprintf (buf, 256, "%7d %7d %s  ", /*j1708EventLog[i].time-baseTime,*/ j1708EventLog[i].time-lastTime, j1708EventLog[i].idle > 0x7FFFFFFF ? 0 : j1708EventLog[i].idle
                        , states[j1708EventLog[i].state]);
            Transmit (debugPort, buf, strlen(buf));
            lastTime = j1708EventLog[i].time;
            switch (j1708EventLog[i].event) {
            case JEV_RecvFromHost:
                snprintf (buf, 256, "Host requested transmission of %d byte packet\r\n", j1708EventLog[i].flags);
                break;
            case JEV_Transmit:
                snprintf (buf, 256, "Adding transmit packet to serial queue\r\n", (short)j1708EventLog[i].flags);
                break;
            case JEV_SerIRQ:
                snprintf (buf, 256, "IRQ: RxHF=%d, ToI=%d, ToNE=%d, Frm=%d, Par=%d, TxHE=%d, TxE=%d, RxNE=%d\r\n",
                    //(j1708EventLog[i].flags & TxFull) != 0,
                    (j1708EventLog[i].flags & RxHalfFull) != 0,
                    (j1708EventLog[i].flags & TimeoutIdle) != 0,
                    (j1708EventLog[i].flags & TimeoutNotEmtpy) != 0,
                    //(j1708EventLog[i].flags & OverrunError) != 0,
                    (j1708EventLog[i].flags & FrameError) != 0,
                    (j1708EventLog[i].flags & ParityError) != 0,
                    (j1708EventLog[i].flags & TxHalfEmpty) != 0,
                    (j1708EventLog[i].flags & TxEmpty) != 0,
                    (j1708EventLog[i].flags & RxBufNotEmtpy) != 0);
                break;
            case JEV_RecvFromBus:
                snprintf (buf, 256, "Received packet from bus\r\n");
                break;
            case JEV_RecvFromBusProc:
                snprintf (buf, 256, "Processed %d byte packet from bus\r\n", j1708EventLog[i].flags);
                break;
            case JEV_Collision:
                snprintf (buf, 256, "Collision detected (%s), count=%d\r\n", msgs[j1708EventLog[i].flags >> 8], j1708EventLog[i].flags & 0x00FF);
                break;
            case JEV_Retry:
                snprintf (buf, 256, "Retrying packet (retry #%d)\r\n", j1708EventLog[i].flags);
                break;
            case JEV_ChecksumErr:
                snprintf (buf, 256, "Checksum error on received packet\r\n");
                break;
            case JEV_Msg:
                snprintf (buf, 256, "### Review message %d\r\n", j1708EventLog[i].flags);
                break;
            default:
                snprintf (buf, 256, "Unknown event (%02X)\r\n", j1708EventLog[i].event);
                break;
            }
            Transmit (debugPort, buf, strlen(buf));

            while (!QueueEmpty(&debugPort->txQueue));
        }
        i++;
    } while (i != j1708EventLogIndex);
}
#endif

/*****************************/
/* J1708EnterCollisionState */
/***************************/
void J1708EnterCollisionState(enum J1708CollisionReason reason) {
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);

    J1708LogEvent (JEV_Collision, reason << 8 | j1708CurCollisionCount);
    j1708CollisionCount++;
    j1708CurCollisionCount++;

    if (j1708CurCollisionCount < 2) {
        j1708RetransmitIdleTime =  ConvertJ1708IdleCountToTimerTicks(J1708_IDLE_TIME + 2 * j1708CurTxMessage.priority);
    } else {
#ifdef TEST_RETRANSMIT_NO_RANDOMIZE
        j1708RetransmitIdleTime =  ConvertJ1708IdleCountToTimerTicks(J1708_IDLE_TIME + 2 * j1708CurTxMessage.priority);
#else
        j1708RetransmitIdleTime =  ConvertJ1708IdleCountToTimerTicks(J1708_IDLE_TIME + 2 * ((MainTimer.timer->Counter & 0x0007) + 1));
#endif
    }

    if (j1708State == JST_Transmitting) {
        j1708RetransmitNeeded = true;
    }
    j1708State = JST_IgnoreRxData;
    j1708CheckingMIDCharForCollision = -1;
    j1708Port->port->intEnable &= ~RxBufNotEmptyIE; // first of all turn this interrupt off, we don't need it anymore

    J1708DebugPrint ("Collision %d, reason %d, total %d", j1708CurCollisionCount, reason, j1708CollisionCount);
    ClearQueue (&j1708Port->txQueue);
    j1708Port->port->txReset = 0;
    j1708Port->port->intEnable &= ~TxHalfEmptyIE;

    RESTORE_IRQ(saveState);
}

/*********************/
/* J1708SetMIDState */
/*******************/
void J1708SetMIDState(UINT16 MID, bool state) {
    if (MID == 0xFFFF) {
        memset (j1708EnabledMIDs, (state ? 0xFF : 0x00), sizeof(j1708EnabledMIDs));
    } else {
        int idx = MID/8;
        int bit = MID%8;
        if (state) {
            j1708EnabledMIDs[idx] |= BIT(bit);
        } else {
            j1708EnabledMIDs[idx] &= ~BIT(bit);
        }
    }
}

/***************************/
/* J1708ResetDefaultPrefs */
/*************************/
void J1708ResetDefaultPrefs() {
    memset (j1708EnabledMIDs, 0, sizeof(j1708EnabledMIDs));
    j1708MIDFilterEnabled = true;
    j1708TransmitConfirm = false;
}

#ifdef _DEBUG
/**********************/
/* J1708PrintMIDInfo */
/********************/
void J1708PrintMIDInfo() {
    char tbuf[256];
    int idx = 0;
    int i = 0;
    msgIdx++;
    snprintf (tbuf, 256, "MID filters are %s.  Transmit confirm is %s\r\n", j1708MIDFilterEnabled ? "enabled" : "disabled", j1708TransmitConfirm ? "enabled" : "disabled");
    idx = strlen(tbuf);
    for (i = 0; i < 64; i+=2) {
        snprintf (tbuf+idx, 6, "%01X%01X%01X%01X.", j1708EnabledMIDs[i] % 16, j1708EnabledMIDs[i] / 16, j1708EnabledMIDs[i+1]%16, j1708EnabledMIDs[i+1]/16);
        idx += 5;

        if (((i % 16) >= 6)  && ((i % 16) < 8)){
            snprintf (tbuf+idx, 3, "..");
            idx += 2;
        }
        if (((i % 16) >= 14) && (i < 60)) {
            snprintf (tbuf+idx, 3, "\r\n");
            idx += 2;
        }
    }
    J1708DebugPrint (tbuf);

}
#endif // _DEBUG
