#ifndef TIMERS_H
#define TIMERS_H

#define TIMER_IRQ_PRIORITY 7

typedef struct _Timer {
   TimerRegisterMap *timer;
	UINT32 wrapCounter;
} Timer;

extern Timer MainTimer;
extern Timer J1708IdleTimer;

void InitializeTimers();
UINT64 GetTimerTime(Timer *timer);

extern inline UINT32 GetJ1708IdleTime();
extern inline void StartJ1708IdleTimer();
extern void CancelJ1708IdleTimer();

#endif // TIMERS_H
