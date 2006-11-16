/*******************************************************************/
/*                                                                 */
/* File:  common.h                                                 */
/*                                                                 */
/* Description:  Common definitions/data structures for Qbridge    */
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
/* 07 Nov 2005  JBR/MKE      1.0     1st Release                   */
/*******************************************************************/

#ifndef COMMON_H
#define COMMON_H

/*****************************/
/* Standard Library Includes */
/*****************************/
#include <stddef.h>
#include <stdbool.h>

/*******************************/
/* Programmer Library Includes */
/*******************************/
#include "basearm.h"
#include "stddefs.h"
#include "str712.h"

/***********/
/* Pragmas */
/***********/

/***********/
/* Defines */
/***********/
// common debugging options
#define _DEBUG
#ifdef _DEBUG
#define DEBUG_SERIAL
#endif

#define HAVE_SNPRINTF
#define PREFER_PORTABLE_SNPRINTF

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/
#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
static inline UINT16 BufToUINT16(UINT8 *buf) { return 256*buf[1] + buf[0]; }

/**************/
/* Prototypes */
/**************/
void Reset(UINT32) __attribute__ ((noreturn));


/******************************/
/* Firmware Header Definition */
/******************************/
#define crcROMMagicL 0x43724321 /* "!CrC"   (little endian) */

#ifndef _ASM_
/*
 * NOTE: the size and definition of this structure must be compatible with CRC.EXE
 * NOTE: make sure the 'C' structure matches the _ASM_ defines
 */
/* WARNING: This section is fixed in size and link location. DO NOT MODIFY!! */
typedef struct {
    unsigned long magic;        /* Magic string 'CrC!' */
    unsigned long codeLen;
    unsigned short crc;
    unsigned short pad;
    void (*entry)(void);
} crcROMHdrDefn;
#else   /* _ASM_ */
#define crcHdr_magic    0
#define crcHdr_codeLen  4
#define crcHdr_crc      8
#endif  /* _ASM_ */

extern unsigned char _FirmwareStartAddr[];
extern unsigned char _RomStartAddr[];
extern unsigned char _BootloaderStartAddr[];

/************/
/* BootFlag */
/************/
#define BOOTFLAG_ENTER_BL   0xA9A99A9A


#endif // COMMON_H
