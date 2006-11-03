#include "StdAfx.h"
#include "TestRP1210.h"
#include "RP1210API.h"
#include "Log.h"
#include "INIMgr.h"

TestRP1210::TestRP1210(void)
{
}

TestRP1210::~TestRP1210(void)
{
}

/*********************/
/* TestRP1210::Test */
/*******************/
void TestRP1210::Test (vector<INIMgr::Devices> &devs, int idx1, int idx2) {
	if ((idx1 < 0) || (idx2 < 0) || (idx1 >= (int)devs.size()) || (idx2 >= (int)devs.size())) {
		log.LogText(_T(" Invalid device index"), Log::Red);
		return;
	}
	
	RP1210API api1(devs[idx1].dll);
	RP1210API api2(devs[idx2].dll);

	if (api1.IsWorking()) {
		log.LogText(devs[idx1].dll + _T(" loaded"), Log::Green);
	} else {
		log.LogText(devs[idx1].dll + _T(" load failed"), Log::Red);
		return;
	}

	if (api2.IsWorking()) {
		log.LogText(devs[idx2].dll + _T(" loaded"), Log::Green);
	} else {
		log.LogText(devs[idx2].dll + _T(" load failed"), Log::Red);
		return;
	}

	// Test version
	log.LogText (_T("Testing: ReadVersion"), Log::Blue);
	{
		char vmaj[100] = "--------------", vmin[100] = "--------------", amaj[100] = "--------------", amin[100] = "--------------";
		api1.pRP1210_ReadVersion(vmaj, vmin, amaj, amin);
		CString vMaj(vmaj), vMin(vmin), aMaj(amaj), aMin(amin);
		log.LogText (_T("    DLL Version: ") + vMaj + _T(".") + vMin + _T("   API Version: ") + aMaj + _T(".") + aMin);
	}
	

	// Test Connect / Disconnect
	log.LogText (_T("Testing: ClientConnect/ClientDisconnect"), Log::Blue);
	for (int i = 0; i < devs[idx1].protocolStrings.size(); i++) {
		int result = api1.pRP1210_ClientConnect(NULL, devs[idx1].deviceID, INIMgr::ToAnsi(devs[idx1].protocolStrings[i]), 0, 0, 0);
		if ((result >= 0) && (result < 128)) {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[i] + _T(" succeeded"));
			
			result = api1.pRP1210_ClientDisconnect(result);
			if (result != 0) {
				LogError (api1, result);
			}
		} else {
			log.LogText(_T("    Connect, protocol=") + devs[idx1].protocolStrings[i] + _T(" Failed"), Log::Red);
			LogError(api1, result);
		}
	}


	log.LogText(_T("Done testing"), Log::Green);
}

/*************************/
/* TestRP1210::LogError */
/***********************/
void TestRP1210::LogError (RP1210API &api, int code, COLORREF clr) {
	char buf[120];
	if (api.pRP1210_GetErrorMsg(code, buf) == 0) {
		log.LogText(_T("        API returned error=") + CString(buf), clr);
	} else {
		CString fmt;
		fmt.Format(_T("        No error code for error %d"), code);
		log.LogText(fmt, clr);
	}
}