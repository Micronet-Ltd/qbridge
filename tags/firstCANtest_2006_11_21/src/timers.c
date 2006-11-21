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
    tcr2.PrescalerDivisionFactor = 5;//new oscillator causes overflow if we attempt
                                     //to keep everything the way it was.... so since we
                                     //have to change anyway, make it so that time calc is easier
                                     //this value was.....(2*bauddiv_9600); // 8 ticks = 1 baud at 9600 -- useful for logging event data on the j1708 bus
                                     //NOW our MainTimer increment is 1/4 microsecond (250nS)

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
    UINT32 t1;
    UINT32 t2;
    UINT32 t3;
    UINT32 t4;
    t2 = timer->timer->Counter;
    t1 = timer->wrapCounter;
    t3 = timer->timer->Counter;
    t4 = timer->wrapCounter;
    if( (t2 > t3) || (t1 != t4) ){   //wrap occured right before our very eyes!
        t2 = timer->timer->Counter;
        t1 = timer->wrapCounter;
    }
    UINT64 retVal = t1;
    retVal <<= 16;
    retVal |= t2;
    return retVal;
}

/********************/
/* Get_uS_TimeStamp */
/********************/
UINT32 Get_uS_TimeStamp( void ) {
    //this routine seems to take a long time... about 2uS
    //I found that reading hw registers can take about 8 wait states
    //I found that reading memory can take about 3 wait states
    //NOTE: This method works unless interrupts are disabled
    // also, a 32 bit number in uS gives a time of about 71 minutes between rollovers
    // this should be adequate for measuring most things... the problem will be if you
    // are attempting to measure some time near the 71 minute mark then you may need the 64 bit version
    //NOTE: Although this method is known to work, due to the speed and architecture of the
    // processor, it is possible for us to read the counter as it rolls to 0 and then read
    // the wrapCounter before the interrupt occurs... in that case, t2 is NOT greater than
    // t3 and so we do not reread the wrapCounter... to solve this problem, I've had to add
    // an additional check
    Timer *timer = &MainTimer;
    UINT32 t1;
    UINT32 t2;
    UINT32 t3;
    UINT32 t4;
    t2 = timer->timer->Counter;
    t1 = timer->wrapCounter;
    t3 = timer->timer->Counter;
    t4 = timer->wrapCounter;
    if( (t2 > t3) || (t1 != t4) ){   //wrap occured right before our very eyes!
        t2 = timer->timer->Counter;
        t1 = timer->wrapCounter;
    }
    UINT32 retVal = t1;    //Remember our MainTimer is in 1/4 microsecond ticks... so divide by 4 makes it 1uS tick
    retVal <<= 14;         //we should shift by 16, but whole thing will be shifted right by 2
    retVal |= (t2 >> 2);
    return retVal;
}

/****************************/
/* GetTimerTimeInBaudTicks */
/**************************/
UINT32 GetMainTimeInBaudTicks() {
    UINT64 time = GetTimerTime(&MainTimer);
    //now to convert this number to baud ticks (9600 baud)
    // baud tick rate is 1/9600 = 104.166uS per tick
    // our MainTimer runs at a rate of .25uS per tick
    // so our scale factor has to be .25/104.166 = .0024 (need to divide by 416.6)
    time >>= 1;         //half microsecond resolution is okay
    time &= 0xffffffffffull; //we are multiplying by 24bit... want 24x40 = 64
    time *= 80531;      //.0024*2^24 rsult is now in Q24 notation, this routine should only return the "whole" part
    time += 8388608;    //round by adding 1/2 (.5*2^24)

    return( time>>24 );
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
