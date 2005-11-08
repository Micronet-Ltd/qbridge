/*******************************************************************/
/*                                                                 */
/* File:  flash.h                                                  */
/*                                                                 */
/* Description:  Flash programming routines (Intel flash family)   */
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
/* 19 Sep 2003  MK Elwood    1.0     Added when flash routines     */
/*                                   broken out of main.c          */
/*******************************************************************/

#ifndef	FLASH_H
#define	FLASH_H

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

/**********/
/* Macros */
/**********/

/**************/
/* Prototypes */
/**************/
unsigned long ReadFlashID(void);
int BootldrUpgrade(unsigned char *BootRamPtr, unsigned long BootLen);
int ParmUpgrade(unsigned char *ParmRamPtr, unsigned long ParmLen);
void KRNLUpgrade(unsigned char *KRNLRamPtr, unsigned long KRNLLen);
void EraseSection(unsigned char *section, unsigned char *name);

#endif /* FLASH_H */
