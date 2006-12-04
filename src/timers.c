#include "common.h"
#include "timers.h"
#include "eic.h"
#include "serial.h"

Timer MainTimer;
Timer TestTimer;

void Timer1IRQ() __attribute__((interrupt("IRQ")));

static UINT32 J1708_idle_time=0;
static UINT32 J1708_last_transition_time=0;
static bool J1708_bus_not_idle=FALSE;


/*********************/
/* InitializeTimers */
/*******************/
void InitializeTimers() {
    TimerControlRegister1 tcr1 = {0};
    TimerControlRegister2 tcr2 = {0};

    // Setup the Main Timer, used to:
    //  1. time all events
    //  2. capture edge times of J1708 bus
    //  3. generate a J1708 bus idle timer
    //  4. pseudo random number for backing off from J1708 collisions
    MainTimer.wrapCounter = 0;
    MainTimer.timer = (TimerRegisterMap *)(TIMER1_REG_BASE);

    tcr1.value = 0;
    tcr2.value = 0;
    tcr1.TimerCountEnable = 1;
    tcr1.InputEdgeA = 1;
    tcr1.InputEdgeB = 0;
//    tcr2.InputCaptureAInterruptEnable = 1;
//    tcr2.InputCaptureBInterruptEnable = 0;  //only concerned with bus idle time, not active time
    tcr2.TimerOverflowInterruptEnable = 1;
    tcr2.PrescalerDivisionFactor = 5;   //given our 24MHz Pb2 frequency, divide by 6 (ie 5+1) gives F=4MHz, .25uS per tick
    MainTimer.timer->ControlRegister1 = tcr1.value;
    MainTimer.timer->ControlRegister2 = tcr2.value;
    MainTimer.timer->Counter = 0; // reset counter
    RegisterEICHdlr(EIC_TIMER1, Timer1IRQ, TIMER_IRQ_PRIORITY);
    EICEnableIRQ(EIC_TIMER1);

    // Setup the J1708 Baud Timer, used to:
    //  1. sync up to the BaudRate Generator
    TestTimer.wrapCounter = 0;
    TestTimer.timer = (TimerRegisterMap *)(TIMER0_REG_BASE);

    tcr1.value = 0;
    tcr2.value = 0;
    tcr1.TimerCountEnable = 0;  //but don't let it run until we can also start J1708 baud rate generator
//    tcr1.InputEdgeA = 1;
//    tcr1.InputEdgeB = 0;
//    tcr2.InputCaptureAInterruptEnable = 1;
//    tcr2.InputCaptureBInterruptEnable = 0;  //only concerned with bus idle time, not active time
//    tcr2.TimerOverflowInterruptEnable = 1;
    tcr2.PrescalerDivisionFactor = 155;   //given our 24MHz Pb2 frequency, divide by 156 (ie 155+1) as the baud rate generator would do
    TestTimer.timer->ControlRegister1 = tcr1.value;
    TestTimer.timer->ControlRegister2 = tcr2.value;
    TestTimer.timer->Counter = 0; // reset counter
}

/**************/
/* StopTimers */
/**************/
void StopTimers(void)
{
    MainTimer.timer->ControlRegister1 = 0;
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
/* Timer1IRQ */
/************/
void Timer1IRQ() {
    if (MainTimer.timer->StatusRegister & TimerOverflow) {
        TimerWrap(&MainTimer);
        J1708_idle_time++;
        if( J1708_idle_time >= 40000 ) J1708_idle_time=40000;
    }
    EICClearIRQ(EIC_TIMER1);
}

/*****************/
/* GetTimerTime */
/***************/
UINT64 GetTimerTime(Timer *timer) {
    IRQSTATE saveState = 0;
    UINT32 t1;
    UINT32 t2;
    UINT32 t3;
    DISABLE_IRQ(saveState);
    t2 = timer->timer->Counter;
    t1 = timer->wrapCounter;
    t3 = timer->timer->Counter;
    if( (t2 > t3) || (timer->timer->StatusRegister & TimerOverflow) ){   //wrap occured right before our very eyes!
        t2 = timer->timer->Counter;
        t1++;
    }
    RESTORE_IRQ(saveState);
    UINT64 retVal = t1;
    retVal <<= 16;
    retVal |= t2;
    return retVal;
}

/**********************/
/* GetTime32          */
/**********************/
UINT32 GetTime32( void ) {
    IRQSTATE saveState = 0;
    Timer *timer = &MainTimer;
    UINT32 t1;
    UINT16 t2;
    UINT32 t3;
    DISABLE_IRQ(saveState);
    t2 = timer->timer->Counter;
    t1 = timer->wrapCounter;
    t3 = timer->timer->Counter;
    if( (t2 > t3) || (timer->timer->StatusRegister & TimerOverflow) ){   //wrap occured right before our very eyes!
        t2 = timer->timer->Counter;
        t1++;
    }
    RESTORE_IRQ(saveState);
    return( (t1 << 16) | t2 );
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
    IRQSTATE saveState = 0;
    Timer *timer = &MainTimer;
    UINT32 t1;
    UINT32 t2;
    UINT32 t3;
    DISABLE_IRQ(saveState);
    t2 = timer->timer->Counter;
    t1 = timer->wrapCounter;
    t3 = timer->timer->Counter;
    if( (t2 > t3) || (timer->timer->StatusRegister & TimerOverflow) ){   //wrap occured right before our very eyes!
        t1++;
    }
    RESTORE_IRQ(saveState);
    UINT32 retVal = t1;    //Remember our MainTimer is in 1/4 microsecond ticks... so divide by 4 makes it 1uS tick
    retVal <<= 14;         //we should shift by 16, but whole thing will be shifted right by 2
    retVal |= (t3 >> 2);
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

//#############################################################################
//#############################################################################
//#######  J 1 7 0 8    I D L E    T I M E R    H A N D L E R    C O D E
//#######  J 1 7 0 8    I D L E    T I M E R    H A N D L E R    C O D E
//#######  J 1 7 0 8    I D L E    T I M E R    H A N D L E R    C O D E
//#############################################################################
//#############################################################################
// Note that the J1708 spec indicates the idle time is to be
//    10 + 2*MsgPriority (measured in bit time) MsgPriority = 1-8
//  when a collision occurs (twice in a row) and you have to retry, the time is
//    10 + 2*(Rand# + 1) (measured in bit time) Rand # = 0-7
//  so our min time is 12bits and our max time is 10+2*8=26bits
// J1708 specifies the baud rate as 9600, thus a bit time is 1/9600 = 104.167uS
// so our min time is 1.25mS... and max time is 2.7083mS
//Given that our timer tick rate is 0.25uS per tick, these times correspond
//to 5000 counts for the min time and 10,833.3 counts for the max time
//
//This time needs to saturate so that if the bus is idle for a very long time
//a transfer may begin immediately
//
//
//This code is designed to 'poll' the idle timer.  This could have been setup
//to use interrupts, but if there is a misbehaved device on the bus, it could
//bring our device to it's knees servicing interrupts.  Since we have hardware
//that will capture the time of the edge (rising or falling edge of the J1708
//receive signal) we will take advantage of that feature.  The time will be
//captured and held.  We append the rollover counter onto this time and can
//do so accurately as long as we poll more often that our counter rolls over.
//At this point in time, the rollover occurs every 16mS and our typical poll
//rate (when not busy) is 26uS (a factor of 600+).  It is therefore my beliefe
//that we have enough margin to do this accurately.

/************************/
/* StartJ1708IdleTimer */
/**********************/
void StartJ1708IdleTimer() {
    J1708_idle_time = 0;
    J1708_last_transition_time = GetTime32();
}

#define J1708_BUS_ASSERTED() ((((IOPortRegisterMap *)(IOPORT1_REG_BASE))->PD & BIT(4)) == 0 )
#define UNHANDELED_BUS_TRANSITION() (MainTimer.timer->StatusRegister & (InputCaptureFlagA | InputCaptureFlagB) )

/******************************/
/* ResetJ1708IdleTimeIfNeeded */
/******************************/
// Since we are polling, we may have missed an event.  However, the last
//event is captured and held for us by the hardware.  We do not attempt
//to determine which edge occurred first (we could look at the two times,
//rising edge vs falling edge, and try to resolve things that way).  We
//simply look first at the falling edge, then the rising edge. In this way,
//we give priority to the rising edge (bus idle) as it will get the final
//say.  But then to ensure that if the last event was truely the falling edge
//we sample the bus and if it is asserted (low), we cancel the timer.
void ResetJ1708IdleTimerIfNeeded() {
    if( MainTimer.timer->StatusRegister & InputCaptureFlagB ) {// a low transition has been detected since we last checked
        J1708_idle_time = 0;
        MainTimer.timer->StatusRegister = (UINT16)(~(InputCaptureFlagB));
        J1708_bus_not_idle = TRUE;
    }
    if( MainTimer.timer->StatusRegister & InputCaptureFlagA ) {// | (InputCaptureFlagB)) ) {
        //a rising edge has occured, let's reset our timestamp that indicates time of edge
        //and also reset our rollover counter that we use to saturate once enough time
        //has elapsed that we no longer care how long it has really been
        J1708_idle_time = 0;    //reset the rollover counter
        UINT32 t1 = MainTimer.timer->InputCaptureA; //only edge we care about is rising edge (J1708 bus not actively being driven)
        UINT32 t2 = MainTimer.wrapCounter;          //memorize time of this edge... but watch for rollover (handled before we got to this point in the code)
        UINT32 t3 = MainTimer.timer->Counter;
        if( (t1 > t3) && ((MainTimer.timer->StatusRegister & TimerOverflow)==0) ) {
            t2 = MainTimer.wrapCounter-1;
        }
        J1708_last_transition_time = (t2 << 16) | t1; //we save a 32bit timestamp
        MainTimer.timer->StatusRegister = (UINT16)(~(InputCaptureFlagA));
        J1708_bus_not_idle = FALSE;
    }
    if( J1708_BUS_ASSERTED() ) {
        J1708_idle_time = 0;
        J1708_bus_not_idle = TRUE;
    }
}


/*********************/
/* GetJ1708IdleTime */
/*******************/
UINT32 GetJ1708IdleTime() {
    if( J1708_bus_not_idle ) return( 0 );
    if( J1708_idle_time >= 40000 ) return( 0xffffffff );
    UINT32 t1 = J1708_last_transition_time;
    UINT32 t2 = GetTime32();
// since the timer is not interrupt driven, we don't need to debounce
//    UINT32 t3 = J1708_last_transition_time;
//    if( t1 != t3 )  return( 0 ); // a transition happened right before our eyes!
    return( t2 - t1 );
}



static const UINT32 counttotimerticks[] = { 5000, //(12*(1/9600) / .00000025),
                                            5834,//(14*(1/9600) / .00000025),
                                            6667,//(16*(1/9600) / .00000025),
                                            7500,//(18*(1/9600) / .00000025),
                                            8334,//(20*(1/9600) / .00000025),
                                            9167,//(22*(1/9600) / .00000025),
                                           10000, //(24*(1/9600) / .00000025),
                                           10834, //(26*(1/9600) / .00000025),
                                          };

/*************************************/
/* ConvertJ1708IdleCountToTimerTicks */
/*************************************/
UINT32 ConvertJ1708IdleCountToTimerTicks( UINT8 cnt ) {
    AssertPrint( cnt >= 12, "Can't convert 1708 cnt %d to timer ticks", cnt );
    AssertPrint( cnt <= 26, "Can't convert 1708 cnt %d to timer ticks", cnt );
    AssertPrint( (cnt & 1) == 0, "Can't convert 1708 cnt %d to timer ticks", cnt );
    cnt -= 12;
    cnt >>= 1;
    return( counttotimerticks[cnt] );
}
