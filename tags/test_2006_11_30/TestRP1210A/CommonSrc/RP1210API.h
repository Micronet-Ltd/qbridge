#pragma once

#include "Log.h"
class RP1210API {
public:
	RP1210API(CString dllBaseName);
	~RP1210API(void);

	inline bool IsWorking() { return valid ; }

	typedef short (WINAPI *fxRP1210_ClientConnect) ( HWND, short, char *, long, long, short );
	typedef short (WINAPI *fxRP1210_ClientDisconnect) ( short );
	typedef short (WINAPI *fxRP1210_SendMessage) ( short, char*, short, short, short );
	typedef short (WINAPI *fxRP1210_ReadMessage) ( short, char*, short, short );
	typedef short (WINAPI *fxRP1210_SendCommand) ( short, short, char*, short );
	typedef void (WINAPI *fxRP1210_ReadVersion) ( char*, char*, char*, char* );
	typedef short (WINAPI *fxRP1210_GetErrorMsg) ( short, char* );
	typedef short (WINAPI *fxRP1210_GetHardwareStatus) ( short, char*, short, short );
	fxRP1210_ClientConnect pRP1210_ClientConnect;
	fxRP1210_ClientDisconnect pRP1210_ClientDisconnect;
	fxRP1210_ReadMessage pRP1210_ReadMessage;
	fxRP1210_SendMessage pRP1210_SendMessage;
	fxRP1210_SendCommand pRP1210_SendCommand;
	fxRP1210_ReadVersion pRP1210_ReadVersion;
	fxRP1210_GetErrorMsg pRP1210_GetErrorMsg;
	fxRP1210_GetHardwareStatus pRP1210_GetHardwareStatus;
	HMODULE hRP1210DLL;
private:
#ifndef WINCE
	FARPROC GetProcAddress(HMODULE hModule, LPCTSTR lpProcName);
#endif

	template <typename T> void LoadProc (T & toSet, CString name, bool &stillValid) {
		if (hRP1210DLL == NULL) { return; }

		toSet = (T)(GetProcAddress(hRP1210DLL, name));
		if ( toSet == NULL ) {
			toSet = (T)(GetProcAddress(hRP1210DLL, name + _T("@24")));
			if ( toSet == NULL ) {
				log.LogText(_T("Error: Could not find procedure \"") + name + _T("\" in DLL!\n"), Log::Red);
				TRACE(_T("Error: Could not find procedure \"%s\" in DLL!\n"), name );
			}
		}
		if (toSet == NULL) {
			stillValid = false;
		}
	}
	bool valid;
};
