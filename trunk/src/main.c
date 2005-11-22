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
#include "eic.h"
#include "protocol232.h"
#include "timers.h"
#include "J1708.h"

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
#define RESET(flag) do { \
   /* Set register r0 to contain the BootFlag, and reboot */ \
   asm volatile("\tMOV r0, %0\n\tMOV pc, %1\n" : : "r" (flag), "r" (_RomStartAddr) : "r0" ); \
} while (0)

/********************/
/* Global Variables */
/********************/

const char CopyrightMsg[] __attribute__((section(".Copyright"))) = {
	"Copyright 2005 QSI Corporation - all rights reserved"
};

/* The qbridge stack */
unsigned long qbridge_stack[2048] __attribute__((section(".stack"))) = { 0 };

/* 
 * The interrupt stack for the arm processors - since all exceptions except Rx receive
 * end up locking the machine, all the interrupt handlers (most of which end
 * up in irq_lockup) should be able to share a single stack.
 *
 * Jeremy, fix the comment above if necessary. - MKE
 */
unsigned long irq_stack[1024] __attribute__((section(".irqstack"))) = { 0 };

/* The firmware entry point */
void __start(void);

/* Firmware CRC table.  This table has to be filled in later by the crc tool */
crcROMHdrDefn crcTbl __attribute__ ((section(".firmwarehdr"))) = { crcROMMagicL, 0, 0, 0, __start };


/************/
/* Routines */
/************/
void ValidateProgramState();


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

	extern SerialPort *debugPort;
	EnsureQueueFree(&debugPort->txQueue, 80);

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

	DebugPrint("Unhandled exception: cpsr=0x%x lr(user pc)=0x%x", cpsr, lr);

	/* Now lock the machine */
	// We want to ensure that the debug serial port has been flushed
	IRQSTATE saveState = 0;
	DISABLE_IRQ(saveState);

	while (true) {
		StuffTxFifo(debugPort);
	}
}

void LockProgram();

/********/
/* main */
/********/
int main(void) {
	/* Clocks already setup by bootloader */
	InitializeEIC();
	InitializeAllSerialPorts();
	InitializeTimers();
	InitializeJ1708();
	Initialize232Protocol();

	extern const unsigned char BuildDateStr[];
	DebugPrint ("Starting QBridge. Version %s. %s.", VERSION, BuildDateStr);
	while (1) {
		ValidateProgramState();
		ProcessReceived232Data();
		Transmit232IfReady();
		ProcessJ1708TransmitQueue();
		ProcessJ1708RecvPacket();
	}

	LockProgram();
}


/*************************/
/* ValidateProgramState */
/***********************/
void ValidateProgramState() {
	static bool stackWarned = false;

	if (!stackWarned) {
		if (qbridge_stack[sizeof(qbridge_stack) / (10*sizeof(qbridge_stack[0]))] != 0xAABBCCDD) {
			DebugPrint ("Warning:  Main stack is almost or completely full");
			stackWarned = true;
		}
		if (irq_stack[sizeof(irq_stack) / (10*sizeof(irq_stack[0]))] != 0xAABBCCDD) {
			DebugPrint ("Warning:  IRQ stack is almost or completely full");
			stackWarned = true;
		}
	}
}

/****************/
/* LockProgram */
/**************/
void LockProgram() {
	extern int allocPoolIdx;
	extern const int MaxAllocPool;

	DebugPrint ("End of program reached. . . . Entering pass through mode.");
	DebugPrint ("Allocation Pool at index %d / %d", allocPoolIdx, MaxAllocPool );

	while(1) {
		if (!QueueEmpty(&(hostPort->rxQueue))) {
			UINT8 buf[50];
			int len = DequeueBuf(&(hostPort->rxQueue), buf, 50);
			Transmit (debugPort, buf, len);
		} 
		if (!QueueEmpty(&(debugPort->rxQueue))) {
			UINT8 buf[50];
			int len = DequeueBuf(&(debugPort->rxQueue), buf, 50);
			Transmit (hostPort, buf, len);
		} 
	}
}

/*********/
/* Reset */
/*********/
void Reset(void)
{
   IRQSTATE oldIRQ;

   /* Disable interrupts here */
   DISABLE_IRQ(oldIRQ);

	/* Disable timers */
	StopTimers();

	/* Disable uarts */
	DisableAllSerialPorts();

	RESET(BOOTFLAG_ENTER_BL);

	/* Forever loop makes noreturn happy */
	for(;;);
}
