#include "common.h"
#include "J1708.h"
#include "serial.h"
#include "timers.h"

J1708Queue j1708Queue;

/********************/
/* InitializeJ1708 */
/******************/
void InitializeJ1708() {
	SetPortSettings(j1708Port, 9600, 8, 'N', 1);

	j1708Queue.head = 0;
	j1708Queue.tail = 0;

	StartJ1708IdleTimer();
}

/************************/
/* GetNextJ1708Message */
/**********************/
J1708Message * GetNextJ1708Message() {
	if (j1708Queue.head == j1708Queue.tail) {
		return NULL;
	}


}

/******************************/
/* ProcessJ1708TransmitQueue */
/****************************/
void ProcessJ1708TransmitQueue() {
	// Is there anything to transmit?
	if (j1708Queue.head == j1708Queue.tail) {
		return;
	}

	// Is the transmission queue empty?
	if (!QueueEmpty(&j1708Port->txQueue)) {
		return;
	}

	// Is transmission fifo empty? (can only send J1708 when we have completed all other packets)
	if (!IsTxFifoEmpty(j1708Port)) {
		return;
	}

	J1708Message *msg = GetNextJ1708Message();
	if (msg == NULL) {
		DebugPrint ("Unexpected null message retreived");
		return;
	}

	// Not trying to transmit anything, now let us see if we have been idle long enough
	UINT32 busAccessTime = J1708_IDLE_TIME + 2 * msg->priority;
	if (busAccessTime > GetJ1708IdleTime()) {
		return; // bus has not been idle long enough
	}

	Transmit (j1708Port, msg->data, msg->len);
	j1708Queue.tail = (j1708Queue.tail + 1) % J1708_QUEUE_SIZE;

}
