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
#include "common.h"

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

/***************/
/* DequeueBuf */
/*************/
int DequeueBuf(CircleQueue *queue, UINT8*buf, int bufLen) {
	if (queue->count < bufLen) {
		bufLen = queue->count;
	}

	int count;
	for (count=0; count < bufLen; count++, buf++) {
		*buf = queue->data[queue->tail];
		queue->tail = (queue->tail + 1) % QUEUE_SIZE;
	}
	queue->count -= bufLen;

	return bufLen;
}

/********************/
/* EnsureQueueFree */
/******************/
void EnsureQueueFree(CircleQueue *queue, int count) {
	if (QUEUE_SIZE - queue->count >= count) {
		return;
	}

	int dCount = count - (QUEUE_SIZE - count);
	for (dCount--; dCount >= 0; dCount--) {
		DequeueOne(queue);
	}
}
