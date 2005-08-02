#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "queue.h"

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

extern const BaudTableEntry BaudRateTable[];

struct UARTRegisterMap {
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
};

union UARTSettingsMap {
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
};

struct SerialPort {
	UARTRegisterMap volatile * const port;
	CircleQueue rxQueue;
	CircleQueue txQueue;
};

UARTRegisterMap volatile * const com1 = APB1_BASE_ADDRESS + 0x4000;
UARTRegisterMap volatile * const com2 = APB1_BASE_ADDRESS + 0x5000;

#endif // SERIAL_H