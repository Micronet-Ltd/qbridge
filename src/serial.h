#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "queue.h"


typedef struct _SerialPort {
	UARTRegisterMap volatile * port;
	CircleQueue rxQueue;
	CircleQueue txQueue;
} SerialPort;

extern SerialPort com1;
extern SerialPort com2;


void InitializeAllSerialPorts();
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits);
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng);
void StuffTxFifo(SerialPort *port);
void DebugPrint(char *formatStr, ...);

extern inline bool PortTxFifoFull(SerialPort *port);


#endif // SERIAL_H
