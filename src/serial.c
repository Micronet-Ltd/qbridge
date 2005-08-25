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

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

// Assumes a baud clock of 14.7456 MHz
/*#define bauddiv_460800  0x2
#define bauddiv_230400  0x4 
#define bauddiv_115200  0x8
#define bauddiv_57600   0x10
#define bauddiv_38400   0x18
#define bauddiv_19200   0x30
#define bauddiv_14400   0x40
#define bauddiv_9600    0x60
#define bauddiv_4800    0xc0
#define bauddiv_2400    0x180
#define bauddiv_1200    0x300
#define bauddiv_600     0x600
*/

// Assumes a baud clock of 16.0 MHz
#define bauddiv_460800  2
#define bauddiv_230400  4 
#define bauddiv_115200  9
#define bauddiv_57600   17
#define bauddiv_38400   26
#define bauddiv_19200   52
#define bauddiv_14400   69
#define bauddiv_9600    104
#define bauddiv_4800    208
#define bauddiv_2400    417
#define bauddiv_1200    833
#define bauddiv_600     1667


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

static void InitializePort (SerialPort *port, EIC_SOURCE src, void (*hdlr)(void));

SerialPort com1;
SerialPort com2;
SerialPort com3;
SerialPort com4;
IOPortRegisterMap * ioPort0;
IOPortRegisterMap * ioPort1;

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
	// These should really be initialized as constants, but something seems broken about doing that
	ioPort0 = (IOPortRegisterMap *)IOPORT0_REG_BASE;
	ioPort1 = (IOPortRegisterMap *)IOPORT1_REG_BASE;
	com1.port = (UARTRegisterMap volatile * const)(UART0_REG_BASE);
	com2.port = (UARTRegisterMap volatile * const)(UART1_REG_BASE);
	com3.port = (UARTRegisterMap volatile * const)(UART2_REG_BASE);
	com4.port = (UARTRegisterMap volatile * const)(UART3_REG_BASE);

	// OK, this bit of magic was not documented anywhere that Mike & I could find.  I finally dredged up some source
	// code off the internet which set this pins in this manner and it seemed to make everything work.
	// Note:  We may want to change one of these once we decide on a 485 port.  We should probably 1/2 duplex everything on that port
	// Configure the GPIO transmit pins as alternate function push pull 
   GPIO_Config(ioPort0, UART0_Tx_Pin | UART1_Tx_Pin | UART2_Tx_Pin | UART3_Tx_Pin, GPIO_AF_PP);
	// Configure the GPIO receive pins as Input Tristate CMOS 
   GPIO_Config(ioPort0, UART0_Rx_Pin | UART1_Rx_Pin | UART2_Rx_Pin | UART3_Rx_Pin, GPIO_IN_TRI_CMOS);

	InitializePort(&com1, EIC_UART0, Com1IRQ);
	InitializePort(&com2, EIC_UART1, Com2IRQ);
	//InitializePort(&com3, EIC_UART2, Com3IRQ);
	//InitializePort(&com4, EIC_UART3, Com4IRQ);

}

/*******************/
/* InitializePort */
/*****************/
static void InitializePort (SerialPort *port, EIC_SOURCE src, void (*hdlr)(void)) {
	IRQSTATE saveState = 0;
	DISABLE_IRQ(saveState);
	InitializeQueue(&(port->rxQueue));
	InitializeQueue(&(port->txQueue));
	RESTORE_IRQ(saveState);

	SetPortSettings(port, 19200l, 8, 'N', 1);
	port->port->intEnable = (RxHalfFullIE | TimeoutNotEmptyIE | OverrunErrorIE | FrameErrorIE | ParityErrorIE);
	port->port->guardTime = 0;
	port->port->timeout = 8;

	RegisterEICHdlr(src, hdlr, SERIAL_IRQ_PRIORITY);
	EICEnableIRQ(src);
}

/********************/
/* SetPortSettings */
/******************/
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits) {
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
	map.run = 1;
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
	if (retVal < leng) {
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
	return (port->port->status & RxBufNotEmtpy) != 0; 
}

/***************/
/* DebugPrint */
/*************/
void DebugPrint(char *formatStr, ...) {
	char buf[256];
	int len;
	va_list ap;
	va_start(ap, formatStr);
	//strncpy(buf, formatStr, 510);
	vsnprintf(buf, 510, formatStr, ap);
	va_end(ap);
	len = strlen(buf);
	
	// Append a \r\n to the output
	buf[len] = '\r';
	buf[len+1] = '\n';
	buf[len+2] = '\0';
	len += 2;

	Transmit (&com2, buf, len);
}

/*******************/
/* DebugCorePrint */
/*****************/
void DebugCorePrint(char *toPrint) {
	Transmit (&com2, toPrint, strlen(toPrint));
}

/************/
/* Com1IRQ */
/**********/
void Com1IRQ() {
	HandleComIRQ(&com1);
	EICClearIRQ(EIC_UART0);
}

/************/
/* Com2IRQ */
/**********/
void Com2IRQ() {
	HandleComIRQ(&com2);
	EICClearIRQ(EIC_UART1);
}
/************/
/* Com3IRQ */
/**********/
void Com3IRQ() {
	HandleComIRQ(&com3);
	EICClearIRQ(EIC_UART2);
}

/************/
/* COM4IRQ */
/**********/
void Com4IRQ() {
	HandleComIRQ(&com4);
	EICClearIRQ(EIC_UART3);
}

/*****************/
/* HandleComIRQ */
/***************/
void HandleComIRQ(SerialPort *port) {
	// Data received
	if ((port->port->status & (RxHalfFull | TimeoutNotEmtpy)) != 0) {
		ProcessRxFifo(port);
	}

	// Tx Fifo partially empty
	if ((port->port->status & TxHalfEmpty) != 0) {
		StuffTxFifo(port);
	}

#ifdef DEBUG_SERIAL
	// Overrun
	if ((port->port->status & OverrunError) != 0) {
		DebugPrint ("Serial overrun error on com%d", GetPortNumber(port) );
	}

	// Framing error
	if ((port->port->status & FrameError) != 0) {
		DebugPrint ("Framing error error on com%d", GetPortNumber(port));
	}

	if ((port->port->status & ParityError) != 0) {
		DebugPrint ("Parity error on com%d", GetPortNumber(port));
	}
#endif
}
