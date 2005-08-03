#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"


#define QUEUE_SIZE 512
typedef struct _CircleQueue {
	UINT8 data[QUEUE_SIZE];
	UINT16 head;
	UINT16 tail;
	UINT16 count;
} CircleQueue;


void InitializeQueue(CircleQueue *queue);
inline bool QueueFull (CircleQueue *queue) { return queue->count == QUEUE_SIZE; }
inline bool QueueEmpty (CircleQueue *queue) { return queue->count == 0; }
int Enqueue(CircleQueue *queue, const UINT8* data, UINT16 length);
inline UINT8 DequeueOne (CircleQueue *queue) { 
	if (QueueEmpty(queue)) { 
		return 0; 
	} else { 
		UINT8 retVal = queue->data[queue->tail];
		queue->tail = (queue->tail + 1) % QUEUE_SIZE;
		queue->count--;
		return retVal;
	}
}



#endif // QUEUE_H