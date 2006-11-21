#include "StdAfx.h"
#include "TestRP1210.h"
#include "RP1210API.h"
#include "Log.h"
#include "INIMgr.h"
#include "DlgTestWinNotify.h"

#pragma warning (disable:4996)

#define PARM(a,b) (a), L#a L##b

#define MS_VC_EXCEPTION 0x406D1388
typedef struct tagTHREADNAME_INFO
{
   DWORD dwType; // Must be 0x1000.
   LPCSTR szName; // Pointer to name (in user addr space).
   DWORD dwThreadID; // Thread ID (-1=caller thread).
   DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;

/******************/
/* SetThreadName */
/****************/
void SetThreadName( DWORD dwThreadID, LPCSTR szThreadName)
{
   THREADNAME_INFO info;
   info.dwType = 0x1000;
   info.szName = szThreadName;
   info.dwThreadID = dwThreadID;
   info.dwFlags = 0;

   __try
   {
      RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
   }
   __except(EXCEPTION_CONTINUE_EXECUTION)
   {
   }
}


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
								toKillClient(-1), sendSpectrum(false)
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

/************************************/
/* TestRP1210::TestCustomMultiread */
/**********************************/
void TestRP1210::TestCustomMultiread(INIMgr::Devices &dev, INIMgr::Devices &dev2) {
	int cl1 = api1->pRP1210_ClientConnect (NULL, dev.deviceID, "J1708", 0,0,0);
	int cl2 = api1->pRP1210_ClientConnect (NULL, dev.deviceID, "J1708", 0,0,0);
	int dev2Cl = api2->pRP1210_ClientConnect(NULL, dev2.deviceID, "J1708", 0,0,0);

	char multiSendFilter = TestMultisend;
	int result = api1->pRP1210_SendCommand(SetMessageFilteringForJ1708, cl1, &multiSendFilter, 1);
	int result2 = api1->pRP1210_SendCommand(SetMessageFilteringForJ1708, cl2, &multiSendFilter, 1);
	//int result = api1->pRP1210_SendCommand(SetAllFiltersToPass, cl1, NULL, 0);

	char msg[] = { 4, TestMultisend, '*' };
	int sendResult = api1->pRP1210_SendMessage (cl2, msg, sizeof(msg), false, true);
	int sendResult2 = api2->pRP1210_SendMessage(dev2Cl, "\x04pHello World", 13, false, true);

	char rxBuf[1024];
	int rcvResult = api1->pRP1210_ReadMessage (cl1, rxBuf, sizeof(rxBuf), true);

	if (rcvResult < 0) {
		LogError (*api1, -rcvResult);
	} else {
		rxBuf[rcvResult] = 0;
		TRACE ("Msg 1 (%d bytes) = %s\n", rcvResult-4, rxBuf+4);
	}

	int rcvResult2= api1->pRP1210_ReadMessage (cl1, rxBuf, sizeof(rxBuf), true);
	if (rcvResult2 >= 0) {
		rxBuf[rcvResult2] = 0;
		TRACE ("Msg 2 (%d bytes) = %s\n", rcvResult2-4, rxBuf+4);
		log.LogText (_T("Got message, but shouldn't have"), Log::Red);
	}

	return;
}

/**********************************/
/* TestRP1210::TestCustomAdvSend */
/********************************/
void TestRP1210::TestCustomAdvSend(INIMgr::Devices &dev) {
	CString testName("Zero Length message");
	char *data = NULL;
	int len = 0;

	int errClient = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
	bool failed = false;
	if (!IsValid(errClient)) {
		log.LogText (_T("    Error opening connection (") + testName + _T(")"), Log::Red);
		LogError(*api1, errClient);
		failed = true;
	} else {
		int result;
		result = api1->pRP1210_SendMessage (errClient, data, len, false, true);
		if (result != ERR_MESSAGE_NOT_SENT) {
			log.LogText (_T("    Wrong error returned (") + testName + _T(")"), Log::Red);
			LogError(*api1, result);
			failed = true;
		} else {
			log.LogText(_T("    Passed: ") + testName);
		}
		
		result = api1->pRP1210_ClientDisconnect(errClient);
		if (!IsValid(result)) {
			log.LogText (_T("    Error closing connection (") + testName + _T(")"), Log::Red);
			LogError(*api1, result);
			failed = true;
		}
	}
}

struct CustomResetInfo {
	RP1210API *api2;
	int secondaryClient;
};
/****************************/
/* CustomResetSecondThread */
/**************************/
UINT __cdecl CustomResetSecondThread ( LPVOID pParam ) {
	int count = 0;
	CustomResetInfo *cri = (CustomResetInfo *)pParam;
	RP1210API *api2 = cri->api2;
	while (true) {
		DWORD time = GetTickCount();
		char basicJ1708TxBuf [] = { 4, 112, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', ' ', '-', '-', '-', '-' };

		int result = api2->pRP1210_SendMessage(cri->secondaryClient, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, true);
		while (GetTickCount() < time+500) { Sleep(10); }
	}
	return 0;
}


/********************************/
/* TestRP1210::TestCustomReset */
/******************************/
void TestRP1210::TestCustomReset (INIMgr::Devices &dev, INIMgr::Devices &dev2) {
	static CustomResetInfo cri;
	cri.secondaryClient = api2->pRP1210_ClientConnect(NULL, dev2.deviceID, "J1708", 0, 0, 0);
	cri.api2 = api2;
	AfxBeginThread(CustomResetSecondThread, (LPVOID)&cri, 0, 0);


	int primaryClient = api1->pRP1210_ClientConnect (NULL, dev.deviceID, "J1708", 0,0,0);

	int result = api1->pRP1210_SendCommand(ResetDevice, primaryClient, NULL, 0);
	if (result != 0) {
		log.LogText(_T("    Device did not reset!"), Log::Red);
		LogError(*api1, result);
		return;
	}

	result = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
	if (!IsValid(result)) {
		log.LogText(_T("    MAJOR ERROR.  Did not reconnect.  Everthing else will probably fail!!!!"), Log::Red);
		LogError (*api1, result);
	} else {
		api1->pRP1210_ClientDisconnect(result);
	}

	log.LogText (_T("Done"));
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

// specialized tests to hone in on errors
//TestCustomMultiread(devs[idx1], devs[idx2]);
//TestCustomAdvSend(devs[idx1]);
//TestCustomAdvSend(devs[idx1]);
//TestCustomReset(devs[idx1], devs[idx2]);
//return;

	TestReadVersion();
	TestConnect(devs, idx1);
	TestMulticonnect(devs, idx1);

	// Setup thread for secondary J1708 communication
	secondaryClient = api2->pRP1210_ClientConnect(NULL, devs[idx2].deviceID, "J1708", 0, 0, 0);
	if (!IsValid(secondaryClient)) {
		log.LogText (_T("    Connecting to secondary device failed"), Log::Red);
		LogError (*api2, secondaryClient);
		secondaryClient = -1;
		goto end;
	}
	SetupWorkerThread (helperTxThread, SecondaryDeviceTXThread, false, _T("Secondary_TX"));
	SetupWorkerThread (helperRxThread, SecondaryDeviceRXThread, false, _T("Secondary_RX"));

	// Setup Primary Client
	int primaryClient = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, "J1708", 0, 0, 0);
	if (!IsValid(primaryClient)) {
		log.LogText (_T("    Connecting to primary device failed"), Log::Red);
		LogError (*api1, primaryClient);
		primaryClient = -1;
		goto end;
	}
	// Set filter states to pass
	api1->pRP1210_SendCommand(SetAllFiltersToPass, primaryClient, NULL, 0);

	// RON:  Comment or uncomment these to test
	TestBasicRead(devs[idx1], primaryClient);
	TestAdvancedRead(devs[idx1], primaryClient);
	TestMultiRead(devs[idx1], primaryClient);
	TestBasicSend(primaryClient);
	TestAdvancedSend(devs[idx1], primaryClient);
	TestWinNotify(devs[idx1]);
//
//	TestSendCommandReset(devs[idx1], primaryClient);
	TestFilterStatesOnOffMessagePassOnOff(devs[idx1], primaryClient);
	TestFilters (devs[idx1], primaryClient);

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

#ifdef WINCE
	size_t targetCount = 16;
#else
	size_t targetCount = 4;
#endif
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
			for (i = 0; (i < 3) && (GetTickCount() < tick+3500); ) {
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
				if ((d1 < 350) || (d1 > 650)) {
					CString msg;
					msg.Format(_T("    Possible timestamp problem.  Interval1 was: %d ms (%d, %d)"), d1, tickTimeStamps[0], tickTimeStamps[1]);
					log.LogText(msg, 0x0080FF);
					tsConcern = true;
				}
				if ((d2 < 450) || (d2 > 550)) {
					CString msg;
					msg.Format(_T("    Possible timestamp problem.  Interval2 was: %d ms (%d, %d)"), d2, tickTimeStamps[1], tickTimeStamps[2]);
					log.LogText(msg, 0x0080FF);
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
				SetupWorkerThread(worker, KillSpecifiedConnection, true, _T("Adv_read_kill_connection"));
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
//#pragma message ("Once filters are working, retest with proper filtering.  Remove the comment from 'Unexpected read data'")
	int i;
	int connections[4];
	bool passed = true;
	sendSpectrum = false;
	for (i = 0; i < 4; i++) {
TRACE (_T("Opening connection %d\n"), i);
		connections[i] = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
		if (!IsValid(connections[i])) {
			LogError(*api1, connections[i]);
			passed = false;
		}
		char multiSendFilter = TestMultisend;
		VerifyValidSendCommand(SetAllFilterStatesToDiscard, _T("Discard"), connections[i], NULL, 0, false);
		int result = api1->pRP1210_SendCommand(SetMessageFilteringForJ1708, connections[i], &multiSendFilter, 1);
		//int result = api1->pRP1210_SendCommand(SetAllFiltersToPass, connections[i], &multiSendFilter, 0);
		if (!IsValid(result)) {
			LogError(*api1, result);
			passed = false;
		}
	}

	// send a message out on each conneciton
	for (i = 0; i < 4; i++) {
TRACE (_T("Transmitting on %d (%d)\n"), i, connections[i]);
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
TRACE (_T("Reading on %d (%d)\n"), i, connections[i]);
		char rxBuf[256] = {0};
		bool foundMessage[4] = { false, false, false, false };
		int result = api1->pRP1210_ReadMessage(connections[i], rxBuf, sizeof(rxBuf), false);
		while (result > 0) {
			RecvMsgPacket pkt(rxBuf, result);
			if ((pkt.data.size() == 2) && (pkt.data[0] == TestMultisend) && (pkt.data[1] < 4)) {
				foundMessage[pkt.data[1]] = true;
			} else {
rxBuf[result] = '\0';
TRACE (("Odd Read %d bytes %s\n"), pkt.data.size(), rxBuf+4);
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
TRACE (_T("Closing connection %d\n"), i);
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
		int i = 0;
		CString state = (j == 0) ? _T("Blocking") : _T("Non-Blocking");
		bool blocking = j == 0;

		char txBuf[] = { 4, PrimarySend, '0' };
		DWORD time = GetTickCount();
		for (i = 0; i < 3; i++) {
			txBuf[2] = i+'0';
			api1->pRP1210_SendMessage(primaryClient, txBuf, sizeof(txBuf), false, blocking);
		}
		time = GetTickCount() - time;
		CString msg;
		msg.Format(_T("    Tx time was %d"), time);
		if (blocking && (time < 100))

		Sleep(40);
		DWORD tick = GetTickCount();
		i = 0;
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
						goto doubleBreak;
						break;
					}
				}
			}
			Sleep(10);
		}
doubleBreak:

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
				} else {
					log.LogText(_T("    Passed: ") + testName);
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
//log.LogText (_T("    Skipping Invalid Priority Test.  Don't skip this once Ron has fixed the problem"), Log::Red);
	ErrorSend (this, "Fpfty hour weeks are rough", 10, dev, _T("Invalid priority"), ERR_NOT_ADDED_TO_BUS, failed);
	ErrorSend (this, NULL, 0, dev, _T("Zero Length message"), ERR_MESSAGE_NOT_SENT, failed);
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
				SetupWorkerThread(worker, KillSpecifiedConnection, true, _T("Adv_Send_Kill_Conn"));
			}
		
			// This timing here is very tricky.  It may not be possible to reliably test this.
			// The iterations and sleep value probably vary for each processor.
			//Sleep(300);
			for (int i = 0; i < 5000; i++) {
				result = api1->pRP1210_SendMessage(toKillClient, txBuffer, sizeof(txBuffer), false, true);

				if (result == ERR_CLIENT_DISCONNECTED) {
					log.LogText(_T("    Passed (disconnect while blocking on send)"));
					break;
				} else if (result == 0) {
					continue;
				} else if (result == ERR_INVALID_CLIENT_ID) {
					log.LogText(_T("    Possible failure while blocking on send.  Client may have been disconnected between loop iterations.   Retest may be in order"), 0x8000FF);
					break;
				} else {
					log.LogText(_T("    Failed (disconnect while blocking on send,   Wrong error returned)"), Log::Red);
					LogError(*api1, result);
					break;
				}
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

/******************************/
/* TestRP1210::VerifyConnect */
/****************************/
int TestRP1210::VerifyConnect(INIMgr::Devices &dev, char *protocol) {
	int result = api1->pRP1210_ClientConnect (NULL, dev.deviceID, protocol, 0,0,0);
	if (!IsValid(result)) {
		log.LogText (_T("    Connection failure"), Log::Red);
		LogError(*api1, result);
	}
	return result;
}

/*********************************/
/* TestRP1210::VerifyDisconnect */
/*******************************/
int TestRP1210::VerifyDisconnect(int clientID) {
	if (!IsValid(clientID)) {
		return -1;
	}
	int result = api1->pRP1210_ClientDisconnect (clientID);
	if (!IsValid(result)) {
		log.LogText (_T("    Disconnect failure"), Log::Red);
		LogError(*api1, result);
	}
	return result;
}


/*****************************************/
/* TestRP1210::VerifyInvalidSendCommand */
/***************************************/
void TestRP1210::VerifyInvalidSendCommand(int cmd, CString text, int clientID, char *cmdData, int len, int expectedResult) {
	int result = api1->pRP1210_SendCommand(cmd, clientID, cmdData, len);
	if (result == expectedResult) {
		log.LogText(_T("    SendCommand(") + text + _T(") passed"));
	} else if (result == 0) {
		log.LogText (_T("    SendCommand(") + text + _T(") failed.  Expected an error, but command returned success(0)"), Log::Red);
	} else {
		log.LogText(_T("    SendCommand(") + text + _T(") failed."), Log::Red);
		LogError (*api1, result);
	}
}

/***************************************/
/* TestRP1210::VerifyValidSendCommand */
/*************************************/
void TestRP1210::VerifyValidSendCommand (int cmd, CString text, int clientID, char *cmdData, int len, bool logSuccess) {
	int result = api1->pRP1210_SendCommand(cmd, clientID, cmdData, len);
	if (result == 0) {
		if (logSuccess) {
			log.LogText(_T("    SendCommand(") + text + _T(") passed"));
		}
	} else {
		log.LogText(_T("    SendCommand(") + text + _T(") failed."), Log::Red);
		LogError (*api1, result);
	}	
}

/*****************************/
/* TestRP1210::VerifiedRead */
/***************************/
int TestRP1210::VerifiedRead (int clientID, char *rxBuf, int rxLen, bool block) {
	int result = api1->pRP1210_ReadMessage (clientID, rxBuf, rxLen, block);
	if (result < 0) {
		log.LogText (_T("    Receive error"), Log::Red);
		LogError (*api1, -result);
	} 
//if (result >= 0) { rxBuf[result] = 0; }
//TRACE (_T("Verified Read.  Result=%d.  Data=%s\n"), result, (result >= 0) ? CString(rxBuf) : CString(""));
	return result;
}


/*************************************/
/* TestRP1210::TestSendCommandReset */
/***********************************/
void TestRP1210::TestSendCommandReset(INIMgr::Devices &dev, int &primaryClient) {
	log.LogText (_T("Testing: SendCommand(Reset)"), Log::Blue);

	int result = api1->pRP1210_SendCommand(ResetDevice, primaryClient, NULL, 0);
	if (result != 0) {
		log.LogText(_T("    Device did not reset!"), Log::Red);
		LogError(*api1, result);
		return;
	}

	// Verify that the conneciton was broken
	result = api1->pRP1210_SendCommand(ResetDevice, primaryClient, NULL, 0);
	if (result != ERR_INVALID_CLIENT_ID) {
		log.LogText (_T("    Device did not reset"), Log::Red);
		LogError (*api1, result);
	} else {
		log.LogText(_T("    Device properly reset"));
	}

	// Reestablish connection
	result = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
	if (!IsValid(result)) {
		log.LogText(_T("    MAJOR ERROR.  Did not reconnect.  Everthing else will probably fail!!!!"), Log::Red);
		LogError (*api1, result);
	}
	primaryClient = result;

	// Now try a reset with 2 clients connected
	int client2 = api1->pRP1210_ClientConnect(NULL, dev.deviceID, "J1708", 0,0,0);
	if (!IsValid(client2)) {
		log.LogText(_T("    Oops"), Log::Red);
		LogError (*api1, client2);
	}

	result = api1->pRP1210_SendCommand(ResetDevice, client2, NULL, 0);
	if (result == ERR_MULTIPLE_CLIENTS_CONNECTED) {
		log.LogText(_T("    Device properly refused to reset with multiple clients"));
	} else {
		log.LogText (_T("   Device reset, or returned unexpected error during reset with multiple connected clients"));
		LogError (*api1, result);
	}

	// Try with an invalid client
	VerifyInvalidClientIDSendCommand (ResetDevice, _T("ResetDevice"), NULL, 0);

	// Try with invalid command
	VerifyInvalidSendCommand (PARM(ResetDevice, "<Invalid command data>"), primaryClient, "\x01", 1, ERR_INVALID_COMMAND);
	
	VerifyDisconnect(client2);
}


/*****************************************************/
/* TestRP1210::TestFiterStatesOnOffMessagePassOnOff */
/***************************************************/
void TestRP1210::TestFilterStatesOnOffMessagePassOnOff(INIMgr::Devices &dev, int primaryClient) {
	log.LogText (_T("Testing: SendCommand(SetAllFiltersToDiscard, SetAllFiltersToPass, SetMessageReceive)"), Log::Blue);
	char rxBuf[1000];
	int result;
	
	VerifyInvalidClientIDSendCommand (PARM(SetAllFilterStatesToDiscard,""), NULL, 0);
	VerifyInvalidClientIDSendCommand (PARM(SetAllFiltersToPass,""), NULL, 0);
	VerifyInvalidClientIDSendCommand (PARM(SetMessageReceive,""), "\x01", 1);
	VerifyInvalidSendCommand(PARM(SetMessageReceive, "<Invalid parm>"), primaryClient, "\x02", 1, ERR_INVALID_COMMAND);
	VerifyInvalidSendCommand(PARM(SetMessageReceive, "<Invalid parm len (2)>"), primaryClient, "\x01\x00", 2, ERR_INVALID_COMMAND);
	VerifyInvalidSendCommand(PARM(SetMessageReceive, "<Invalid parm len (0)>"), primaryClient, NULL, 0, ERR_INVALID_COMMAND);

	VerifyInvalidSendCommand(PARM(SetAllFilterStatesToDiscard, "<Invalid parm len>"), primaryClient, "\x01", 1, ERR_INVALID_COMMAND);
	VerifyInvalidSendCommand(PARM(SetAllFiltersToPass, "<Invalid parm len>"), primaryClient, "\x01", 1, ERR_INVALID_COMMAND);


	// Read while filters discarding
	int clientDiscardAll = VerifyConnect (dev);
	int clientAcceptAll = VerifyConnect(dev);
	VerifyValidSendCommand (PARM(SetAllFiltersToPass,""), clientAcceptAll, NULL, 0, false);
	VerifyValidSendCommand (PARM(SetAllFilterStatesToDiscard,""), clientDiscardAll, NULL, 0, false);
	struct {
		int cmd;
		LPCTSTR cmdAsString;
		char *cmdData;
		int cmdDataLen;
		LPCTSTR resultZeroText;
		LPCTSTR resultNonZeroText;
		bool resultShouldBeNonZero;
	} commandInfo[] = {
		{ PARM(SetAllFilterStatesToDiscard,""), NULL, 0, _T("Passed: (discarding, nothing read)"), _T("Failed: (discarding, something read)"), false },
		{ PARM(SetAllFiltersToPass,""), NULL, 0, _T("Failed: (accepting, nothing read)"), _T("Passed: (accepting, something read)"), true },
		{ PARM(SetMessageReceive,""), "\x00", 1, _T("Passed: (MsgRcv=false, nothing read)"), _T("Failed: (MsgRcv=false, something read)"), false },
		{ PARM(SetMessageReceive,""), "\x01", 1, _T("Failed: (MsgRcv=true, nothing read)"), _T("Passed: (MsgRcv=true, something read)"), true }
	};
	for (int j = 0; j < 4; j++) {
		VerifyValidSendCommand (commandInfo[j].cmd, commandInfo[j].cmdAsString, primaryClient, commandInfo[j].cmdData, commandInfo[j].cmdDataLen, false);
		while (VerifiedRead(primaryClient, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
		while (VerifiedRead(clientDiscardAll, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
		while (VerifiedRead(clientAcceptAll, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer

		Sleep (1500); // should have received something by now
		result = VerifiedRead(primaryClient, rxBuf, sizeof(rxBuf), false);
		if (result == 0) {
			log.LogText (CString(_T("    ")) + commandInfo[j].resultZeroText, commandInfo[j].resultShouldBeNonZero ? Log::Red : Log::Black);
		} else {
			log.LogText(CString(_T("    ")) + commandInfo[j].resultNonZeroText, commandInfo[j].resultShouldBeNonZero ? Log::Black : Log::Red);
		}

		result = VerifiedRead (clientDiscardAll, rxBuf, sizeof(rxBuf), false);
		if (result != 0) {
			log.LogText (_T("    Discarding client received message!  Filters are messed up!!!"), Log::Red);
		}
		result = VerifiedRead (clientAcceptAll, rxBuf, sizeof(rxBuf), false);
		if (result <= 0) {
			log.LogText (_T("    Accepting client failed to received message!"), Log::Red);
		}
	}

	VerifyDisconnect (clientDiscardAll);
	VerifyDisconnect (clientAcceptAll);
}

/****************************/
/* TestRP1210::TestFilters */
/**************************/
void TestRP1210::TestFilters(INIMgr::Devices &dev, int primaryClient) {
	const int delay = 15000;
	log.LogText (_T("Testing: SendCommand(SetMessageFilteringForJ1708)"), Log::Blue);

	struct CollectMessageArray {
		bool msgRcvdFrom [6];
		CollectMessageArray() {
			Clear();
		}

		void Clear() {
			memset (msgRcvdFrom, 0, sizeof(msgRcvdFrom));
		}

		void Parse (TestRP1210 *test, int clientID, bool warnOutOfRange) {
			char rxBuf[1024];
			int result = test->VerifiedRead(clientID, rxBuf, sizeof(rxBuf), false);
			while (result > 0) {
				RecvMsgPacket pkt(rxBuf, result);
				if (pkt.data.size() >= 1) {
TRACE (_T("Received message (woor=%d).  MID=%d, msg=%s\n"), warnOutOfRange, pkt.data[0], CString(rxBuf+4));
					int idx = pkt.data[0] - (TestRP1210::SecondaryPeriodicTX+1);
					if ((idx >= 0) && (idx < 6)) {
						msgRcvdFrom[idx] = true;
					} else if (warnOutOfRange) {
						CString msg;
						msg.Format (_T("    Received message with an unexpected MID (%d)"), pkt.data[0]);
						log.LogText (msg, Log::Red);
					}
				}
				result = test->VerifiedRead(clientID, rxBuf, sizeof(rxBuf), false);
			}
		}

		bool Test (bool state0, bool state1, bool state2, bool state3, bool state4, bool state5, CString name) {
			bool stateArray[6] = { state0, state1, state2, state3, state4, state5 };
			if (memcmp (stateArray, msgRcvdFrom, sizeof(stateArray)) == 0) { return true; }
			CString expected, got;
			for (int i = 0; i < 6; i++) {
				expected += stateArray[i] ? _T("true , ") : _T("false, ");
				got += msgRcvdFrom[i] ? _T("true , ") : _T("false, ");
			}
			expected.TrimRight(_T(", "));
			got.TrimRight(_T(", "));
			log.LogText(_T("    Test failed (") + name + _T("): expected: [") + expected + _T("]  got: [") + got + _T("]"), Log::Red);
			return false;
		}

	};


	char rxBuf[1024];

	int clTx = VerifyConnect(dev);
	int clRx = VerifyConnect(dev);

	if (!IsValid(clTx) || (!IsValid(clRx))) {
		log.LogText(_T("    Unable to test filters."), Log::Red);

		VerifyDisconnect (clTx);
		VerifyDisconnect (clRx);
		return;
	}


	// test invalid
	VerifyInvalidClientIDSendCommand (PARM(SetMessageFilteringForJ1708,""), NULL, 0);

	// test main
	// -- shouldn't be necessary -- VerifyValidSendCommand (PARM(SetMessageReceive,""), primaryClient, "\x01", 1, false);
	// -- shouldn't be necessary -- VerifyValidSendCommand (PARM(SetMessageReceive,""), clRx, "\x01", 1, false);
	VerifyValidSendCommand (PARM(SetAllFilterStatesToDiscard, ""), primaryClient, NULL, 0, false);
	VerifyValidSendCommand (PARM(SetAllFiltersToPass, ""), clRx, NULL, 0, false);

	while (VerifiedRead(primaryClient, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
	sendSpectrum = true;

	char filterArray[] = { SecondaryPeriodicTX+1, SecondaryPeriodicTX+2, SecondaryPeriodicTX+3, SecondaryPeriodicTX+4, SecondaryPeriodicTX+5, SecondaryPeriodicTX+6 };

	// Filter on the first three
	CollectMessageArray primary, alt;
	VerifyValidSendCommand(PARM(SetMessageFilteringForJ1708,""), primaryClient, filterArray+0, 3, false);
	primary.Clear();
	alt.Clear();
	Sleep(delay);
	primary.Parse(this, primaryClient, true);
	alt.Parse(this, clRx, false);
	if (primary.Test(true, true, true, false, false, false, _T("primary <enable 3 filters>")) && alt.Test(true, true, true, true, true, true, _T("alt1"))) {
		log.LogText(_T("    Passed: enable 3 filters"));
	}

	// filter on the first four
	VerifyValidSendCommand(PARM(SetMessageFilteringForJ1708,""), primaryClient, filterArray+2, 2, false);
	while (VerifiedRead(primaryClient, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
	while (VerifiedRead(clRx, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
	primary.Clear();
	alt.Clear();
	Sleep(delay);
	primary.Parse(this, primaryClient, true);
	alt.Parse(this, clRx, false);
	if (primary.Test(true, true, true, true, false, false, _T("primary <enable 4 filters>")) && alt.Test(true, true, true, true, true, true, _T("alt2"))) {
		log.LogText(_T("    Passed: enable 4 filters"));
	}

	// disable all filters
	VerifyValidSendCommand(PARM(SetAllFilterStatesToDiscard,""), primaryClient, NULL, 0, false);
	while (VerifiedRead(primaryClient, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
	while (VerifiedRead(clRx, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
	primary.Clear();
	alt.Clear();
	Sleep(delay);
	primary.Parse(this, primaryClient, true);
	alt.Parse(this, clRx, false);
	if (primary.Test(false, false, false, false, false, false, _T("primary <disable all filters>")) && alt.Test(true, true, true, true, true, true, _T("alt3"))) {
		log.LogText(_T("    Passed: disable all filters"));
	}

	// now try from a shared port client
	sendSpectrum = false;
	Sleep(delay/3);
	while (VerifiedRead(primaryClient, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer
	while (VerifiedRead(clRx, rxBuf, sizeof(rxBuf), false) > 0); 	// clear read buffer

	// Filter on the last three
	VerifyValidSendCommand(PARM(SetMessageFilteringForJ1708,""), primaryClient, filterArray+3, 3, false);
	primary.Clear();
	alt.Clear();
	for (int i = 1; i <= 6; i++) {
		char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX + i, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
		int result = api1->pRP1210_SendMessage(clTx, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, false);
		if (!IsValid(result)) {
			LogError (*api1, result);
		}
	}

	Sleep(delay/15);
	primary.Parse(this, primaryClient, true);
	alt.Parse(this, clRx, false);
	if (primary.Test(false, false, false, true, true, true, _T("primary <enable 3 filters, same client tx>")) && alt.Test(true, true, true, true, true, true, _T("alt4"))) {
		log.LogText(_T("    Passed: enable 3 filters (same client tx)"));
	}

	// setup identical filters on both
	VerifyValidSendCommand (PARM(SetAllFilterStatesToDiscard, ""), primaryClient, NULL, 0, false);
	VerifyValidSendCommand (PARM(SetAllFilterStatesToDiscard, ""), clRx, NULL, 0, false);
	VerifyValidSendCommand(PARM(SetMessageFilteringForJ1708,""), primaryClient, filterArray+0, 3, false);
	VerifyValidSendCommand(PARM(SetMessageFilteringForJ1708,""), clRx, filterArray+0, 3, false);
	primary.Clear();
	alt.Clear();
	for (int i = 1; i <= 6; i++) {
		char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX + i, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
		int result = api1->pRP1210_SendMessage(clRx, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, false);
		if (!IsValid(result)) {
			LogError (*api1, result);
		}
	}
	Sleep(delay);
	primary.Parse(this, primaryClient, true);
	alt.Parse(this, clRx, false);
	if (primary.Test(true, true, true, false, false, false, _T("primary <test multiread copy>")) && alt.Test(false, false, false, false, false, false, _T("alt <multiread copy>"))) {
		log.LogText(_T("    Passed: multiread filter clone"));
	}


	VerifyDisconnect (clTx);
	VerifyDisconnect (clRx);
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
void TestRP1210::SetupWorkerThread(CWinThread *&thread, AFX_THREADPROC threadProc, bool autoDelete, CString threadName) {
	thread = AfxBeginThread(threadProc, (LPVOID)this, 0, CREATE_SUSPENDED);
	thread->m_bAutoDelete = autoDelete;

	SetThreadName(thread->m_nThreadID, INIMgr::ToAnsi(threadName));

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
			DoEvents();
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
		DWORD time = GetTickCount();
		//char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
		char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', ' ', '-', '-', '-', '-' };
		basicJ1708TxBuf[1] = SecondaryPeriodicTX;
		char strBuf[24];
		sprintf (strBuf, "%05d", count++);
		memcpy(basicJ1708TxBuf + 14, strBuf+strlen(strBuf)-4, 4);
		int result = thisObj->api2->pRP1210_SendMessage(thisObj->secondaryClient, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, true);
		if (!IsValid(result)) {
			thisObj->LogError (*(thisObj->api2), result);
		}

		if (thisObj->sendSpectrum) {
TRACE ("Sending Spectrum\n");
			for (int j = SecondaryPeriodicTX+1; j <= SecondaryPeriodicTX+6; j++) {
				basicJ1708TxBuf[1] = j;
				result = thisObj->api2->pRP1210_SendMessage(thisObj->secondaryClient, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, false);
				if (!IsValid(result)) {
					thisObj->LogError (*(thisObj->api2), result);
				}
			}
		}

		while (GetTickCount() < time+500) { Sleep(10); }
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

