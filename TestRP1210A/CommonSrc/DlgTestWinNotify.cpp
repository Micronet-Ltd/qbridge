// ..\..\CommonSrc\DlgTestWinNotify.cpp : implementation file
//

#include "stdafx.h"
#include "DlgTestWinNotify.h"
#include "TestRP1210.h"
#include "log.h"
#include "RP1210API.h"

extern bool IsValid(int result);
// DlgTestWinNotify dialog

IMPLEMENT_DYNAMIC(DlgTestWinNotify, CDialog)

/***************************************/
/* DlgTestWinNotify::DlgTestWinNotify */
/*************************************/
DlgTestWinNotify::DlgTestWinNotify(TestRP1210 *inTest, INIMgr::Devices &inDev, bool inIsJ1708, CWnd* pParent /*=NULL*/)
	: CDialog(DlgTestWinNotify::IDD, pParent), test(inTest), dev(inDev), recvCount(0), isJ1708(inIsJ1708)
{
	notifyMsg = RegisterWindowMessage(dev.msgString);
	errMsg = RegisterWindowMessage(dev.errString);
}

/****************************************/
/* DlgTestWinNotify::~DlgTestWinNotify */
/**************************************/
DlgTestWinNotify::~DlgTestWinNotify()
{
}

/*************************************/
/* DlgTestWinNotify::DoDataExchange */
/***********************************/
void DlgTestWinNotify::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(DlgTestWinNotify, CDialog)
END_MESSAGE_MAP()


// DlgTestWinNotify message handlers

/*********************************/
/* DlgTestWinNotify::WindowProc */
/*******************************/
LRESULT DlgTestWinNotify::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_APP) {
		UpdateWindow();
		PerformTest();
		EndDialog(IDOK);
	} else if (message == errMsg) {
		if ((wParam == ERR_TXMESSAGE_STATUS) || (wParam == ERR_CLIENT_DISCONNECTED)) {
			int msgNum = (int)(lParam & 0x7F);
			bool success = (lParam & 0x80) == 0;

			if (wParam == ERR_CLIENT_DISCONNECTED) {
				CString msg;
				msg.Format (_T("    Notice: client was disconnected before message #%d was sent"), msgNum);
				log.LogText(msg, 0x3030B0);
			}				

			if (!success) {
				CString msg;
				msg.Format (_T("    Notice: message #%d was not sent successfully"), msgNum);
				log.LogText(msg, 0x1010C0);
			}
			sent.insert(msgNum);
		} else if (wParam == 0xFFFFFFFF) {
			TRACE (_T("  Got this message %d.  Tick Count=%d\n"), lParam, GetTickCount());
		}
	} else if (message == notifyMsg) {
		recvCount++;
	}

	return CDialog::WindowProc(message, wParam, lParam);
}

/***********************************/
/* DlgTestWinNotify::OnInitDialog */
/*********************************/
BOOL DlgTestWinNotify::OnInitDialog()
{
	CDialog::OnInitDialog();

TRACE (_T("Init Dialog\n"));
	PostMessage(WM_APP);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

/************************************/
/* DlgTestWinNotify::FlushMessages */
/**********************************/
void DlgTestWinNotify::FlushMessages() {
	MSG msg;
	while (PeekMessage(&msg, *this, notifyMsg, notifyMsg, PM_REMOVE));
	while (PeekMessage(&msg, *this, errMsg, errMsg, PM_REMOVE));
}

/**********************************/
/* DlgTestWinNotify::PerformTest */
/********************************/
void DlgTestWinNotify::PerformTest() {
	char txJ1708[] = { 4, TestRP1210::PrimarySend, 'H', 'i', ' ', 'T', 'h', 'e', 'r', 'e' };
	TxBuffer txJ1939 (0x004400, 1, 4, 100, 255, "Hi There", 4);

	char *realTx;
	int realTxSize;
	char *protocolString;
	if (isJ1708) {
		realTx = txJ1708;
		realTxSize = sizeof(txJ1708);
		protocolString = "J1708";
	} else {
		realTx = txJ1939;
		realTxSize = txJ1939;
		protocolString = "J1939";
	}
	
	int result = -1;
	int i;
	bool passed = true;
	DWORD tick;
	set<int> msgNums;
	bool floodPass = false;
TRACE (_T("Starting Notify test\n"));
	int clientID = test->api1->pRP1210_ClientConnect(*this, dev.deviceID, protocolString, 0,0,0);
	if (!IsValid(clientID)) {
		log.LogText (_T("    Couldn't open client for testing"), Log::Red);
		test->LogError(*test->api1, clientID);
		passed = false;
		goto veryEnd;
	}

	if (!isJ1708) {
		test->VerifyProtectAddress(test->api1, clientID, 100, 1, 1);
	}

	result = test->api1->pRP1210_SendCommand (TestRP1210::SetAllFiltersToPass, clientID, "", 0);
	if (!IsValid(result)) {
		log.LogText(_T("    Unable to set filter states"), Log::Red);
		test->LogError(*test->api1, result);
		passed = false;
		goto end;
	}

	FlushMessages();

	// Test basic send message
	sent.clear();
	recvCount = 0;
PostMessage(errMsg, 0xFFFFFFFF, 1);
	for (i = 0; i < 15; i++) {
		result = test->api1->pRP1210_SendMessage(clientID, realTx, realTxSize, true, false);
		if (IsValid(result)) {
			msgNums.insert(result);
		} else {
			test->LogError (*test->api1, result);
			passed = false;
		}
	}
PostMessage(errMsg, 0xFFFFFFFF, 2);
	// try blocking for a bit.  This ensures that everything else has gone out
	result = test->api1->pRP1210_SendMessage(clientID, realTx, realTxSize, false, true);
PostMessage(errMsg, 0xFFFFFFFF, 3);

	// Now wait for 2 seconds or until the sets match
	tick = GetTickCount();
	while (tick + 2500 > GetTickCount()) {
		Sleep(20);
		TestRP1210::DoEvents();

		if (sent == msgNums) { 
			break;
		}
	}
TRACE (_T("After wait loop\n"));
	if (sent != msgNums) {
		vector <int> leftover(msgNums.size());
		size_t count = set_difference(msgNums.begin(), msgNums.end(), sent.begin(), sent.end(), leftover.begin()) - leftover.begin();
		CString msg;
		msg.Format(_T("    Didn't get all the notifications when sending messages.  %d notifications missing"), count);
		log.LogText (msg, Log::Red);
	} else {
		log.LogText (_T("    Received all notifications"));
	}

	// Try flooding the thing and see what happens
	// we test twice.  Once where we flood the thing with a pause between events, then again at full speed
	for (int j = 0; j < 2; j++) {
		floodPass = false;
		for (i = 0; i < 200; i++) {
			result = test->api1->pRP1210_SendMessage(clientID, realTx, realTxSize, true, false);
TRACE (_T("j=%d, i=%d, result=%d\n"), j, i, result);
			if (result == ERR_MAX_NOTIFY_EXCEEDED) {
				floodPass = true;
			} else if (result == 0) {
				log.LogText (_T("    Received 0 as a message number from SendMessage!"), Log::Red);
			} else if (!IsValid(result)) {
				TRACE (_T("Error (%d) at %d of flood\n"), result, i);
				test->LogError(*test->api1, result);
			}

			if (j == 0) {
				Sleep(10);
			}
		}

		CString speedString = (j == 0) ? _T("with delay") : _T("full speed");
		if (floodPass) {
			log.LogText (_T("    Flood (") + speedString + _T(") test passed"));
		} else {
			if (isJ1708) {
				log.LogText (_T("    Flood (") + speedString + _T(") test failed. (for delay test, this can be OK)"), Log::Red);
			} else {
				log.LogText (_T("    Flood (") + speedString + _T(") test is inconclusive for J1939"));
			}
		}

		if (j == 0) {
			TestRP1210::DoEvents();
			Sleep (1500); // Give everhting a chance to breath before we kill things
		}
	}

	Sleep(3000);
	// by now, we should have at least 4 message, and probably more messages receive notified.
	if (recvCount < 4) {
		CString msg;
		msg.Format (_T("    Failed receive notify.  Not enough message (%d) received"), recvCount);
		log.LogText(msg, Log::Red);
	} else {
		CString msg;
		msg.Format (_T("    Pass receive notify.  %d received"), recvCount);
		log.LogText(msg);
	}


PostMessage(errMsg, 0xFFFFFFFF, 4);
end:
	// try blocking for a bit.  This ensures that everything else has gone out
	result = test->api1->pRP1210_SendMessage(clientID, realTx, realTxSize, false, true);

	result = test->api1->pRP1210_ClientDisconnect(clientID);
	if (!IsValid(result)) {
		test->LogError(*test->api1, result);
	}
veryEnd:
	if (passed) {
		log.LogText(_T("    Passed"));
	} else {
		log.LogText(_T("    Failed"), Log::Red);
	}

	// allow any messages to catch up with us
	tick = GetTickCount();
	while (tick + 1000 > GetTickCount()) {
		Sleep(20);
		TestRP1210::DoEvents();
	}
TRACE (_T("Ending Notify test\n"));

}