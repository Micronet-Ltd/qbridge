#include "StdAfx.h"

#include "QBRDGE32.h"
#include "rp1210a_impl.h"
#include <stdio.h>

Connection connections[maxClientID+1];
Thread udpThread;
int assignPort;

/******************/
/* InitializeDLL */
/****************/
void InitializeDLL() {
	WM_RP1210_MESSAGE_MESSAGE = RegisterWindowMessage(QBRIDGE_RP1210_MESSAGE_STRING);
	WM_RP1210_ERROR_MESSAGE = RegisterWindowMessage(QBRIDGE_RP1210_ERROR_STRING);
	//hEvent = CreateEvent(NULL, true, false, NULL);

	_DbgTrace(_T("Initialize DLL Start\r\n"));
	assignPort = -1;
}

/******************/
/* GetStatusInfo */
/****************/
void GetStatusInfo(TCHAR *buf, int bufLen) {
	//_sntprintf(buf, bufLen, _T("Mapped Handles %d, freeHandles %d, transmitsPending %d, rxQueueSize %d"), transmitHandles.GetMappedHandlesCount(),
	//	transmitHandles.GetFreeHandlesCount(), commCoordinator.GetTransmitConfirmActionsCount(), (connection != NULL ? connection->rxQueue.size() : -1) );
	_tcsncpy(buf, _T("Blablabla"), bufLen);
	_DbgTrace(_T("GetStatusInfo\n"));
}

/*********************/
/* CreateConnection */
/*******************/
//connType 1 = J1708, connType 2 = 1939
RP1210AReturnType CreateConnection(int connType, int comPort, HWND hwndClient, long lTxBufferSize, long lRcvBufferSize, short nIsAppPacketizingIncomingMsgs) { 
if (lTxBufferSize <= 0) { lTxBufferSize = 8192; }
	if (lRcvBufferSize <= 0) { lRcvBufferSize = 8192; }

	//get new clientid from wince driver, if fail then return err_hardware...
	int cid = comPort; // send comport value, & get back client id value
	if (QueryDriverApp(QUERY_NEW_CLIENTID_PKT, GetAssignPort(), cid, NULL, 0, 0) == false) {
		return ERR_HARDWARE_NOT_RESPONDING;
	}
	else {
		// add connection to array list
		if (cid == -1) {
			return ERR_CLIENT_AREA_FULL;
		}
		else if (cid == -2) {
			return ERR_INVALID_DEVICE;
		}
		else {
			if (connType == 1) {
				_DbgTrace(_T("created j1708 conn\n"));
				connections[cid].SetupConnection(lTxBufferSize, lRcvBufferSize, Conn_J1708, hwndClient);
				_DbgTrace(_T("created j1708 done\n"));
				return cid;
			}
			else if (connType == 2) {
				_DbgTrace(_T("created j1939 conn\n"));
				for (int i = 0; i <= maxClientID; i++) {
					if (connections[cid].GetConnectionType() == Conn_J1939 && 
						connections[cid].nIsAppPacketizingIncomingMsgs != nIsAppPacketizingIncomingMsgs &&
						connections[cid].comPort == comPort)
					{
						_DbgTrace(_T("conn 1939 err conn not allowed\n"));
						return ERR_CONNECT_NOT_ALLOWED;
					}
				}
				connections[cid].SetupConnection(lTxBufferSize, lRcvBufferSize, Conn_J1939, hwndClient);
				connections[cid].comPort = comPort;
				connections[cid].nIsAppPacketizingIncomingMsgs = nIsAppPacketizingIncomingMsgs;
				_DbgTrace(_T("created j1939 done\n"));
				return cid;
			}
			return ERR_INVALID_PROTOCOL;
		}
	}	
}

/***************/
/* Disconnect */
/*************/
RP1210AReturnType Disconnect(short nClientID) {
	if (nClientID < 0 || nClientID > maxClientID) {
		return ERR_INVALID_CLIENT_ID;
	}
	if (connections[nClientID].GetConnectionType() == Conn_Invalid) {
		return ERR_DLL_NOT_INITIALIZED;
	}
	int cid = (int) nClientID;
	if (QueryDriverApp(QUERY_DISCONNECT_CLIENTID_PKT, GetAssignPort(), cid, NULL, 0, 0) == false) {
		return ERR_MISC_COMMUNICATION;
	}
	connections[nClientID].getHwStatusReturnCode = ERR_CLIENT_DISCONNECTED;
	connections[nClientID].Clear();

	//send message to blocking reads for clientID here

	return 0;
}

/**********************/
/* SendRP1210Message */
/********************/
RP1210AReturnType SendRP1210Message (short nClientID, char far* fpchClientMessage, short nMessageSize, short nNotifyStatusOnTx, short nBlockOnSend, CritSection &cs) {
	if (nClientID < 0 || nClientID > maxClientID) {
		return ERR_INVALID_CLIENT_ID;
	}
	if (connections[nClientID].GetConnectionType() == Conn_Invalid) {
		return ERR_CLIENT_DISCONNECTED;
	}

	if (nNotifyStatusOnTx && (connections[nClientID].GetHwnd() == 0)) {
		return ERR_WINDOW_HANDLE_REQUIRED;
	}

	if (nBlockOnSend && nNotifyStatusOnTx) {
		return ERR_BLOCKED_NOTIFY;
	}

	if (connections[nClientID].GetConnectionType() == Conn_J1708) {
		//J1708
		if (nMessageSize > 21) {
			return ERR_MESSAGE_TOO_LONG;
		}

		PACKET_TYPE queryType;
		if (nBlockOnSend) {
			queryType = QUERY_J1708MSG_BLOCK_PKT;
		}
		else {
			queryType = QUERY_J1708MSG_PKT;
		}
		//send j1708 message to driver app.
		int cid = (int) nClientID;
		_DbgTrace(_T("before query driver app send\n"));
		if (QueryDriverApp(queryType, GetAssignPort(), cid, fpchClientMessage, nMessageSize, 0) == true) {
			int msgId = cid;
			if (nBlockOnSend || nNotifyStatusOnTx) {
				connections[nClientID].AddTransaction(nNotifyStatusOnTx, msgId);
			}
			if (nBlockOnSend) {
				//wait for signal here
				HANDLE hEvent = connections[nClientID].GetTransEvent(nNotifyStatusOnTx, msgId);

				int BufLen = 80;
				char buff[80];				
				BufLen = _snprintf(buff, 80, 
					"Wait event %d, client %d, msgid %d\n", hEvent, nClientID, msgId);				
				TCHAR tbuf[80];
				for (int i = 0; i < 80; i++) {
					tbuf[i] = buff[i];
				}
				tbuf[BufLen] = 0;
				_DbgTrace(tbuf);
				
				cs.Pause();
				::WaitForSingleObject(hEvent, 70000); //SetEvent for release?
				cs.Unpause();
				::CloseHandle(hEvent);

				int returnCode = connections[nClientID].GetReturnCode(nNotifyStatusOnTx, msgId);
				connections[nClientID].RemoveTransaction(nNotifyStatusOnTx, msgId);

				//_DbgTrace(_T("after query driver app send 1\n"));
				return returnCode;
			} 
			else if (nNotifyStatusOnTx) {
				if (cid == 0) {
					//message queue 1-127 full
					//_DbgTrace(_T("after query driver app send 2\n"));
					return ERR_MAX_NOTIFY_EXCEEDED;
				}
				else {
					TRACE (_T("Sent packet to qbridge, notify mode.  PacketID=%d\n"), cid);
					return cid;
				}
			} else {
				//_DbgTrace(_T("after query driver app send 3\n"));
				// message sent to QBridge -- no notify or blockign requested
				return 0;
			}
		} else {
			_DbgTrace(_T("after query driver app send 4\n"));
			return ERR_HARDWARE_NOT_RESPONDING;
		}
	}
				_DbgTrace(_T("after query driver app send 5\n"));
	return ERR_MESSAGE_NOT_SENT;
}

/**********************/
/* ReadRP1210Message */
/********************/
RP1210AReturnType ReadRP1210Message (short nClientID, char far* fpchAPIMessage, short nBufferSize, short nBlockOnRead, CritSection &cs) {
	if (nClientID < 0 || nClientID > maxClientID) {
		return -ERR_INVALID_CLIENT_ID;
	}
	if (connections[nClientID].GetConnectionType() == Conn_Invalid) {
		return -ERR_CLIENT_DISCONNECTED;
	}

	if (connections[nClientID].recvMsgQueue.size() == 0) {
		if (nBlockOnRead) {
			cs.Pause();
			//listen for event
			HANDLE hEvent = GetReadEvent();
			connections[nClientID].recvMsgEvents.push_back(hEvent);
			::WaitForSingleObject(hEvent, 10000); //SetEvent for release?
			cs.Unpause();
			::CloseHandle(hEvent);
		}
		else {
			return 0;
		}
	}

	int msgLen = 0;
	int errId = connections[nClientID].GetReadMsg(fpchAPIMessage, nBufferSize, msgLen);
	if (errId != 0) {
		return -errId;
	}
	return msgLen;
}

/****************/
/* SendCommand */
/**************/
RP1210AReturnType SendCommand (short nCommandNumber, short nClientID, char far* fpchClientCommand, short nMessageSize) {
	if (nClientID < 0 || nClientID > maxClientID) {
		return ERR_INVALID_CLIENT_ID;
	}
	if (connections[nClientID].GetConnectionType() == Conn_Invalid) {
		return ERR_DLL_NOT_INITIALIZED;
	}

	int cid = (int)nClientID;
	int idNum = (int)nCommandNumber;
	if (QueryDriverApp(QUERY_SEND_COMMAND, GetAssignPort(), cid, fpchClientCommand, nMessageSize, idNum)) {
		short returnCode = (short)cid;	
		if ( (nCommandNumber == CMD_RESET_DEVICE || 
			nCommandNumber == CMD_UPGRADE_FIRMWARE) && returnCode == 0) {
			//success, device reset, client not connected
			Disconnect(nClientID);
		}
		return returnCode;
	}
	else
	{
		return ERR_HARDWARE_NOT_RESPONDING;
	}
	return 0;
}

/**********************/
/* GetHardwareStatus */
/********************/
RP1210AReturnType GetHardwareStatus (short nClientID, char far* fpchClientInfo, short nInfoSize, short nBlockOnRequest, CritSection &cs) {
	if (nClientID < 0 || nClientID > maxClientID) {
		return ERR_INVALID_CLIENT_ID;
	}
	if (nInfoSize < 16) {
		return ERR_MESSAGE_TOO_LONG;
	}

	if (nBlockOnRequest) {
		cs.Pause();
		//wait for signal here
		HANDLE hEvent = connections[nClientID].AddGetHWStatusNotify(nClientID);
		::WaitForSingleObject(hEvent, 10000); //SetEvent for release?
		cs.Unpause();
		::CloseHandle(hEvent);
	}

	int cid = nClientID;
	int numbJ1708clients = 0;
	if (QueryDriverApp(QUERY_NUMBER_J1708_CONN, GetAssignPort(), cid, NULL, 0, 0)) {
		numbJ1708clients = cid;
	}

	memset (fpchClientInfo, 0, nInfoSize);

	fpchClientInfo[0] = 4 | (connections[nClientID].GetConnectionType() != Conn_Invalid ? 1 : 0);
	fpchClientInfo[1] = numbJ1708clients;
	fpchClientInfo[4] = 1 | (connections[nClientID].GetConnectionType() == Conn_J1708 ? 2 : 0);
	fpchClientInfo[5] = numbJ1708clients;

	if (nBlockOnRequest) {
		return connections[nClientID].getHwStatusReturnCode;
	}
	else {
		return 0;
	}
}

/***********************/
/* ConnectToDriverApp */
/*********************/
bool ConnectToDriverApp(){
	if (GetAssignPort() > 0) {
		return true;
	}
	// start mutex
	HANDLE hMutex = ::CreateMutex(NULL, FALSE, _T("qbridgeDLLMutex"));
	if (hMutex == NULL)
		return false;
	if(WAIT_OBJECT_0 == ::WaitForSingleObject(hMutex, 10000))
	{
		int dmy;
		if (QueryDriverApp(QUERY_NEWPORT_PKT, DRIVER_LISTEN_PORT-1, dmy, NULL, 0, 0) == false) {
			if (OpenDriverApp() == false) {
				_DbgTrace(_T("Error opening driver app.\n"));
				::ReleaseMutex(hMutex);
				return false;
			}
			::Sleep(750);
			if (QueryDriverApp(QUERY_NEWPORT_PKT, DRIVER_LISTEN_PORT-1, dmy, NULL, 0, 0) == false) {
				_DbgTrace(_T("Error retrieving assigned port from driver application\n"));
				::ReleaseMutex(hMutex);
				return false;
			}
			if (QueryDriverApp(QUERY_PROCID_PKT, GetAssignPort(), dmy, NULL, 0, 0) == false) {
				_DbgTrace(_T("Error retrieving Process ID from driver application\n"));
				::ReleaseMutex(hMutex);
				return false;
			}
		}
		::ReleaseMutex(hMutex);
	}
	// end mutex
	if (GetAssignPort() <= 0) {
		_DbgTrace(_T("GetAssignPort return <= 0"));
		return false;
	}
	return true;
}

/***********************/
/* GetSendQueryPacket */
/*********************/
int GetSendQueryPacket(PACKET_TYPE queryId, char* sendBuf, int bufLen, 
					   int numData, char* outData, int outDataLen,
					   int idNum) 
{
	_DbgTrace(_T("getsendquerypkt\n"));
	if (queryId == QUERY_HELLO_PKT) {  
		strncpy(sendBuf, "hello", bufLen);
		return int(strlen(sendBuf));
	}
	else if (queryId == QUERY_PROCID_PKT) {
		strncpy(sendBuf, "procid", bufLen);
		return int(strlen(sendBuf));
	}
	else if (queryId == QUERY_NEW_CLIENTID_PKT) {
		return _snprintf(sendBuf, bufLen, "%d,newclientid;", numData);
	}
	else if (queryId == QUERY_NEWPORT_PKT) {
		strncpy(sendBuf, "init", bufLen);
		return int(strlen(sendBuf));
	}
	else if (queryId == QUERY_DISCONNECT_CLIENTID_PKT) {
		return _snprintf(sendBuf, bufLen, "%d,disconnect;", numData);
	}
	else if (queryId == QUERY_J1708MSG_PKT || queryId == QUERY_J1708MSG_BLOCK_PKT) {
		int len = 0;
		if (queryId == QUERY_J1708MSG_BLOCK_PKT) {
			len = _snprintf(sendBuf, bufLen, "%d,j1708blockmsg;", numData);
		}
		else {
			len = _snprintf(sendBuf, bufLen, "%d,j1708msg;", numData);
		}
		for (int i=0; i < outDataLen; i++) {
			sendBuf[i+len] = outData[i];
		}
		return (len + outDataLen);
	}
	else if (queryId == QUERY_SEND_COMMAND) {
		int len = 0;
		//<clientId>|<command#>,sendcommand;<cmd buffer>
		len = _snprintf(sendBuf, bufLen, "%d|%d,sendcommand;", numData, idNum);
		for (int i=0; i < outDataLen; i++) {
			sendBuf[i+len] = outData[i];
		}
		return (len + outDataLen);
	}
	else if (queryId == QUERY_NUMBER_J1708_CONN) {
		//<clientId>,<query>;
		int len = 0;
		len = _snprintf(sendBuf, bufLen, "%d,queryj1708clients;", numData);
		return len;
	}
	return 0;
}

/*******************/
/* QueryDriverApp */
/*****************/
bool QueryDriverApp(PACKET_TYPE queryId, int localPort, 
					int& intRetVal, char* outData, int outDataLen,
					int idNum) 
{
	// send udp packet and verify if driver application is already running		
	WSADATA wsaData;
	SOCKET RecvSocket;
	sockaddr_in RecvAddr;
	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);
	const int BufLen = 1024;
	char RecvBuf[BufLen];
	int timeoutTime = 5000; // 5 sec
	int retryLimit = 2;
	int retryNum = 0;	
	//for select func:
	fd_set fdReadSet;
	fd_set fdErrorSet;
    TIMEVAL timeout = {5, 0}; // 5 second timeout
	bool isTimeout = false;

	_DbgTrace(_T("QueryDriverApp()\n"));
		
	char sendBuf[300];
	int sendBufLen = GetSendQueryPacket(queryId, sendBuf, sizeof(sendBuf), 
		intRetVal, outData, outDataLen, idNum);

	if (queryId == QUERY_SEND_COMMAND) {
		//timeoutTime = 180000;
		timeout.tv_sec = 180;
	}
	if (queryId == QUERY_NEW_CLIENTID_PKT) {
		//timeoutTime = 60000;
		timeout.tv_sec = 60;
	}

	//-----------------------------------------------
	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		return false;
	}

	int rv;
	do {
		// Create a receiver socket to receive datagrams
		RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		//setsockopt(RecvSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeoutTime, sizeof(timeoutTime));

		// Bind the socket to any address and the specified port.
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(localPort);
		//RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		bind(RecvSocket, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));

		//Send a query packet
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(DRIVER_LISTEN_PORT);
		RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		retryNum++;
		//SendTo
		sendto(RecvSocket, sendBuf,	sendBufLen,	0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
		
		FD_ZERO(&fdReadSet);
		FD_ZERO(&fdErrorSet);
		FD_SET(RecvSocket, &fdReadSet);
		FD_SET(RecvSocket, &fdErrorSet);
		if(select(0, &fdReadSet, NULL, &fdErrorSet, &timeout) != 1)
		{
			_DbgTrace(_T("Select in query timeout\n"));
			isTimeout = true;
			closesocket(RecvSocket);
		}
		else {
			if (FD_ISSET(RecvSocket, &fdReadSet)) {
				// Call the recvfrom function to receive datagrams
				// on the bound socket.
				_DbgTrace(_T("Receiving datagrams...\n"));
				rv = recvfrom(RecvSocket, 
					RecvBuf, 
					BufLen, 
					0, 
					(SOCKADDR *)&SenderAddr, 
					&SenderAddrSize);

				if (rv != SOCKET_ERROR)
				{
					isTimeout = false;
					retryNum = retryLimit;
				}
				else
				{
					closesocket(RecvSocket);
				}
			}
			else {
				isTimeout = true;
				closesocket(RecvSocket);
				WSACleanup();
				return false;
			}
		}
	}
	while (retryNum < retryLimit);

	if ((rv == SOCKET_ERROR) | (isTimeout == true)) {
		if (WSAETIMEDOUT == WSAGetLastError() ) 
		{
			// no reply from app.
			_DbgTrace(_T("No Reply From app. timed out\n"));
		}
		else {
			_DbgTrace(_T("Communication Error with Win App."));
		}
		WSACleanup();
		return false;
	}

	TCHAR tbuf[BufLen];
	RecvBuf[rv] = 0;
	ToUnicode(RecvBuf, tbuf, BufLen);
	_DbgTrace(_T("recv: "));
	_DbgTrace(tbuf);
	_DbgTrace(_T("*\n"));

	closesocket(RecvSocket);
	WSACleanup();

	if (queryId == QUERY_NEWPORT_PKT) {
		assignPort = atoi(RecvBuf);
		void * arg = NULL;
		//start unblocking udp listening thread
		if (udpThread.valid == false) {
			udpThread.SetupThread(UDPListenFunc, arg);
			udpThread.Resume();
			::Sleep(20);
		}
	}
	else if (queryId == QUERY_PROCID_PKT) {
		int procId = atoi(RecvBuf);
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION,FALSE,procId);
		DWORD lpExitCode;
		if (!GetExitCodeProcess(hProcess, &lpExitCode)) {
			_DbgTrace(_T("error getting exit code process\n"));
			return false;
		}
		if (lpExitCode != STILL_ACTIVE) {
			_DbgTrace(_T("process is not active\n"));
			return false;
		}
		_DbgTrace(_T("Process recieved and active!\n"));
	}
	else if (queryId == QUERY_NEW_CLIENTID_PKT) {
		intRetVal = atoi(RecvBuf);
		if (intRetVal < 0 || intRetVal > 127) {
			return false;
		}
	}
	else if (queryId == QUERY_DISCONNECT_CLIENTID_PKT) {
		if (strcmp(RecvBuf, "ok") != 0) {
			return false;
		}
	}
	else if (queryId == QUERY_J1708MSG_PKT || queryId == QUERY_J1708MSG_BLOCK_PKT) {
		intRetVal = atoi(RecvBuf);
	}
	else if (queryId == QUERY_SEND_COMMAND) {
		intRetVal = atoi(RecvBuf);
	}
	else if (queryId == QUERY_NUMBER_J1708_CONN) {
		intRetVal = atoi(RecvBuf);
	}

	return true;
}

/******************/
/* OpenDriverApp */
/****************/
bool OpenDriverApp() {	
	_DbgTrace(_T("\nOpenDriverApp()\n"));
	//LPTSTR szCmdline=_tcsdup(TEXT("WindowsApplication1.exe"));
	//LPPROCESS_INFORMATION procInfo;
	//CreateProcess(szCmdline, NULL, NULL, NULL, false, CREATE_NEW_CONSOLE, NULL, NULL, NULL, procInfo);

	
	_DbgTrace(_T("OpenDriverFunc&^&#^*()\n"));
	SHELLEXECUTEINFO execInfo;
	memset(&execInfo, 0, sizeof(execInfo));

	execInfo.cbSize = sizeof(execInfo);
	execInfo.fMask = 0;
	execInfo.hwnd = 0;
	execInfo.lpVerb = _T("open"); //operation to perform
	//execInfo.lpFile = _T("WindowsApplication2.exe");
	execInfo.lpFile = _T("QBridgeWinCEDriver.exe");

	execInfo.lpParameters = _T("");
	execInfo.lpDirectory = 0;
	execInfo.nShow = SW_SHOW;
	execInfo.hInstApp = 0;
	if (ShellExecuteEx(&execInfo) == FALSE) {
		return false;
	}
	else
	{
		return true;
	}
}

/**********************/
/* SendUDPClosePacket */
/********************/
void SendUDPClosePacket() {
	if (assignPort <= 0) {
		return;
	}
  _DbgTrace(_T("SendUDPClosePacket()\n"));

  char sendBuf[] = "close";
  int bufLen = 5;
  SendUDPPacket(inet_addr("127.0.0.1"), DRIVER_LISTEN_PORT, assignPort, sendBuf, bufLen);
  assignPort = -1;
}

/******************/
/* SendUDPPacket */
/****************/
void SendUDPPacket(unsigned long toIPAddr, int toPort, int fromPort, const char* buf, int bufLen) {
	
  	//trans udp here
  WSADATA wsaData;
  SOCKET SendSocket;
  sockaddr_in ReplyAddr;
  sockaddr_in ToAddr;

  //---------------------------------------------
  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
	  return;
  }

  //---------------------------------------------
  // Create a socket for sending data
  SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
  // Bind the socket to any address and the specified port.
  if (fromPort > 0) {
	  ReplyAddr.sin_family = AF_INET;
	  ReplyAddr.sin_port = htons(fromPort);
	  //ReplyAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	  ReplyAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	  bind(SendSocket, (SOCKADDR *) &ReplyAddr, sizeof(ReplyAddr));
  }

  //---------------------------------------------
  // Set up the RecvAddr structure with the IP address of
  // the receiver (in this example case "123.456.789.1")
  // and the specified port number.
  ToAddr.sin_family = AF_INET;
  ToAddr.sin_port = htons(toPort);
  ToAddr.sin_addr.s_addr = toIPAddr;

  //---------------------------------------------
  // Send a datagram to the receiver
  sendto(SendSocket, 
    buf, 
    bufLen, 
    0, 
    (SOCKADDR *) &ToAddr, 
    sizeof(ToAddr));

  //---------------------------------------------
  // When the application is finished sending, close the socket.
  closesocket(SendSocket);

  //---------------------------------------------
  // Clean up and quit.
  WSACleanup();
}

/******************/
/* GetAssignPort */
/****************/
int GetAssignPort() {
	return assignPort;
}

/******************/
/* UDPListenFunc */
/****************/
static DWORD __stdcall UDPListenFunc(void * args) {
	// this function is a seperate thread always listening on the assigned udp port plus 1.
	WSADATA wsaData;
	SOCKET RecvSocket;
	sockaddr_in RecvAddr;
	int Port = assignPort + 1;
	const int BufLen = 1024;
	char RecvBuf[BufLen];
	sockaddr_in SenderAddr;
	int SenderAddrSize = sizeof(SenderAddr);
	//int timeoutTime = 5000; //5 sec

	//for select func:
	fd_set fdReadSet;
	fd_set fdErrorSet;
	TIMEVAL timeout = {5, 0}; // 5 second timeout
	timeout.tv_sec = 5;
	timeout.tv_usec = 500;


	_DbgTrace(_T("UDPListenFunc() Start\n"));

	//-----------------------------------------------
	// Initialize Winsock
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
		return 0;
	}
	
	//-----------------------------------------------
	// Create a receiver socket to receive datagrams
	RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	//setsockopt(RecvSocket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeoutTime, sizeof(timeoutTime));

	//-----------------------------------------------
	// Bind the socket to any address and the specified port.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(Port);
	//RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	bind(RecvSocket, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));


	int rv;
	do {
		//---------------------------------------------
		//Send a query packet
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(DRIVER_LISTEN_PORT);
		RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		char SendBuf[30];
		int len = GetSendQueryPacket(QUERY_HELLO_PKT, SendBuf, sizeof(SendBuf), 0, NULL, 0, 0);

		//_DbgTrace(_T("RD1"));
		sendto(RecvSocket, 
			SendBuf, 
			len, 
			0, 
			(SOCKADDR *) &RecvAddr, 
			sizeof(RecvAddr));

		FD_ZERO(&fdReadSet);
		FD_ZERO(&fdErrorSet);
		FD_SET(RecvSocket, &fdReadSet);
		FD_SET(RecvSocket, &fdErrorSet);

		if(select(0, &fdReadSet, NULL, &fdErrorSet, &timeout) != 1)
		{
			_DbgTrace(_T("Select 1 timeout\n"));
			break;
		}
		else {
			if (FD_ISSET(RecvSocket, &fdReadSet)) {
				// listen for ack from hello
				//_DbgTrace(_T("RD2"));
				rv = recvfrom(RecvSocket, 
					RecvBuf, 
					BufLen, 
					0, 
					(SOCKADDR *)&SenderAddr, 
					&SenderAddrSize);

				if (rv == SOCKET_ERROR)
				{
					_DbgTrace(_T("UDPListen RecvFrom Error\n"));
					break;
				}
			}
			else {
				_DbgTrace(_T("select 1 error\n"));
				break;
			}
		}
		
		RecvBuf[rv] = 0;
		if (strcmp(RecvBuf, "ack") == 0) {
			//_DbgTrace(_T("hello unblocking got ack\n"));
		}
		else {
			ProcessDataPacket(RecvBuf, RecvSocket, RecvAddr);
		}

		for (int i = 0; i < 5; i++)
		{
			FD_ZERO(&fdReadSet);
			FD_ZERO(&fdErrorSet);
			FD_SET(RecvSocket, &fdReadSet);
			FD_SET(RecvSocket, &fdErrorSet);

			if(select(0, &fdReadSet, NULL, &fdErrorSet, &timeout) != 1)
			{
				_DbgTrace(_T("Select 2 timeout\n"));
				break;
			}
			else {
				if (FD_ISSET(RecvSocket, &fdReadSet)) {
					rv = recvfrom(RecvSocket, 
						RecvBuf, 
						BufLen, 
						0, 
						(SOCKADDR *)&SenderAddr, 
						&SenderAddrSize);

					if (rv == SOCKET_ERROR)
					{
						int a = WSAGetLastError();
						if (WSAETIMEDOUT != a) {
							_DbgTrace(_T("socket error\n"));
							break;
						}
					}
					else {
						RecvBuf[rv] = 0;
						if (strcmp(RecvBuf, "ack") == 0) {
							//_DbgTrace(_T("hello unblocking got ack\n"));
						}
						else {
							ProcessDataPacket(RecvBuf, RecvSocket, RecvAddr);
						}
					}
				}
				else {
					_DbgTrace(_T("Select 2 error set\n"));
					break;
				}
			}		
		}
		if (udpThread.valid == false) {
			break;
		}
	}
	while (true);
	
	//no response from C# app, clean up
	CritSection cs;
	DLLCleanUp();

	//-----------------------------------------------
	// Close the socket when finished receiving datagrams
	_DbgTrace(_T("Finished receiving. Closing socket.\n"));
	closesocket(RecvSocket);

	//-----------------------------------------------
	// Clean up and exit.
	_DbgTrace(_T("UDPLFUNC END\n"));
	WSACleanup();
	return 0;
}

/**********************/
/* ProcessDataPacket */
/********************/
void ProcessDataPacket(char* data, SOCKET RecvSocket, sockaddr_in RecvAddr)
{
	if (strcmp(data, "hello") == 0) {
		// send ack
		const int bufLen = 30;
		char sendBuf[bufLen];		
		strncpy(sendBuf, "ack", bufLen);
		int len = int(strlen(sendBuf));
		sendto(RecvSocket, sendBuf, len, 0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
	}
	else {
		// process format: <pkt type>, <client id>, <trans id>, <is notify 0 or 1>, <msg len>;<msg>
		char pktType[100];
		size_t i = 0;
		for (i = 0; i < strlen(data); i++) {
			if (data[i] == ',') {
				pktType[i] = 0;
				break;
			}
			pktType[i] = data[i];
		}
		i++;
		char cid[10];
		int j = 0;
		for (i; i < strlen(data); i++) {
			if (data[i] == ',') {
				cid[j] = 0;
				break;
			}
			cid[j] = data[i];
			j++;
		}
		i++;
		char tid[10];
		j = 0;
		for (i; i < strlen(data); i++) {
			if (data[i] == ',') {
				tid[j] = 0;
				break;
			}
			tid[j] = data[i];
			j++;
		}
		i++;
		char nid[10];
		j = 0;
		for (i; i < strlen(data); i++) {
			if (data[i] == ',') {
				nid[j] = 0;
				break;
			}
			nid[j] = data[i];
			j++;
		}
		i++;
		char tmp[10]; // get msg len
		j=0;
		for (i; i < strlen(data); i++) {
			if (data[i] == ';') {
				tmp[j] = 0;
				break;
			}
			tmp[j] = data[i];
			j++;
		}
		i++;
		int msgLen = atoi(tmp);
		char msg[300];
		int k = 0;
		for (size_t p = i; p < (i+msgLen); p++) {
			msg[k] = data[p];
			k++;
		}
		msg[k] = 0;
		int clientid = atoi(cid);
		int transid = atoi(tid);
		short isnotify = atoi(nid);
		if (strcmp(pktType, "sendJ1708commerr") == 0) {
			connections[clientid].UpdateTransaction(isnotify, transid, ERR_INVALID_DEVICE);
			_DbgTrace(_T("sendJ1708commerr"));
		}
		else if (strcmp(pktType, "sendJ1708replytimeout") == 0) {
			connections[clientid].UpdateTransaction(isnotify, transid, ERR_HARDWARE_NOT_RESPONDING);
			_DbgTrace(_T("sendJ1708replytimeout"));
		}
		else if (strcmp(pktType, "sendJ1708confirmfail") == 0) {
			connections[clientid].UpdateTransaction(isnotify, transid, ERR_NOT_ADDED_TO_BUS);
			_DbgTrace(_T("sendJ1708confirmfail"));
		}
		else if (strcmp(pktType, "sendJ1708success") == 0) {
			CritSection cs;
			cs.Pause();
			/*for (int i = 0; i < 80; i++) {
				if (connections[clientid].UpdateTransaction(isnotify, transid, 0)) {
					break;
				}
				::Sleep(10);
			}*/
			if (connections[clientid].UpdateTransaction(isnotify, transid, 0) == FALSE) {
				_DbgTrace(_T("sendj1708succes EARLY :(\n"));
			}
			cs.Unpause();
			_DbgTrace(_T("sendJ1708success"));
		}
		else if (strcmp(pktType, "readmessage") == 0) {
			connections[clientid].AddReadMsg(msg, msgLen);
			_DbgTrace(_T("new rp1210 readmessage"));
		}
	}
}

/***************/
/* DLLCleanUp */
/*************/
void DLLCleanUp()
{
	for (int i = 0; i <= maxClientID; i++) {
		connections[i].Clear();
	}
	assignPort = -1;
	udpThread.valid = false;
}

/*****************/
/* GetReadEvent */
/***************/
HANDLE GetReadEvent() {	
	readEventId++;
	if (readEventId < 1 || readEventId > 100000) {
		readEventId = 1;
	}
	wchar_t lpName[30];
	swprintf(lpName, L"blockOnRead_Event_%d", readEventId);
	return ::CreateEvent(NULL, FALSE, FALSE, lpName);
}
