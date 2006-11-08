#include "StdAfx.h"
#include "TestRP1210.h"
#include "RP1210API.h"
#include "Log.h"
#include "INIMgr.h"
#include "DlgTestWinNotify.h"

#pragma warning (disable:4996)


// Note:
// Things to search for when implementing 1939 include:
// "J1708"
// SetMessageFilteringForJ1708
// SetAllFiltersToPass

CRITICAL_SECTION CritSection::critSection;
bool CritSection::critSectionInited;

bool TestRP1210::firstAlreadyCreated = false;

bool IsValid(int result) { return (result >= 0) && (result < 128); }

/***************************/
/* TestRP1210::TestRP1210 */
/*************************/
TestRP1210::TestRP1210(void) : helperTxThread(NULL), helperRxThread(NULL), threadsMustDie(false), secondaryClient(-1), api1(NULL), api2(NULL),
								toKillClient(-1)
{
	if (firstAlreadyCreated) {
		throw (CString("Cannot create more than one of these objects at a time"));
	}
	firstAlreadyCreated = true;

	CritSection::Init();
}

/****************************/
/* TestRP1210::~TestRP1210 */
/**************************/
TestRP1210::~TestRP1210(void)
{
	firstAlreadyCreated = false;
	KillThreads();

	if ((secondaryClient >= 0) && (secondaryClient <= 127)) {
		int result = api2->pRP1210_ClientDisconnect(secondaryClient);
		if (result != 0) {
			LogError(*api2, result);
		}
	}

	delete api1;
	delete api2;
}

/****************************/
/* TestRP1210::KillOrphans */
/**************************/
void TestRP1210::KillOrphans(vector<INIMgr::Devices> &devs, int idx1, int idx2) {
	if ((idx1 < 0) || (idx2 < 0) || (idx1 >= (int)devs.size()) || (idx2 >= (int)devs.size())) {
		log.LogText(_T(" Invalid device index"), Log::Red);
		return;
	}
	
	RP1210API api1(devs[idx1].dll);
	RP1210API api2(devs[idx2].dll);

	CritSection::Init();
	for (int i = 0; i < 16; i++) {
		int result1 = api1.pRP1210_ClientDisconnect(i);
		int result2 = api2.pRP1210_ClientDisconnect(i);

		if (result1 != 0) {
			LogError(api1, result1);
		}
		if (result2 != 0) {
			LogError(api2, result2);
		}
	}
}

/*************************/
/* TestRP1210::DoEvents */
/***********************/
bool TestRP1210::DoEvents() {
	MSG msg;
	while (::PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE)) {
		if (!AfxGetThread()->PumpMessage())  {
			_ASSERT(false);
			::MessageBox(NULL, _T("Error processing windows message."), _T("Download error"), MB_ICONEXCLAMATION);
			return false;
		}
	}
	return true;
}



/*********************/
/* TestRP1210::Test */
/*******************/
void TestRP1210::Test (vector<INIMgr::Devices> &devs, int idx1, int idx2) {
	if ((idx1 < 0) || (idx2 < 0) || (idx1 >= (int)devs.size()) || (idx2 >= (int)devs.size())) {
		log.LogText(_T(" Invalid device index"), Log::Red);
		return;
	}
	
	api1 = new RP1210API(devs[idx1].dll);
	api2 = new RP1210API(devs[idx2].dll);

	if (api1->IsWorking()) {
		log.LogText(devs[idx1].dll + _T(" loaded"), Log::Green);
	} else {
		log.LogText(devs[idx1].dll + _T(" load failed"), Log::Red);
		return;
	}

	if (api2->IsWorking()) {
		log.LogText(devs[idx2].dll + _T(" loaded"), Log::Green);
	} else {
		log.LogText(devs[idx2].dll + _T(" load failed"), Log::Red);
		return;
	}

	TestReadVersion();
//TestConnect(devs, idx1);
//TestMulticonnect(devs, idx1);

	// Setup thread for secondary J1708 communication
	secondaryClient = api2->pRP1210_ClientConnect(NULL, devs[idx2].deviceID, "J1708", 0, 0, 0);
	if (!IsValid(secondaryClient)) {
		log.LogText (_T("    Connecting to secondary device failed"), Log::Red);
		LogError (*api2, secondaryClient);
		secondaryClient = -1;
		goto end;
	}
	SetupWorkerThread (helperTxThread, SecondaryDeviceTXThread, false);
	SetupWorkerThread (helperRxThread, SecondaryDeviceRXThread, false);

	// Setup Primary Client
	int primaryClient = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, "J1708", 0, 0, 0);
	if (!IsValid(primaryClient)) {
		log.LogText (_T("    Connecting to primary device failed"), Log::Red);
		LogError (*api1, primaryClient);
		primaryClient = -1;
		goto end;
	}
	// Set filter states to pass
char dummy;
	api1->pRP1210_SendCommand(SetAllFiltersToPass, primaryClient, &dummy, 0);

//	TestBasicRead(devs[idx1], primaryClient);
//	TestAdvancedRead(devs[idx1], primaryClient);
//	TestMultiRead(devs[idx1], primaryClient);
//	TestBasicSend(primaryClient);
//	TestAdvancedSend(devs[idx1], primaryClient);
	TestWinNotify(devs[idx1]);

end:
	if (IsValid(primaryClient)) {
		api1->pRP1210_ClientDisconnect(primaryClient);	
	}
	log.LogText(_T("Done testing"), Log::Green);
}

/********************************/
/* TestRP1210::TestReadVersion */
/******************************/
void TestRP1210::TestReadVersion() {
	// Test version
	log.LogText (_T("Testing: ReadVersion"), Log::Blue);
	{
		char vmaj[100] = "--------------", vmin[100] = "--------------", amaj[100] = "--------------", amin[100] = "--------------";
		api1->pRP1210_ReadVersion(vmaj, vmin, amaj, amin);
		CString vMaj(vmaj), vMin(vmin), aMaj(amaj), aMin(amin);
		log.LogText (_T("    DLL Version: ") + vMaj + _T(".") + vMin + _T("   API Version: ") + aMaj + _T(".") + aMin);
	}
}

/****************************/
/* TestRP1210::TestConnect */
/**************************/
void TestRP1210::TestConnect(vector<INIMgr::Devices> &devs, int idx1) {
	// Test Connect / Disconnect
	log.LogText (_T("Testing: ClientConnect/ClientDisconnect"), Log::Blue);
	for (size_t i = 0; i < devs[idx1].protocolStrings.size(); i++) {
		int result = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, INIMgr::ToAnsi(devs[idx1].protocolStrings[i]), 0, 0, 0);
		if ((result >= 0) && (result < 128)) {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[i] + _T(" succeeded"));
			
			result = api1->pRP1210_ClientDisconnect(result);
			if (result != 0) {
				log.LogText(_T("    Disconnect Failure!"), Log::Red);
				LogError (*api1, result);
			}
		} else {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[i] + _T(" Failed"), Log::Red);
			LogError(*api1, result);
		}
	}
}

/*********************************/
/* TestRP1210::TestMulticonnect */
/*******************************/
void TestRP1210::TestMulticonnect(vector<INIMgr::Devices> &devs, int idx1) {
	log.LogText (_T("Testing: Multiconnect"), Log::Blue);
	vector<int> ids;
	size_t targetCount = 4;
	for (size_t i = 0; i < targetCount; i++) {
		int result = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, INIMgr::ToAnsi(devs[idx1].protocolStrings[0]), 0, 0, 0);
		if ((result >= 0) && (result < 128)) {
			ids.push_back(result);
		} else {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[0] + _T(" Failed"), Log::Red);
			LogError(*api1, result);
		}
		log.LogDot();
	}
	if (ids.size() == targetCount) {
		log.LogText(_T("    Success"));
	} else {
		log.LogText (_T("    Failed"), Log::Red);
	}
	for (size_t i = 0; i < ids.size(); i++) {
		int result = api1->pRP1210_ClientDisconnect(ids[i]);
		if (result != 0) {
			LogError (*api1, result);
		}
	}
}

/******************************/
/* TestRP1210::TestBasicRead */
/****************************/
void TestRP1210::TestBasicRead(INIMgr::Devices &dev, int primaryClient) {
	log.LogText (_T("Testing: ReadMessage (basic)"), Log::Blue);
	{
		char buf[256];
		int i;
		int j;

		for (j = 0; j < 2; j++) {
			CString state = (j == 0) ? _T("Blocking") : _T("Non-Blocking");
			bool blocking = j == 0;
			// This statement is intended to launch the helper process, so that the timeout is valid.
			// It also clears out any existing messages in the queue.
			while (api1->pRP1210_ReadMessage(primaryClient, buf, sizeof(buf), false) > 0);


			DWORD tick = GetTickCount();
			DWORD tickTimeStamps[3];
			for (i = 0; (i < 3) && (GetTickCount() < tick+2500); ) {
				int result = api1->pRP1210_ReadMessage(primaryClient, buf, sizeof(buf), blocking);
				if (result < 0) {
					log.LogText(_T("    Receive error"), Log::Red);
					LogError (*api1, -result);
					continue;
				}
				if (result == 0) { // Nothing received
					continue;
				}
				char *msg = buf+4;
				if (msg[0] == SecondaryPeriodicTX) {
					RecvMsgPacket p((unsigned char *)buf, result);
					tickTimeStamps[i] = (DWORD)(((__int64)p.timeStamp * (__int64)dev.timeStampWeight) / 1000); // Normalize to milliseconds
					// got what we are looking for.
					i++;
				}
			}
			if (i == 3) {
				log.LogText (_T("    Pass Basic Read (") + state + _T(")"));

				// Check timestamps to see if they look reasonable
				int d1 = tickTimeStamps[1] - tickTimeStamps[0];
				int d2 = tickTimeStamps[2] - tickTimeStamps[1];
				bool tsConcern = false;
				if ((d1 < 450) || (d1 > 550)) {
					CString msg;
					msg.Format(_T("    Possible timestamp problem.  Interval1 was: %d ms (%d, %d)"), d1, tickTimeStamps[0], tickTimeStamps[1]);
					log.LogText(msg, Log::Red);
					tsConcern = true;
				}
				if ((d2 < 450) || (d2 > 550)) {
					CString msg;
					msg.Format(_T("    Possible timestamp problem.  Interval2 was: %d ms (%d, %d)"), d2, tickTimeStamps[1], tickTimeStamps[2]);
					log.LogText(msg, Log::Red);
					tsConcern = true;
				}
				if (!tsConcern) {
					log.LogText(_T("    Time stamps (") + state + _T(") seem good"));
				}
			} else {
				log.LogText (_T("    Timeout (" + state + _T(")")), Log::Red);
			}
		}
	}
}

/*********************************/
/* TestRP1210::TestAdvancedRead */
/*******************************/
void TestRP1210::TestAdvancedRead(INIMgr::Devices &dev, int primaryClient) {
	log.LogText (_T("Testing: ReadMessage (advanced)"), Log::Blue);
	
	char largeBuf[256];
	char smallBuf[5];
	int result;

	// Invalid Client ID
	result = api1->pRP1210_ReadMessage(127, largeBuf, sizeof(largeBuf), false);
	if (result == -ERR_INVALID_CLIENT_ID) {
		log.LogText(_T("    Passed (attempt to read with invalid ID.)"));
	} else {
		log.LogText(_T("    Failed (attempt to read with invalid ID.  Wrong error returned)"), Log::Red);
		LogError (*api1, -result);
	}

	// Try killing a connection during a blocking read
	// Also verifies that we can call a second function while blocking on a read!
	toKillClient = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0, 0, 0);
	if (IsValid(toKillClient)) {
		char filter[] = { NeverSent }; 
		int result = api1->pRP1210_SendCommand(SetMessageFilteringForJ1708, toKillClient, filter, sizeof(filter));
		if (IsValid(result)) {
			CWinThread *worker;
			{
				CritSection crit; // need to get rid of this variable as soon as we are ready to read so that the other thread can progress
				SetupWorkerThread(worker, KillSpecifiedConnection, true);
			}
			result = api1->pRP1210_ReadMessage(toKillClient, largeBuf, sizeof(largeBuf), true);
		
			if (result == ERR_CLIENT_DISCONNECTED) {
				log.LogText(_T("    Passed (disconnect while blocking on read)"));
			} else if (result >= 0) {
				CString msg;
				msg.Format(_T("    Failed (disconnect while blocking on read) %d bytes read"), result);
				log.LogText(msg, Log::Red);
			} else {
				log.LogText(_T("    Failed (disconnect while blocking on read,   Wrong error returned)"), Log::Red);
				LogError(*api1, -result);
			}
		} else {
			LogError(*api1, result);
		} 
	} else {
		log.LogText (_T("   Unable to open helper connection!"), Log::Red);
	}

	// slip in a random valid call
	result = api1->pRP1210_ReadMessage(primaryClient, largeBuf, sizeof(largeBuf), true);
	if (result < 0) {
		log.LogText(_T("    Unexpected error return code"), Log::Red);
		LogError (*api1, -result);
	}

	result = api1->pRP1210_ReadMessage(primaryClient, smallBuf, sizeof(smallBuf), true);
	if (result == -ERR_MESSAGE_TOO_LONG) {
		log.LogText(_T("    Passed (read, insufficient buffer)"));
	} else {
		log.LogText(_T("    Failed (read, insufficient buffer)"), Log::Red);
		if (result < 0) { LogError (*api1, -result); }
	}
}

/******************************/
/* TestRP1210::TestMultiRead */
/****************************/
void TestRP1210::TestMultiRead(INIMgr::Devices &dev, int primaryClient) {
	log.LogText (_T("Testing: ReadMessage (multiread)"), Log::Blue);
#pragma message ("Once filters are working, retest with proper filtering.  Remove the comment from 'Unexpected read data'")
	int i;
	int connections[4];
	bool passed = true;
	for (i = 0; i < 4; i++) {
TRACE ("Opening connection %d\n", i);
		connections[i] = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
		if (!IsValid(connections[i])) {
			LogError(*api1, connections[i]);
			passed = false;
		}
		char multiSendFilter = TestMultisend;
		//int result = api1->pRP1210_SendCommand(SetMessageFilteringForJ1708, connections[i], &multiSendFilter, 1);
		int result = api1->pRP1210_SendCommand(SetAllFiltersToPass, connections[i], &multiSendFilter, 0);
		if (!IsValid(result)) {
			LogError(*api1, result);
			passed = false;
		}
	}

	// send a message out on each conneciton
	for (i = 0; i < 4; i++) {
TRACE ("Transmitting on %d (%d)\n", i, connections[i]);
		char msg[] = { 4, TestMultisend, i };
		int result = api1->pRP1210_SendMessage(connections[i], msg, sizeof(msg), false, true);
		//int result = api2->pRP1210_SendMessage(secondaryClient, msg, sizeof(msg), false, true);
		if (!IsValid(result)) {
			LogError(*api1, result);
			passed = false;
		}
	}

	// read all the messages out of each buffer
	for  (i = 0; i < 4; i++) {
TRACE ("Reading on %d (%d)\n", i, connections[i]);
		char rxBuf[256];
		bool foundMessage[4] = { false, false, false, false };
		int result = api1->pRP1210_ReadMessage(connections[i], rxBuf, sizeof(rxBuf), false);
		while (result > 0) {
			RecvMsgPacket pkt(rxBuf, result);
			if ((pkt.data.size() == 2) && (pkt.data[0] == TestMultisend) && (pkt.data[1] < 4)) {
				foundMessage[pkt.data[1]] = true;
			} else {
				log.LogText (_T("    Unexpected read data"), Log::Red);
				passed = false;
			}
			result = api1->pRP1210_ReadMessage(connections[i], rxBuf, sizeof(rxBuf), false);
		}
		if (result < 0) {
			log.LogText(_T("    Unexpected error on read"), Log::Red);
			passed = false;
			LogError(*api1, -result);
			break;
		}

		for (int j = 0; j < 4; j++) {
			if ((j != i) && (!foundMessage[j])) {
				CString msg;
				msg.Format (_T("    Client %d didn't receive message %d"), i, j);
				log.LogText(msg, Log::Red);
				passed = false;
			} else if ((j == i) && foundMessage[j]) {
				CString msg;
				msg.Format (_T("    Client %d unexpectedly received message %d"), i, j);
				log.LogText(msg, Log::Red);
				passed = false;
			}
		}
	}

	for (i = 0; i < 4; i++) {
TRACE ("Closing connection %d\n", i);
		int result = api1->pRP1210_ClientDisconnect(connections[i]);
		if (!IsValid(result)) {
			LogError(*api1, result);
			passed = false;
		}
	}

	if (passed) {
		log.LogText(_T("    Passed"));
	} else {
		log.LogText(_T("    Failed"), Log::Red);
	}
}

/******************************/
/* TestRP1210::TestBasicSend */
/****************************/
void TestRP1210::TestBasicSend (int primaryClient) {
	log.LogText (_T("Testing: SendMessage (basic)"), Log::Blue);

	{
		CritSection sec;
		secondaryRxMsgs.clear();
	}

	int j;

	for (j = 0; j < 2; j++) {
		CString state = (j == 0) ? _T("Blocking") : _T("Non-Blocking");
		bool blocking = j == 0;

		char txBuf[] = { 4, PrimarySend, '0' };
		DWORD time = GetTickCount();
		for (int i = 0; i < 3; i++) {
			txBuf[2] = i+'0';
			api1->pRP1210_SendMessage(primaryClient, txBuf, sizeof(txBuf), false, blocking);
		}
		time = GetTickCount() - time;
		CString msg;
		msg.Format(_T("    Tx time was %d"), time);
		if (blocking && (time < 100))

		Sleep(40);
		DWORD tick = GetTickCount();
		int i = 0;
		while (tick + (blocking ? 25 : 2000) > GetTickCount()) {
			{
				CritSection sec;
				while (secondaryRxMsgs.size() > 0) {
					RecvMsgPacket pkt = *secondaryRxMsgs.begin();
					secondaryRxMsgs.pop_front();
					
					if (pkt.data.size() != 2) {
						continue;
					}
					if (pkt.data[0] != PrimarySend) {
						continue;
					}
					if (pkt.data[1] != i + '0') {
						log.LogText(_T("    Unexpected packet data contents"), Log::Red);
						continue;
					}
					i++;
					if (i >= 3) {
						break;
					}
				}
			}
			Sleep(10);
		}

		if (i >= 3) {
			log.LogText(_T("    Passed (") + state + _T(")"));
		} else {
			CString msg;
			msg.Format (_T("    Failed. (") + state + _T(") %d msgs received"), i);
			log.LogText(msg, Log::Red);
		}
	}
}

/*********************************/
/* TestRP1210::TestAdvancedSend */
/*******************************/
void TestRP1210::TestAdvancedSend(INIMgr::Devices &dev, int primaryClient) {
	log.LogText (_T("Testing: SendMessage (advanced)"), Log::Blue);

	// send to invalid ClientID
	char txBuffer[] = { 4, PrimarySend, 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };
	int result = api1->pRP1210_SendMessage(127, txBuffer, sizeof(txBuffer), false, true);
	if (result == ERR_INVALID_CLIENT_ID) {
		log.LogText(_T("    Passed (attempt to send with invalid ID.)"));
	} else {
		log.LogText(_T("    Failed (attempt to send with invalid ID.  Wrong error returned)"), Log::Red);
		LogError (*api1, result);
	}


	// send several illegal strings
	struct ErrorSend {
		ErrorSend(TestRP1210 *owner, char *data, int len, const INIMgr::Devices &dev, CString testName, int expectedReturnValue, bool &failed) { 
			int errClient = owner->api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
			if (!IsValid(errClient)) {
				log.LogText (_T("    Error opening connection (") + testName + _T(")"), Log::Red);
				owner->LogError(*owner->api1, errClient);
				failed = true;
			} else {
				int result;
				result = owner->api1->pRP1210_SendMessage (errClient, data, len, false, true);
				if (result != expectedReturnValue) {
					log.LogText (_T("    Wrong error returned (") + testName + _T(")"), Log::Red);
					owner->LogError(*owner->api1, result);
					failed = true;
				}
				
				result = owner->api1->pRP1210_ClientDisconnect(errClient);
				if (!IsValid(result)) {
					log.LogText (_T("    Error closing connection (") + testName + _T(")"), Log::Red);
					owner->LogError(*owner->api1, result);
					failed = true;
				}
			}
		}
	};

	bool failed = false;
	ErrorSend (this, "Fpfty hour weeks are rough", 10, dev, _T("Invalid priority"), ERR_MESSAGE_NOT_SENT, failed);
	ErrorSend (this, "", 0, dev, _T("Zero Length message"), ERR_MESSAGE_NOT_SENT, failed);
	char buf[1000] = { 4, PrimarySend, 'a', 'b', 'c', 'd', 'e' };
	ErrorSend (this, buf, sizeof(buf), dev, _T("Message Too Long"), ERR_MESSAGE_TOO_LONG, failed);
		
	if (!failed) {
		log.LogText(_T("    Passed (several error arrays to SendMessage)"));
	} else {
		log.LogText(_T("    Failed (several error arrays to SendMessage)"), Log::Red);
	}

	// Try killing a connection during a blocking send
	// Also verifies that we can call a second function while blocking on a send!
	toKillClient = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0, 0, 0);
	if (IsValid(toKillClient)) {
		char filter[] = { NeverSent }; 
		int result = api1->pRP1210_SendCommand(SetMessageFilteringForJ1708, toKillClient, filter, sizeof(filter));
		if (IsValid(result)) {
			char txBuffer[] = { 7, PrimarySend, 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };

			CWinThread *worker;
			{
				CritSection crit; // need to get rid of this variable as soon as we are ready to read so that the other thread can progress
				SetupWorkerThread(worker, KillSpecifiedConnection, true);
			}
		
			// This timing here is very tricky.  It may not be possible to reliably test this.
			// The iterations and sleep value probably vary for each processor.
			Sleep(300);
			for (int i = 0; i < 10; i++) {
				result = api1->pRP1210_SendMessage(toKillClient, txBuffer, sizeof(txBuffer), false, false);
				if (!IsValid(result)) {
					LogError(*api1, result);
				}
			}
			result = api1->pRP1210_SendMessage(toKillClient, txBuffer, sizeof(txBuffer), false, true);
		
			if (result == ERR_CLIENT_DISCONNECTED) {
				log.LogText(_T("    Passed (disconnect while blocking on send)"));
			} else if (result == 0) {
				CString msg;
				msg.Format(_T("    Failed (SendMessage returned 0.  This may actually not be an error as the timing is very tricky to setup)"), result);
				log.LogText(msg, Log::Red);
			} else {
				log.LogText(_T("    Failed (disconnect while blocking on send,   Wrong error returned)"), Log::Red);
				LogError(*api1, -result);
			}
		} else {
			LogError(*api1, result);
		} 
	} else {
		log.LogText (_T("   Unable to open helper connection!"), Log::Red);
	}
}

/******************************/
/* TestRP1210::TestWinNotify */
/****************************/
void TestRP1210::TestWinNotify(INIMgr::Devices &dev) {
	log.LogText (_T("Testing: Window Notification"), Log::Blue);
	DlgTestWinNotify dlg(this, dev);
	dlg.DoModal();
}

/*************************/
/* TestRP1210::LogError */
/***********************/
void TestRP1210::LogError (RP1210API &api, int code, COLORREF clr) {
	CritSection crit;
	char buf[120];
	if (api.pRP1210_GetErrorMsg(code, buf) == 0) {
		log.LogText(_T("        API returned error=") + CString(buf), clr);
	} else {
		CString fmt;
		fmt.Format(_T("        No error code for error %d"), code);
		log.LogText(fmt, clr);
	}
}

/**********************************/
/* TestRP1210::SetupWorkerThread */
/********************************/
void TestRP1210::SetupWorkerThread(CWinThread *&thread, AFX_THREADPROC threadProc, bool autoDelete) {
	thread = AfxBeginThread(threadProc, (LPVOID)this, 0, CREATE_SUSPENDED);
	thread->m_bAutoDelete = autoDelete;
	ResumeThread(thread->m_hThread);
}

/****************************/
/* TestRP1210::KillThreads */
/**************************/
void TestRP1210::KillThreads() {
	threadsMustDie = true;

	DestroyThread(helperTxThread);
	DestroyThread(helperRxThread);

	threadsMustDie = false;
}

/******************************/
/* TestRP1210::DestroyThread */
/****************************/
void TestRP1210::DestroyThread(CWinThread *&thread) {
	if (thread != NULL) {
		DWORD exitCode = 1;
		VERIFY(GetExitCodeThread(thread->m_hThread, &exitCode));
		while (exitCode == STILL_ACTIVE) {
			Sleep(1);
			VERIFY(GetExitCodeThread(thread->m_hThread, &exitCode));
		}
		delete thread;
		thread = NULL;
	}
}


/****************************************/
/* TestRP1210::SecondaryDeviceTXThread */
/**************************************/
UINT __cdecl TestRP1210::SecondaryDeviceTXThread( LPVOID pParam ) {
	TestRP1210 *thisObj = (TestRP1210*)pParam;
	int count = 0;
	while (!thisObj->threadsMustDie) {
		//char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
		char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', ' ', '-', '-', '-', '-' };
		char strBuf[24];
		sprintf (strBuf, "%05d", count++);
		memcpy(basicJ1708TxBuf + 14, strBuf+strlen(strBuf)-4, 4);
		int result = thisObj->api2->pRP1210_SendMessage(thisObj->secondaryClient, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, true);
		if (result > 127) {
			thisObj->LogError (*(thisObj->api2), result);
		}
		Sleep (500);
	}
	return 0;
}

/****************************************/
/* TestRP1210::SecondaryDeviceRXThread */
/**************************************/
UINT __cdecl TestRP1210::SecondaryDeviceRXThread( LPVOID pParam ) {
	TestRP1210 *thisObj = (TestRP1210*)pParam;

	thisObj->api2->pRP1210_SendCommand(SetAllFiltersToPass, thisObj->secondaryClient, "", 0);
	//int resultEcho = thisObj->api2->pRP1210_SendCommand(SetEchoTxMsgs, thisObj->secondaryClient, "1", 1);
	while (!thisObj->threadsMustDie) {
		char buf[525] = {0};
		int result = thisObj->api2->pRP1210_ReadMessage(thisObj->secondaryClient, buf, sizeof(buf), true);
		
		// Ignore custom error messages (probably indicating that a block timed out)
		if (result < 0) {
			if (result < -192) {
				continue;
			}
			thisObj->LogError (*(thisObj->api2), -result);
		} else {
			CritSection sec;
TRACE (_T("Received %d byte message %s\n"), result, CString(buf+4));
			thisObj->secondaryRxMsgs.push_back(RecvMsgPacket((unsigned char *)buf, result));
		}
		Sleep(10);
	}

	return 0;
}

/****************************************/
/* TestRP1210::KillSpecifiedConnection */
/**************************************/
UINT __cdecl TestRP1210::KillSpecifiedConnection( LPVOID pParam ) {
	TestRP1210 *thisObj = (TestRP1210*)pParam;

	CritSection crit;
	Sleep(500);
	int result = thisObj->api1->pRP1210_ClientDisconnect(thisObj->toKillClient);
	if (!IsValid(result)) {
		thisObj->LogError(*thisObj->api1, result);
	}

	return 0;
}

