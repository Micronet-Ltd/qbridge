#ifndef INTERRUPT_H
#define INTERRUPT_H
//#############################################################################
//#### Interrupt priority defines for the qbridge
//#############################################################################
//
//since the J1708 bus is so time sensitive as far as getting off, especially
//when a collision occurs, we have had to enable our interrupts to be nested.
//But we don't want every interrupt to be able to interrupt the other interrupts
//since so much testing has been done without it.  The only interrupt we want
//to be able to interrupt the others is the J1708 com port interrupt so we can
//quickly test for a collision and get off the bus if one has occured.  For this
//reason, all our interrupts will be programmed with the same priority except
//for the J1708 comport interrupt.  If multiple interrupts are pending when
//enabled, our hardware will prioritize them as follows (from low to hi):
//  debug serial port, host serial port, CAN bus interrupt, timer 1 interrupt
//
#define MAX_INT_NEST_LEVEL 2
#define AN_INT_STACK_SIZE 1024 //words = 4k bytes

#define TIMER_IRQ_PRIORITY  8
#define CAN_IRQ_PRIORITY    8
#define SERIAL_IRQ_PRIORITY 8
#define J1708_SERIAL_IRQ_PRIORITY 9    //highest priority so we can get off the bus when collision occurs



#endif
