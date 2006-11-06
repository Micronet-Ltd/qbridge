#include "StdAfx.h"
#include "TestRP1210.h"
#include "RP1210API.h"
#include "Log.h"
#include "INIMgr.h"

CRITICAL_SECTION CritSection::critSection;
bool CritSection::critSectionInited;

bool TestRP1210::firstAlreadyCreated = false;

/***************************/
/* TestRP1210::TestRP1210 */
/*************************/
TestRP1210::TestRP1210(void) : helperTxThread(NULL), helperRxThread(NULL), threadsMustDie(false), secondaryClient(-1), api1(NULL), api2(NULL)
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

	// Test version
	log.LogText (_T("Testing: ReadVersion"), Log::Blue);
	{
		char vmaj[100] = "--------------", vmin[100] = "--------------", amaj[100] = "--------------", amin[100] = "--------------";
		api1->pRP1210_ReadVersion(vmaj, vmin, amaj, amin);
		CString vMaj(vmaj), vMin(vmin), aMaj(amaj), aMin(amin);
		log.LogText (_T("    DLL Version: ") + vMaj + _T(".") + vMin + _T("   API Version: ") + aMaj + _T(".") + aMin);
	}
	

	// Test Connect / Disconnect
/*	log.LogText (_T("Testing: ClientConnect/ClientDisconnect"), Log::Blue);
	for (size_t i = 0; i < devs[idx1].protocolStrings.size(); i++) {
		int result = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, INIMgr::ToAnsi(devs[idx1].protocolStrings[i]), 0, 0, 0);
		if ((result >= 0) && (result < 128)) {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[i] + _T(" succeeded"));
			
			result = api1->pRP1210_ClientDisconnect(result);
			if (result != 0) {
				log.LogText(_T("    Disconnect Failure!"), Log::Red);
				LogError (api1, result);
			}
		} else {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[i] + _T(" Failed"), Log::Red);
			LogError(api1, result);
		}
	}

	log.LogText (_T("Testing: Multiconnect"), Log::Blue);
	vector<int> ids;
	size_t targetCount = 16;
	for (size_t i = 0; i < targetCount; i++) {
		int result = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, INIMgr::ToAnsi(devs[idx1].protocolStrings[0]), 0, 0, 0);
		if ((result >= 0) && (result < 128)) {
			ids.push_back(result);
		} else {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[0] + _T(" Failed"), Log::Red);
			LogError(api1, result);
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
			LogError (api1, result);
		}
	}
*/

	// Setup thread for secondary J1708 communication
	secondaryClient = api2->pRP1210_ClientConnect(NULL, devs[idx2].deviceID, "J1708", 0, 0, 0);
	if ((secondaryClient > 128) || (secondaryClient < 0)) {
		log.LogText (_T("    Connecting to secondary device failed"), Log::Red);
		LogError (*api2, secondaryClient);
		secondaryClient = -1;
		goto end;
	}
	SetupWorkerThread (helperTxThread, SecondaryDeviceTXThread);
	SetupWorkerThread (helperRxThread, SecondaryDeviceRXThread);

	// Try a basic blocking receive:
	int primaryClient = api1->pRP1210_ClientConnect(NULL, devs[idx1].deviceID, "J1708", 0, 0, 0);
	if ((primaryClient >= 128) || (primaryClient < 0)) {
		log.LogText (_T("    Connecting to primary device failed"), Log::Red);
		LogError (*api1, primaryClient);
		primaryClient = -1;
		goto end;
	}
Sleep(3000);
	log.LogText (_T("Testing: ReadMessage (block, basic)"), Log::Blue);
	{
		char buf[256];
		int i;
		// This statement is intended to launch the helper process, so that the timeout is valid.  Otherwise it is useless
		int result1 = api1->pRP1210_ReadMessage(primaryClient, buf, sizeof(buf), true);
		DWORD tick = GetTickCount();
		for (i = 0; (i < 3) && (GetTickCount() < tick+250000); ) {
			int result = api1->pRP1210_ReadMessage(primaryClient, buf, sizeof(buf), true);
			if (result < 0) {
				log.LogText(_T("    Receive error"), Log::Red);
				LogError (*api1, -result);
				continue;
			}
			char *msg = buf+4;
			if (msg[0] == SecondaryPeriodicTX) {
				// got what we are looking for.
				i++;
			}
		}
		if (i == 3) {
			log.LogText (_T("    Pass"));
		} else {
			log.LogText (_T("    Timeout"), Log::Red);
		}
	}


end:
	if ((primaryClient >= 0) && (primaryClient < 128)) {
		api1->pRP1210_ClientDisconnect(primaryClient);	
	}
	log.LogText(_T("Done testing"), Log::Green);
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
void TestRP1210::SetupWorkerThread(CWinThread *&thread, AFX_THREADPROC threadProc) {
	thread = AfxBeginThread(threadProc, (LPVOID)this, 0, CREATE_SUSPENDED);
	thread->m_bAutoDelete = false;
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
	while (!thisObj->threadsMustDie) {
		char basicJ1708TxBuf [] = { 4, SecondaryPeriodicTX, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd' };
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
	while (!thisObj->threadsMustDie) {
		
	}
	return 0;
}
