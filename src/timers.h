#ifndef TIMERS_H
#define TIMERS_H

#define TIMER_IRQ_PRIORITY 7

typedef struct _Timer {
   TimerRegisterMap *timer;
	UINT64 wrapCounter;
} Timer;

Timer timer1;

void InitializeTimers();


#endif // TIMERS_H
