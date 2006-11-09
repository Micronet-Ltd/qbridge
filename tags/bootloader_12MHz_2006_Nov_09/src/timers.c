#include "common.h"
#include "timers.h"
#include "eic.h"
#include "serial.h"

Timer MainTimer;
Timer J1708IdleTimer;
Timer J1708RxInterruptTimer;

void Timer0IRQ() __attribute__((interrupt("IRQ")));
void Timer1IRQ() __attribute__((interrupt("IRQ")));
void Timer2IRQ() __attribute__((interrupt("IRQ")));


/*********************/
/* InitializeTimers */
/*******************/
void InitializeTimers() {
    TimerControlRegister1 tcr1 = {0};
    TimerControlRegister2 tcr2 = {0};

    // Setup the main timer
    MainTimer.wrapCounter = 0;
    MainTimer.timer = (TimerRegisterMap *)(TIMER0_REG_BASE);

    tcr1.value = 0;
    tcr2.value = 0;
    tcr1.TimerCountEnable = 1;
    tcr2.TimerOverflowInterruptEnable = 1;
    tcr2.PrescalerDivisionFactor = (2*bauddiv_9600); // 8 ticks = 1 baud at 9600 -- useful for logging event data on the j1708 bus

    MainTimer.timer->ControlRegister1 = tcr1.value;
    MainTimer.timer->ControlRegister2 = tcr2.value;
    MainTimer.timer->Counter = 0; // reset counter
    RegisterEICHdlr(EIC_TIMER0, Timer0IRQ, TIMER_IRQ_PRIORITY);
    EICEnableIRQ(EIC_TIMER0);

    // Setup the J1708 idle timer
    J1708IdleTimer.wrapCounter = 0;
    J1708IdleTimer.timer = (TimerRegisterMap *)(TIMER2_REG_BASE);

    tcr1.value = 0;
    tcr2.value = 0;
    tcr1.TimerCountEnable = 0;
    tcr2.TimerOverflowInterruptEnable = 1;
    // J1708 uses 9600 baud.  Also PClk1 and PClk2 are the same speed
    // this prescaler should result in a timer that is exactly 16x a bit time.
    // Call GetJ1708IdleTime to get the amount of idle time (in multiples of
    // bit times)
    tcr2.PrescalerDivisionFactor = bauddiv_9600;
    J1708IdleTimer.timer->ControlRegister1 = tcr1.value;
    J1708IdleTimer.timer->ControlRegister2 = tcr2.value;
    J1708IdleTimer.timer->Counter = 0; // reset counter
    RegisterEICHdlr(EIC_TIMER2, Timer2IRQ, TIMER_IRQ_PRIORITY);
    EICEnableIRQ(EIC_TIMER2);

    // Setup the J1708 RX interrupt timer
    // This timer is also used for the pseudo random number for
    // backing off from a collision
    J1708RxInterruptTimer.wrapCounter = 0;
    J1708RxInterruptTimer.timer = (TimerRegisterMap *)(TIMER1_REG_BASE);

    tcr1.value = 0;
    tcr2.value = 0;
    tcr1.TimerCountEnable = 1;
    tcr2.TimerOverflowInterruptEnable = 0;
    tcr1.InputEdgeA = 1;
    tcr1.InputEdgeB = 0;
    tcr2.InputCaptureAInterruptEnable = 1;
    tcr2.InputCaptureBInterruptEnable = 1;
    tcr2.TimerOverflowInterruptEnable = 0;
    tcr2.PrescalerDivisionFactor = 1;
    J1708RxInterruptTimer.timer->ControlRegister1 = tcr1.value;
    J1708RxInterruptTimer.timer->ControlRegister2 = tcr2.value;
    J1708RxInterruptTimer.timer->Counter = 0; // reset counter
    RegisterEICHdlr(EIC_TIMER1, Timer1IRQ, TIMER_IRQ_PRIORITY);
    EICEnableIRQ(EIC_TIMER1);

}

/**************/
/* StopTimers */
/**************/
void StopTimers(void)
{
    MainTimer.timer->ControlRegister1 = 0;
    J1708IdleTimer.timer->ControlRegister1 = 0;
    J1708RxInterruptTimer.timer->ControlRegister1 = 0;
}


/**************/
/* TimerWrap */
/************/
void TimerWrap(Timer *timer) {
    if (timer->timer->StatusRegister & TimerOverflow) {
        timer->wrapCounter++;
        timer->timer->StatusRegister = ~TimerOverflow;
    } else {
        DebugPrint ("Unwanted timer interrupt %04x", timer->timer->StatusRegister);
        timer->timer->StatusRegister = (UINT16)(~(UINT32)(InputCaptureFlagA | OutputCompareFlagA | InputCaptureFlagB | OutputCompareFlagB));
    }

}

/**************/
/* Timer0IRQ */
/************/
void Timer0IRQ() {
    TimerWrap(&MainTimer);
    EICClearIRQ(EIC_TIMER0);
}

/**************/
/* Timer2IRQ */
/************/
void Timer2IRQ() {
    TimerWrap(&J1708IdleTimer);
    EICClearIRQ(EIC_TIMER2);
}

/**************/
/* Timer1IRQ */
/************/
void Timer1IRQ() {
    StartJ1708IdleTimer();
    J1708RxInterruptTimer.timer->StatusRegister = (UINT16)(~((InputCaptureFlagA) | (InputCaptureFlagB)));
    EICClearIRQ(EIC_TIMER1);
}

/*****************/
/* GetTimerTime */
/***************/
UINT64 GetTimerTime(Timer *timer) {
    UINT64 retVal = timer->wrapCounter;
    retVal <<= 16;
    retVal |= timer->timer->Counter;
    return retVal;
}

/****************************/
/* GetTimerTimeInBaudTicks */
/**************************/
UINT32 GetMainTimeInBaudTicks() {
    return (MainTimer.wrapCounter << 13) | (MainTimer.timer->Counter >> 3);
}

/************************/
/* StartJ1708IdleTimer */
/**********************/
void StartJ1708IdleTimer() {
    J1708IdleTimer.timer->Counter=0;
    J1708IdleTimer.timer->ControlRegister1 |= TIMER_COUNT_ENABLE;
    // This unusual value accounts for the fact that the timer resets to 0xFFFC and will wrap after only 4 ticks.
    // This does mean that if you call GetJ1708IdleTime within 4 timer ticks of reset, you will get an extremely
    // large value.  However, since the idle period must always be larger than 4 ticks, this should not matter.
    J1708IdleTimer.wrapCounter = 0xFFFFFFFF;
}

/*********************/
/* GetJ1708IdleTime */
/*******************/
int GetJ1708IdleTime() {
    if (J1708IdleTimer.timer->ControlRegister1 & TIMER_COUNT_ENABLE) {
        // Since the counter resets to FFFC instead of 0000, we have to add 4.
        // Since this counter runs at 16x the speed of the UART, we divide by 16
        return (J1708IdleTimer.wrapCounter << 12) + (((UINT16)(J1708IdleTimer.timer->Counter + 4)) / 16);
    } else {
DebugPrint ("Timer off!??");
        return 0;
    }
}

/*************************/
/* CancelJ1708IdleTimer */
/***********************/
void CancelJ1708IdleTimer() {
    J1708IdleTimer.timer->ControlRegister1 &= ~TIMER_COUNT_ENABLE;
}
