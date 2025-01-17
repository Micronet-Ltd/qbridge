#pragma once
#pragma warning(disable:4996)

#include "INIMgr.h"
#include <algorithm>
#pragma warning(disable:4996)

class RP1210API;

#define ERR_ADDRESS_CLAIM_FAILED				146
#define ERR_ADDRESS_LOST						153
#define ERR_ADDRESS_NEVER_CLAIMED			157
#define ERR_BLOCK_NOT_ALLOWED					155
#define ERR_BUS_OFF								151
#define ERR_CANNOT_SET_PRIORITY				147
#define ERR_CHANGE_MODE_FAILED				150
#define ERR_CLIENT_ALREADY_CONNECTED		130
#define ERR_CLIENT_AREA_FULL					131
#define ERR_CLIENT_DISCONNECTED				148
#define ERR_CODE_NOT_FOUND						154
#define ERR_COMMAND_NOT_SUPPORTED			143
#define ERR_CONNECT_NOT_ALLOWED				149
#define ERR_COULD_NOT_TX_ADDRESS_CLAIMED	152
#define ERR_DEVICE_IN_USE						135
#define ERR_DLL_NOT_INITIALIZED				128
#define ERR_FREE_MEMORY							132
#define ERR_HARDWARE_NOT_RESPONDING			142
#define ERR_HARDWARE_STATUS_CHANGE			162
#define ERR_INVALID_CLIENT_ID					129
#define ERR_INVALID_COMMAND					144
#define ERR_INVALID_DEVICE						134
#define ERR_INVALID_PROTOCOL					136
#define ERR_MAX_FILTERS_EXCEEDED				161
#define ERR_MAX_NOTIFY_EXCEEDED				160
#define ERR_MESSAGE_NOT_SENT					159
#define ERR_MESSAGE_TOO_LONG					141
#define ERR_MULTIPLE_CLIENTS_CONNECTED		156
#define ERR_NOT_ENOUGH_MEMORY					133
#define ERR_RX_QUEUE_CORRUPT					140
#define ERR_RX_QUEUE_FULL						139
#define ERR_TXMESSAGE_STATUS					145
#define ERR_TX_QUEUE_CORRUPT					138
#define ERR_TX_QUEUE_FULL						137
#define ERR_WINDOW_HANDLE_REQUIRED			158

#define ERR_FW_FILE_READ						192
#define ERR_FW_UPGRADE							193
#define ERR_BLOCKED_NOTIFY						194
#define ERR_NOT_ADDED_TO_BUS					195
#define ERR_MISC_COMMUNICATION					196
#define ERR_RECV_OPERATION_TIMEOUT				197
#define ERR_J1939_SEND_RTS_CTS_TIMEOUT			198
#define ERR_INVALID_MSG_PACKET					199	


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

struct RecvMsgPacket {
	inline RecvMsgPacket (const char* inData, int inLen) { Set ((const unsigned char *)inData, inLen); }
	inline RecvMsgPacket (const unsigned char* inData, int inLen) { Set (inData, inLen); }
	inline void Set(const unsigned char * inData, int inLen) {
		if (inLen < 4) { 
			timeStamp = 0;
		} else { 
			unsigned char buf[4];
			memcpy (buf, inData, 4);
			reverse(buf, buf+4);
			memcpy(&timeStamp, buf, 4);
			data.insert(data.begin(), inData+4, inData+inLen);
		}
	}
	UINT32 timeStamp;
	vector<unsigned char> data;
	int GetPGN() { if (data.size() < 3) { return -1; } int retVal=0; copy(data.begin(), data.begin()+3, (unsigned char *)(&retVal)); return retVal; }
	int GetSource() { if (data.size() < 5) { return 0; } return data[4]; }
	int GetDest() { if (data.size() < 6) { return 0; } return data[5]; }
	int Get1939Data(char *buf, int &leng) { 
		if (data.size() < 6) {
			leng = 0;
			return 0;
		}
		if (leng > int(data.size()-6)) { 
			leng = int(data.size()-6);
		}
		copy (data.begin()+6, data.begin()+leng+6, buf);
		return int(data.size()) - 6;
	}
	CString Dissect1939();
};


class TxBuffer {
public:
	TxBuffer (int pgn, bool how, int priority, int srcAddr, int dstAddr, const char *inData, int dataLen);
	TxBuffer (const TxBuffer & txb) : len(txb.len) { Copy(txb); }
	~TxBuffer () { Free(); }
	TxBuffer & operator = (const TxBuffer &txb) { if (&txb != this) { Free(); Copy(txb); }  return *this; }

	operator char *() { return data; }
	operator int () { return len; }
	operator short() { return len; }

private:
	int len;
	char *data;
	void Free() { delete[] data; data = NULL; len = 0; }
	void Copy (const TxBuffer &txb) { data = new char[txb.len]; memcpy(data, txb.data, len); }
};

class TestRP1210
{
public:
	TestRP1210(void);
	~TestRP1210(void);
	enum MIDCodes { SecondaryPeriodicTX = 112, PrimarySend = 113, TestMultisend = 114, NeverSent = 126 };
	enum Commands { ResetDevice = 0, SetAllFiltersToPass = 3, SetMessageFilteringForJ1939 = 4, SetMessageFilteringForCAN = 5, 
					SetMessageFilteringForJ1708 = 7, GenericDriverCmd = 14, SetJ1708Mode = 15, SetEchoTxMsgs = 16, 
					SetAllFilterStatesToDiscard = 17, SetMessageReceive = 18, ProtectJ1939Address = 19 };
	enum FilterFlag { FILTER_PGN = 0x01, FILTER_SOURCE = 0x04, FILTER_DESTINATION = 0x08, FILTER_PRIORITY = 0x02 };

	void Test (const set<CString> &testList, vector<INIMgr::Devices> &devs, int idx1, int idx2);
	static void LogError (RP1210API &api, int code, COLORREF clr = 0x000090);
	static void KillOrphans(vector<INIMgr::Devices> &devs, int idx1, int idx2);
	static bool DoEvents();

private:
	static UINT __cdecl SecondaryDeviceTXThread( LPVOID pParam );
	static UINT __cdecl SecondaryDeviceRXThread( LPVOID pParam );
	static UINT __cdecl KillSpecifiedConnection( LPVOID pParam );
	void KillThreads();
	static void DestroyThread(CWinThread *&thread);
	void SetupWorkerThread(CWinThread *&thread, AFX_THREADPROC threadProc, bool autoDelete, CString threadName);

	RP1210API *api1, *api2;
	CWinThread *helperTxThread;
	CWinThread *helperRxThread;
	int secondaryClient;
	int secondary1939Client;
	int toKillClient;
	DWORD killTime;
	static DWORD baseThreadID;

	bool threadsMustDie;
	static bool firstAlreadyCreated;
	bool sendSpectrum;
	bool sendJ1708;
	bool sendJ1939;

	void TestReadVersion();
	void TestConnect(vector<INIMgr::Devices> &devs, int idx1);
	void TestMulticonnect(vector<INIMgr::Devices> &devs, int idx1);
	void TestBasicRead(INIMgr::Devices &dev, int primaryClient);
	void TestAdvancedRead(INIMgr::Devices &dev, int primaryClient);
	void TestMultiRead(INIMgr::Devices &dev, int primaryClient);
	void TestBasicSend (int primaryClient);
	void TestAdvancedSend(INIMgr::Devices &dev, int primaryClient);
	void TestWinNotify(INIMgr::Devices &dev);

	void Test1939AddressClaim (INIMgr::Devices &dev1, INIMgr::Devices &dev2);
	void Test1939BasicRead (INIMgr::Devices &dev);
	void Test1939AdvancedRead(INIMgr::Devices &dev1, INIMgr::Devices &dev2);
	void Test1939BasicSend(INIMgr::Devices &dev1, INIMgr::Devices &dev2);
	void Test1939WinNotify(INIMgr::Devices &dev);
	void Test1939Filters(INIMgr::Devices dev1, INIMgr::Devices dev2);

	enum BlockType { BLOCK_UNTIL_DONE = 0, POST_MESSAGE = 1, RETURN_BEFORE_COMPLETION = 2 };
	static bool VerifyProtectAddress (RP1210API *api, int clientID, int address, int vehicalSystem, int identityNum, BlockType block = BLOCK_UNTIL_DONE, int expectedError = 0); 
	static bool VerifyProtectAddress (RP1210API *api, int clientID, int address, bool arbitraryAddress, int industryGroup, int vehSysInst, int vehSys, int function, int funcInst, int ecuInst, int mfgCode, int identityNum, BlockType block, int expectedError); 
	static int VerifyConnectAndPassFilters(RP1210API *api, INIMgr::Devices &dev, char *protocol);
	static int VerifyConnect(RP1210API *api, INIMgr::Devices &dev, char *protocol);
	static int VerifyDisconnect(RP1210API *api, int clientID);
	static int VerifiedRead (RP1210API *api, int clientID, char *rxBuf, int rxLen, bool block);
	static bool VerifiedSend (RP1210API *api, int clientID, char *txBuf, int txLen, bool block, int expectedResult = 0, TCHAR * desc = _T(""));
	static void VerifyValidSendCommand (RP1210API *api, int cmd, CString text, int clientID, char *cmdData, int len, bool logSuccess);
	static void VerifyInvalidSendCommand(RP1210API *api, int cmd, CString text, int clientID, char *cmdData, int len, int expectedResult);
	static void VerifyInvalidClientIDSendCommand(RP1210API *api, int cmd, CString text, char *cmdData, int len) { VerifyInvalidSendCommand (api, cmd, text + _T("<invalid client ID>"), 127, cmdData, len, ERR_INVALID_CLIENT_ID); }
	static void FlushReads(RP1210API *api, int clientID);

	
	void TestSendCommandReset(INIMgr::Devices &dev);
	void TestFilterStatesOnOffMessagePassOnOff(INIMgr::Devices &dev, int primaryClient);
	void TestFilters(INIMgr::Devices &dev, int primaryClient);

	void TestGenericMultiread(INIMgr::Devices &dev, bool isJ1708);
	void TestGenericSend (bool isJ1708, int primaryClient);
	void TestCustom (INIMgr::Devices &dev1, INIMgr::Devices &dev2);


	list <RecvMsgPacket> secondaryRxMsgs;
	friend struct ErrorSend;
	friend class DlgTestWinNotify;
	friend struct CollectMessageArray;
};
