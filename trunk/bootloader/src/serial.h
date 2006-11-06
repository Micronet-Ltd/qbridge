#ifndef SERIAL_H
#define SERIAL_H

#include "common.h"

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
} SerialPort;

extern SerialPort *hostPort;

void InitializeHostPort();
void ShutdownPort (SerialPort *port);
bool SetPortSettings (SerialPort *port, UINT32 baud, UINT8 dataBits, UINT8 parity, UINT8 stopBits);
UINT32 Transmit (SerialPort *port, UINT8 *data, int leng);
void print(char *formatStr, ...);
void PollReceive (SerialPort *port, unsigned char *rxbuf, int maxNChars,int *numreceived);



extern inline bool IsTxFifoEmpty(SerialPort *port);
extern inline bool PortTxFifoFull(SerialPort *port);
extern inline bool PortRxFifoNotEmpty(SerialPort *port);

static inline int GetPortNumber (SerialPort *port) { return (((UINT32)(port->port)-UART0_REG_BASE)/(UART1_REG_BASE-UART0_REG_BASE)) + 1; }


#endif // SERIAL_H
