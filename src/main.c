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
#define ARM_GET_USER_PC() ({ unsigned long cpc=0; asm volatile("STMFD sp!, {r15}^\nNOP\nLDMFD sp!, {%0}\n" : "=r" (cpc) :); cpc; })

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
	unsigned long cpsr, lr, pc;
	int mode;

	cpsr = ARM_GET_CPSR();
	lr = ARM_GET_LR();
	mode = cpsr & ARM_MODE_MASK;
	pc = ARM_GET_USER_PC();

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

	DebugPrint("Unhandled exception: cpsr=0x%x lr=0x%x pc=0x%x", cpsr, lr, pc);

	/* Now lock the machine */
	// We want to ensure that the debug serial port has been flushed
	IRQSTATE saveState = 0;
	DISABLE_IRQ(saveState);

	while (true) {
		StuffTxFifo(debugPort);
	}
}

/********************/
/* InitializeClocks */
/********************/
void InitializeClocks(void)
{
	RCCUREGS * const rccu = (RCCUREGS *)RCCU_REG_BASE;
	PCUREGS * const pcu = (PCUREGS *)PCU_REG_BASE;

	/* Wait for internal voltage regulator to settle */
	while ((pcu->pwrcr & VR_OK) == 0) ;

	/* 
	 * Set up the PLL:
	 * CLK2 = CLK3 = CK/2 (8 MHz for 16 MHz oscillator input)
	 * MX = 01b (multiply by 16)
	 * DX = 001b (divide by 4)
	 * FREF_RANGE = 1 (CLK2 > 3 MHz)
	 * Therefore, RCLK = 32 MHz
	 */
	rccu->pll1cr = 0x73;

	/* Wait for PLL lock */
	while ((rccu->cfr & LOCK) == 0) ;

	/* Switch to PLL clock */
	rccu->cfr |= CSU_CKSEL;

	/* 
	 * Set up peripheral clocks:
	 * MCLK = 32 MHz (no divisor, default)
	 * PCLK1 = 16 MHz (divide by 2)
	 * PCLK2 = 16 MHz (divide by 2)
	 */
	pcu->pdivr = 0x0101;

	/* Disable peripheral clocks (External Memory and USB) to save power */
	rccu->per = 0;
}

void LockProgram();

/********/
/* main */
/********/
int main(void) {
	InitializeClocks();
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
