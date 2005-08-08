#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#ifndef _MSC_VER
#include <stdbool.h>
#endif

#include "basearm.h"
#include "stddefs.h"
#include "str712.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))



#ifdef _MSC_VER
#undef DISABLE_IRQ
#undef RESTORE_IRQ
#define DISABLE_IRQ(p)
#define RESTORE_IRQ(p)
#endif


#endif // COMMON_H
