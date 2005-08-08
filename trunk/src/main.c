/*******************************************************************/
/*                                                                 */
/* File:  api.c                                                    */
/*                                                                 */
/* Description: STR712 Qbridge (J1708/J1939 to serial converter)   */
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
/* 08-Aug-05  JBR/MKE        1.0     1st Release                   */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <stdlib.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "serial.h"

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/

/********************/
/* Global Variables */
/********************/
extern unsigned char BuildDate[];

const char CopyrightMsg[] __attribute__((section(".Copyright"))) = {
	"Copyright 2005 QSI Corporation - all rights reserved"
};

/* The qbridge stack */
unsigned long qbridge_stack[1024] __attribute__((section(".stack"))) = { 0 };

/* 
 * The interrupt stack for the arm processors - since all exceptions except Rx receive
 * end up locking the machine, all the interrupt handlers (most of which end
 * up in irq_lockup) should be able to share a single stack.
 *
 * Jeremy, fix the comment above if necessary. - MKE
 */
unsigned long irq_stack[256] __attribute__((section(".irqstack"))) = { 0 };


/************/
/* Routines */
/************/

/**************/
/* irq_lockup */
/**************/
void irq_lockup(void)
{
	/* 
	 * Jeremy, you can re-enable the print code when you have a serial
	 * driver working.
	 */
	unsigned long cpsr, lr;
	int mode;

	cpsr = ARM_GET_CPSR();
	lr = ARM_GET_LR();
	mode = cpsr & ARM_MODE_MASK;

	switch (mode) {
	case ARM_MODE_FIQ:
		DebugPrint("FIQ");
		break;
	case ARM_MODE_IRQ:
		DebugPrint("IRQ");
		break;
	case ARM_MODE_SVC:
		DebugPrint("SWI");
		break;
	case ARM_MODE_ABT:
		DebugPrint("ABORT");
		break;
	case ARM_MODE_UND:
		DebugPrint("UNDEFINED INSN");
		break;
	default:
		DebugPrint("UNKNOWN");
		//DebugPrint(errmsg);
		for(;;) ; /* Lock up here for default case */
	}

	DebugPrint("Unhandled exception: cpsr=0x%x lr=0x%x\r\n", cpsr, lr);

	/* Now lock the machine */
	for(;;) ; 
}

/********/
/* main */
/********/
int main(void) {
	InitializeAllSerialPorts();
	Transmit (&com1, "Hello World", 11);



	DebugPrint ("End of program reached. . . . Locking terminal");
	while(1);
}
