#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "queue.h"

#define SERIAL_IRQ_PRIORITY 8

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


// Assumes a baud clock of 16.0 MHz
/*#define bauddiv_460800  2
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
*/

typedef struct _SerialPort {
	UARTRegisterMap volatile * port;
	CircleQueue rxQueue;
	CircleQueue txQueue;
} SerialPort;

extern SerialPort *debugPort;
extern SerialPort *j1708Port;
extern SerialPort *hostPort;

void InitializeAllSerialPorts();
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits);
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng);
void StuffTxFifo(SerialPort *port);
void ProcessRxFifo (SerialPort *port);

extern inline bool IsTxFifoEmpty(SerialPort *port);


void DebugPrint(char *formatStr, ...);
void DebugCorePrint(char *toPrint);

extern inline bool PortTxFifoFull(SerialPort *port);
extern inline bool PortRxFifoNotEmpty(SerialPort *port);

void Com1IRQ() __attribute__ ((interrupt("IRQ")));
void Com2IRQ() __attribute__ ((interrupt("IRQ")));
void Com3IRQ() __attribute__ ((interrupt("IRQ")));
void Com4IRQ() __attribute__ ((interrupt("IRQ")));
void HandleComIRQ(SerialPort *port);

static inline int GetPortNumber (SerialPort *port) { return (((UINT32)(port->port)-UART0_REG_BASE)/(UART1_REG_BASE-UART0_REG_BASE)) + 1; }


#endif // SERIAL_H
