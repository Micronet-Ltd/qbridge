#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"
#include "queue.h"

#define SERIAL_IRQ_PRIORITY 8

// Assumes a baud clock of 24.0000 MHz
//#define bauddiv_460800  3.25  // %error = 7.7 --too large
//#define bauddiv_230400  6.5   // %error = 7.5 --too large to work
#define bauddiv_115200  13      // %error = .16
#define bauddiv_57600   26      // %error = .16
#define bauddiv_38400   39      // %error = .16
#define bauddiv_19200   78      // %error = .16
#define bauddiv_14400   104     // %error = .16
#define bauddiv_9600    156     // %error = .16
#define bauddiv_4800    313     // %error = .16
#define bauddiv_2400    625     // %error = 0
#define bauddiv_1200    1250    // %error = 0
#define bauddiv_600     2500    // %error = 0

typedef struct _SerialPort {
    UARTRegisterMap volatile * port;
    CircleQueue rxQueue;
    CircleQueue txQueue;
} SerialPort;

extern SerialPort *debugPort;
extern SerialPort *j1708Port;
extern SerialPort *hostPort;

void InitializeAllSerialPorts();
void DisableAllSerialPorts(void);
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits, bool setrun);
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng);
void StuffTxFifo(SerialPort *port);
void ProcessRxFifo (SerialPort *port);

extern inline bool IsTxFifoEmpty(SerialPort *port);

#ifdef _DEBUG
#define AssertPrint(assertExpression, args...) _AssertPrint (assertExpression, #assertExpression, __FILE__, __LINE__, args)
#else
#define AssertPrint(assertExpression, args...)
#endif

void _AssertPrint (bool assertExpression, char *assertStr, char *file, int line, char *formatStr, ...);
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
