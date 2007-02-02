/*******************************************************************/
/*                                                                 */
/* File:  serial.c                                                 */
/*                                                                 */
/* Description: QBridge serial drivers (bootloader version)        */
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
/* 04-Nov-05  MKE            1.1     Stripped down for bootloader  */
/*******************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "serial.h"

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

static void InitializePort (SerialPort *port);

SerialPort com4;
IOPortRegisterMap * ioPort0 = (IOPortRegisterMap *)IOPORT0_REG_BASE;

SerialPort *hostPort=NULL;

void GPIO_Config (IOPortRegisterMap *GPIOx, UINT16 Port_Pins, Gpio_PinModes GPIO_Mode) {
  switch (GPIO_Mode)
  {
      /* Most of these are omitted from the bootloader version to save space */
    case GPIO_HI_AIN_TRI:
    case GPIO_IN_TRI_TTL:
    case GPIO_INOUT_WP:
    case GPIO_OUT_OD:
    case GPIO_OUT_PP:
    case GPIO_AF_OD:
         break;

    case GPIO_IN_TRI_CMOS:
      GPIOx->PC0&=~Port_Pins;
      GPIOx->PC1|=Port_Pins;
      GPIOx->PC2&=~Port_Pins;
      break;

    case GPIO_AF_PP:
      GPIOx->PC0|=Port_Pins;
      GPIOx->PC1|=Port_Pins;
      GPIOx->PC2|=Port_Pins;
      break;
  }
}

/**********************/
/* InitializeHostPort */
/**********************/
void InitializeHostPort()
{
    hostPort     = &com4;

    com4.port = (UARTRegisterMap volatile * const)(UART3_REG_BASE);

    // OK, this bit of magic was not documented anywhere that Mike & I could find.  I finally dredged up some source
    // code off the internet which set this pins in this manner and it seemed to make everything work.
    // Note:  We may want to change one of these once we decide on a 485 port.  We should probably 1/2 duplex everything on that port
    // Configure the GPIO transmit pins as alternate function push pull
   GPIO_Config(ioPort0, UART3_Tx_Pin, GPIO_AF_PP);
    // Configure the GPIO receive pins as Input Tristate CMOS
   GPIO_Config(ioPort0, UART3_Rx_Pin, GPIO_IN_TRI_CMOS);

    InitializePort(&com4);
}

/*******************/
/* InitializePort */
/*****************/
static void InitializePort (SerialPort *port)
{
    SetPortSettings(port, 115200l, 8, 'N', 1);
    port->port->intEnable = 0;
    port->port->guardTime = 0;
    port->port->timeout = 8;
}

/*****************/
/* ShutdownPort  */
/*****************/
void ShutdownPort (SerialPort *port)
{
    port->port->portSettings &= ~BIT(8);
}

/********************/
/* SetPortSettings */
/******************/
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits)
{
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

    port->port->BaudRate = divisor;
    port->port->portSettings = map.value;
    port->port->txReset = 0;
    port->port->rxReset = 0;
    return true;
}



/*************/
/* Transmit */
/***********/
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng)
{
    while (leng--) {
        port->port->txBuffer = *data++;
        while (PortTxFifoFull(port)) ; /* Wait for space if necessary */
    }
    return 0;
}

/*******************/
/* PortTxFifoFull */
/*****************/
bool PortTxFifoFull(SerialPort *port) {
    return (port->port->status & TxFull) != 0;
}

/****************/
/* PollReceive  */
/****************/
void PollReceive (SerialPort *port, unsigned char *rxbuf, int maxNChars,int *numreceived)
{
    *numreceived = 0;

    while (PortRxFifoNotEmpty(port) && *numreceived < maxNChars) {
        *rxbuf = port->port->rxBuffer;
        ++rxbuf;
        ++(*numreceived);
    }

}

/***********************/
/* PortRxFifoNotEmpty */
/*********************/
inline bool PortRxFifoNotEmpty(SerialPort *port) {
    return (port->port->status & RxBufNotEmpty) != 0;
}

/******************/
/* IsTxFifoEmpty */
/****************/
inline bool IsTxFifoEmpty(SerialPort *port) {
    return (port->port->status & TxEmpty) != 0;
}

/*********/
/* print */
/*********/
void print(char *formatStr, ...)
{
    char buf[256];
    int len;
    va_list ap;

    va_start(ap, formatStr);
    vsnprintf(buf, 255, formatStr, ap);
    va_end(ap);
    len = strlen(buf);

    Transmit (hostPort, buf, len);
}
