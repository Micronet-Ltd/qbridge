// QBRDGE32.cpp : Defines the entry point for the DLL application.
//

#include "Stdafx.h"
#include "QBRDGE32.h"
#include "rp1210a_impl.h"

#define CRC_CITT_ONLY
#define INC_CRC_FUNCS
#include "crc.h"

static UINT16 crcTable[256] = {0, 0};
const int CRCSize = 2;

CRITICAL_SECTION CritSection::critSection;
bool CritSection::critSectionInited = false;;

char *errorStrings[128] = {0};

#ifdef _WIN32_WCE
#define IS_PTR_VALID(ptr, size, rwflags) (true)
#else
#define IS_PTR_VALID(ptr, size, rwflags) (_CrtIsValidPointer(ptr, size, rwflags))
#endif

#define ARRAYLEN(arr) (sizeof(arr)/sizeof(arr[0]))

/**********************/
/* SetupErrorStrings */
/********************/
void SetupErrorStrings() {
#define SET_ERR_STR(num,str) { errorStrings[num-128] = #num ":" str; }
//					|-123456789-123456789-123456789-123456789-123456789-123456789-123456789-12345678|
	SET_ERR_STR (ERR_ADDRESS_CLAIM_FAILED,"The API was not able to claim the requested address.");
	SET_ERR_STR (ERR_ADDRESS_LOST,"The API was forced to concede the address to another node.");
	SET_ERR_STR (ERR_ADDRESS_NEVER_CLAIMED,"J1939 client never issued a protect address.");	//|
	SET_ERR_STR (ERR_BLOCK_NOT_ALLOWED,"Returned under WindowsTM 3.1x if blocking requested.");
	SET_ERR_STR (ERR_BUS_OFF,"The API was unable to transmit a CAN packet. Hardware is BUS_OFF");
	SET_ERR_STR (ERR_CANNOT_SET_PRIORITY,"ERR_CANNOT_SET_PRIORITY");									//|
	SET_ERR_STR (ERR_CHANGE_MODE_FAILED,"Change_1708_Mode tried, but multiple clients connected.");
	SET_ERR_STR (ERR_CLIENT_ALREADY_CONNECTED,"A client is already connected to the device.");
	SET_ERR_STR (ERR_CLIENT_AREA_FULL,"The maximum number of connections has been reached.");	//|
	SET_ERR_STR (ERR_CLIENT_DISCONNECTED,"Disconnect called while blocking on another call.");
	SET_ERR_STR (ERR_CODE_NOT_FOUND,"No description available for the requested error code.");
	SET_ERR_STR (ERR_COMMAND_NOT_SUPPORTED,"The command number is not supported by the API DLL.");
	SET_ERR_STR (ERR_CONNECT_NOT_ALLOWED,"Only one connection is allowed in the requested Mode.");
	SET_ERR_STR (ERR_COULD_NOT_TX_ADDRESS_CLAIMED,"The API was not able to request an address.");
	SET_ERR_STR (ERR_DEVICE_IN_USE,"The device is in use.  Multiple connections not supported");
	SET_ERR_STR (ERR_DLL_NOT_INITIALIZED,"Call RP1210_ClientConnect first.");						//|
	SET_ERR_STR (ERR_FREE_MEMORY,"An error occurred in the process of memory de-allocation");
	SET_ERR_STR (ERR_HARDWARE_NOT_RESPONDING,"The device hardware interface is not responding.");
	SET_ERR_STR (ERR_HARDWARE_STATUS_CHANGE,"The status of the device hardware has changed.");
	SET_ERR_STR (ERR_INVALID_CLIENT_ID,"The client ID provided is invalid or unrecognized");
	SET_ERR_STR (ERR_INVALID_COMMAND,"The command number or parameters are wrongly specified");
	SET_ERR_STR (ERR_INVALID_DEVICE,"The specified device ID is invalid.");							//|
	SET_ERR_STR (ERR_INVALID_PROTOCOL,"The specified protocol is invalid or unsupported.");	//|
	SET_ERR_STR (ERR_MAX_FILTERS_EXCEEDED,"ERR_MAX_FILTERS_EXCEEDED");								//|
	SET_ERR_STR (ERR_MAX_NOTIFY_EXCEEDED,"Notification is requested and no handles are available");
	SET_ERR_STR (ERR_MESSAGE_NOT_SENT,"Returned if blocking is used and the message was not sent");
	SET_ERR_STR (ERR_MESSAGE_TOO_LONG,"The message being to be transmitted is too long.");		//|
	SET_ERR_STR (ERR_MULTIPLE_CLIENTS_CONNECTED,"Device not reset, more clients are connected");
	SET_ERR_STR (ERR_NOT_ENOUGH_MEMORY,"Unable to allocate enough memory to create a new client");
	SET_ERR_STR (ERR_RX_QUEUE_CORRUPT,"The API DLL’s message receive queue is corrupt.");		//|
	SET_ERR_STR (ERR_RX_QUEUE_FULL,"The API DLL’s message receive queue is full.");				//|
	SET_ERR_STR (ERR_TXMESSAGE_STATUS,"A queued message was sent from the device.");				//|
	SET_ERR_STR (ERR_TX_QUEUE_CORRUPT,"The API DLL’s transmit message queue is corrupt.");		//|
	SET_ERR_STR (ERR_TX_QUEUE_FULL,"The API DLL’s transmit message queue is full.");				//|
	SET_ERR_STR (ERR_WINDOW_HANDLE_REQUIRED,"Supply window handle to request the notify status.");
	SET_ERR_STR (ERR_FW_FILE_READ,"Unable to open the requested file for firmware upgrade.");
	SET_ERR_STR (ERR_FW_UPGRADE,"An error occured upgrading the device's firmware.");
	SET_ERR_STR (ERR_BLOCKED_NOTIFY,"Notification not supported for blocking calls.");
	SET_ERR_STR (ERR_NOT_ADDED_TO_BUS,"The QBridge was unable to place the message on the bus.");
	SET_ERR_STR (ERR_MISC_COMMUNICATION,"Misc error communicating with QBridge driver process.");
	SET_ERR_STR (ERR_RECV_OPERATION_TIMEOUT,"No messages recieved, waiting operation aborted.");

#undef SET_ERR_STRING
}

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
			CritSection::Init();
//			_DbgTrace (_T("Process Attach!!!!!!!!!!!!!!\n"));
			{
				CritSection cs;
				InitializeDLL();
				SetupErrorStrings();
				createCITTTable(crcTable);
			}
			break;
		case DLL_THREAD_ATTACH:
			//_DbgTrace(_T("DLL_THREAD_ATTACH!\n"));
			break;
		case DLL_THREAD_DETACH:
		//	_DbgTrace(_T("DLL_THREAD_DETACH!\n"));
			break;
		case DLL_PROCESS_DETACH:
		//	_DbgTrace(_T("DLL_PROCESS_DETACH!\n"));
			break;
    }
    return TRUE;
}

/*************************/
/* RP1210_ClientConnect */
/***********************/
RP1210A_API RP1210AReturnType WINAPI RP1210_ClientConnect (HWND hwndClient, short nDeviceID, char far* fpchProtocol, long lTxBufferSize, long lRcvBufferSize, short nIsAppPacketizingIncomingMsgs) {
	CritSection cs;
	if (ConnectToDriverApp() == false) {
		_DbgTrace(_T("ConnectToDriverApp RP1210_ClientConnect failed"));
		return ERR_MISC_COMMUNICATION;
	}
	if ((nDeviceID >= QBRIDGE_COM1) && (nDeviceID <= QBRIDGE_COM6)) {
		int comPort = nDeviceID;

		if (strcmp(fpchProtocol, QBRIDGE_J1708_PROTOCOL) == 0) {
			return CreateConnection(1, comPort, hwndClient, lTxBufferSize, lRcvBufferSize, 0);			
		} else if (strcmp(fpchProtocol, QBRIDGE_J1939_PROTOCOL) == 0) {
			return CreateConnection(2, comPort, hwndClient, lTxBufferSize, lRcvBufferSize, nIsAppPacketizingIncomingMsgs);	
		} else {
			return ERR_INVALID_PROTOCOL;
		}
	} else {
		return ERR_INVALID_DEVICE;
	}
}

/****************************/
/* RP1210_ClientDisconnect */
/**************************/
RP1210AReturnType RP1210A_API WINAPI RP1210_ClientDisconnect (short nClientID) {
	CritSection cs;
	if (ConnectToDriverApp() == false) {
		_DbgTrace(_T("RP1210_ClientDisconnect failed"));
		return ERR_MISC_COMMUNICATION;
	}
	return Disconnect(nClientID);
}

/***********************/
/* RP1210_GetErrorMsg */
/*********************/
RP1210AReturnType RP1210A_API WINAPI RP1210_GetErrorMsg (short nErrorCode, char far* fpchDescription) {
	CritSection cs;
	ASSERT (IS_PTR_VALID(fpchDescription, 80, TRUE));
	strcpy(fpchDescription, "");
	if (nErrorCode < 128) {
		strcpy (fpchDescription, "No Error");
		return ERR_CODE_NOT_FOUND;
	} else {
		int index = nErrorCode - 128;
		if ((index < ARRAYLEN(errorStrings)) && (errorStrings[index] != NULL)) {
			strncpy(fpchDescription, errorStrings[index], 79);
			return 0;
		} else {
			strcpy (fpchDescription, "Unknown error");
			return ERR_CODE_NOT_FOUND;
		}
	}
}

/***********************/
/* RP1210_SendMessage */
/*********************/
RP1210AReturnType RP1210A_API WINAPI RP1210_SendMessage (short nClientID, char far* fpchClientMessage, short nMessageSize, short nNotifyStatusOnTx, short nBlockOnSend) {
	CritSection cs;

	return SendRP1210Message (nClientID, fpchClientMessage, nMessageSize, nNotifyStatusOnTx, nBlockOnSend, cs);
}

/***********************/
/* RP1210_ReadMessage */
/*********************/
RP1210AReturnType RP1210A_API WINAPI RP1210_ReadMessage (short nClientID, char far* fpchAPIMessage, short nBufferSize, short nBlockOnRead) {
	CritSection cs;
	ASSERT (IS_PTR_VALID(fpchAPIMessage, nBufferSize, TRUE));
	
	return ReadRP1210Message (nClientID, fpchAPIMessage, nBufferSize, nBlockOnRead, cs);
}

/***********************/
/* RP1210_SendCommand */
/*********************/
RP1210AReturnType RP1210A_API WINAPI RP1210_SendCommand (short nCommandNumber, short nClientID, char far* fpchClientCommand, short nMessageSize) {
	CritSection cs;
	//ASSERT (IS_PTR_VALID(fpchClientCommand, nMessageSize, TRUE));
	return SendCommand (nCommandNumber, nClientID, fpchClientCommand, nMessageSize, cs);
}

/*************************/
/* RP1210_GetStatusInfo */
/***********************/
RP1210A_API void WINAPI RP1210_GetStatusInfo(TCHAR *buf, int size) {
	CritSection cs;
	if (ConnectToDriverApp() == false) {
		_DbgTrace(_T("RP1210_GetStatusInfo failed"));
	}
	GetStatusInfo(buf, size);
	return;
}

/***********************/
/* RP1210_ReadVersion */
/*********************/
RP1210A_API void WINAPI RP1210_ReadVersion (char far* fpchDLLMajorVersion,	char far* fpchDLLMinorVersion,	char far* fpchAPIMajorVersion,	char far* fpchAPIMinorVersion) 
{
	CritSection cs;
	fpchDLLMajorVersion[0] = 0x31;
	fpchDLLMinorVersion[0] = 0x31;
	fpchAPIMajorVersion[0] = '2';
	fpchAPIMinorVersion[0] = '0';
}

/*****************************/
/* RP1210_GetHardwareStatus */
/***************************/
RP1210AReturnType RP1210A_API WINAPI RP1210_GetHardwareStatus (short nClientID, char far* fpchClientInfo, short nInfoSize, short nBlockOnRequest) {
	CritSection cs;
	ASSERT (IS_PTR_VALID(fpchClientInfo, nInfoSize, TRUE));

	return GetHardwareStatus (nClientID, fpchClientInfo, nInfoSize, nBlockOnRequest, cs);
}

/****************/
/* RP1210Trace */
/**************/
void RP1210Trace(_TCHAR *formatStr, ...) {
	static bool newLine = true;
	_TCHAR buf[2048];
	_TCHAR *bufPtr = buf;

	if (newLine) {
		DWORD count = GetTickCount();
		bufPtr += _sntprintf(buf, 2048, _T("%03d.%03d: "), (count/1000)%1000, count%1000);
	}

	va_list list;
	va_start (list, formatStr);
	_vsntprintf (bufPtr, 2000, formatStr, list); 
	va_end(list);

	size_t len = _tcslen(buf);
	if ((len > 0) && (buf[len-1] == '\n')) {
		newLine = true;
	} else if (len > 0) {
		newLine = false;
	}

	OutputDebugString(buf);
}

/**************/
/* ToUnicode */
/************/
void ToUnicode(char *buf, TCHAR *unicodeBuf, int size) {
    int zeroPos = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, (int)strlen(buf), unicodeBuf, size-1);
    if (zeroPos >= 0) {
        unicodeBuf[zeroPos]  = 0;
    }
}

/***********/
/* ToAnsi */
/*********/
void ToAnsi(const TCHAR *unicodeBuf, char *buf, int size) {
    BOOL defCharUsed = FALSE;
    int zeroPos = WideCharToMultiByte(CP_ACP, 0, unicodeBuf, (int)wcslen(unicodeBuf), buf, size-1, "~", &defCharUsed);
    if (zeroPos >= 0) {
        buf[zeroPos]  = 0;
    }
} 

/***************/
/* _DbgTrace */
/*************/
void _DbgTrace(_TCHAR *formatStr, ...)
{
	return;
#ifdef UDP_DEBUG_SEND
	static bool newLine = true;
	_TCHAR buf[2048];
	_TCHAR *bufPtr = buf;

	if (newLine) {
		DWORD count = GetTickCount();
		bufPtr += _sntprintf(buf, 2048, _T("%03d.%03d: "), (count/1000)%1000, count%1000);
	}

	va_list list;
	va_start (list, formatStr);
	_vsntprintf (bufPtr, 2000, formatStr, list); 
	va_end(list);

	int len = (int)_tcslen(buf);
	if ((len > 0) && (buf[len-1] == '\n')) {
		newLine = true;
	} else if (len > 0) {
		newLine = false;
	}
	
	//trans udp here
  WSADATA wsaData;
  SOCKET SendSocket;
  sockaddr_in RecvAddr;
  int Port = UDP_DEBUG_PORT;  
  char SendBuf[2048];

  for (int i = 0; i < len; i++)
  {
	SendBuf[i] = (char)buf[i];	  
  }

  //---------------------------------------------
  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
	  return;
  }

  //---------------------------------------------
  // Create a socket for sending data
  SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  //---------------------------------------------
  // Set up the RecvAddr structure with the IP address of
  // the receiver (in this example case "123.456.789.1")
  // and the specified port number.
  RecvAddr.sin_family = AF_INET;
  RecvAddr.sin_port = htons(Port);
  RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  //---------------------------------------------
  // Send a datagram to the receiver
  sendto(SendSocket, 
    SendBuf, 
    len, 
    0, 
    (SOCKADDR *) &RecvAddr, 
    sizeof(RecvAddr));

  //---------------------------------------------
  // When the application is finished sending, close the socket.
  closesocket(SendSocket);

  //---------------------------------------------
  // Clean up and quit.
  WSACleanup();
#endif
}

void _DbgPrintErrCode(DWORD errCode) {	
	if (errCode == WSANOTINITIALISED) {
		_DbgTrace(_T("A successful AfxSocketInit must occur before using this API.\r\n"));
	}
	else if (errCode == WSAENETDOWN) {
		_DbgTrace(_T("The Windows CE Sockets implementation detected that the network subsystem failed. \r\n"));
	}
	else if (errCode == WSAEADDRINUSE) {
		_DbgTrace(_T("The specified address is already in use. See the SO_REUSEADDR socket option under SetSockOpt.\r\n"));
	}
	else if (errCode == WSAEFAULT) {
		_DbgTrace(_T("The nSockAddrLen argument is too small—less than the size of a SOCKADDR structure.\r\n"));
	}
	else if (errCode == WSAEINPROGRESS) {
		_DbgTrace(_T("A blocking Windows CE Sockets call is in progress.\r\n"));
	}
	else if (errCode == WSAEAFNOSUPPORT) {
		_DbgTrace(_T("This port does not support the specified address family.\r\n"));
	}
	else if (errCode == WSAEINVAL) {
		_DbgTrace(_T("The socket is already bound to an address.\r\n"));
	}
	else if (errCode == WSAENOBUFS) {
		_DbgTrace(_T("Not enough buffers available; too many connections.\r\n"));
	}
	else if (errCode == WSAENOTSOCK) {
		_DbgTrace(_T("The descriptor is not a socket.\r\n"));
	}
	else if (errCode == WSAEINTR) {
		_DbgTrace(_T("WSAEINTR\r\n"));
	}
	else if (errCode == WSAEACCES) {
		_DbgTrace(_T("WSAEACCES\r\n"));
	}
	else if (errCode == WSAEACCES) {
		_DbgTrace(_T("WSAEACCES\r\n"));
	}
	else if (errCode == WSAEACCES) {
		_DbgTrace(_T("WSAEACCES\r\n"));
	}
	else if (errCode == 0) {
		_DbgTrace(_T("Ecode 0\r\n"));
	}
	else{
		_DbgTrace(_T("EEE::"));
		int BufLen = 20;
		char buff[20];
		
		BufLen = _snprintf(buff, 20, "Err: %d", errCode);
		
		TCHAR tbuf[20];
		for (int i = 0; i < 20; i++) {
			tbuf[i] = buff[i];
		}
		tbuf[BufLen] = 0;
		_DbgTrace(tbuf);
		_DbgTrace(_T("\n"));

	}
}