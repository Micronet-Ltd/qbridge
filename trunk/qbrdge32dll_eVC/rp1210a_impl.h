#pragma once

#ifndef RP1210A_IMPL_H
#define RP1210A_IMPL_H

#include "Log.h"

static UINT WM_RP1210_ERROR_MESSAGE = 0;
static UINT WM_RP1210_MESSAGE_MESSAGE = 0;
static int BLOCK_TIMEOUT = 60000;

class CritSection {
public:
	CritSection() : paused(false) { ASSERT (critSectionInited); EnterCriticalSection(); }
	virtual ~CritSection() { if (!paused) {LeaveCriticalSection(); } ASSERT (!paused); }
	void Pause() { if (!paused) { paused = true; LeaveCriticalSection(); } }
	void Unpause() { ASSERT (paused); if (paused) { paused = false; EnterCriticalSection(); } }

	inline static void Init() { ::InitializeCriticalSection(&critSection); critSectionInited = true; }

	static inline BOOL TryEnterCriticalSection() { return ::TryEnterCriticalSection (&critSection); }
	static inline void TryLeaveCriticalSection() { ::LeaveCriticalSection(&critSection); }
private:
	bool paused;
	inline void EnterCriticalSection() { ::EnterCriticalSection(&critSection); }
	inline void LeaveCriticalSection() { ::LeaveCriticalSection(&critSection); }

	static CRITICAL_SECTION critSection;
	static bool critSectionInited;
};

void InitializeDLL();
void GetStatusInfo(TCHAR *buf, int bufLen);
RP1210AReturnType CreateConnection(int connType, int comPort, HWND hwndClient, long lTxBufferSize, long lRcvBufferSize, short nIsAppPacketizingIncomingMsgs);
RP1210AReturnType Disconnect(short clientID);
RP1210AReturnType SendRP1210Message (short nClientID, char far* fpchClientMessage, short nMessageSize, short nNotifyStatusOnTx, short nBlockOnSend, CritSection &cs);
RP1210AReturnType ReadRP1210Message (short nClientID, char far* fpchAPIMessage, short nBufferSize, short nBlockOnRead, CritSection &cs);
RP1210AReturnType SendCommand (short nCommandNumber, short nClientID, char far* fpchClientCommand, short nMessageSize, CritSection &cs);
RP1210AReturnType GetHardwareStatus (short nClientID, char far* fpchClientInfo, short nInfoSize, short nBlockOnRequest, CritSection &cs);

void DLLCleanUp();
int GetAssignPort();
bool OpenDriverApp();
void SendUDPClosePacket();
bool ConnectToDriverApp();
static DWORD __stdcall UDPListenFunc(void* args);
void ProcessDataPacket(char* data, SOCKET RecvSocket, sockaddr_in RecvAddr);
void SendUDPPacket(unsigned long toIPAddr, int toPort, int fromPort, const char* buf, int bufLen);
bool QueryDriverApp(PACKET_TYPE queryId, int localPort, int& intRetVal, char* outData, int outDataLen, int idNum);

extern int readEventId;
static int readEventId;
HANDLE GetReadEvent();

enum ConnectionType {Conn_Invalid = 0, Conn_J1708, Conn_J1939};

class Connection {
public:
	Connection() : 
		txQueueMax(8000), rxQueueMax(8000), connType(Conn_Invalid), hwnd(0) { 
        }
	~Connection() {  
    }

	short nIsAppPacketizingIncomingMsgs;
	int comPort;

	inline ConnectionType GetConnectionType() { return connType; }
	inline HWND GetHwnd() { return hwnd; }

	void SetupConnection(int inTxQueueMax, int inRxQueueMax, ConnectionType connectionType, HWND inHwnd)
	{
		Clear();
		txQueueMax = inTxQueueMax;
		rxQueueMax = inRxQueueMax;
		hwnd = inHwnd;	
		getHwStatusReturnCode = 0;
		if (connType == Conn_Invalid && connectionType != Conn_Invalid) {
			connType = connectionType;	
			ConnectionStatusChanged();	
		}
		connType = connectionType;
		transactions.clear();
	}

    void PopSetReadEvent()
    {
        if (recvMsgEvents.size() != 0) {
            HANDLE &blocked = recvMsgEvents.front();
            ::SetEvent(blocked);
            recvMsgEvents.pop_front();
        }
    }

	void Clear() {
        // send reply to pending transactions
		typedef list<Transaction>::iterator transIter;
		list <transIter> toErase;
		for (transIter it = transactions.begin(); it != transactions.end(); it++) {
			Transaction &t = *it;			
			if (t.isNotify) {				
				//	wchar_t buff[100];
//					swprintf(buff, 100, L"Send FreeMsgId2: %d\n", t.transId);
//					_DbgTrace(buff);
				// send freeMsgId	
				char sendBuf[40];
				int len = _snprintf(sendBuf, 40, "%d,freeMsgId;", t.transId);				
				SendUDPPacket(inet_addr("127.0.0.1"), DRIVER_LISTEN_PORT, GetAssignPort(), sendBuf, len);
			}
			if (t.isNotify && hwnd != 0) {
				//_DbgTrace(_T("POST error client disconnect\n"));

				::PostMessage(GetHwnd(), WM_RP1210_ERROR_MESSAGE, ERR_CLIENT_DISCONNECTED, t.transId+128);				
				toErase.push_back(it);
			}
			else {
				//notify blocking send msg.
				t.returnCode = ERR_CLIENT_DISCONNECTED;
				::SetEvent(t.transEvent);
			}
		}
		for (list<transIter>::iterator jt = toErase.begin(); jt != toErase.end(); jt++) {
			transactions.erase(*jt);
		}

		txQueueMax = 8000;
		rxQueueMax = 8000;
		if (connType != Conn_Invalid) {
            Log::WriteRaw(LogLev::ErrClientDisconnected, L"Setting connType to Invalid (Connection::Clear)");
			connType = Conn_Invalid;			
			ConnectionStatusChanged();
		}	
		hwnd = 0;
		nIsAppPacketizingIncomingMsgs = 0;
		comPort = 0;
		//loop and unblock all read functions
		while (recvMsgEvents.size() != 0) {
            PopSetReadEvent();
		}
		recvMsgQueue.clear();
	}

	void AddTransaction(short isNotify, int transId) {
		Transaction t;
		t.transId = transId;
		t.isNotify = isNotify;
		t.isJ1939AddrClaim = false;
		if (!isNotify) {			
			wchar_t lpName[30];
			swprintf(lpName, L"blockOnSend_Event_%d", transId);
			t.transEvent = ::CreateEvent(NULL, FALSE, FALSE, lpName);
		}
		else {
			//wchar_t buff[100];
			//swprintf(buff, 100, L"Notify ID1: %d\n", transId);
			//_DbgTrace(buff);
		}
		transactions.push_back(t);
	}
	void AddTransaction(short isNotify, int transId, bool isJ1939AddrClaim, int claimAddr) {
		Transaction t;
		t.transId = transId;
		t.isNotify = isNotify;
		t.isJ1939AddrClaim = isJ1939AddrClaim;
		t.claimAddr = claimAddr;
		if (!isNotify) {			
			wchar_t lpName[30];
			swprintf(lpName, L"blockOnSend_Event_%d", transId);
			t.transEvent = ::CreateEvent(NULL, FALSE, FALSE, lpName);
			t.returnCode = ERR_NOT_ADDED_TO_BUS;
		}
		else {
			//wchar_t buff[100];
//			swprintf(buff, 100, L"Notify ID2: %d\n", transId);
//			_DbgTrace(buff);
		}
		transactions.push_back(t);
	}
	HANDLE GetTransEvent(short isNotify, int transId) {	
		for (list <Transaction>::iterator it = transactions.begin(); it != transactions.end(); it++) {
			Transaction &t = *it;
			if (t.isNotify == isNotify && t.transId == transId) {
				return t.transEvent;
			}
		}
		return 0;
	}
	int GetReturnCode(short isNotify, int transId) {
		if (connType == Conn_Invalid) {
			return ERR_INVALID_CLIENT_ID;
		}
		for (list<Transaction>::iterator it = transactions.begin(); 
			it != transactions.end(); it++) 
		{
			Transaction &t = *it;
			if (t.isNotify == isNotify && t.transId == transId) {
				return t.returnCode;
			}
		}
		return 0;
	}
	bool UpdateTransaction(short isNotify, int transId, int returnCode) {
		bool result = false;
		for (list<Transaction>::iterator it = transactions.begin(); it != transactions.end(); it++) {
			Transaction &t = *it;
			if (t.isNotify == isNotify && t.transId == transId) {
				if (isNotify && t.isJ1939AddrClaim == false) {
					//send msg to hwnd
					if (returnCode == 0) {
						//Success in SendMessage		
						//_DbgTrace(_T("POST error client success tx\n"));				
						::PostMessage(GetHwnd(), WM_RP1210_ERROR_MESSAGE, ERR_TXMESSAGE_STATUS, transId);						
					}
					else {
						//Error in SendMessage
						//_DbgTrace(_T("POST error client txt\n"));
						::PostMessage(GetHwnd(), WM_RP1210_ERROR_MESSAGE, ERR_TXMESSAGE_STATUS, transId+128);						
					}
					RemoveTransaction(isNotify, transId);
					result = true;
				}
				else if (isNotify && t.isJ1939AddrClaim == true) {
					//send msg to hwnd
					if (returnCode != 0) {
						//Error in addr claim
						//_DbgTrace(_T("POST error client txt\n"));
						::PostMessage(GetHwnd(), WM_RP1210_ERROR_MESSAGE, ERR_ADDRESS_LOST, t.claimAddr);						
					}
					RemoveTransaction(isNotify, transId);
					result = true;
				}
				else {
					//notify blocking send msg.
					int BufLen = 80;
					char buff[80];				
					BufLen = _snprintf(buff, 80, 
						"Set trans event %p \n", t.transEvent);				
					TCHAR tbuf[80];
					for (int i = 0; i < 80; i++) {
						tbuf[i] = buff[i];
					}
					tbuf[BufLen] = 0;
					//_DbgTrace(tbuf);

					t.returnCode = returnCode;	
					for (int j = 0; j < 40; j++) {
						result = (::SetEvent(t.transEvent) != 0);
						if (result) {
							break;
						}
						::Sleep(10);
					}
				}
				return result;
			}
		}
		return result;
	}
	void RemoveTransaction(short isNotify, int transId) {	
		typedef list<Transaction>::iterator transIter;
		for (transIter it = transactions.begin(); it != transactions.end(); it++) {
			Transaction &t = *it;
			if (t.transId == transId && t.isNotify == isNotify) {
				if (t.isNotify) {					
					//wchar_t buff[100];
//					swprintf(buff, 100, L"Send FreeMsgId1: %d\n", transId);
//					_DbgTrace(buff);
					// send freeMsgId	
					char sendBuf[40];
					int len = _snprintf(sendBuf, 40, "%d,freeMsgId;", t.transId);				
					SendUDPPacket(inet_addr("127.0.0.1"), DRIVER_LISTEN_PORT, GetAssignPort(), sendBuf, len);
				}
				transactions.erase(it);
				return;
			}
		}
	}

	int GetReadMsg(char *buff, int buffLen, int &msgLen) {
		if (GetConnectionType() == Conn_Invalid) {
			return ERR_INVALID_CLIENT_ID;
		}
		if (recvMsgQueue.size() == 0) {
            PopSetReadEvent();
			return ERR_RECV_OPERATION_TIMEOUT;
		}
		RecvMsg &rm = recvMsgQueue.front();
		if (buffLen < rm.getLen()) {
			return ERR_MESSAGE_TOO_LONG;
		}
		memcpy (buff, rm.getBuf(), rm.getLen());
		msgLen = rm.getLen();

		recvMsgQueue.pop_front();
		return 0;
	}

	void AddReadMsg(char *msg, int msgLen) {
		TRACE(_T("ADDREADMSG to recvQ\r\n"));
		recvMsgQueue.push_back(RecvMsg(msg, msgLen));
		if (recvMsgEvents.size() != 0) {
			TRACE(_T("SETREADEVENT\r\n"));
            PopSetReadEvent();
		}
		else if (hwnd != 0) {
			// send notification
			//_DbgTrace(_T("POST notify\n"));	
			TRACE(_T("POSTREAD Notify\r\n"));
			::PostMessage(GetHwnd(), WM_RP1210_MESSAGE_MESSAGE, 0, 0);
		}
	}

	HANDLE AddGetHWStatusNotify(int clientId) {	
		wchar_t lpName[30];
		int idx = (int) hwStatusEvents.size();
		swprintf(lpName, L"blockOnHWStatus_Event_%d_%d", clientId, idx);
		HANDLE hw_event = ::CreateEvent(NULL, FALSE, FALSE, lpName);
		hwStatusEvents.push_back(hw_event);
		return hw_event;
	}

	// notifiy HWStatus Events of connection status changed
	void ConnectionStatusChanged() {
		for (list<HANDLE>::iterator it = hwStatusEvents.begin(); it != hwStatusEvents.end(); it++) {
			HANDLE &t = *it;
			::SetEvent(t);
		}
		hwStatusEvents.clear();		
	}

	int getHwStatusReturnCode;

	struct RecvMsg {
		RecvMsg (char *msg, int msgLen) : data(NULL), dataLen(0) { Set (msg, msgLen); }
		~RecvMsg () { delete[] data; }
		RecvMsg (const RecvMsg &inMsg) : data(NULL), dataLen(0) { Set (inMsg.data, inMsg.dataLen); }
		RecvMsg & operator = (const RecvMsg &inMsg) { delete[] data; data = NULL; dataLen = 0; Set (inMsg.data, inMsg.dataLen); return *this;}

		inline const char * getBuf() { return data; }
		inline int getLen() { return dataLen; }
	private:
		void Set(char *msg, int msgLen) { 
			ASSERT (data == NULL); 
			data = new char[msgLen]; 
			dataLen = msgLen; 
			memcpy(data, msg, msgLen);
		}
		char *data;
		int dataLen;
	};
	list<RecvMsg> recvMsgQueue;
	list<HANDLE> recvMsgEvents; //notify blocking functions when msg recieved

private:
	list<HANDLE> hwStatusEvents; //notify blocking HardwareStatus functions when connection changes
	HWND hwnd;
	int txQueueMax;
	int rxQueueMax;
	ConnectionType connType;
	struct Transaction {
		//inital info
		short isNotify;
		int transId;
		HANDLE transEvent;
		int returnCode;
		bool isJ1939AddrClaim;
		int claimAddr;
	};
	list<Transaction> transactions;
};

class Thread
{
public:
	Thread() : valid(false) {}
	bool SetupThread ( DWORD (WINAPI * pFun) (void* arg), void* pArg)
    {
        Log::WriteRaw(LogLev::Debug, L"Thread::SetupThread");       
		if (valid) {
			return false;
		}
		valid = true;
        _handle = CreateThread (
            NULL, // Security attributes
            0, // Stack size
            pFun,
            pArg,
            CREATE_SUSPENDED,
            &_tid);
		return true;
    }
	~Thread () { } //CloseHandle (_handle);
    void Resume () { ResumeThread (_handle); }
    void WaitForDeath ()
    {
        WaitForSingleObject (_handle, 2000);
    }

	bool valid;
private:
    HANDLE _handle;
    DWORD  _tid;     // thread id
};

const static int maxClientID = 127;

extern Connection connections[maxClientID+1];
extern Thread udpThread;
extern int assignPort;

#endif // RP1210A_IMPL_H
