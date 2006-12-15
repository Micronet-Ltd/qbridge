#ifndef CAN_H
#define CAN_H

#include "common.h"
#include "queue.h"
#include "interrupt.h"


//Public methods
void InitializeCANBusController( void );
bool setCANBaud( UINT32 desired_baud );
void setCANTestMode( CANTestModesEnum myTestMode );

void InitializeCAN( void );
void InitializeCANstructs( void );
void ProcessCANTransmitQueue( void );
void ProcessCANRecievePacket( void );
int CANaddTxPacket( UINT8 type, UINT32 CAN_id, UINT8 *data, UINT8 len  );
int GetFreeCANtxBuffers( void );
void CANResetDefaultPrefs( void );


void DisableAllCANFilters(void);
int setCANfilter( UINT32 mask, UINT32 value );
void unsetCANfilter( UINT8 filter_position );
int findCANfilter( UINT32 mask, UINT32 value );
int read_CAN_filter( int filter_position, UINT32 *mask, UINT32 *value );
void EnableCANReceiveALL( void );
void DisableCANReceiveALL( void );
void DisableCANTxIP( void );
void ClearCANTxQueue( void );
void ClearCANRxQueue( void );
void CANRestart( void );


extern bool CANtransmitConfirm;
extern bool CANBusOffNotify;
extern bool CANAutoRestart;

typedef struct {
    UINT32 CAN_Identifier;  //msbit is our indicator as to whether this is STANDARD=1 or EXTENDED=0
    UINT8 data[8];
    UINT32 id;
    UINT8 len;  //number of data bytes
    UINT8 src;  //source of this message, 0=232 interface, 1=received from CAN, 2=transmit confirm
} CAN_message;

#define STANDARD_CAN_FLAG 0x80000000
#define SRC_232 0
#define SRC_RCV 1
#define SRC_TXC 2
#define SRC_BOF 3

#define CAN_QUEUE_SIZE 100

typedef struct {
    int head;
    int tail;
    CAN_message CAN_messages[CAN_QUEUE_SIZE];
} CAN_queue;

//Message object usage (1-32 (CAN_MIF_NUM_MSG_OBJS))....
#define ALLMSG 31 // receive all messages
#define SNDMSG 32 // send messages from this location

#define NUM_USER_CAN_FILTERS 25 //30   <<--- this is limited by our implementation of host serial communications, it's max transfer len is 255, and each extended CAN filter requires 9 bytes

#define DEFAULT_CAN_BAUD_RATE 250000l   //as spec'd in J1939

//Message interface usage...
//   IF1_Regs used for non interrupt code
//   IF2_Regs used for interrupt service code

#endif // CAN_H
