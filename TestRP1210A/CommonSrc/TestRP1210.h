#pragma once

#include "INIMgr.h"

class RP1210API;

class CritSection {
public:
	CritSection() : paused(false) { ASSERT (critSectionInited); EnterCriticalSection(); }
	virtual ~CritSection() { if (!paused) {LeaveCriticalSection(); } ASSERT (!paused); }
	void Pause() { if (!paused) { paused = true; LeaveCriticalSection(); } }
	void Unpause() { ASSERT (paused); if (paused) { paused = false; EnterCriticalSection(); } }

	inline static void Init() { if (!critSectionInited) { ::InitializeCriticalSection(&critSection); critSectionInited = true; } }

	static inline BOOL TryEnterCriticalSection() { return ::TryEnterCriticalSection (&critSection); }
	static inline void TryLeaveCriticalSection() { ::LeaveCriticalSection(&critSection); }
private:
	bool paused;
	inline void EnterCriticalSection() { ::EnterCriticalSection(&critSection); }
	inline void LeaveCriticalSection() { ::LeaveCriticalSection(&critSection); }

	static CRITICAL_SECTION critSection;
	static bool critSectionInited;
};


class TestRP1210
{
public:
	TestRP1210(void);
	~TestRP1210(void);
	enum MIDCodes { SecondaryPeriodicTX = 112 };

	void Test (vector<INIMgr::Devices> &devs, int idx1, int idx2);
	void LogError (RP1210API &api, int code, COLORREF clr = 0x000090);

private:
	static UINT __cdecl SecondaryDeviceTXThread( LPVOID pParam );
	static UINT __cdecl SecondaryDeviceRXThread( LPVOID pParam );
	void KillThreads();
	static void DestroyThread(CWinThread *&thread);
	void SetupWorkerThread(CWinThread *&thread, AFX_THREADPROC threadProc);

	RP1210API *api1, *api2;
	CWinThread *helperTxThread;
	CWinThread *helperRxThread;
	int secondaryClient;

	bool threadsMustDie;
	static bool firstAlreadyCreated;
};
