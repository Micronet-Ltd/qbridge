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

//extern int _snprintf_r_ex(struct _reent *data, FILE * fp, _CONST char *fmt0, va_list ap);

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
        print ("Out of \"heap\" memory");
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
#define TMPSIZE 32
    char tmpCopy[TMPSIZE];
    int count = 0;
    const char *p = fmt0;
    char *b = buf;

    tmpCopy[TMPSIZE-1] = '\0';
    //va_start(ap, formatStr);
    while ((count < n-1) && (*p != '\0')) {
        if (*p == '%') {
            // We support only a simplified subset of snprintf
            // %d
            // %#d
            // %x       %X
            // %#x  %#X
            // %s
            // where # is a single digit

            int fieldSize = -1;
            char fillChar = ' ';
retry:
            p++;
            if (*p == '\0') {
                break;
            } else if (*p == 's') {
                int inc = strlen(strncpy(b, va_arg(ap, char *), (n-1)-count));
                count += inc;
                b += inc;
            } else if (*p == '0') {
                fillChar = '0';
                goto retry;
            } else if ((*p >= '1') && (*p <= '9')) {
                fieldSize = *p - '0';
                goto retry;
            } else if (*p == 'u') {
                unsigned int value = va_arg(ap, int);
                int pos = TMPSIZE-2;
                memset(tmpCopy, '*', TMPSIZE-1);
                while (value != 0) {
                    tmpCopy[pos] = (value % 10)  + '0';
                    value /= 10;
                    pos--;
                }
                if (fieldSize > (TMPSIZE-2-pos)) {
                    pos = TMPSIZE-2-fieldSize;
                }
                int inc = strlen(strncpy(b, tmpCopy+pos+1, (n-1)-count));
                count += inc;
                b += inc;
            } else if (*p == 'd') {
                int value = va_arg(ap, int);
                int pos = TMPSIZE-2;
                bool neg = value < 0;
                if (neg) {
                    value = -value;
                }
                memset(tmpCopy, fillChar, TMPSIZE-1);
                while (value != 0) {
                    tmpCopy[pos] = (value % 10)  + '0';
                    value /= 10;
                    pos--;
                }
                if (pos == TMPSIZE-2) {
                    tmpCopy[pos] = '0';
                    pos--;
                }
                if (neg) {
                    tmpCopy[pos] = '-';
                    pos--;
                }

                if (fieldSize > (TMPSIZE-2-pos)) {
                    pos = TMPSIZE-2-fieldSize;
                }
                int inc = strlen(strncpy(b, tmpCopy+pos+1, (n-1)-count));
                count += inc;
                b += inc;
            } else if ((*p == 'x') || (*p == 'X')) {
                char base = (*p == 'x') ? ('a'-10) : ('A'-10);
                unsigned int value = va_arg(ap, int);
                int pos = TMPSIZE-2;
                memset(tmpCopy, fillChar, TMPSIZE-1);
                while (value != 0) {
                    int tmpDig = value % 16;

                    if (tmpDig <= 9) {
                        tmpCopy[pos] = tmpDig + '0';
                    } else {
                        tmpCopy[pos] = tmpDig + base;
                    }
                    value /= 16;
                    pos--;
                }

                if (pos == TMPSIZE-2) {
                    tmpCopy[pos] = '0';
                    pos--;
                }

                if (fieldSize > (TMPSIZE-2-pos)) {
                    pos = TMPSIZE-2-fieldSize;
                }
                int inc = strlen(strncpy(b, tmpCopy+pos+1, (n-1)-count));
                count += inc;
                b += inc;
            }

        } else {
            *b = *p;
            b++;
            count++;
        }
        p++;
    };
    //va_end(ap);
    if (buf != NULL) {
        *b = '\0';
    }

    return count;

#undef TMPSIZE

#if 0
    FILE f;
    int ret;

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
#endif
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
