#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdbool.h>

#include "basearm.h"
#include "stddefs.h"
#include "str712.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

static inline UINT16 BufToUINT16(UINT8 *buf) { return 256*buf[1] + buf[0]; }

// common debugging options
#define _DEBUG
#ifdef _DEBUG
#define DEBUG_SERIAL
#endif


#define HAVE_SNPRINTF 
#define PREFER_PORTABLE_SNPRINTF 

#endif // COMMON_H
