/*******************************************************************/
/*                                                                 */
/* File:  misc.h                                                   */
/*                                                                 */
/* Description: Miscellaneous routines for Qlarity Bootloader.     */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2003 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 26 Aug 2003  MK Elwood    1.0     Updated                       */
/*******************************************************************/

#ifndef MISC_H
#define MISC_H

/* Do not include these definitions in assembly files */
#ifndef _ASM_

/*****************************/
/* Standard Library Includes */
/*****************************/

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
typedef void (*voidFuncPtr) (void);

/**********/
/* Macros */
/**********/

/*****************/
/* Global/Extern */
/*****************/
extern void *CITTTablePtr;

/**************/
/* Prototypes */
/**************/
void irq_lockup(void);
void bootKRNL(int doPrint, void *startAddr);
void lockMachine(void);

#endif  /* #ifndef _ASM */

#endif /* MISC_H */
