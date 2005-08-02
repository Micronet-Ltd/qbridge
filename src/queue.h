#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"


#define QUEUE_SIZE 512
struct CircleQueue {
	UINT8 data[QUEUE_SIZE];
	UINT16 head;
	UINT16 tail;
	UINT16 count;
};


void InitializeQueue(CircleQueue *queue);
inline bool QueueFull (CircleQueue *queue) { return count == QUEUE_SIZE; }
inline bool QueueEmpty (CircleQueue *queue) { return count == 0; }
int Enqueue(CircleQueue *queue, const UINT8* data, UINT16 length);



#endif // QUEUE_H