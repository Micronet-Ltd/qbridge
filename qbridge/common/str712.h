/*******************************************************************/
/*                                                                 */
/* File:  str712.h                                                 */
/*                                                                 */
/* Description: Header for ST STR712 processor hardware.           */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2005 QSI Corporation                  */
/*                                                                 */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 02 Aug 2005  MK Elwood    1.0     1st Release                   */
/*******************************************************************/

#ifndef STR712_H
#define STR712_H

/************************/
/* Memory Map Addresses */
/************************/

/* Flash ROM */
#define FLASH_PHYS_BASE 0x40000000

/* SRAM */
#define RAM_PHYS_BASE   0x20000000

/* Internal registers */
#define PRCCU_REG_BASE  0xa0000000
#define APB1_REG_BASE   0xc0000000
#define APB2_REG_BASE   0xe0000000


/*****************************/
/* STR712 internal registers */
/*****************************/

/*********/
/* PRCCU */
/*********/
#define RCCU_REG_BASE       (PRCCU_REG_BASE)

#define RCCU_CCR_OFFSET             0x00
#define RCCU_CFR_OFFSET             0x08
#define RCCU_PLL1CR_OFFSET          0x18
#define RCCU_PER_OFFSET             0x1c
#define RCCU_SMR_OFFSET             0x20

#ifndef _ASM_
typedef struct
{
    volatile unsigned long ccr;
    volatile unsigned long pad1;
    volatile unsigned long cfr;
    volatile unsigned long pad2;
    volatile unsigned long pad3;
    volatile unsigned long pad4;
    volatile unsigned long pll1cr;
    volatile unsigned long per;
    volatile unsigned long smr;
} RCCUREGS;
#endif /* _ASM_ */

/* CFR Register Bits */
#define LOCK        BIT(1)
#define CSU_CKSEL   BIT(0)


#define PCU_REG_BASE            (PRCCU_REG_BASE + 0x40)

#define PCU_MDIVR_OFFSET            0x00
#define PCU_PDIVR_OFFSET            0x04
#define PCU_RSTR_OFFSET             0x08
#define PCU_PLL2CR_OFFSET           0x0c
#define PCU_BOOTCR_OFFSET           0x10
#define PCU_PWRCR_OFFSET            0x14

#ifndef _ASM_
typedef struct
{
    volatile unsigned long mdivr;
    volatile unsigned long pdivr;
    volatile unsigned long rstr;
    volatile unsigned long pll2cr;
    volatile unsigned long bootcr;
    volatile unsigned long pwrcr;
} PCUREGS;
#endif /* _ASM_ */


/* PWRCR Register Bits */
#define VR_OK       BIT(12)


/*******/
/* APB */
/*******/

/*******/
/* I2C */
/*******/
#define I2C0_REG_BASE       (APB1_REG_BASE + 0x1000)
#define I2C1_REG_BASE       (APB1_REG_BASE + 0x2000)


/********/
/* UART */
/********/
#define UART0_REG_BASE      (APB1_REG_BASE + 0x4000)
#define UART1_REG_BASE      (APB1_REG_BASE + 0x5000)
#define UART2_REG_BASE      (APB1_REG_BASE + 0x6000)
#define UART3_REG_BASE      (APB1_REG_BASE + 0x7000)

#define TXBUFFER_OFFSET 0x04

#ifndef _ASM_
enum UartInterrupts {
    RxHalfFullIE        = 0x0100,
    TimeoutIdleIE       = 0x0080,
    TimeoutNotEmptyIE   = 0x0040,
    OverrunErrorIE      = 0x0020,
    FrameErrorIE        = 0x0010,
    ParityErrorIE       = 0x0008,
    TxHalfEmptyIE       = 0x0004,
    TxEmptyIE           = 0x0002,
    RxBufNotEmptyIE = 0x0001,
};

enum UartStatusBits {
    TxFull              = 0x0200,
    RxHalfFull          = 0x0100,
    TimeoutIdle         = 0x0080,
    TimeoutNotEmtpy = 0x0040,
    OverrunError        = 0x0020,
    FrameError          = 0x0010,
    ParityError         = 0x0008,
    TxHalfEmpty         = 0x0004,
    TxEmpty             = 0x0002,
    RxBufNotEmpty       = 0x0001,
};

typedef struct _UARTRegisterMap {
    volatile UINT32 BaudRate;
    volatile UINT32 txBuffer;
    volatile UINT32 rxBuffer;
    volatile UINT32 portSettings;
    volatile UINT32 intEnable;
    volatile UINT32 status;
    volatile UINT32 guardTime;
    volatile UINT32 timeout;
    volatile UINT32 txReset;
    volatile UINT32 rxReset;
} UARTRegisterMap;

typedef union _UARTSettingsMap {
    UINT32 value;
    struct {
        UINT32 mode:3;
        UINT32 stopBits:2;
        UINT32 parityOdd:1;
        UINT32 loopBack:1;
        UINT32 run:1;
        UINT32 rxEnable:1;
        UINT32 reserved1:1;
        UINT32 fifoEnable:1;
        UINT32 reserved2:21;
    };
} UARTSettingsMap;

#define UART0_Rx_Pin BIT(8)   /*  TQFP 64: pin N° 63 , TQFP 144 pin N° 143 */
#define UART0_Tx_Pin BIT(9)   /*  TQFP 64: pin N° 64 , TQFP 144 pin N° 144 */

#define UART1_Rx_Pin BIT(10)  /*  TQFP 64: pin N° 1  , TQFP 144 pin N° 1   */
#define UART1_Tx_Pin BIT(11)  /*  TQFP 64: pin N° 2  , TQFP 144 pin N° 3   */

#define UART2_Rx_Pin BIT(13) /*  TQFP 64: pin N° 5  , TQFP 144 pin N° 9   */
#define UART2_Tx_Pin BIT(14)  /*  TQFP 64: pin N° 6  , TQFP 144 pin N° 10  */

#define UART3_Rx_Pin BIT(1)   /*  TQFP 64: pin N° 52 , TQFP 144 pin N° 123 */
#define UART3_Tx_Pin BIT(0)   /*  TQFP 64: pin N° 53 , TQFP 144 pin N° 124 */

#endif /* _ASM_ */

/*******/
/* USB */
/*******/

/*******/
/* CAN */
/*******/
#define CAN_REG_BASE        (APB1_REG_BASE + 0x9000)
#ifndef _ASM_

//#define CAN_IF1_REG_BASE  (APB1_REG_BASE + 0x9020)
//#define CAN_IF2_REG_BASE  (APB1_REG_BASE + 0x9080)
typedef struct _CAN_IFn_RegisterMap {
    volatile UINT16 IFn_CRR;    //9020 or 9080
    volatile UINT16 pad0;       //9022 or 9082
    volatile UINT16 IFn_CMR;    //9024 or 9084
    volatile UINT16 pad1;       //9026 or 9086
    volatile UINT16 IFn_M1R;    //9028 or 9088
    volatile UINT16 pad2;       //902a or 908a
    volatile UINT16 IFn_M2R;    //902c or 908c
    volatile UINT16 pad3;       //902e or 908e
    volatile UINT16 IFn_A1R;    //9030 or 9090
    volatile UINT16 pad4;       //9032 or 9092
    volatile UINT16 IFn_A2R;    //9034 or 9094
    volatile UINT16 pad5;       //9036 or 9096
    volatile UINT16 IFn_MCR;    //9038 or 9098
    volatile UINT16 pad6;       //903a or 909a
    volatile UINT16 IFn_DA1;    //903c or 909c  //aka CAN_IFn_DAnR or CAN_IFn_DA1R
    volatile UINT16 pad7;       //903e or 909e
    volatile UINT16 IFn_DA2;    //9040 or 90A0  //aka CAN_IFn_DAnR or CAN_IFn_DA2R
    volatile UINT16 pad8;       //9042 or 90A2
    volatile UINT16 IFn_DB1;    //9044 or 90A4  //aka CAN_IFn_DBnR or CAN_IFn_DB1R
    volatile UINT16 pad9;       //9046 or 90A6
    volatile UINT16 IFn_DB2;    //9048 or 90A8  //aka CAN_IFn_DBnR or CAN_IFn_DB2R
    volatile UINT16 pad10;      //904a or 90Aa
    volatile UINT16 pad11;      //904c or 90Ac
    volatile UINT16 pad12;      //904e or 90Ae
} CAN_IFn_RegisterMap;


typedef struct _CANRegisterMap {
    volatile UINT16 ControlReg; //00        //CAN_CR (name as found in manual)
    volatile UINT16 pad0;       //02
    volatile UINT16 StatusReg;  //04        //CAN_SR (name as found in manual)
    volatile UINT16 pad1;       //06
    volatile UINT16 ErrorReg;   //08        //CAN_ERR (name as found in manual)
    volatile UINT16 pad2;       //0a
    volatile UINT16 BitTimeReg; //0c        //CAN_BTR (name as found in manual)
    volatile UINT16 pad3;       //0e
    volatile UINT16 InterruptID;//10        //CAN_IDR (name as found in manual)
    volatile UINT16 pad4;       //12
    volatile UINT16 TestReg;    //14        //CAN_TESTR (name as found in manual)
    volatile UINT16 pad5;       //16
    volatile UINT16 BRP_ExtReg; //18        //CAN_BRPR (name as found in manual)
    volatile UINT16 pad6;       //1a
    volatile UINT16 pad7;       //1c
    volatile UINT16 pad8;       //1e
    CAN_IFn_RegisterMap IF1_Regs;
//  volatile UINT16 IF1_CRR;    //20
//  volatile UINT16 pad9;       //22
//  volatile UINT16 IF1_CMR;    //24
//  volatile UINT16 pad10;      //26
//  volatile UINT16 IF1_M1R;    //28
//  volatile UINT16 pad11;      //2a
//  volatile UINT16 IF1_M2R;    //2c
//  volatile UINT16 pad12;      //2e
//  volatile UINT16 IF1_A1R;    //30
//  volatile UINT16 pad13;      //32
//  volatile UINT16 IF1_A2R;    //34
//  volatile UINT16 pad14;      //36
//  volatile UINT16 IF1_MCR;    //38
//  volatile UINT16 pad15;      //3a
//  volatile UINT16 IF1_DA1;    //3c
//  volatile UINT16 pad17;      //3e
//  volatile UINT16 IF1_DA2;    //40
//  volatile UINT16 pad18;      //42
//  volatile UINT16 IF1_DB1;    //44
//  volatile UINT16 pad19;      //46
//  volatile UINT16 IFn_DB2;    //48
//  volatile UINT16 pad20;      //4a
//  volatile UINT16 pad21;      //4c
//  volatile UINT16 pad22;      //4e
    volatile UINT8  pad23[0x80-0x50];//50-7f
    CAN_IFn_RegisterMap IF2_Regs;
//  volatile UINT16 IF2_CRR;    //80
//  volatile UINT16 pad24;      //82
//  volatile UINT16 IF2_CMR;    //84
//  volatile UINT16 pad25;      //86
//  volatile UINT16 IF2_M1R;    //88
//  volatile UINT16 pad26;      //8a
//  volatile UINT16 IF2_M2R;    //8c
//  volatile UINT16 pad27;      //8e
//  volatile UINT16 IF2_A1R;    //90
//  volatile UINT16 pad28;      //92
//  volatile UINT16 IF2_A2R;    //94
//  volatile UINT16 pad29;      //96
//  volatile UINT16 IF2_MCR;    //98
//  volatile UINT16 pad30;      //9a
//  volatile UINT16 IF2_DA1;    //9c
//  volatile UINT16 pad32;      //9e
//  volatile UINT16 IF2_DA2;    //A0
//  volatile UINT16 pad33;      //A2
//  volatile UINT16 IF2_DB1;    //A4
//  volatile UINT16 pad34;      //A6
//  volatile UINT16 IF2_DB2;    //A8
//  volatile UINT16 pad35;      //Aa
//  volatile UINT16 pad36;      //Ac
//  volatile UINT16 pad37;      //Ae
    volatile UINT8  pad38[0x100-0xb0];//b0-ff
    volatile UINT16 TxR1R;      //100
    volatile UINT16 pad39;      //102
    volatile UINT16 TxR2R;      //104
    volatile UINT8  pad40[0x120-0x106];//106-11f
    volatile UINT16 ND1R;       //120
    volatile UINT16 pad41;      //122
    volatile UINT16 ND2R;       //124
    volatile UINT8  pad42[0x140-0x126];//126-13f
    volatile UINT16 IP1R;       //140
    volatile UINT16 pad43;      //142
    volatile UINT16 IP2R;       //144
    volatile UINT8  pad44[0x160-0x146];//146-15f
    volatile UINT16 MV1R;       //160
    volatile UINT16 pad45;      //162
    volatile UINT16 MV2R;       //164
} CANRegisterMap;

#define CAN_MIF_NUM_MSG_OBJS 32

#define CANBitTiming(BRP, TSeg1, TSeg2, SJW) (((TSeg2 & 0x07) << 12) | \
                                              ((TSeg1 & 0x0f) <<  8) | \
                                              ((SJW   & 0x03) <<  6) | \
                                              ((BRP   & 0x3f) <<  0))

typedef enum {
    Test = 0x80,    //test mode enable when set
    CCE  = 0x40,    //congiguration change enabled when set (write access to bit timing register)
    DAR  = 0x20,    //disable automatic retransmission when set
    EIE  = 0x08,    //error interrupt enable when set
    SIE  = 0x04,    //status change interrupt enable when set
    IE   = 0x02,    //module interrupt enable when set, disable when clear
    Init = 0x01,    //Initialization mode when set, normal when clear
} CANControlRegisterBits;

typedef enum {
    BOff = 0x80,    //Busoff status, 1=CAN module is in bus off state
    EWarn= 0x40,    //Error warning, 1=at least one of the error counters has reached the warning limit of 96
    EPass= 0x20,    //Error Passive, 1=CAN core is in the error passive state
    RxOk = 0x10,    //Received a Message Successfully, 1=a message has been successfully recieved since last reset of this bit by the CPU
    TxOk = 0x08,    //Transmitted a Message Successfully, 1= msg sent (and ack'd by at least one other node) since last reset of this bit by the CPU
    LECmsk  = 0x07, //Mask to obtain just the Last Error Code
} CANStatusRegisterBits;

typedef enum {
    NoErr = 0x00,       //no error
    StuffErr = 0x01,    //More than 5 equal bits in a sequence have occurred in part of a received message where it is not allowed
    FormErr  = 0x02,    //A fixed format part of a received frame has the wrong format
    AckErr   = 0x03,    //Message transmitted was not acknowledged
    Bit1Err  = 0x04,    //During transmit, recessive expected, but dominant recieved (not checked during arbitration)
    Bit0Err  = 0x05,    //Durint transmit, dominant expected, but recessive recieved (not checked during arbitration)
    CRCErr   = 0x06,    //CRC checksum was incorrect in received message
    unused   = 0x07,    //unused
} CANLastErrorCodes;

typedef enum {
    CAN_Rx_Pin = 0x80,  //0=CAN bus is dominant, 1=CAN bus is recessive
    CAN_Tx_PinMask = 0x60, //00=can controlled, 01=output sample point on tx, 10=drive dominant, 11=drive recessive
    LBack = 0x10,       //1=loop back mode enabled
    Silent = 0x08,      //1=silent mode enabled
    Basic = 0x04,       //1=basic mode enabled, no message processing, 1F1 regs= tx buffer, 1F2 regs = rx buffer
} CANTestRegisterBits;

typedef enum{
    Test_Silent = 2 ,   //no activity will occur on the bus due to this device
    Test_Loop_Back = 1, //transmissions will still occur, but internally, the transmitter is connected to the receiver
    Test_Hot_SelfTest = 3, //loop back and silent together so you can test while connected to a CAN bus
    Test_No_Test_Mode = 0, //not test mode
} CANTestModesEnum;

#define CAN_MIF_CRR_BUSY 0x8000
typedef enum{
    WR        =0x80,//1=write message registers into the addressed message object (registers->RAM), 0=Read (RAM->registers)
    Mask      =0x40,//if write, 1=transfer ID mask, Dir mask and xtnd to message object
    Arb       =0x20,//if write, 1=transfer identifier, dir and xtd and msgvalid to message object
    Control   =0x10,//if write, 1=transfer control bits to message object
    ClrIntPend=0x08,//if write, ignored
    TxRqNewDat=0x04,//if write, set the TxRequest Bit
    DataA     =0x02,//if write, transfer data bytes 3:0 to message object
    DataB     =0x01,//if write, transfer data bytes 7:4 to message object
//  Mask      =0x40,//if read, 1=transfer ID mask, Dir mask and xtnd to message registers
//  Arb       =0x20,//if read, 1=transfer identifier, dir and xtd and msgvalid to message registers
//  Control   =0x10,//if read, 1=transfer control bits to message registers
//  ClrIntPend=0x08,//if read, clear intpnd bit in the message object (RAM)
//  TxRqNewDat=0x04,//if read, clear the new data bit in the message object (RAM)
//  DataA     =0x02,//if read, transfer data bytes 3:0 to message registers
//  DataB     =0x01,//if read, transfer data bytes 7:4 to message registers
} CAN_IFN_CMRBits; //Command Mask Register

typedef enum{
    MXtd    = 0x8000, //1=extended id format (29 bit) identifier (Mask)
    MDir    = 0x4000, //1=direction (Mask...1=compare, 0=don't care about direction for match)
    MIDmask = 0x1fff, //mask off id bits
    MIDshift = 16,    //number of bits to shift ID to get just the bits that go in this register
} CAN_IFN_M2Rbits;  //Mask register

#define M2RID(id) ( ((id) >> MIDshift) & MIDMask )

typedef enum{
    MsgVal  = 0x8000, //1=message is valid in this memory location
    Xtd     = 0x4000, //1=extended id format (29 bit) identifier
    Dir     = 0x2000, //1=direction is transmit, 0=direction is receive
    IDMask  = 0x1fff, //mask off id bits
    IDshift = 16,     //number of bits to shift ID to get just the bits that go in this register
} CAN_IFN_A2Rbits;  //Message Arbitration Register

#define A2RID(id) ( ((id) >> IDshift) & IDMask )
#define STANDARD_CAN_FLAG 0x80000000

typedef enum{
    NewDat  = 0x8000, //1=New data has been written to the message object (either by the application or the hw message handler)
    MsgLst  = 0x4000, //1=Message LOST (hw message handler stored new data to the message object when NewDat was still set)
    IntPnd  = 0x2000, //1=this message object is the source of an interrupt
    UMask   = 0x1000, //1=use acceptance mask for filtering
    TxIE    = 0x0800, //1=transmit interrupt enable
    RxIE    = 0x0400, //1=receive interrupt enable
    RmtEn   = 0x0200, //1=remote enable (at reception of a Remote Frame, TxRqst is set)
    TxRqst  = 0x0100, //1=transmission of this object is requested and is not yet complete
    EoB     = 0x0080, //1=End of Buffer (for setting up FIFO) should always be set for single message
    DLCmsk  = 0x000f, //mask off the Data Length Code
} CAN_IFN_MCRbits;  //message Control Register

#define CAN_Rx_Pin BIT(11)  /*  TQFP 64: pin N° 52 , TQFP 144 pin N° 123 */
#define CAN_Tx_Pin BIT(12)  /*  TQFP 64: pin N° 53 , TQFP 144 pin N° 124 */

#endif /* _ASM_ */

/********/
/* BSPI */
/********/

/********/
/* HDLC */
/********/

/*******/
/* XTI */
/*******/
#define XTI_REG_BASE     (APB2_REG_BASE + 0x1000)

#define XTI_SR_OFFSET    0x1c
#define XTI_CTRL_OFFSET  0x24
#define XTI_MRH_OFFSET   0x28
#define XTI_MRL_OFFSET   0x2c
#define XTI_TRH_OFFSET   0x30
#define XTI_TRL_OFFSET   0x34
#define XTI_PRH_OFFSET   0x38
#define XTI_PRL_OFFSET   0x3c

#ifndef _ASM_
typedef struct {
    volatile unsigned long pad1[7];
    volatile unsigned long sr;   /* 0x1c */
    volatile unsigned long pad2;
    volatile unsigned long ctrl; /* 0x24 */
    volatile unsigned long mrh;
    volatile unsigned long mrl;
    volatile unsigned long trh;
    volatile unsigned long trl;
    volatile unsigned long prh;
    volatile unsigned long prl;
} XTIREGS;

typedef enum {
    XTI_SW,          /* S/W interrupt */
    XTI_USB,         /* USB wake-up event */
    XTI_PORT2_8,     /* Port 2.8 */
    XTI_PORT2_9,     /* Port 2.9 */
    XTI_PORT2_10,    /* Port 2.10 */
    XTI_PORT2_11,    /* Port 2.11 */
    XTI_PORT1_11,    /* Port 1.11 */
    XTI_PORT1_13,    /* Port 1.13 */
    XTI_PORT1_14,    /* Port 1.14 */
    XTI_PORT0_1,     /* Port 0.1 */
    XTI_PORT0_2,     /* Port 0.2 */
    XTI_PORT0_6,     /* Port 0.6 */
    XTI_PORT0_8,     /* Port 0.8 */
    XTI_PORT0_10,    /* Port 0.10 */
    XTI_PORT0_13,    /* Port 0.13 */
    XTI_PORT0_15     /* Port 0.15 */
} XTI_SOURCE;

enum irq_sense { IRQ_FALLING, IRQ_RISING };
#endif /* _ASM_ */

/********/
/* GPIO */
/********/

#define IOPORT0_REG_BASE (APB2_REG_BASE + 0x3000)
#define IOPORT1_REG_BASE (APB2_REG_BASE + 0x4000)
#define IOPORT2_REG_BASE (APB2_REG_BASE + 0x5000)

#define GPIO_PORT_DATA_OFFSET 0x0c
#define P1_4_MASK 0x10

#ifndef _ASM_
typedef struct _IOPortRegisterMap {
    volatile UINT16 PC0;
    volatile UINT16 pad0;
    volatile UINT16 PC1;
    volatile UINT16 pad1;
    volatile UINT16 PC2;
    volatile UINT16 pad2;
    volatile UINT16 PD;
    volatile UINT16 pad3;
} IOPortRegisterMap;

typedef enum _Gpio_PinModes {
  GPIO_HI_AIN_TRI,
  GPIO_IN_TRI_TTL,
  GPIO_IN_TRI_CMOS,
  GPIO_INOUT_WP,
  GPIO_OUT_OD,
  GPIO_OUT_PP,
  GPIO_AF_OD,
  GPIO_AF_PP
} Gpio_PinModes;

#define GPIO_SET(p,b) (((IOPortRegisterMap *)(IOPORT##p##_REG_BASE))->PD |= BIT(b))
#define GPIO_CLR(p,b) (((IOPortRegisterMap *)(IOPORT##p##_REG_BASE))->PD &= ~BIT(b))

#endif /* _ASM_ */

/*******/
/* ADC */
/*******/

/*********/
/* TIMER */
/*********/

#define TIMER0_REG_BASE (APB2_REG_BASE + 0x9000)
#define TIMER1_REG_BASE (APB2_REG_BASE + 0xA000)
#define TIMER2_REG_BASE (APB2_REG_BASE + 0xB000)
#define TIMER3_REG_BASE (APB2_REG_BASE + 0xC000)

#define COUNTER_OFFSET 0x10
#define STATUS_OFFSET  0x1c
#define INPUT_CAPTURE_A_OR_B 0x9000

#ifndef _ASM_
typedef struct _TimerRegisterMap {
    volatile UINT16 InputCaptureA;
    volatile UINT16 padICA;
    volatile UINT16 InputCaptureB;
    volatile UINT16 padICB;
    volatile UINT16 OutputCompareA;
    volatile UINT16 padOCA;
    volatile UINT16 OutputCompareB;
    volatile UINT16 padOCB;
    volatile UINT16 Counter;
    volatile UINT16 padCounter;
    volatile UINT16 ControlRegister1;
    volatile UINT16 padCR1;
    volatile UINT16 ControlRegister2;
    volatile UINT16 padCR2;
    volatile UINT16 StatusRegister;
} TimerRegisterMap;

typedef union _TimerControlRegister1 {
    UINT16 value;
    struct {
        UINT16 ExternalClockEnable:1;
        UINT16 ExternalClockEdge:1;
        UINT16 InputEdgeA:1;
        UINT16 InputEdgeB:1;
        UINT16 PulseWidthModulation:1;
        UINT16 OnePulseMode:1;
        UINT16 OutputCompareAEnable:1;
        UINT16 OutputComapreBEnable:1;
        UINT16 OutputLevelA:1;
        UINT16 OutputLevelB:1;
        UINT16 ForcedOutputComapreA:1;
        UINT16 ForcedOutputCompareB:1;
        UINT16 Reserved:2;
        UINT16 PWMI:1;
        UINT16 TimerCountEnable:1;
    };
} TimerControlRegister1;
#define TIMER_COUNT_ENABLE BIT(15)

typedef union _TimerControlRegister2 {
    UINT16 value;
    struct {
        UINT16 PrescalerDivisionFactor:8;
        UINT16 Reserved:3;
        UINT16 OutputCompareBInterruptEnalbe:1;
        UINT16 InputCaptureBInterruptEnable:1;
        UINT16 TimerOverflowInterruptEnable:1;
        UINT16 OutputCompareAInterruptEnalbe:1;
        UINT16 InputCaptureAInterruptEnable:1;
    };
} TimerControlRegister2;

typedef enum _TimerStatusRegisterValues {
    InputCaptureFlagA       = BIT(15),
    OutputCompareFlagA  = BIT(14),
    TimerOverflow           = BIT(13),
    InputCaptureFlagB       = BIT(12),
    OutputCompareFlagB  = BIT(11),
} _TimerStatusRegisterValues;
#endif /* _ASM_ */

/*******/
/* RTC */
/*******/

/*******/
/* WDG */
/*******/

/*******/
/* EIC */
/*******/
#define EIC_REG_BASE        (0xfffff800)

#define EIC_ICR_OFFSET   0x00
#define EIC_CICR_OFFSET  0x04
#define EIC_CIPR_OFFSET  0x08
#define EIC_IVR_OFFSET   0x18
#define EIC_FIR_OFFSET   0x1C
#define EIC_IER_OFFSET   0x20
#define EIC_IPR_OFFSET   0x40
#define EIC_SIR_OFFSET   0x60

#ifndef _ASM_
typedef struct {
    volatile unsigned long icr;
    volatile unsigned long cicr;
    volatile unsigned long cipr;
    volatile unsigned long pad1[3];
    volatile unsigned long ivr;     /* 0x18 */
    volatile unsigned long fir;
    volatile unsigned long ier;
    volatile unsigned long pad2[7];
    volatile unsigned long ipr;     /* 0x40 */
    volatile unsigned long pad3[7];
    volatile unsigned long sir[32]; /* 0x60 */
} EICREGS;

/* STR712 interrupt sources */
typedef enum {
    EIC_TIMER0,      /* Timer 0 */
    EIC_FLASH,       /* Flash */
    EIC_PRCCU,       /* PRCCU */
    EIC_RTC,         /* Real Time Clock */
    EIC_WDG,         /* Watchdog Timer */
    EIC_XTI,         /* External interrupt */
    EIC_USB_HP,      /* USB high priority */
    EIC_I2C0ERR,     /* I2C 0 error */
    EIC_I2C1ERR,     /* I2C 1 error */
    EIC_UART0,       /* UART 0 */
    EIC_UART1,       /* UART 1 */
    EIC_UART2,       /* UART 2 */
    EIC_UART3,       /* UART 3 */
    EIC_SPI0,        /* SPI 0 */
    EIC_SPI1,        /* SPI 1 */
    EIC_I2C0,        /* I2C 0 Rx/Tx */
    EIC_I2C1,        /* I2C 1 Rx/Tx */
    EIC_CAN,         /* CAN */
    EIC_ADC,         /* ADC */
    EIC_TIMER1,      /* Timer 1 */
    EIC_TIMER2,      /* Timer 2 */
    EIC_TIMER3,      /* Timer 3 */
    EIC_RESERVED1,   /* Reserved */
    EIC_RESERVED2,   /* Reserved */
    EIC_RESERVED3,   /* Reserved */
    EIC_HDLC,        /* HDLC */
    EIC_USB_LP,      /* USB low priority */
    EIC_RESERVED4,   /* Reserved */
    EIC_RESERVED5,   /* Reserved */
    EIC_TIMER0_OVF,  /* Timer 0 overflow */
    EIC_TIMER0_OC1,  /* Timer 0 output compare 1 */
    EIC_TIMER0_OC2   /* Timer 0 output compare 2 */
} EIC_SOURCE;
#endif /* _ASM_ */

#define IRQ_EN      0x1
#define FIQ_EN      0x2

/*********/
/* FLASH */
/*********/
#define FLASH_REG_BASE   (0x40100000)

#define FLASH_CR0_OFFSET 0x00
#define FLASH_CR1_OFFSET 0x04
#define FLASH_DR0_OFFSET 0x08
#define FLASH_DR1_OFFSET 0x0C
#define FLASH_AR_OFFSET  0x10
#define FLASH_ER_OFFSET  0x14

#define FPROT_REG_BASE   (0x4010DFB0)

#define FPROT_NVWPAR_OFFSET 0x00
#define FPROT_NVAPR0_OFFSET 0x08
#define FPROT_NVAPR1_OFFSET 0x0C


#ifndef _ASM_
typedef struct {
    volatile unsigned long cr0;
    volatile unsigned long cr1;
    volatile unsigned long dr0;
    volatile unsigned long dr1;
    volatile unsigned long ar;
    volatile unsigned long er;
} FLASHREGS;

typedef struct {
    volatile unsigned long nvwpar;
    volatile unsigned long pad;
    volatile unsigned long nvapr0; /* 0x08 */
    volatile unsigned long nvapr1;
} FPROTREGS;
#endif /* _ASM_ */

/* CR0 Bits */
#define FLASHCMD_START      0x80000000
#define FLASHCMD_SUSPEND    0x40000000

#define FLASHCMD_WPG            0x20000000
#define FLASHCMD_DWPG       0x10000000
#define FLASHCMD_SER            0x08000000
#define FLASHCMD_SPR            0x01000000

#define FLASH_BSY1          0x00000004
#define FLASH_BSY0          0x00000002
#define FLASH_BSY           0x00000016

/* CR1 Bits */
#define FLASH_B1STAT            0x02000000
#define FLASH_B0STAT            0x01000000

#define FLASH_B1F1          0x00020000
#define FLASH_B1F0          0x00010000

#define FLASH_B0F7          0x00000080
#define FLASH_B0F6          0x00000040
#define FLASH_B0F5          0x00000020
#define FLASH_B0F4          0x00000010
#define FLASH_B0F3          0x00000008
#define FLASH_B0F2          0x00000004
#define FLASH_B0F1          0x00000002
#define FLASH_B0F0          0x00000001

#define FLASH_ARMASK            0x001FFFFC
#define FLASH_ERMASK       0x000001CF

#define BANK1_ADDR          0x000C0000
#define BANK1F1             0x00002000

#define BANK0F1


#endif /* STR712_H */
