#ifndef TIMERS_H
#define TIMERS_H

#define TIMER_IRQ_PRIORITY 10

typedef struct _Timer {
   TimerRegisterMap *timer;
    volatile UINT32 wrapCounter;
} Timer;

extern Timer MainTimer;

void InitializeTimers();
void StopTimers(void);
UINT64 GetTimerTime(Timer *timer);

extern UINT32 GetJ1708IdleTime();

extern inline void StartJ1708IdleTimer();
extern UINT32 GetMainTimeInBaudTicks();

UINT32 Get_uS_TimeStamp( void ); //uses main timer
#define One_millisecond              1000 //<---- in uS
#define One_Eighth_of_a_Second     125000 //<---- in uS
#define One_Quarter_of_a_Second    250000 //<---- in uS
#define One_Half_of_a_Second       500000 //<---- in uS
#define Three_Quarters_of_a_Second 750000 //<---- in uS
#define One_Second                1000000 //<---- in uS

extern UINT32 ConvertJ1708IdleCountToTimerTicks( UINT8 cnt ); //into units of main timer
extern void   ResetJ1708IdleTimerIfNeeded( void );
extern UINT32 GetTime32( void ); //main timer

#endif // TIMERS_H
