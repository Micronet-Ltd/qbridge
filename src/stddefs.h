/*******************************************************************/
/*                                                                 */
/* File:  stddefs.h                                                */
/*                                                                 */
/* Description:  Standard definitions for Qlarity software.        */
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
/* xx-xxx-xx    who          1.0     1st Release                   */
/*******************************************************************/

#ifndef STDDEFS_H
#define STDDEFS_H

#ifdef _MSC_VER
/* Reduce the warning level of defining an array of 0 items for microsoft compiler */
#pragma warning( 4 : 4200)
#endif /* _MSC_VER */


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
//#ifdef  NULL
//#undef  NULL
//#endif

//#ifdef  FALSE
//#undef  FALSE
//#endif
//#ifdef  TRUE
//#undef  TRUE
//#endif

//#ifdef _ASM_
#define FALSE	0
#define TRUE	1
//#else
//typedef enum { FALSE = 0, TRUE } BOOLEAN;
//#endif

#ifndef NULL
#define NULL	(void *) 0
#endif

/* Maximum integer numbers */
#define MAX_INT32 2147483647
#define MIN_INT32 (-2147483647 - 1)

#define MIN_UINT16 0
#define MAX_UINT16 0xffff

#define pi ((float) 3.1415926536)

/************/
/* TypeDefs */
/************/
#ifndef _ASM_

//#ifdef _MSC_VER
/* Disable warnings about benign redefinitions of type */
//#pragma warning(disable : 4142)
//#else /* _MSC_VER */
/* If we don't have a microsoft compiler, we need to define these types. */
//#define __int8 char
//#define __int16 short
//#define __int32 long
//#endif /* _MSC_VER */

typedef signed char INT8;
typedef short INT16;
typedef int INT32;
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;

#ifdef _MSC_VER
typedef __int64 INT64;
#else /* _MSC_VER */
typedef long long int INT64;
#endif /* _MSC_VER */

/* This needs to stay an 8-bit item, or things need to be changed in the firmware! */
typedef unsigned char BOOLEAN;

typedef float FLT32;

typedef struct {
	INT32 top;
	INT32 left;
	INT32 bottom;
	INT32 right;
} rect;

#endif /* _ASM_ */

/**********/
/* Macros */
/**********/
/* A few helpful pre-processing macros that we use often enough */
#define TOKEN_CONCAT2(pretok,posttok)  pretok##posttok
#define TOKEN_CONCAT(pretok,posttok)   TOKEN_CONCAT2(pretok,posttok)

#define TO_STR2(strval)                #strval
#define TO_STR(strval)                 TO_STR2(strval)

/* Reading and writing to hardware registers */
#define WRITE_REG32(addr, val)	(*(volatile unsigned long *)(addr) = (val))
#define READ_REG32(addr)			(*(volatile unsigned long *)(addr))
#define WRITE_REG16(addr, val)	(*(volatile unsigned short *)(addr) = (val))
#define READ_REG16(addr)			(*(volatile unsigned short *)(addr))
#define WRITE_REG8(addr, val)		(*(volatile unsigned char *)(addr) = (val))
#define READ_REG8(addr)				(*(volatile unsigned char *)(addr))

/* Figure out the number of elements in an initialized array */
#define ITEM_SIZEOF(x) (sizeof(x) / sizeof(x[0]))

#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define ABS(z) (((z) < 0) ? -(z) : (z))
#define BYONE(z) (((z) == 0) ? 0 : (((z) < 0) ? -1 : 1))

/* sz must be a power of 2 */
#define ALIGN(n,sz) (((n) | ((sz) - 1)) ^ ((sz) - 1))

/* Converting a float to an int always truncates decimal places towards 0 (ie -5.6 -> -5) */
#define ROUND(a) ((INT32) (((a) > 0) ? ((a) + .5) : ((a) - .5)))

#define BIT(x) (1 << (x))

/* Get a power of two */
#define POW2(n) BIT(n)

#ifdef SIM
/* 
 * These functions aren't standard C functions - we need to map them for simulation.
 * They are defined functions in newlib (used in UCOS build).
 */
#define sinf(x) ((float) sin(x))
#define cosf(x) ((float) cos(x))
#define tanf(x) ((float) tan(x))
#define asinf(x) ((float) asin(x))
#define acosf(x) ((float) acos(x))
#define atanf(x) ((float) atan(x))
#define fabsf(x) ((float) fabs(x))
#define sqrtf(x) ((float) sqrt(x))
#define powf(x,y) ((float) pow(x,y))
#define expf(x) ((float) exp(x))
#define logf(x) ((float) log(x))
#endif /* SIM */

#ifdef _MSC_VER
/* Method for doing #warning in visual C++ */
#define warn(desc)            message(__FILE__ "(" TO_STR(__LINE__) ") : #warning : " desc)
/* To use this method of warning, simply write '#pragma warn("str")' */
#endif /* _MSC_VER */

/**************/
/* Prototypes */
/**************/

#endif /* STDDDEFS_H */






