/*******************************************************************/
/*                                                                 */
/* File:  main.c                                                   */
/*                                                                 */
/* Description: The Qlarity Bootloader                             */
/*                                                                 */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <stdlib.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "stddefs.h"
#include "str712.h"

#include "misc.h"
#include "serial.h"
#include "flash.h"

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

/********************/
/* global variables */
/********************/
extern unsigned char BuildDate[];

const char CopyrightMsg[] __attribute__((section(".Copyright"))) = {
	"Copyright 2000-05 QSI Corporation - all rights reserved"
};

/* The bootloader stack */
unsigned long bldr_stack[1024] __attribute__((section(".stack"))) = { 0 };

/* 
 * The interrupt stack for the arm processors - since all exceptions except Rx receive
 * end up locking the machine, all the interrupt handlers (most of which end
 * up in irq_lockup) should be able to share a single stack.
 */
unsigned long irq_stack[256] __attribute__((section(".irqstack"))) = { 0 };

UINT32 BootFlag;

/************/
/* Routines */
/************/

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

/**************/
/* bootloader */
/**************/
void bootloader(void) {
   UINT8 rxbuf[16];
	int numchars = 0;

	print("Entering QSI Qbridge Bootloader " VERSION "\r\n");

	for (;;) {
		/* Loopback mode */
		PollReceive(hostPort, rxbuf, sizeof(rxbuf), &numchars);
		if (numchars) {
			Transmit(hostPort, rxbuf, numchars);
		}
	}
	
	/* Never executes to here!!!! */
}

/********/
/* main */
/********/
int main(void) {
	/* 
	 * NOTE: The bootloader does not use interrupts at all, they
	 * are never enabled. All serial routines use polled I/O.
	 */
	InitializeClocks();

	InitializeHostPort();

	/* See if kernel requested the bootloader */
	if (BootFlag == BOOTFLAG_ENTER_BL) {
		print("NOTICE--received request from kernel to enter bootloader\r\n");
		/* So if the user shuts off his machine, it doesn't retain memory to come back in */
		BootFlag = 0;
		goto EnterBootloader;
	}

	/* Invalid kernels will return from this function */
	bootKRNL(0, (void *) _FirmwareStartAddr);

	/* Bootloader */
EnterBootloader:
	bootloader();

	/* Never returns!!!! */
	return (0);
}
