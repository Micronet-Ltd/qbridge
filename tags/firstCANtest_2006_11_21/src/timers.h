#ifndef TIMERS_H
#define TIMERS_H

#define TIMER_IRQ_PRIORITY 10

typedef struct _Timer {
   TimerRegisterMap *timer;
    volatile UINT32 wrapCounter;
} Timer;

extern Timer MainTimer;
extern Timer J1708IdleTimer;
extern Timer J1708RxInterruptTimer;

void InitializeTimers();
void StopTimers(void);
UINT64 GetTimerTime(Timer *timer);

// HACK:  GetJ1708IdleTime is flagged as an int function rather than a UINT32
// function deliberately.  The problem is if the timer wraps for the first time
// during an interrupt (somewhat likely), then the wrap count is 0xFFFFFFFF,
// but the timer counter is zero, resulting a negative return value.
// since a negative value will compare as less than the minimum idle time
// we won't attempt to transmit here.  If we used a UINT, then the bus would
// appear to have been idle for a VERY long time when, in fact it had become
// idle about 1/4 bit time before.
// Ultimately the right solution might be to somehow allow the timer interrupt
// to be nested within serial interrupts, but this could cause real issues since
// I don't know how to do that without the possibility of nested serial
// interrupts as well, which would cause major problems.
extern inline int GetJ1708IdleTime();

extern inline void StartJ1708IdleTimer();
extern void CancelJ1708IdleTimer();
extern UINT32 GetMainTimeInBaudTicks();

UINT32 Get_uS_TimeStamp( void ); //uses main timer
#define One_millisecond              1000 //<---- in uS
#define One_Eighth_of_a_Second     125000 //<---- in uS
#define One_Quarter_of_a_Second    250000 //<---- in uS
#define One_Half_of_a_Second       500000 //<---- in uS
#define Three_Quarters_of_a_Second 750000 //<---- in uS
#define One_Second                1000000 //<---- in uS

#endif // TIMERS_H
