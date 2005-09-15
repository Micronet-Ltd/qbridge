#ifndef J1708_H
#define J1708_H

#define J1708_IDLE_TIME 10
#define J1708_QUEUE_SIZE 16

typedef struct _J1708Message {
	UINT8 priority;
	UINT8 len;
	UINT8 data[21];
} J1708Message;

typedef struct _J1708Queue {
	int head;
	int tail;
	J1708Message msgs[J1708_QUEUE_SIZE];
} J1708Queue;
extern J1708Queue j1708Queue;

void InitializeJ1708();
J1708Message *GetNextJ1708Message();


#endif //J1708_H
