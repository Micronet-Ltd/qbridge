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

#define _DEBUG

/********************/
/* InitializeQueue */
/******************/
void InitializeQueue(CircleQueue *queue) {
    queue->head = 0;
    queue->tail = 0;
}

/************/
/* Enqueue */
/**********/
int Enqueue(CircleQueue *queue, const UINT8* data, UINT16 length) {
    int insertCount;
	int availCount;
	
	//available valid bytes in the buffer = ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	//available space for putting new data in the buffer = QUEUE_SIZE - validDataBytesInTheBuffer;
	//using a "power of two" for QUEUE_SIZE allows us to simplify this to: (head-tail) & (QUEUE_SIZE-1);
	//availCount = QUEUE_SIZE - ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	availCount = (QUEUE_SIZE-1) - ((queue->head - queue->tail) & (QUEUE_SIZE-1));  //full must leave one space, otherwise full and empty look the same after placing the last byte in the queue
	if( availCount <= 0)
		return 0;

    // will the data fit?
    if (length > availCount) {
		length = availCount;
    }

    for (insertCount = 0; insertCount < length; insertCount++, data++) {
        queue->data[queue->head] = *data;
        queue->head = (queue->head + 1) & (QUEUE_SIZE-1);
    }

    return length;
}

/**************/
/* QueueFull */
/************/
bool QueueFull (CircleQueue *queue) {
	return ((queue->head - queue->tail) & (QUEUE_SIZE-1)) >= (QUEUE_SIZE-1);
}

/***************/
/* QueueEmpty */
/*************/
bool QueueEmpty (CircleQueue *queue) {
    return queue->head == queue->tail;
}

/***************/
/* DequeueOne */
/*************/
UINT8 DequeueOne (CircleQueue *queue) {
    if (QueueEmpty(queue)) {
        return 0;
    } else {
        UINT8 retVal = queue->data[queue->tail];
        queue->tail = (queue->tail + 1) & (QUEUE_SIZE-1);
        return retVal;
    }
}

/***************/
/* DequeueBuf */
/*************/
int DequeueBuf(CircleQueue *queue, UINT8*buf, int bufLen) {
	int availCount;
	
	//available valid bytes in the buffer = ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	//using a "power of two" for QUEUE_SIZE allows us to simplify this to: (head-tail) & (QUEUE_SIZE-1);
	availCount = ((queue->head - queue->tail) & (QUEUE_SIZE-1));
	if( availCount <= 0)
		return 0;

    if (availCount < bufLen) {
        bufLen = availCount;
    }

    int count;
    for (count=0; count < bufLen; count++, buf++) {
        *buf = queue->data[queue->tail];
        queue->tail = (queue->tail + 1) & (QUEUE_SIZE - 1);
    }

    return bufLen;
}

/********************/
/* EnsureQueueFree */
/******************/
void EnsureQueueFree(CircleQueue *queue, int count) {
	int availCount;
	
	//available valid bytes in the buffer = ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	//available space for putting new data in the buffer = QUEUE_SIZE - validDataBytesInTheBuffer;
	//using a "power of two" for QUEUE_SIZE allows us to simplify this to: (head-tail) & (QUEUE_SIZE-1);
	//availCount = QUEUE_SIZE - ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	availCount = (QUEUE_SIZE-1) - ((queue->head - queue->tail) & (QUEUE_SIZE-1));  //full must leave one space, otherwise full and empty look the same after placing the last byte in the queue
	if( availCount >= count)
		return;

    int dCount = count - availCount;
    for (dCount--; dCount >= 0; dCount--) {
        DequeueOne(queue);
    }
}

/*************************/
/* QueueValidBytesCount */
/***********************/
int QueueValidBytesCount (CircleQueue *queue) {
	int availCount;
	
	//available valid bytes in the buffer = ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	//available space for putting new data in the buffer = QUEUE_SIZE - validDataBytesInTheBuffer;
	//using a "power of two" for QUEUE_SIZE allows us to simplify this to: (head-tail) & (QUEUE_SIZE-1);
	availCount = ((queue->head - queue->tail) & (QUEUE_SIZE-1));
	return availCount;
}

/*****************************/
/* QueueSpaceAvailableCount */
/***************************/
int QueueSpaceAvailableCount (CircleQueue *queue) {
	int availCount;
	
	//available valid bytes in the buffer = ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	//available space for putting new data in the buffer = QUEUE_SIZE - validDataBytesInTheBuffer;
	//using a "power of two" for QUEUE_SIZE allows us to simplify this to: (head-tail) & (QUEUE_SIZE-1);
	//availCount = QUEUE_SIZE - ((QUEUE_SIZE +(queue->head-queue->tail)) % QUEUE_SIZE);
	availCount = (QUEUE_SIZE-1) - ((queue->head - queue->tail) & (QUEUE_SIZE-1));  //full must leave one space, otherwise full and empty look the same after placing the last byte in the queue
	if( availCount < 0 )
		availCount = 0;
    return availCount;
}

