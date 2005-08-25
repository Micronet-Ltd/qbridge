/*******************************************************************/
/*                                                                 */
/* File:  stubs.c                                                  */
/*                                                                 */
/* Description: QBridge stubs for newlib                           */
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
/* 08-Aug-05  JBR            1.0     1st Release                   */
/*******************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "common.h"
#include "serial.h"

extern int _snprintf_r_ex(struct _reent *data, FILE * fp, _CONST char *fmt0, va_list ap);

const int MaxAllocPool = 128;
UINT32 allocPool[128];
int allocPoolIdx = 0;

/*************/
/* _calloc_r */
/*************/
/* This function can be used by mprec - so we'll re-route it */
void *_calloc_r(struct _reent *r, size_t nobj, size_t n)
{
	int oldAllocPoolIdx = allocPoolIdx;
	// Get the number of 4 byte indexes we need
	allocPoolIdx += (nobj + sizeof(UINT32)-1) / sizeof(UINT32); 
	if (allocPoolIdx > MaxAllocPool) {
		DebugCorePrint ("Out of \"heap\" memory");
		return NULL;
	}

		
	return &(allocPool[oldAllocPoolIdx]);
}

/*************/
/* vsnprintf */
/*************/
/* These are a little bit less size wise than the newlib routines (and don't suck in all the stupid file stuff) */
int vsnprintf(char *buf, size_t n, _CONST char *fmt0, va_list ap)
{
   int ret;
   FILE f;

   f._flags = __SWR | __SSTR;
   f._bf._base = f._p = (unsigned char *) buf;
   f._bf._size = f._w = (n > 0 ? n - 1 : 0);
   f._data = _REENT;

   ret = _snprintf_r_ex(_REENT, &f, fmt0, ap);

   /* NULL terminate - if we wrote something */
   if (n > 0) {
      *f._p = 0;
   }

   return ret;
}

/************/
/* snprintf */
/************/
int snprintf(char *buf, size_t n, _CONST char *fmt0, ...)
{
   int ret;
	va_list ap;

	va_start(ap, fmt0);
   ret = vsnprintf(buf, n, fmt0, ap);
	va_end(ap);

   return ret;
}
