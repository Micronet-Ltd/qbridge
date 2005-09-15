#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"


#define QUEUE_SIZE 512
typedef struct _CircleQueue {
	UINT32 count;
	UINT32 head;
	UINT32 tail;
	UINT8 data[QUEUE_SIZE];
} CircleQueue;


void InitializeQueue(CircleQueue *queue);
int Enqueue(CircleQueue *queue, const UINT8* data, UINT16 length);
extern inline bool QueueFull (CircleQueue *queue);
extern inline bool QueueEmpty (CircleQueue *queue);
extern inline UINT8 DequeueOne (CircleQueue *queue);
int DequeueBuf(CircleQueue *queue, UINT8*buf, int bufLen);
void EnsureQueueFree(CircleQueue *queue, int count);



#endif // QUEUE_H

