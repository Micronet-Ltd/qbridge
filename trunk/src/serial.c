/*******************************************************************/
/*                                                                 */
/* File:  serial.c                                                 */
/*                                                                 */
/* Description: QBridge serial drivers                             */
/*                                                                 */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2005 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 08-Aug-05  JBR            1.0     1st Release                   */
/*******************************************************************/

#include "serial.h"
#include "eic.h"
#include "J1708.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>



typedef struct _BaudTableEntry {
    UINT32 baud;
    UINT32 divisor;
} BaudTableEntry;

static const BaudTableEntry BaudRateTable[] = {
   { 115200l, bauddiv_115200},
   { 57600l,  bauddiv_57600},
   { 38400l,  bauddiv_38400},
   { 19200l,  bauddiv_19200},
   { 14400l,  bauddiv_14400},
   { 9600l,   bauddiv_9600},
   { 4800l,   bauddiv_4800},
   { 2400l,   bauddiv_2400},
   { 1200l,   bauddiv_1200},
   { 600l,    bauddiv_600},
};

static void InitializePort (SerialPort *port, EIC_SOURCE src, void (*hdlr)(void), bool setrun, int portpriority );
static void DisablePort(SerialPort *port);

//SerialPort com1;
SerialPort com2;
SerialPort com3;
SerialPort com4;
IOPortRegisterMap * ioPort0;
IOPortRegisterMap * ioPort1;

SerialPort *debugPort=NULL;
SerialPort *j1708Port=NULL;
SerialPort *hostPort=NULL;

void GPIO_Config (IOPortRegisterMap *GPIOx, UINT16 Port_Pins, Gpio_PinModes GPIO_Mode) {
  switch (GPIO_Mode)
  {
    case GPIO_HI_AIN_TRI:
      GPIOx->PC0&=~Port_Pins;
      GPIOx->PC1&=~Port_Pins;
      GPIOx->PC2&=~Port_Pins;
      break;

    case GPIO_IN_TRI_TTL:
      GPIOx->PC0|=Port_Pins;
      GPIOx->PC1&=~Port_Pins;
      GPIOx->PC2&=~Port_Pins;
      break;

    case GPIO_IN_TRI_CMOS:
      GPIOx->PC0&=~Port_Pins;
      GPIOx->PC1|=Port_Pins;
      GPIOx->PC2&=~Port_Pins;
      break;

    case GPIO_INOUT_WP:
      GPIOx->PC0|=Port_Pins;
      GPIOx->PC1|=Port_Pins;
      GPIOx->PC2&=~Port_Pins;
      break;

    case GPIO_OUT_OD:
      GPIOx->PC0&=~Port_Pins;
      GPIOx->PC1&=~Port_Pins;
      GPIOx->PC2|=Port_Pins;
      break;

    case GPIO_OUT_PP:
      GPIOx->PC0|=Port_Pins;
      GPIOx->PC1&=~Port_Pins;
      GPIOx->PC2|=Port_Pins;
      break;

    case GPIO_AF_OD:
      GPIOx->PC0&=~Port_Pins;
      GPIOx->PC1|=Port_Pins;
      GPIOx->PC2|=Port_Pins;
      break;

    case GPIO_AF_PP:
      GPIOx->PC0|=Port_Pins;
      GPIOx->PC1|=Port_Pins;
      GPIOx->PC2|=Port_Pins;
      break;
  }
}


/*****************************/
/* InitializeAllSerialPorts */
/***************************/
void InitializeAllSerialPorts() {
    debugPort = &com3;
    hostPort  = &com4;
    j1708Port = &com2;

    // These should really be initialized as constants, but something seems broken about doing that
    ioPort0 = (IOPortRegisterMap *)IOPORT0_REG_BASE;
    ioPort1 = (IOPortRegisterMap *)IOPORT1_REG_BASE;
    //com1.port = (UARTRegisterMap volatile * const)(UART0_REG_BASE);
    com2.port = (UARTRegisterMap volatile * const)(UART1_REG_BASE);
    com3.port = (UARTRegisterMap volatile * const)(UART2_REG_BASE);
    com4.port = (UARTRegisterMap volatile * const)(UART3_REG_BASE);

    // OK, this bit of magic was not documented anywhere that Mike & I could find.  I finally dredged up some source
    // code off the internet which set this pins in this manner and it seemed to make everything work.
    // Note:  We may want to change one of these once we decide on a 485 port.  We should probably 1/2 duplex everything on that port
//#ifdef _DEBUG
   // Configure the GPIO transmit pins as alternate function push pull
   GPIO_Config(ioPort0, /*UART0_Tx_Pin |*/ UART1_Tx_Pin | UART2_Tx_Pin | UART3_Tx_Pin, GPIO_AF_PP);
    // Configure the GPIO receive pins as Input Tristate CMOS
   GPIO_Config(ioPort0, /*UART0_Rx_Pin |*/ UART1_Rx_Pin | UART2_Rx_Pin | UART3_Rx_Pin, GPIO_IN_TRI_CMOS);
//#else
//   // Configure the GPIO transmit pins as alternate function push pull
//   GPIO_Config(ioPort0, /*UART0_Tx_Pin |*/ UART1_Tx_Pin | UART3_Tx_Pin, GPIO_AF_PP);
//    // Configure the GPIO receive pins as Input Tristate CMOS
//   GPIO_Config(ioPort0, /*UART0_Rx_Pin |*/ UART1_Rx_Pin | UART3_Rx_Pin, GPIO_IN_TRI_CMOS);
//#endif

    //InitializePort(&com1, EIC_UART0, Com1IRQ, TRUE, SERIAL_IRQ_PRIORITY);
    InitializePort(&com2, EIC_UART1, Com2IRQ, FALSE, J1708_SERIAL_IRQ_PRIORITY);   //Important not to start the J1708 port until a timer can be started at the same time
    InitializePort(&com3, EIC_UART2, Com3IRQ, TRUE, SERIAL_IRQ_PRIORITY);
    InitializePort(&com4, EIC_UART3, Com4IRQ, TRUE, SERIAL_IRQ_PRIORITY);
}

/*******************/
/* InitializePort */
/*****************/
static void InitializePort (SerialPort *port, EIC_SOURCE src, void (*hdlr)(void), bool setrun, int portpriority) {
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    InitializeQueue(&(port->rxQueue));
    InitializeQueue(&(port->txQueue));
    RESTORE_IRQ(saveState);

    SetPortSettings(port, 115200l, 8, 'N', 1, setrun);
    port->port->intEnable = (RxHalfFullIE | TimeoutNotEmptyIE | OverrunErrorIE | FrameErrorIE | ParityErrorIE);
    port->port->guardTime = 0;
    port->port->timeout = 8;

    RegisterEICHdlr(src, hdlr, portpriority);
    EICEnableIRQ(src);
}

/*************************/
/* DisableAllSerialPorts */
/*************************/
void DisableAllSerialPorts(void)
{
    DisablePort(debugPort);
    DisablePort(hostPort);
    DisablePort(j1708Port);
}

/***************/
/* DisablePort */
/***************/
static void DisablePort(SerialPort *port)
{
    port->port->intEnable = 0;
    port->port->portSettings = 0;
}

/********************/
/* SetPortSettings */
/******************/
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits, bool setrun ) {
    UINT8 modeSettings[3][3] = {
        { 0x0, 0x3, 0x3 }, // 7 Data Bits ( N E O )
        { 0x1, 0x7, 0x7 }, // 8 Data Bits ( N E O )
        { 0x4, 0x0, 0x0 }, // 9 Data Bits ( N E O )
    };
    int i;
    int parityIndex = -1;
    UINT32 divisor= 0;

    for (i = 0; i < ARRAY_SIZE(BaudRateTable); i++) {
        if (BaudRateTable[i].baud == baud) {
            divisor = BaudRateTable[i].divisor;
            break;
        }
    }

    if (divisor == 0) {
        return false;
    }

    UARTSettingsMap map;
    map.reserved2 = 0;
    map.fifoEnable = 1;
    map.reserved1 = 0;
    map.rxEnable = 1;
    if( setrun ) map.run = 1; else map.run = 0;
    map.loopBack = 0;

    if (stopBits == 1) {
        map.stopBits = 0x1;
    } else if (stopBits == 2) {
        map.stopBits = 0x03;
    } else {
        return false;
    }

    switch (parity) {
    case 'E':
        parityIndex = 1;
        map.parityOdd = 0;
        break;
    case 'O':
        parityIndex = 2;
        map.parityOdd = 1;
        break;
    case 'N':
        parityIndex = 0;
        map.parityOdd = 0;
        break;
    }

    if ((parityIndex < 0) || (dataBits < 7) || (dataBits > 9)) {
        return false;
    }
    map.mode = modeSettings[dataBits-7][parityIndex];

    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    port->port->BaudRate = divisor;
    port->port->portSettings = map.value;
    port->port->txReset = 0;
    port->port->rxReset = 0;
    RESTORE_IRQ(saveState);
    return true;
}

/*************/
/* Transmit */
/***********/
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng) {
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    UINT32 retVal = Enqueue(&(port->txQueue), data, leng);
#ifdef DEBUG_SERIAL
    if ((retVal < leng) && (port != debugPort)) {
        DebugPrint ("Serial com%d transmit queue has overrun.", GetPortNumber(port));
    }
#endif
    StuffTxFifo(port);
    RESTORE_IRQ(saveState);
    return retVal;
}

/****************/
/* StuffTxFifo */
/**************/
void StuffTxFifo(SerialPort *port) {
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    while (!QueueEmpty(&(port->txQueue)) && !PortTxFifoFull(port)) {
        port->port->txBuffer = DequeueOne(&port->txQueue);
    }

    // Enable/Disable the interrupt as appropraite
    if (QueueEmpty(&(port->txQueue))) {
        port->port->intEnable &= ~TxHalfEmptyIE;
    } else {
        port->port->intEnable |= TxHalfEmptyIE;
    }
    RESTORE_IRQ(saveState);
}

/*******************/
/* PortTxFifoFull */
/*****************/
bool PortTxFifoFull(SerialPort *port) {
    return (port->port->status & TxFull) != 0;
}

UINT8 portValue;
/******************/
/* ProcessRxFifo */
/****************/
void ProcessRxFifo (SerialPort *port) {
    IRQSTATE saveState = 0;
    DISABLE_IRQ(saveState);
    while (PortRxFifoNotEmpty(port)) {
        portValue = port->port->rxBuffer;
        Enqueue (&(port->rxQueue), &portValue, 1);
    }
#ifdef DEBUG_SERIAL
    if (QueueFull(&(port->rxQueue))) {
        DebugPrint ("Serial com%d receive queue is full.", GetPortNumber(port));
    }
#endif
    RESTORE_IRQ(saveState);
}

/***********************/
/* PortRxFifoNotEmpty */
/*********************/
inline bool PortRxFifoNotEmpty(SerialPort *port) {
    return (port->port->status & RxBufNotEmpty) != 0;
}

#ifdef _DEBUG
/****************/
/* AssertPrint */
/**************/
void _AssertPrint (bool assertExpression, char *assertStr, char *file, int line, char *formatStr, ...) {
    if (assertExpression) {
        return;
    }
    char buf[256] = "    #";
    int len;

    DebugPrint ("Assert warning: (%s) tested false (%s line %d)", assertStr, file, line);
    len = strlen(buf);
    va_list ap;
    va_start(ap, formatStr);
    vsnprintf (buf+len, 251, formatStr, ap);
    len = strlen(buf);
    va_end(ap);
    buf[len+0] = '#';
    buf[len+1] = '\r';
    buf[len+2] = '\n';
    buf[len+3] = '\0';
    len += 3;

    Transmit (debugPort, buf, len);
}

/***************/
/* DebugPrint */
/*************/
void _DebugPrint(char *formatStr, ...) {
    char buf[256] = "* ";
    int len;
    va_list ap;
    va_start(ap, formatStr);
    //strncpy(buf, formatStr, 255);
    vsnprintf(buf+2, 249, formatStr, ap);
    va_end(ap);
    len = strlen(buf);

    if (len < 3) {
        extern const unsigned char BuildDateStr[];
        strcpy (buf+len, formatStr);
        len = strlen(buf);
        strcpy(buf+len, "<need reboot?>  ");
        len = strlen(buf);
        strcpy(buf+len, BuildDateStr);
        len = strlen(buf);
    }
    // Append a \r\n to the output
    buf[len+0] = ' ';
    buf[len+1] = '*';
    buf[len+2] = '\r';
    buf[len+3] = '\n';
    buf[len+4] = '\0';
    len += 4;

    Transmit (debugPort, buf, len);
}

/*******************/
/* DebugCorePrint */
/*****************/
void _DebugCorePrint(char *toPrint) {
    Transmit (debugPort, toPrint, strlen(toPrint));
}
#endif

static void comIRQHandle_with_reenable( void (*fp)( SerialPort *x ), SerialPort *mycom ){
    SETUP_NEST_INTERRUPT( AN_INT_STACK_SIZE * sizeof(int) );
    (*fp)( mycom );
    UNSET_NEST_INTERRUPT();
}

/************/
/* Com1IRQ */
/**********/
/*void Com1IRQ() {
//    HandleComIRQ(&com1);
    comIRQHandle_with_reenable( HandleComIRQ, &com1 );
    EICClearIRQ(EIC_UART0);
}*/

/************/
/* Com2IRQ */
/**********/
void Com2IRQ() {
    J1708ComIRQHandle();    //since this is our highest priority interrupt, we don't bother to reenable interrupts
    EICClearIRQ(EIC_UART1);
}
/************/
/* Com3IRQ */
/**********/
void Com3IRQ() {
    //HandleComIRQ(&com3);
    comIRQHandle_with_reenable( HandleComIRQ, &com3 );
    EICClearIRQ(EIC_UART2);
}

/************/
/* COM4IRQ */
/**********/
void Com4IRQ() {
    //HandleComIRQ(&com4);
    comIRQHandle_with_reenable( HandleComIRQ, &com4 );
    EICClearIRQ(EIC_UART3);
}

/*****************/
/* HandleComIRQ */
/***************/
void HandleComIRQ(SerialPort *port) {
    bool handled = false;
    // Data received
    if ((port->port->status & (RxHalfFull | TimeoutNotEmtpy)) != 0) {
        ProcessRxFifo(port);
        handled = true;
    }

    // Tx Fifo partially empty
    if ((port->port->status & TxHalfEmpty) != 0) {
        StuffTxFifo(port);
        handled = true;
    }

    // Overrun
    if ((port->port->status & OverrunError) != 0) {
        (void)port->port->rxBuffer; //this is what actually clears this interrupt
        port->port->rxReset = 0;    //must do something to clear the interrupt or we will hang the system!
        DebugPrint ("Serial overrun error on com%d", GetPortNumber(port) );
        handled = true;
    }

    // Framing error
    if ((port->port->status & FrameError) != 0) {
        (void)port->port->rxBuffer;
        port->port->rxReset = 0;    //must do something to clear the interrupt or we will hang the system!
        DebugPrint ("Framing error error on com%d", GetPortNumber(port));
        handled = true;
    }

    if ((port->port->status & ParityError) != 0) {
        (void)port->port->rxBuffer;
        port->port->rxReset = 0;    //must do something to clear the interrupt or we will hang the system!
        DebugPrint ("Parity error on com%d", GetPortNumber(port));
        handled = true;
    }

    if (!handled) {
        if (port != debugPort) {
            DebugPrint ("Unknown interrupt on com%d.  IntEnable=%04X  Status=%04X", GetPortNumber(port), port->port->intEnable, port->port->status);
        }
    }

}

/******************/
/* IsTxFifoEmpty */
/****************/
inline bool IsTxFifoEmpty(SerialPort *port) {
    return (port->port->status & TxEmpty) != 0;
}
