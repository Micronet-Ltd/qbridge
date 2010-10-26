#ifndef EIC_H
#define EIC_H

#include "common.h"

void InitializeEIC(void);
int RegisterEICHdlr(EIC_SOURCE src, void (*hdlr)(void), unsigned int priority);
int RegisterEICExtHdlr(XTI_SOURCE src, void (*hdlr)(void), unsigned int priority, enum irq_sense edge);
void EICEnableIRQ(EIC_SOURCE src);
void EICDisableIRQ(EIC_SOURCE src);
void EICClearIRQ(EIC_SOURCE src);
void XTIClearIRQ(XTI_SOURCE src);

#endif //EIC_H
