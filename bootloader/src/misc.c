/*******************************************************************/
/*                                                                 */
/* File:  misc.c                                                   */
/*                                                                 */
/* Description:  Miscellaneous routines for Qbridge Bootloader.    */
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
/* 07 Nov 2005  MK Elwood    1.0     Stolen from Qlarity BL        */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <stdarg.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "stddefs.h"
#include "str712.h"
#include "misc.h"
#include "serial.h"

#define INC_CRC_FUNCS
#define CRC_CITT_ONLY
#include "crc.h"


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
CRC_TABLETYPE CITTTable[256];
void *CITTTablePtr = CITTTable;

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
		//print(errmsg);
		for(;;) ; /* Lock up here for default case */
	}

	print("Unhandled exception: cpsr=0x%x lr(user pc)=0x%x", cpsr, lr);

	/* Now lock the machine */
	// We want to ensure that the debug serial port has been flushed
	IRQSTATE saveState = 0;
	DISABLE_IRQ(saveState);

	for (;;);
}

/************/
/* bootKRNL */
/************/
void bootKRNL(int verbose, void *startAddr) 
{
	unsigned short crc;
	crcROMHdrDefn * crcHdr = (crcROMHdrDefn *) startAddr;
	voidFuncPtr func;

	createCITTTable(CITTTablePtr);

	if (crcHdr->magic == crcROMMagicL) {
		/* Verify the checksum */
		crc = calcCITT(CITTTablePtr, ((unsigned char *) startAddr) + sizeof(crcROMHdrDefn), 
                     crcHdr->codeLen - sizeof(crcROMHdrDefn));
		if (crc != crcHdr->crc) {
			print("ERROR--KRNL stored crc=$%x does not match calc crc=$%x\r\n",
					  crcHdr->crc, crc);
			return;
		}
	} else {
		print("NOTICE--No KRNL present\r\n");
		return;
	}

	if (verbose) {
		print("Booting new KRNL. . .\r\n");
	}

	/* Disable interrupts permanently - System mode */
  	asm volatile ("MSR cpsr_c, #0xdf");

	ShutdownPort(hostPort);

	func = (voidFuncPtr) crcHdr->entry;
	(*func) ();

	/* NEVER RETURNS!!! */
}

/***************/
/* lockMachine */
/***************/
void lockMachine(void)
{
	/* Disable interrupts permanently - System mode */
	asm volatile ("MSR cpsr_c, #0xdf");

	print("Locking this machine due to ERROR -- please reboot.\r\n");
	for (;;);
}
