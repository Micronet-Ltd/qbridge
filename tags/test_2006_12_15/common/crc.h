/*******************************************************************/
/*                                                                 */
/* File:  crc.h                                                    */
/*                                                                 */
/* Description:                                                    */
/*                                                                 */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2000 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 18-Feb-02    RNM          1.0     Incorporated all crc          */
/*                                   functions into this header    */
/*                                   (since they are so common)    */
/*******************************************************************/

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
#ifndef BIT
#define BIT(b) (1L << (b))
#endif /* BIT */

#define BITMASK(w) (((BIT((w) - 1) - 1L) << 1) | 1L)

#define createCITTTable(table) createCRCTable((table), 16, 0x1021, FALSE)
#define calcCITT(table, data, len) calcCRC(0xFFFF, (table), 16, FALSE, 0x0, FALSE, (data), (len))
#define contCalcCITT(crc, table, data, len) calcCRC((crc), (table), 16, FALSE, 0x0, FALSE, (data), (len))

#ifdef CRC_CITT_ONLY
#define CRC_IGNORE_REFLECT
#define CRC_TABLETYPE UINT16
#endif /* CRC_CITT_ONLY */

#ifndef CRC_TABLETYPE
#define CRC_TABLETYPE UINT32
#endif /* CRC_TABLETYPE */

#define CRC_TABLEPTR CRC_TABLETYPE *

/************/
/* TypeDefs */
/************/

/**********/
/* Macros */
/**********/

/**************/
/* Prototypes */
/**************/
/* If CRC_IGNORE_REFLECT is defined, reflections are turned off (always false), regardless of the parameter */
/* This will make for a bit smaller of code */
void createCRCTable(void *table, int width, UINT32 poly, BOOLEAN reflectIn);
UINT32 calcCRC(UINT32 crc, const void *table, int width, BOOLEAN reflectIn, 
               UINT32 xoronout, BOOLEAN reflectOut, const UINT8 *data, UINT32 len);

/*************/
/* Functions */
/*************/
/* These functions are only included if INC_CRC_FUNCS is specified */
#ifdef INC_CRC_FUNCS

/***********/
/* reflect */
/***********/
#ifndef CRC_IGNORE_REFLECT
UINT32 reflectBits(UINT32 val, UINT32 width)
{
   UINT32 i;
   UINT32 ret = 0;

   for (i = 0; i < width; i++) {
      /* Shift in a 0 */
      ret <<= 1L;
      if ((val & 1L) != 0) {
         /* Now make it a 1 */
         ret |= 1L;
      }
      val >>= 1L;
   }

   return ret;
}
#endif /* CRC_IGNORE_REFLECT */

/******************/
/* createCRCTable */
/******************/
/* This function is used to generate a CRC table, which is used 
 * in calculating CRC's. More information on specifying CRC
 * algorithms can be found in "A PAINLESS GUIDE TO CRC 
 * ERROR DETECTION ALGORITHMS" by Ross N. Williams.
 * Some common algorithms:
 * CRC-16/CITT : width = 16, poly = 0x1021, reflect = false
 * CRC-32 : width = 32, poly = 0x04C11DB7, reflect = true
 * CRC-16 : width = 16, poly = 0x8005, reflect = true
 * Modbus : width = 16, poly = 0x8005, reflect = true
 */
void createCRCTable(void *table, int width, UINT32 poly, BOOLEAN reflectIn)
{
   UINT32 i, j;
   UINT32 reg;
   UINT32 topbit;
   UINT32 mask;
   int shift;

   topbit = BIT(width - 1);
   mask = BITMASK(width);
   shift = width - 8;

   for (i = 0; i < 256; i++) {
#ifndef CRC_IGNORE_REFLECT
      if (reflectIn != FALSE) {
         reg = reflectBits(i, 8) << shift;
      } else {
#endif /* CRC_IGNORE_REFLECT */
         reg = i << shift;
#ifndef CRC_IGNORE_REFLECT
      }
#endif /* CRC_IGNORE_REFLECT */
      for (j = 0; j < 8; j++) {
         reg = (reg << 1) ^ (((reg & topbit) != 0)? poly:0);
      }
#ifndef CRC_IGNORE_REFLECT
      if (reflectIn != FALSE) {
         reg = reflectBits(reg, width);
      }
#endif /* CRC_IGNORE_REFLECT */
      ((CRC_TABLEPTR) table)[i] = reg & mask;
   }
}

/***********/
/* calcCRC */
/***********/
/* This function calculates the CRC for a set of data, 
 * using the parameters to describe the algorithm. More 
 * information on specifying CRC algorithms can be found 
 * in "A PAINLESS GUIDE TO CRC ERROR DETECTION ALGORITHMS" 
 * by Ross N. Williams.
 * To continue calculating the CRC for a set of data in
 * multiple parts, simply call the function subsequentially 
 * with the initial CRC (crc) set to the calculated CRC
 * from the previous call. In all but the final call,
 * xoronout should be 0 and reflectout should be false.
 *
 * Some common algorithms:
 * CRC-16/CITT : width = 16, reflectIn = false, crc = 0xFFFF, reflectout = false, xoronout = 0x0000
 * CRC-32 : width = 32, reflectIn = true, crc = 0xFFFFFFFF, reflectout = true, xoronout = 0xFFFFFFFF
 * CRC-16 : width = 16, reflectIn = true, crc = 0x0, reflectout = true, xoronout = 0x0
 * Modbus : width = 16, reflectIn = true, crc = 0xFFFF, reflectout = true, xoronout = 0x0
 */
UINT32 calcCRC(UINT32 crc, const void *table, int width, BOOLEAN reflectIn, UINT32 xoronout, BOOLEAN reflectOut, const UINT8 *data, UINT32 len)
{
   UINT32 i;
   int shift;

#ifndef CRC_IGNORE_REFLECT
   if (reflectIn != FALSE) {
      crc = reflectBits(crc, width);
      for (i = 0; i < len; i++) {
         crc = ((CRC_TABLEPTR) table)[(crc ^ data[i]) & 0xFFL] ^ (crc >> 8);
      }
   } else {
#endif /* CRC_IGNORE_REFLECT */
      shift = (width - 8);
      for (i = 0; i < len; i++) {
         crc = ((CRC_TABLEPTR) table)[((crc >> shift) ^ data[i]) & 0xFFL] ^ (crc << 8);
      }
#ifndef CRC_IGNORE_REFLECT
   }
   if (reflectOut != reflectIn) {
      crc = reflectBits(crc, width);
   }
#endif /* CRC_IGNORE_REFLECT */

   crc = (crc ^ xoronout) & BITMASK(width);
   return crc;
}

#endif /* INC_CRC_FUNCS */
