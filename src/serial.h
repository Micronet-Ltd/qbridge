#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "queue.h"

#define SERIAL_IRQ_PRIORITY 8

typedef struct _SerialPort {
	UARTRegisterMap volatile * port;
	CircleQueue rxQueue;
	CircleQueue txQueue;
} SerialPort;

extern SerialPort com1;
extern SerialPort com2;
extern SerialPort com3;
extern SerialPort com4;


void InitializeAllSerialPorts();
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits);
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng);
void StuffTxFifo(SerialPort *port);
void ProcessRxFifo (SerialPort *port);

void DebugPrint(char *formatStr, ...);

extern inline bool PortTxFifoFull(SerialPort *port);
extern inline bool PortRxFifoNotEmpty(SerialPort *port);

void Com1IRQ();
void Com2IRQ();
void Com3IRQ();
void Com4IRQ();
void HandleComIRQ(SerialPort *port);


#endif // SERIAL_H
