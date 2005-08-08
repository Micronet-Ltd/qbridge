/*******************************************************************/
/*                                                                 */
/* File:  queue.c                                                  */
/*                                                                 */
/* Description: QBridge utility queues                             */
/*                                                                 */
/*                                                                 */
/*******************************************************************/
/*      This program, in any form or on any media, may not be      */
/*        used for any purpose without the express, written        */
/*                      consent of QSI Corp.                       */
/*                                                                 */
/*             (c) Copyright 2005 QSI Corporation                  */
/*******************************************************************/
/* Release History                                                 */
/*                                                                 */
/*  Date        By           Rev     Description                   */
/*-----------------------------------------------------------------*/
/* 08-Aug-05  JBR            1.0     1st Release                   */
/*******************************************************************/

#include "queue.h"

#if _MSC_VER
#include <stdio.h>
#include <crtdbg.h>
void PrintQ(CircleQueue *q) {
	printf ("h=%d t=%d count=%d\n", q->head, q->tail, q->count);
}

void main() {
	CircleQueue q;
	InitializeQueue(&q);

	PrintQ(&q);
	Enqueue (&q, (UINT8 *)"Hello World", 11);
	Enqueue (&q, (UINT8 *)"Goodbye World", 13);
	PrintQ(&q);

	UINT8 buf[25] = "";
	int i;
	for (i = 0; i < 20; i++) {
		buf[i] = DequeueOne(&q);
	}
	PrintQ(&q);
	for (i = 20; i < 25; i++) {
		buf[i] = DequeueOne(&q);
	}
	PrintQ(&q);
	printf ("buf=%s\n", buf);

	UINT8 inserter[100];
	UINT8 deserter[2000];
	for (i = 0; i < 100; i++) {
		inserter[i] = i+32;
	}

	printf ("%d\n", Enqueue(&q, inserter, 100));
	printf ("%d\n", Enqueue(&q, inserter, 100));
	printf ("%d\n", Enqueue(&q, inserter, 100));
	printf ("%d\n", Enqueue(&q, inserter, 100));
	printf ("%d\n", Enqueue(&q, inserter, 100));
	PrintQ(&q);
	printf ("%d\n", Enqueue(&q, inserter, 100));
	PrintQ(&q);
	printf ("%d\n", Enqueue(&q, inserter, 100));

	for (i = 0; i < QUEUE_SIZE; i++) {
		deserter[i] = DequeueOne(&q);
		_ASSERT (deserter[i] == (i % 100) + 32);
	}
	PrintQ(&q);
	DequeueOne(&q);
	PrintQ(&q);
}

#endif


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
	int insertCount;

	// will the data fit?
	if (length + queue->count > QUEUE_SIZE) {
		if (QueueFull(queue)) {
			return 0;
		}
		length = QUEUE_SIZE - queue->count;
	}
	
	for (insertCount = 0; insertCount < length; insertCount++, data++) {
		queue->data[queue->head] = *data;
		queue->head = (queue->head + 1) % QUEUE_SIZE;
	}
	queue->count += length;
	
	return length;
}

/**************/
/* QueueFull */
/************/
bool QueueFull (CircleQueue *queue) { 
	return queue->count == QUEUE_SIZE; 
}

/***************/
/* QueueEmpty */
/*************/
bool QueueEmpty (CircleQueue *queue) { 
	return queue->count == 0; 
}

/***************/
/* DequeueOne */
/*************/
UINT8 DequeueOne (CircleQueue *queue) { 
	if (QueueEmpty(queue)) { 
		return 0; 
	} else { 
		UINT8 retVal = queue->data[queue->tail];
		queue->tail = (queue->tail + 1) % QUEUE_SIZE;
		queue->count--;
		return retVal;
	}
}
