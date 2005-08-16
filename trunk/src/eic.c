/*******************************************************************/
/*                                                                 */
/* File:  eic.c                                                    */
/*                                                                 */
/* Description:  Enhanced Interrupt Controller code.               */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2005 QSI Corporation                  */
/*******************************************************************/

/*****************************/
/* Standard Library Includes */
/*****************************/

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "common.h"

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/
#define EIC_MAX_PRIORITY  15

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/

/********************/
/* Global Variables */
/********************/
static EICREGS * const eic = (EICREGS *)EIC_REG_BASE;
static XTIREGS * const xti = (XTIREGS *)XTI_REG_BASE;

/************/
/* Routines */
/************/

/*****************/
/* InitializeEIC */
/*****************/
void InitializeEIC(void)
{
	/* 
	 * Sadly, the EIC is not very smart. The upper 16 bits of the
	 * interrupt vector is fixed for all interrupts. This is fine as
	 * long as all ISR addresses live in the same 64k of code space.
	 * If the code gets larger than this, we should consider creating a
	 * code section to contain all ISRs, and link it properly to
	 * guarantee that we don't violate this condition.
	 *
	 * Note that we store an address (not a jump opcode) into the IVR.
	 */
	eic->ivr = RAM_PHYS_BASE & 0xffff0000; /* JEREMY CHANGE THIS TO RAM_PHYS_BASE for your debugging build */
	//eic->ivr = FLASH_PHYS_BASE & 0xffff0000; /* JEREMY CHANGE THIS TO RAM_PHYS_BASE for your debugging build */

	/* Enable interrupts */
	eic->icr |= IRQ_EN;
}

/*******************/
/* RegisterEICHdlr */
/*******************/
int RegisterEICHdlr(EIC_SOURCE src, void (*hdlr)(void), unsigned int priority)
{
	if (priority > EIC_MAX_PRIORITY) {
		return -1;
	}

	eic->sir[src] = ((unsigned long)hdlr << 16) | priority;

	return 0;
}

/**********************/
/* RegisterEICExtHdlr */
/**********************/
int RegisterEICExtHdlr(XTI_SOURCE src, void (*hdlr)(void), unsigned int priority, enum irq_sense edge)
{
	/* 
	 * NOTE: The EIC requires a single handler for all XTI sources,
	 * currently the last handler registered will handle all XTI
	 * interrupts. Additional code will be required if more than
	 * one XTI source is to be handled. 
	 *
	 * Also, this code enables the external interrupt in the XTI.
	 * To enable and disable XTI interrupts, use the Enable/DisableEIC
	 * functions with a source of EIC_XTI.
	 */
	if (src > 7) {
		src >>= 4;
		xti->mrh |= BIT(src);

		/* Set the edge trigger polarity */
		if (edge == IRQ_RISING) {
			xti->trh |= BIT(src);
		} else {
			xti->trh &= ~BIT(src);
		}
	} else {
		xti->mrl |= BIT(src);

		/* Set the edge trigger polarity */
		if (edge == IRQ_RISING) {
			xti->trl |= BIT(src);
		} else {
			xti->trl &= ~BIT(src);
		}
	}

	/* Enable XTI interrupts */
	xti->ctrl |= 0x2;

	return RegisterEICHdlr(EIC_XTI, hdlr, priority);
}

/****************/
/* EICEnableIRQ */
/****************/
void EICEnableIRQ(EIC_SOURCE src)
{
	eic->ier |= BIT(src);
}

/*****************/
/* EICDisableIRQ */
/*****************/
void EICDisableIRQ(EIC_SOURCE src)
{
	eic->ier &= ~BIT(src);
}

/***************/
/* EICClearIRQ */
/***************/
void EICClearIRQ(EIC_SOURCE src)
{
	eic->ipr |= BIT(src);
}

/***************/
/* XTIClearIRQ */
/***************/
void XTIClearIRQ(XTI_SOURCE src)
{
	if (src > 7) {
		src >>= 4;
		xti->prh &= ~BIT(src);
	} else {
		xti->prl &= ~BIT(src);
	}

	EICClearIRQ(EIC_XTI);
}
