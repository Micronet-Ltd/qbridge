#include "StdAfx.h"
#include "RP1210API.h"

/*************************/
/* RP1210API::RP1210API */
/***********************/
RP1210API::RP1210API(CString dllBaseName)
{
	pRP1210_ClientConnect = NULL;
	pRP1210_ClientDisconnect = NULL;
	pRP1210_ReadMessage = NULL;
	pRP1210_SendMessage = NULL;
	pRP1210_SendCommand = NULL;
	pRP1210_ReadVersion = NULL;
	pRP1210_GetErrorMsg = NULL;
	pRP1210_GetHardwareStatus = NULL;
	hRP1210DLL = NULL;

	if ( ( hRP1210DLL = LoadLibrary( dllBaseName ) ) == NULL ) {
		TRACE(_T("Error: LoadLibrary( %s ) failed! \n"), dllBaseName );
		valid = false;
		return;
	}
	valid = (hRP1210DLL != NULL);
	LoadProc (pRP1210_ClientConnect, _T("RP1210_ClientConnect"), valid);
	LoadProc (pRP1210_ClientDisconnect, _T("RP1210_ClientDisconnect"), valid);
	LoadProc (pRP1210_ReadMessage, _T("RP1210_ReadMessage"), valid);
	LoadProc (pRP1210_SendMessage, _T("RP1210_SendMessage"), valid);
	LoadProc (pRP1210_SendCommand, _T("RP1210_SendCommand"), valid);
	LoadProc (pRP1210_ReadVersion, _T("RP1210_ReadVersion"), valid);
	LoadProc (pRP1210_GetErrorMsg, _T("RP1210_GetErrorMsg"), valid);
	LoadProc (pRP1210_GetHardwareStatus, _T("RP1210_GetHardwareStatus"), valid);
}

/**************************/
/* RP1210API::~RP1210API */
/************************/
RP1210API::~RP1210API(void)
{
}

#ifndef WINCE
/******************************/
/* RP1210API::GetProcAddress */
/****************************/
FARPROC RP1210API::GetProcAddress(HMODULE hModule, LPCTSTR lpProcName) {
	char ansiName[256];
	WideCharToMultiByte(CP_ACP, 0, lpProcName, -1, ansiName, 256, NULL, NULL);
	return ::GetProcAddress(hModule, ansiName);
}
#endif