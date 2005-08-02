#include "queue.h"

/********************/
/* InitializeQueue */
/******************/
void InitializeQueue(CircleQueue *queue) {
	queue->head = 0;
	queue->tail = 0;
	queue->count = 0;
}

/************/
/* Enqueue */
/**********/
int Enqueue(CircleQueue *queue, const UINT8* data, UINT16 length) {
	// will the data fit?
	if (length + queue->count > QUEUE_SIZE) {
		if (QueueFull()) {
			return 0;
		}
		length = QUEUE_SIZE - queue->count;
	}
	
	UINT8* insertPtr = queue->data + queue->head;
	for (int insertCount = 0; insertCount < length; insertCount++, insertPtr++, data++) {
		*insertPtr = *data;
	}
	queue->count += length;
	queue->head = (queue->head + length) % QUEUE_SIZE;
	
	return length;
}
