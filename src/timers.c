#include "common.h"
#include "timers.h"
#include "eic.h"
#include "serial.h"

Timer timer1;

void Timer1IRQ();


/*********************/
/* InitializeTimers */
/*******************/
void InitializeTimers() {
	TimerControlRegister1 tcr1 = {0};
	TimerControlRegister2 tcr2 = {0};
	
	timer1.wrapCounter = 0;
	timer1.timer = (TimerRegisterMap *)(TIMER1_REG_BASE);

	tcr1.TimerCountEnable = 1;
	tcr2.TimerOverflowInterruptEnable = 1;
	tcr2.PrescalerDivisionFactor = 255;
	timer1.timer->ControlRegister1 = tcr1.value;
	timer1.timer->ControlRegister2 = tcr2.value;
	timer1.timer->Counter = 0; // reset counter

	RegisterEICHdlr(EIC_TIMER1, Timer1IRQ, TIMER_IRQ_PRIORITY);
	//EICEnableIRQ(EIC_TIMER1);
}

/**************/
/* Timer1IRQ */
/************/
void Timer1IRQ() {
	if (timer1.timer->StatusRegister & TimerOverflow) {
		timer1.wrapCounter++;
		timer1.timer->StatusRegister = ~TimerOverflow;
	} else {
		DebugPrint ("Unwanted timer interrupt %04x", timer1.timer->StatusRegister);
		timer1.timer->StatusRegister = (UINT16)(~(UINT32)(InputCaptureFlagA | OutputCompareFlagA | InputCaptureFlagB | OutputCompareFlagB));
	}

	EICClearIRQ(EIC_TIMER1);
}
