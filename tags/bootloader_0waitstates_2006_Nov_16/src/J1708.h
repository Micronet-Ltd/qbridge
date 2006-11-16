#ifndef J1708_H
#define J1708_H

#define J1708_IDLE_TIME 10
#define J1708_QUEUE_SIZE 16
#define J1708_DEFAULT_PRIORITY 8

extern int j1708RecvPacketCount;
extern int j1708IDCounter;
extern int j1708WaitForBusyBusCount;
extern int j1708CollisionCount;
extern int j1708RecvPacketLen;
extern bool j1708MIDFilterEnabled;
extern UINT8 j1708EnabledMIDs[64];
extern bool j1708TransmitConfirm;


typedef struct _J1708Message {
    UINT8 priority;
    UINT8 len;
    int id;
    UINT8 data[21];
} J1708Message;

typedef struct _J1708Queue {
    int head;
    int tail;
    J1708Message msgs[J1708_QUEUE_SIZE];
} J1708Queue;
extern J1708Queue j1708Queue;
extern J1708Message j1708CurTxMessage;

enum J1708CollisionReason { JCR_Framing = 1, JCR_FullComp = 0, JCR_FirstByteMismatch = 2 };
enum J1708State { JST_Passive, JST_Transmitting, JST_IgnoreRxData };
extern enum J1708State j1708State;
extern bool j1708RetransmitNeeded;
extern int  j1708CheckingMIDCharForCollision;
extern int  j1708RetransmitIdleTime;

#ifdef _DEBUG
#define _J1708DEBUG
#endif

#ifdef _J1708DEBUG
void J1708LogEventIdle (UINT8 event, UINT16 flags, UINT32 idleTime);
#define J1708LogEvent(event, flags) J1708LogEventIdle(event, flags, GetJ1708IdleTime())
enum J1708DebugEvents { JEV_None = 0, JEV_RecvFromHost, JEV_Transmit, JEV_SerIRQ, JEV_RecvFromBus, JEV_RecvFromBusProc, JEV_Collision,
                                JEV_Retry, JEV_ChecksumErr, JEV_Msg };
typedef struct _J1708EventLog {
    UINT32 time;
    UINT32 idle;
    UINT16 flags;
    UINT8 event;
    UINT8 state;
} J1708EventLog;
extern J1708EventLog j1708EventLog[256];
extern UINT8 j1708EventLogIndex;
void J1708PrintEventLog();
#define J1708DebugPrint(args...) DebugPrint(args)
void J1708PrintMIDInfo();
#else
#define J1708LogEvent(event, flags)
#define J1708PrintEventLog()
#define J1708DebugPrint(args...)
#endif

void InitializeJ1708();
void ProcessJ1708TransmitQueue();
void ProcessJ1708RecvPacket();

J1708Message *GetNextJ1708Message();
int J1708AddFormattedTxPacket (UINT8 priority, UINT8 *data, UINT8 len);
int J1708AddUnFormattedTxPacket(UINT8 priority, UINT8 *data, UINT8 len);
int GetFreeJ1708TxBuffers();

bool MIDPassesFilter();
void J1708SetMIDState(UINT16 MID, bool state);
void J1708ResetDefaultPrefs();
void J1708ComIRQHandle();
void J1708EnterCollisionState(enum J1708CollisionReason reason);


#endif //J1708_H
