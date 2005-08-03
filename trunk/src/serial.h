#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "queue.h"


struct SerialPort {
	UARTRegisterMap volatile * const port;
	CircleQueue rxQueue;
	CircleQueue txQueue;
};

extern SerialPort com1;


void InitializePort (SerialPort *port);
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits);
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng);
void StuffTxFifo(SerialPort *port);

inline bool PortTxFifoFull(SerialPort *port) { return (port->port->status & TxFull) != 0; }


#endif // SERIAL_H