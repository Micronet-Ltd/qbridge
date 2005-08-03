#include "serial.h"

// Assumes a baud clock of 14.7456 MHz
#define bauddiv_460800  0x2
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


struct BaudTableEntry {
	UINT32 baud;
	UINT32 divisor;
};

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


SerialPort com1 = { (UARTRegisterMap volatile * const)(UART0_REG_BASE) };

/*******************/
/* InitializePort */
/*****************/
void InitializePort (SerialPort *port) {
	IRQSTATE saveState = 0;
	DISABLE_IRQ(saveState);
	InitializeQueue(&(port->rxQueue));
	InitializeQueue(&(port->txQueue));
	RESTORE_IRQ(saveState);

	SetPortSettings(port, 19200, 8, 'N', 1);
	port->port->intEnable = (RxHalfFullIE | TimeoutNotEmptyIE | OverrunErrorIE | FrameErrorIE | ParityErrorIE | TxHalfEmptyIE);
	port->port->guardTime = 0;
	port->port->timeout = 8;
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
	UINT32 divisor=0;
	int parityIndex = -1;

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
	RESTORE_IRQ(saveState);
}