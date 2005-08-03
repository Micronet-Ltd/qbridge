/*******************************************************************/
/*                                                                 */
/* File:  main.c                                                   */
/*                                                                 */
/* Description: STR712 Qbridge (J1708/J1939 to serial converter)   */
/*                                                                 */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <stdlib.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/

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
#if 0
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
		print("FIQ");
		break;
	case ARM_MODE_IRQ:
		print("IRQ");
		break;
	case ARM_MODE_SVC:
		print("SWI");
		break;
	case ARM_MODE_ABT:
		print("ABORT");
		break;
	case ARM_MODE_UND:
		print("UNDEFINED INSN");
		break;
	default:
		print("UNKNOWN");
		print(errmsg);
		for(;;) ; /* Lock up here for default case */
	}

	printf2("Unhandled exception: cpsr=0x%x lr=0x%x\r\n", cpsr, lr);
#endif /* #if 0 */

	/* Now lock the machine */
	for(;;) ; 
}

/********/
/* main */
/********/
int main(void) {
	return (0);
}
