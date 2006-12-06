/*******************************************************************/
/*                                                                 */
/* File:  CAN.c                                                    */
/*                                                                 */
/* Description: QBridge CAN drivers                                */
/*                                                                 */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2006 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 01-Nov-06  GPJ            1.0     1st Release                   */
/*******************************************************************/

//GPJ WHAT TO DO ON BUS FAULTS... CONTROLLER AUTOMATICALLY SETS THE INIT BIT

#include "common.h"
#include "serial.h"
#include "eic.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "protocol232.h"

#include "CAN.h"
//#include "J1939.h"

bool CANtransmitConfirm = FALSE;
bool CANBusOffNotify = FALSE;
bool CANAutoRestart = TRUE;

//Private methods
static void CAN_IRQ_Handler( void ) __attribute__ ((interrupt("IRQ")));

static CAN_message * GetNextCANmessage( void );

typedef struct _CANBaudTableEntry {
    UINT32 baud;
    UINT16 BitTimingRegValue;
} CANBaudTableEntry;

static const CANBaudTableEntry CANBaudRateTable[] = {
//                          +------BRP, Bit Rate Prescaler, we assume a system clock input to the CAN module of 24MHz
//                          |  +---- TSeg1
//                          |  | +--- TSeg2
//                          |  | | +--- SJW, this is the amount of allowed phase adjustment (measured in CAN tq clocks... ie after prescaler)
   {1000000l, CANBitTiming( 1, 7,2,2)}, //NOTE: There are many different solutions that can meet this baud, I've chosen to go with the one that allows for the maximum SJW
   { 500000l, CANBitTiming( 2,11,2,2)}, //NOTE: There are many different solutions that can meet this baud, I've chosen to go with the one that allows for the maximum SJW
   { 250000l, CANBitTiming( 5,11,2,2)}, //NOTE: There are many different solutions that can meet this baud, I've chosen to go with the one that allows for the maximum SJW
   { 125000l, CANBitTiming(11,11,2,2)}, //NOTE: There are many different solutions that can meet this baud, I've chosen to go with the one that allows for the maximum SJW
};

static CANRegisterMap * const CAN = (CANRegisterMap *)CAN_REG_BASE;

//transmit confirm variable
int CANtxc[CAN_MIF_NUM_MSG_OBJS+1]; //plus 1 because of the way the hardware works (numbers them 1-32 instead of 0-31)


/********************************************************************/
/* Initialize the CAN Bus Controller                                */
/*    sets baud to our default value of 250k                        */
/*    ensure that we are not in test mode                           */
/*    clear all pending message objects                             */
/*    ensure no pending interrupts or data transfers                */
/********************************************************************/
void InitializeCANBusController( void ) {
    int i;
    extern void GPIO_Config (IOPortRegisterMap *GPIOx, UINT16 Port_Pins, Gpio_PinModes GPIO_Mode);

    GPIO_Config((IOPortRegisterMap *)IOPORT1_REG_BASE, CAN_Tx_Pin, GPIO_AF_PP );
    GPIO_Config((IOPortRegisterMap *)IOPORT1_REG_BASE, CAN_Rx_Pin, GPIO_IN_TRI_CMOS );
#if 1 //debug //give ourselves a trigger that can be used later
    GPIO_Config((IOPortRegisterMap *)IOPORT1_REG_BASE, BIT(7), GPIO_OUT_PP );
    GPIO_Config((IOPortRegisterMap *)IOPORT1_REG_BASE, BIT(6), GPIO_OUT_PP );
    GPIO_CLR(1,7);
    GPIO_CLR(1,6);
#endif
    CAN->ControlReg = (CANControlRegisterBits)Init;
    setCANBaud( DEFAULT_CAN_BAUD_RATE );  //J1939 specifies 250k baud
    setCANTestMode(Test_No_Test_Mode);
    //clear content of message RAM
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
    for( i=0; i < CAN_MIF_NUM_MSG_OBJS; i++ ) {
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Mask+Arb+Control+DataA+DataB);
        CAN->IF1_Regs.IFn_M1R = 0;
        CAN->IF1_Regs.IFn_M2R = 0;
        CAN->IF1_Regs.IFn_DA1 = 0;
        CAN->IF1_Regs.IFn_DA2 = 0;
        CAN->IF1_Regs.IFn_DB1 = 0;
        CAN->IF1_Regs.IFn_DB2 = 0;
        CAN->IF1_Regs.IFn_A1R = 0;
        CAN->IF1_Regs.IFn_A2R = 0; //this contains the MsgVal bit (bit 15)
        CAN->IF1_Regs.IFn_MCR = 0;
        CAN->IF1_Regs.IFn_CRR = i+1;
        CANtxc[i+1] = 0;            //clear out the transmit confirm holding area
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
    }
    //let's make sure no valid bits are set before clearing the init bit
    if( CAN->ND1R || CAN->ND2R || CAN->IP1R || CAN->IP2R || CAN->MV1R || CAN->MV2R ) {
        DebugPrint ("CAN couldn't/didn't clear new data or int pending or msg valid.");
    }
    CAN->StatusReg = LECmsk;//+TxOk+RxOk;//reset our TxOK, RxOK, and LEC bits
    RegisterEICHdlr( EIC_CAN, CAN_IRQ_Handler, CAN_IRQ_PRIORITY );
    EICEnableIRQ(EIC_CAN);
    CAN->ControlReg = EIE+SIE+IE;
}

void DisableAllCANFilters(void) {
    int i;
    //clear content of message RAM
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
    for( i=0; i < NUM_USER_CAN_FILTERS; i++ ) {
        //get the message valid bit, if set, clear it
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(Arb);
        CAN->IF1_Regs.IFn_CRR = i+1;
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        UINT16 tmp = CAN->IF1_Regs.IFn_A2R;
        if( tmp & MsgVal ){
            CAN->IF1_Regs.IFn_A2R = tmp & ~MsgVal;
            CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Arb);
            CAN->IF1_Regs.IFn_CRR = i+1;
            while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                ;
        }
    }
}

int findCANfilter( UINT32 mask, UINT32 value ){
    //search thru valid messages and see if this one is present
    int i;
    UINT32 enabled_filters;
    UINT32 myxtnd = value & STANDARD_CAN_FLAG ? 0 : Xtd;
    value &= ~STANDARD_CAN_FLAG;
    if( myxtnd ){
        mask  &= 0x1fffffff;
        value &= 0x1fffffff;
    }else{
        mask  &= 0x000007ff;
        mask  <<= 18;
        value &= 0x000007ff;
        value <<= 18;
    }
    value |= MsgVal<<16;
    value |= myxtnd<<16;  //this also has direction... do we ever car about transmits?
    mask  |= MDir<<16;  //we always pay attention to direction
    mask  |= MXtd<<16;  //and, if they are setting a filter, they care about extended or not

    enabled_filters = (CAN->MV2R << 16 ) | CAN->MV1R;
    enabled_filters &= 0x3fffffff;  //mask off bits used for transmit and enable all
    for( i = 0; i<NUM_USER_CAN_FILTERS; i++ ){
        if( (1<<i) & enabled_filters ) {
            //read filter setting
            while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                ;
            CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(Mask+Arb); //don't ClrIntPend or NewDat
            CAN->IF1_Regs.IFn_CRR = i+1;
            while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                ;
            //compare to passed in value
            bool found=FALSE;
            if( myxtnd ) {
                if( (CAN->IF1_Regs.IFn_M1R == (mask & 0xffff))
                 && ((CAN->IF1_Regs.IFn_M2R & ~0x2000) == mask>>16)
                 && (CAN->IF1_Regs.IFn_A1R == (value & 0xffff))
                 && (CAN->IF1_Regs.IFn_A2R == value>>16) ){
                    found = TRUE;
                }
            }else{
                if( ((CAN->IF1_Regs.IFn_M2R & ~0x2000) == mask>>16)
                 && (CAN->IF1_Regs.IFn_A2R == value>>16) ){
                    found = TRUE;
                }
            }
            //if the same, return this index
            if( found )
                return( i + 1 );
        }
    }
    return(0); //filter value not found
}

int read_CAN_filter( int filter_position, UINT32 *mask, UINT32 *value ){
    UINT32 enabled_filters;
    enabled_filters = (CAN->MV2R << 16 ) | CAN->MV1R;
    enabled_filters &= 0x3fffffff;  //mask off bits used for transmit and enable all
    AssertPrint(filter_position > 0, "reading a filter with index less than 0");
    AssertPrint(filter_position <= 31, "reading a filter with index greater than 31");
    if( enabled_filters & (1 << (filter_position-1)) ){ //if filter is ON, return the info as requested
        //read filter setting
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(Mask+Arb); //don't ClrIntPend or NewDat
        CAN->IF1_Regs.IFn_CRR = filter_position;
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        UINT32 t1 = CAN->IF1_Regs.IFn_M1R;
        UINT32 t2 = CAN->IF1_Regs.IFn_M2R;
        UINT32 t3 = CAN->IF1_Regs.IFn_A1R;
        UINT32 t4 = CAN->IF1_Regs.IFn_A2R;
        *value = (t4 & Xtd) ? (((t4 & IDMask) << IDshift) | t3) : (STANDARD_CAN_FLAG | ((t4 & IDMask) >> 2));
        *mask  = (t4 & Xtd) ? (((t2 & IDMask) << IDshift) | t1) : ((t2 & IDMask) >> 2);
        return( 1 );
    }
    return( 0 );
}

int setCANfilter( UINT32 mask, UINT32 value ){
    int i;
    UINT32 enabled_filters;

    UINT32 myxtnd = value & STANDARD_CAN_FLAG ? 0 : Xtd;
    value &= ~STANDARD_CAN_FLAG;
    if( myxtnd ){
        mask  &= 0x1fffffff;
        value &= 0x1fffffff;
    }else{
        mask  &= 0x000007ff;
        mask  <<= 18;
        value &= 0x000007ff;
        value <<= 18;
    }
    value |= MsgVal<<16;
    value |= myxtnd<<16;  //this also has direction... do we ever car about transmits?
    mask  |= MDir<<16;  //we always pay attention to direction
    mask  |= MXtd<<16;  //and, if they are setting a filter, they care about extended or not

    //okay, now find a place to put it
    enabled_filters = (CAN->MV2R << 16 ) | CAN->MV1R;
    enabled_filters |= ~0x3fffffff;  //set bits used for transmit and enable all so this will make no attempt to reuse them
    for( i = 0; i<NUM_USER_CAN_FILTERS; i++ ){
        if( ((1<<i) & enabled_filters) == 0 ){ //use first available slot
            while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                ;
            CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Mask+Arb+Control);
            CAN->IF1_Regs.IFn_M1R = mask;
            CAN->IF1_Regs.IFn_M2R = mask>>16;
            CAN->IF1_Regs.IFn_A1R = value;
            CAN->IF1_Regs.IFn_A2R = value>>16;
            CAN->IF1_Regs.IFn_MCR = RxIE+EoB+UMask;
            CAN->IF1_Regs.IFn_CRR = i+1;
            while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                ;
            return i+1;
        }
    }
    return(0);  //indicate failure
}

void unsetCANfilter( UINT8 filter_position ){
    UINT32 enabled_filters;
    enabled_filters = (CAN->MV2R << 16 ) | CAN->MV1R;
    enabled_filters &= 0x3fffffff;  //mask off bits used for transmit and enable all
    AssertPrint(filter_position > 0, "unsetting a filter with index less than 0");
    AssertPrint(filter_position <= 31, "unsetting a filter with index greater than 31");
    if( enabled_filters & (1 << (filter_position-1)) ){ //if filter is ON, turn it off else do nothing
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(Arb);
        CAN->IF1_Regs.IFn_CRR = filter_position;
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        UINT16 tmp = CAN->IF1_Regs.IFn_A2R;
        CAN->IF1_Regs.IFn_A2R = tmp & ~MsgVal;
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Arb);
        CAN->IF1_Regs.IFn_CRR = filter_position;
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
    }
}

/********************************************************************/
/* Setup to monitor (interrupt) all messages received on CAN BUS    */
/********************************************************************/
void EnableCANReceiveALL( void ) {
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
    CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Mask+Arb+Control+DataA+DataB);
    CAN->IF1_Regs.IFn_M1R = 0;
    CAN->IF1_Regs.IFn_M2R = MDir;   //pay attention to direction
    CAN->IF1_Regs.IFn_DA1 = 0;
    CAN->IF1_Regs.IFn_DA2 = 0;
    CAN->IF1_Regs.IFn_DB1 = 0;
    CAN->IF1_Regs.IFn_DB2 = 0;
    CAN->IF1_Regs.IFn_A1R = 0;
    CAN->IF1_Regs.IFn_A2R = MsgVal; //direction = 0 = receive
    CAN->IF1_Regs.IFn_MCR = RxIE+EoB+UMask;
    CAN->IF1_Regs.IFn_CRR = ALLMSG;
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
}

/********************************************************************/
/* Stop monitor (interrupt) all messages received on CAN BUS        */
/********************************************************************/
void DisableCANReceiveALL( void ) {
    UINT32 enabled_filters;
    enabled_filters = (CAN->MV2R << 16 ) | CAN->MV1R;
    if( enabled_filters & (1 << (ALLMSG-1)) ){ //if filter is ON, turn it off else do nothing
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(Arb);
        CAN->IF1_Regs.IFn_CRR = ALLMSG;
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        UINT16 tmp = CAN->IF1_Regs.IFn_A2R;
        CAN->IF1_Regs.IFn_A2R = tmp & ~MsgVal;
        CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Arb);
        CAN->IF1_Regs.IFn_CRR = ALLMSG;
        while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
    }
}

/********************************************************************/
/* Stop any transfer in progress from our CAN BUS                   */
/********************************************************************/
void DisableCANTxIP( void ) {
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
    CAN->IF1_Regs.IFn_A2R = 0; //this will clear the message valid bit
    CAN->IF1_Regs.IFn_MCR = 0; //this will clear the tx request bit
    CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Arb+Control);
    CAN->IF1_Regs.IFn_CRR = SNDMSG;
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
    CAN->ControlReg |= (CANControlRegisterBits)Init;
    CAN->ControlReg &= ~(CANControlRegisterBits)Init;
}

void CANRestart( void ){
    CAN->ControlReg |= (CANControlRegisterBits)Init;
    CAN->ControlReg &= ~(CANControlRegisterBits)Init;
}


/********************************************************************/
/* set Baud rate for our CAN bus                                    */
/********************************************************************/
bool setCANBaud( UINT32 desired_baud ) {
    int i;
    UINT16 CANBitTimingRegVal=0;
    UINT16 myControlReg;

    for (i = 0; i < ARRAY_SIZE(CANBaudRateTable); i++) {
        if (CANBaudRateTable[i].baud == desired_baud) {
            CANBitTimingRegVal = CANBaudRateTable[i].BitTimingRegValue;
            break;
        }
    }

    if (CANBitTimingRegVal == 0) {
        return false;
    }
    myControlReg    = CAN->ControlReg;
    CAN->ControlReg = (CANControlRegisterBits)Init;
    CAN->ControlReg = (CANControlRegisterBits) Init + CCE;
    CAN->BitTimeReg = CANBitTimingRegVal;
    CAN->BRP_ExtReg = 0;    //for now, we don't support really slow baud rates
    CAN->ControlReg = myControlReg;

    return true;
}

/********************************************************************/
/* set the CAN Bus Controller for test mode                         */
/********************************************************************/
void setCANTestMode( CANTestModesEnum myTestMode) {
    switch( myTestMode ) {
        case Test_Silent:
            CAN->ControlReg |= (CANControlRegisterBits) Test;
            CAN->TestReg = (CANTestRegisterBits) Silent;
            break;
        case Test_Loop_Back:
            CAN->ControlReg |= (CANControlRegisterBits) Test;
            CAN->TestReg = (CANTestRegisterBits) LBack;
            break;
        case Test_Hot_SelfTest:
            CAN->ControlReg |= (CANControlRegisterBits) Test;
            CAN->TestReg = (CANTestRegisterBits) (LBack | Silent);
            break;
        case Test_No_Test_Mode:
            CAN->ControlReg |= (CANControlRegisterBits) Test;
            CAN->TestReg = 0;
            CAN->ControlReg &= ~((CANControlRegisterBits) Test);
            break;
        default:
            DebugPrint ("Unknown CAN debug mode requested.");
            break;
    }
}


/********************************************************************/
/* Setup to send a messages on CAN BUS                              */
/********************************************************************/
void send_CAN_message( UINT32 id, int dlen, unsigned char *data, int txcid, bool big ) {
    UINT16 *dptr;
    int i;
#ifdef _DEBUG
    AssertPrint(dlen <= 8, "Data length too long" );
    AssertPrint(dlen >= 0, "Data length less than 0?" );
    AssertPrint(data != 0, "Null pointer" );
    if( id & STANDARD_CAN_FLAG ) {
        AssertPrint(((id & ~0x800007ff) == 0), "Bad STANDARD CAN identifier" );
    }else{
        AssertPrint( ((id & ~0x1fffffff) == 0), "Bad extended CAN identifier");
    }
#endif
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
    CAN->IF1_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Mask+Arb+Control+DataA+DataB+TxRqNewDat);
    CAN->IF1_Regs.IFn_M1R = 0;
    CAN->IF1_Regs.IFn_M2R = 0;
    if( id & STANDARD_CAN_FLAG ) { //special flag to indicate STANDARD CAN message (not J1939 standard)
        id &= ~STANDARD_CAN_FLAG;
        //CAN->IF1_Regs.IFn_A1R = id & 0xffff;
        CAN->IF1_Regs.IFn_A2R = (CAN_IFN_A2Rbits)(MsgVal       | Dir | (id << 2));
    }else{
        CAN->IF1_Regs.IFn_A1R = id & 0xffff;
        CAN->IF1_Regs.IFn_A2R = (CAN_IFN_A2Rbits)(MsgVal | Xtd | Dir | A2RID(id));
    }
    if( CANtransmitConfirm || big) {
        CAN->IF1_Regs.IFn_MCR = (CAN_IFN_MCRbits) TxIE | EoB | (dlen & DLCmsk);
        CANtxc[SNDMSG] = txcid;
    }else{
        CAN->IF1_Regs.IFn_MCR = (CAN_IFN_MCRbits)        EoB | (dlen & DLCmsk);
        CANtxc[SNDMSG] = 0;
    }
    dptr = (UINT16 *)(&CAN->IF1_Regs.IFn_DA1);
    for( i=0; i<dlen/2; i++ ) {
        *dptr = (data[1] << 8) | data[0];
        data += 2;
        dptr += 2;  //sequentially points to IFn_DA1, IFn_DA2, IFn_DB1, IFn_DB2
                    //please note the different sizes of the two pointers!
    }
    if( dlen & 1 ) { //odd transfer
        *dptr = (0 << 8) | data[0];
    }
    CAN->IF1_Regs.IFn_CRR = SNDMSG;
    while( CAN->IF1_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
        ;
}


static UINT8 *dataint;
static int numdataintbytestosend;
static int numdataintbytessent;

void send_big_can_message( UINT32 id, int dlen, unsigned char *data, int txcid) {
    if( dlen > 8 ) {
        //** WARNING ** WARNING ** WARNING **
        //it is assumed that the pointer passed here is pointing to something
        //that will be around for the duration of the transfer (probably NOT a stack variable)
        dataint = &data[8];
        numdataintbytestosend = dlen-8;
        numdataintbytessent = 0;
        send_CAN_message(id, 8, data, txcid, TRUE);
    }else{
        send_CAN_message(id, dlen, data, txcid, FALSE);
    }
}

int TxCnt = 0;
int RxCnt = 0;
int StuffErrCnt = 0;
int FormErrCnt = 0;
int AckErrCnt = 0;
int Bit1ErrCnt = 0;
int Bit0ErrCnt = 0;
int CRCErrCnt = 0;
int LostMessageCnt = 0;
bool can_int_queue_overflow = FALSE;
bool CAN_received = FALSE;

CAN_queue CAN_received_queue;

/********************************************************************/
/* Handle interrupts from the CAN bus controller                    */
/*   essentially there are two sources....                          */
/*       1- status interrupt  (mostly used for error counting)      */
/*       2- message object interrupt                                */
/*   the second (message object interrupt) has two resons           */
/*            a- received new CAN message                           */
/*            b- sent a CAN message (and it was ack'd by someone)   */
/********************************************************************/
static void CAN_IRQ_Handler( void ) {
//    GPIO_SET(1,7);
    bool handled = false;
    UINT16 intid = CAN->InterruptID;
    if( intid == 0x8000 ){ //status interrupt, cleared by reading the status register
        UINT16 intstat = CAN->StatusReg;   //reading status register clears interrupt
        CAN->StatusReg = LECmsk;//TxOk+RxOk;//reset our TxOK, RxOK, and LEC bits
        if( intstat & TxOk ) TxCnt++;
        if( intstat & RxOk ) RxCnt++;
        switch( intstat & LECmsk ){
            case 0:
                break;
            case StuffErr:
                StuffErrCnt++;
                break;
            case FormErr:
                FormErrCnt++;
                break;
            case AckErr:
                AckErrCnt++;
                break;
            case Bit1Err:
                Bit1ErrCnt++;
                break;
            case Bit0Err:
                GPIO_SET(1,7);
                Bit0ErrCnt++;
                break;
            case CRCErr:
                CRCErrCnt++;
                break;
        }
        if( intstat & EWarn ){  //not sure what to do if we get this interrupt
        }
        if( intstat & BOff ) {  //part won't recover from this unless we clear the Init bit again
            //que up a message to go to the host
            CAN_received = TRUE;
            int nextHead = (CAN_received_queue.head + 1) % CAN_QUEUE_SIZE;
            if( nextHead == CAN_received_queue.tail ){
                can_int_queue_overflow = TRUE;   //the queue was full... we'll just overwrite whatever was there, but leave an indicator that it happened
            }
            CAN_message *cmsg = &CAN_received_queue.CAN_messages[CAN_received_queue.head];
            CAN_received_queue.head = nextHead;

            cmsg->len = 0;
            cmsg->id = 0;
            cmsg->src = SRC_BOF;
        }
        if( intstat & EPass ){  //this will not interrupt, but if we are in this state... then what?
        }
        handled = true;
    }else if( intid && (intid <= 32) ) { //message interrupting
        //read message from message object ram
        GPIO_SET(1,7);
        while( CAN->IF2_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;
        CAN->IF2_Regs.IFn_CMR = (CAN_IFN_CMRBits)(Arb+Control+DataA+DataB+ClrIntPend+TxRqNewDat);
        CAN->IF2_Regs.IFn_CRR = intid;
        //wait for HW to actually get message where we can see it
        while( CAN->IF2_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
            ;

        UINT16 mstat = CAN->IF2_Regs.IFn_MCR;
        if( mstat & RxIE ) {    //was it the receive interrupt that brought us here
            //####################################################################################
            //########
            //########   C A N    D A T A    R E C E I V E     I N T E R R U P T
            //########
            //####################################################################################
            CAN_received = TRUE;
            int nextHead = (CAN_received_queue.head + 1) % CAN_QUEUE_SIZE;
            if( nextHead == CAN_received_queue.tail ){
                can_int_queue_overflow = TRUE;   //the queue was full... we'll just overwrite whatever was there, but leave an indicator that it happened
            }
            CAN_message *cmsg = &CAN_received_queue.CAN_messages[CAN_received_queue.head];
            CAN_received_queue.head = nextHead;

            cmsg->len = mstat & DLCmsk;
//?            cmsg->id = ?;
            cmsg->src = SRC_RCV;
            UINT16 tmp = CAN->IF2_Regs.IFn_A2R;
            if( tmp & Xtd ){
                cmsg->CAN_Identifier = ((CAN->IF2_Regs.IFn_A2R & IDMask) << IDshift) | (CAN->IF2_Regs.IFn_A1R);
            }else{
                cmsg->CAN_Identifier = STANDARD_CAN_FLAG | ((CAN->IF2_Regs.IFn_A2R & IDMask) >> 2);
            }

            UINT16 *dptr = (UINT16 *)(&CAN->IF2_Regs.IFn_DA1);
            UINT8 *data = &cmsg->data[0];
            int i;
            for( i=0; i<8/2; i++ ) {
                UINT16 val = *dptr;
                data[0] = val & 0xff;
                data[1] = (val >> 8) & 0xff;
                data += 2;
                dptr += 2;  //sequentially points to IFn_DA1, IFn_DA2, IFn_DB1, IFn_DB2
                            //please note the different sizes of the two pointers!
            }
            if( mstat & MsgLst ) { //if we have "Lost" a message, count it and clear the status
                LostMessageCnt++;
                CAN->IF2_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Control);
                CAN->IF2_Regs.IFn_MCR = mstat & ~(MsgLst+NewDat+TxRqst);
                CAN->IF2_Regs.IFn_CRR = intid;
                while( CAN->IF2_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                    ;
            }

        }else{ //it was the transmit that brought us here
        //####################################################################################
        //########
        //########   C A N    D A T A    T R A N S M I T     I N T E R R U P T
        //########
        //####################################################################################
            //two different reasons to be here, 1-transmit confirm, 2-high speed can transfer
            if( CANtxc[intid] == 0 ){
                int i;
                while( CAN->IF2_Regs.IFn_CRR & CAN_MIF_CRR_BUSY )
                    ;
                CAN->IF2_Regs.IFn_CMR = (CAN_IFN_CMRBits)(WR+Control+DataA+DataB+TxRqNewDat);
                //determine how much we're are going to send this time
                int dlen = numdataintbytestosend - numdataintbytessent;
                if( dlen > 8 ) dlen = 8;
                UINT16 *dptr = (UINT16 *)(&CAN->IF2_Regs.IFn_DA1);
                UINT8 *data = &dataint[numdataintbytessent];

                for( i=0; i<dlen/2; i++ ) {
                    *dptr = (data[1] << 8) | data[0];
                    data += 2;
                    dptr += 2;  //sequentially points to IFn_DA1, IFn_DA2, IFn_DB1, IFn_DB2
                                //please note the different sizes of the two pointers!
                }
                if( dlen & 1 ) { //odd transfer
                    *dptr = (0 << 8) | data[0];
                }
                numdataintbytessent += dlen;
                if( numdataintbytestosend != numdataintbytessent ) {
                    CAN->IF2_Regs.IFn_MCR = (CAN_IFN_MCRbits) TxIE | EoB | (dlen & DLCmsk);
                }else{
                    CAN->IF2_Regs.IFn_MCR = (CAN_IFN_MCRbits)        EoB | (dlen & DLCmsk);
                }

                CAN->IF2_Regs.IFn_CRR = intid;
            }else{
                //do something so transmit confirm knows the transfer has occurred AND has been ack'd by at least one device
                CAN_received = TRUE;
                int nextHead = (CAN_received_queue.head + 1) % CAN_QUEUE_SIZE;
                if( nextHead == CAN_received_queue.tail ){
                    can_int_queue_overflow = TRUE;   //the queue was full... we'll just overwrite whatever was there, but leave an indicator that it happened
                }
                CAN_message *cmsg = &CAN_received_queue.CAN_messages[CAN_received_queue.head];
                CAN_received_queue.head = nextHead;

                cmsg->len = 0;
                cmsg->id = CANtxc[ intid ];
                CANtxc[ intid ] = 0;
                cmsg->src = SRC_TXC;
            }
        }

        handled = true;
    }else{
        DebugPrint("bad CAN interrupt id");
    }
    if (!handled) {
        DebugPrint ("Unknown interrupt on CAN");
    }
    EICClearIRQ(EIC_CAN);

    GPIO_CLR(1,7);
}

//#############################################################################
//#############################################################################
//#############################################################################
//#############################################################################

CAN_queue CAN_tosend_queue;

/********************************************************************/
/* Initialize CAN structures... software structs to handle our stuff*/
/********************************************************************/
void InitializeCANstructs( void ){
    CAN_tosend_queue.head = 0;
    CAN_tosend_queue.tail = 0;
    CAN_received_queue.head = 0;
    CAN_received_queue.tail = 0;
}

/********************************************************************/
/* Initialize CAN                                                   */
/********************************************************************/
void InitializeCAN( void ){
    InitializeCANBusController();
    InitializeCANstructs();
}

void CANResetDefaultPrefs( void ){
    CANtransmitConfirm = FALSE;
    CANBusOffNotify = FALSE;
    CANAutoRestart = TRUE;
    DisableAllCANFilters();
    DisableCANReceiveALL();
    ClearCANTxQueue();
    DisableCANTxIP();   //kill any CAN transmit that may be in progress
    ClearCANRxQueue();
}


/********************************************************************/
/* Process CAN Transmit Queue... filled by RS232 task               */
/********************************************************************/
void ProcessCANTransmitQueue( void ){
    if( CAN_tosend_queue.head == CAN_tosend_queue.tail ) {
        return;
    }

    //is there space available to send this message?
    //  note: this could be expanded to better utilize the 32 entries
    //        allowed by our hardware, but that comes with certain issues
    //        like the fact that they may get transmitted out of order
    if( CAN->TxR2R ) {
        return; //can't continue until previous transmit is complete
    }
    //retransmits????
    CAN_message *msg = GetNextCANmessage();
    if( msg == NULL ){
        DebugPrint("Unexpected null message retreived");
        return;
    }
    //now send the message... be careful, the id we stored was a message
    //identifier to satisfy our 232 protocol... the id we pass to CAN
    //is the CAN identifier (found in our message)
    send_CAN_message(msg->CAN_Identifier, msg->len, msg->data, msg->id, FALSE);
    CAN_tosend_queue.tail = (CAN_tosend_queue.tail + 1 ) % CAN_QUEUE_SIZE;
}

/********************************************************************/
/* Process CAN Recieve Packet...                                    */
/*    filled by interrupt to be passed to232                        */
/********************************************************************/
void ProcessCANRecievePacket( void ){
    if( !CAN_received ){
        return;
    }
    IRQSTATE saveState=0;
    UINT8 cmd;
    UINT8 hostdata[1+4+8];  //type specifier + CAN identifier + data
    UINT8 *hd = hostdata;
    int dl = 0;
    CAN_message *canmsg = &CAN_received_queue.CAN_messages[CAN_received_queue.tail];
    if( canmsg->src == SRC_RCV ) { //something actually passed the filter
        if( canmsg->CAN_Identifier & STANDARD_CAN_FLAG ){
            //standard
            *hd++ = 0;
            *hd++ = canmsg->CAN_Identifier & 0xff;
            *hd++ = (canmsg->CAN_Identifier >> 8) & 0xff;
            dl+=3;
        }else{
            //extended
            *hd++ = 1;
            *hd++ = canmsg->CAN_Identifier & 0xff;
            *hd++ = (canmsg->CAN_Identifier >> 8) & 0xff;
            *hd++ = (canmsg->CAN_Identifier >> 16) & 0xff;
            *hd++ = (canmsg->CAN_Identifier >> 24) & 0xff;
            dl+=5;
        }
        memcpy(hd, canmsg->data, canmsg->len);
        dl += canmsg->len;
        cmd = ReceiveCANPacket;
    }else if( canmsg->src == SRC_TXC ) {    //Transmit confirm
        *hd++ = 1;  //success/fail flag (1=success, 0=fail)
        *hd++ = canmsg->id & 0xff;
        *hd++ = (canmsg->id >> 8) & 0xff;
        *hd++ = (canmsg->id >> 16) & 0xff;
        *hd++ = (canmsg->id >> 24) & 0xff;
        dl += 5;
        cmd = CANTransmitConfirm;
    }else if( canmsg->src == SRC_BOF ) {    //Bus Off (bus error) notification
        cmd = CANbusErr;
    }else{  //anything else should be in this queue
        DebugPrint("Bad source of message found in queue bound for transfer to host!");
        //need to still remove this from the que
        cmd=0;  //don't want to send anything to host
    }

    //now that we have free'd this queue entry,  update tail and CAN_received
    int newtail = (CAN_received_queue.tail + 1 ) % CAN_QUEUE_SIZE;
    DISABLE_IRQ( saveState );
    CAN_received_queue.tail = newtail;
    if( CAN_received_queue.head == newtail ){
        CAN_received = FALSE;
    }
    RESTORE_IRQ(saveState);

    //go ahead and send the data on it's way
    if( cmd != 0 ) {
        QueueTx232Packet( cmd, hostdata, dl );
    }
}


/********************************************************************/
/* GetNextCANmessage from the message queue                         */
/*    always pull from the tail... host port places it at the head  */
/********************************************************************/
static CAN_message * GetNextCANmessage( void ){
    if( CAN_tosend_queue.head == CAN_tosend_queue.tail ){
        return( NULL );
    }
    return( &(CAN_tosend_queue.CAN_messages[CAN_tosend_queue.tail]) );
}


/********************************************************************/
/* CANaddTxPacket to the message queue                              */
/*    this is from the host port so we put it at the head           */
/********************************************************************/
int CANaddTxPacket( UINT8 type, UINT32 CAN_id, UINT8 *data, UINT8 len  ){
    int nextHead = (CAN_tosend_queue.head + 1) % CAN_QUEUE_SIZE;
    if( nextHead == CAN_tosend_queue.tail ){
        return( -1 );   //this means that the queue was full
    }
    if( CAN->ControlReg & (CANControlRegisterBits)Init ) {//the bus has a fault that has caused us to not be able to transmit
        if( CANAutoRestart == TRUE ){
            CAN->ControlReg &= ~((CANControlRegisterBits)Init );
        }
        return( -2 );   //this means that the BUS has a fault condition
    }
    int curHead = CAN_tosend_queue.head;
    CAN_tosend_queue.CAN_messages[curHead].len = len;
    CAN_tosend_queue.CAN_messages[curHead].id  = getPktIDcounter();
    CAN_tosend_queue.CAN_messages[curHead].CAN_Identifier = CAN_id | ((type == 0) ? STANDARD_CAN_FLAG : 0);
    CAN_tosend_queue.CAN_messages[curHead].src = SRC_232;
    //CAN_tosend_queue.CAN_messages[curHead].data = data;
    memcpy(CAN_tosend_queue.CAN_messages[curHead].data, data, len);
    CAN_tosend_queue.head = nextHead;
    return( CAN_tosend_queue.CAN_messages[curHead].id );
}

/**************************/
/* GetFreeCANtxBuffers    */
/**************************/
int GetFreeCANtxBuffers( void ) {
    int numInUse;
    if (CAN_tosend_queue.head > CAN_tosend_queue.tail) {
        numInUse = CAN_tosend_queue.head - CAN_tosend_queue.tail;
    } else {
        numInUse = (CAN_tosend_queue.head + CAN_QUEUE_SIZE) - CAN_tosend_queue.tail;
    }
    return (CAN_QUEUE_SIZE - 1) - numInUse;
}


/********************************************************************/
/* Initialize CAN structures... software structs to handle our stuff*/
/********************************************************************/
void ClearCANTxQueue( void ){
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    CAN_tosend_queue.tail = CAN_tosend_queue.head;
    RESTORE_IRQ(saveState);
}

void ClearCANRxQueue( void ){
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    CAN_received_queue.tail = CAN_received_queue.head;
    RESTORE_IRQ(saveState);
}