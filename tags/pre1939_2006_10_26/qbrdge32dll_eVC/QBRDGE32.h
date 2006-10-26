
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the RP1210A_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// RP1210A_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef __cplusplus
extern "C" {
#endif

#ifdef RP1210A_EXPORTS
#define RP1210A_API __declspec(dllexport)
#else
#define RP1210A_API __declspec(dllimport)
#endif

typedef short RP1210AReturnType;

// RP1210 functions
RP1210A_API RP1210AReturnType WINAPI RP1210_ClientConnect (
	HWND hwndClient,
	short nDeviceID,
	char far* fpchProtocol,
	long lTxBufferSize,
	long lRcvBufferSize,
	short nIsAppPacketizingIncomingMsgs
);

RP1210A_API RP1210AReturnType WINAPI RP1210_ClientDisconnect (
	short nClientID
);

RP1210A_API RP1210AReturnType WINAPI RP1210_SendMessage (
	short nClientID,
	char far* fpchClientMessage,
	short nMessageSize,
	short nNotifyStatusOnTx,
	short nBlockOnSend
);

RP1210A_API RP1210AReturnType WINAPI RP1210_ReadMessage (
	short nClientID,
	char far* fpchAPIMessage,
	short nBufferSize,
	short nBlockOnRead
);

RP1210A_API RP1210AReturnType WINAPI RP1210_SendCommand (
	short nCommandNumber,
	short nClientID,
	char far* fpchClientCommand,
	short nMessageSize
);

RP1210A_API RP1210AReturnType WINAPI RP1210_ReadVersion (
	char far* fpchDLLMajorVersion,
	char far* fpchDLLMinorVersion,
	char far* fpchAPIMajorVersion,
	char far* fpchAPIMinorVersion
);

RP1210A_API RP1210AReturnType WINAPI RP1210_GetErrorMsg (
	short nErrorCode,
	char far* fpchDescription
);

RP1210A_API RP1210AReturnType WINAPI RP1210_GetHardwareStatus (
	short nClientID,
	char far* fpchClientInfo,
	short nInfoSize,
	short nBlockOnRequest
);

RP1210A_API void WINAPI RP1210_GetStatusInfo(TCHAR *buf, int size);

#ifdef __cplusplus
}
#endif

void ToAnsi(const TCHAR *unicodeBuf, char *buf, int size);
void ToUnicode(char *buf, TCHAR *unicodeBuf, int size);


// RP1210 Error codes
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

// RP1210 Commands for the RP1210_SendCommand function
#define CMD_RESET_DEVICE						0
#define CMD_ALL_FILTERS_PASS					3
#define CMD_SET_J1939_FILTERING				4
#define CMD_SET_CAN_FILTERING					5
#define CMD_SET_J1708_FILTERING				7
#define CMD_GENERIC_DRIVER_CMD				14
#define CMD_SET_J1708_MODE						15
#define CMD_SET_ECHO_TRANSMITTED_MSGS		16
#define CMD_ALL_FILTERS_DISCARD				17
#define CMD_SET_MESSAGE_RECEIVE				18
#define CMD_PROTECT_J1939_ADDRESS			19
#define CMD_UPGRADE_FIRMWARE					256

// RP1210 WindowsMessages
#define QBRIDGE_RP1210_MESSAGE_STRING		_T("QBRIDGE_WINDOWS_MESSAGE_STRING")
#define QBRIDGE_RP1210_ERROR_STRING			_T("QBRIDGE_WINDOWS_ERROR_STRING")

// QSI Protocol related stuff
#define QBRIDGE_COM1								1975		
#define QBRIDGE_COM4								(QBRIDGE_COM1+3)
#define QBRIDGE_COM5								(QBRIDGE_COM4+1)
#define QBRIDGE_COM6								(QBRIDGE_COM5+1)
#define QBRIDGE_J1708_PROTOCOL				"J1708"
#define QBRIDGE_J1939_PROTOCOL				"J1939"

const int DRIVER_LISTEN_PORT = 11234;

const int UDP_DEBUG_PORT = 23233;
#define UDP_DEBUG_SEND

enum PACKET_TYPE {
	QUERY_HELLO_PKT = 1,
	QUERY_PROCID_PKT = 2,
	QUERY_NEW_CLIENTID_PKT = 3,
	QUERY_NEWPORT_PKT = 4,
	QUERY_DISCONNECT_CLIENTID_PKT = 5,
	QUERY_J1708MSG_PKT = 6,
	QUERY_J1708MSG_BLOCK_PKT = 7,
	QUERY_SEND_COMMAND = 8,
	QUERY_NUMBER_J1708_CONN = 9
};
