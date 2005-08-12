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

#if 0
extern int _snprintf_r_ex(struct _reent *data, FILE * fp, _CONST char *fmt0, va_list ap);
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

/***********/
/* sprintf */
/***********/
int sprintf(char *buf, _CONST char *fmt0, ...)
{
   int ret;
	va_list ap;

	va_start(ap, fmt0);
   ret = vsnprintf(buf, INT_MAX, fmt0, ap);
	va_end(ap);

   return ret;
}
#endif
