
#if _WIN32_WCE

#if !defined(AFX_STDAFX_H__174AF967_442C_4851_8B51_3AA4CEBE03C4__INCLUDED_)
#define AFX_STDAFX_H__174AF967_442C_4851_8B51_3AA4CEBE03C4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// Insert your headers here
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include <list>
#include <map>
#include <deque>
#include <vector>
using namespace std;

#include <windows.h>

#include <TCHAR.H>
#include <Winsock2.h>
#include <stdlib.h>
//#include <direct.h>
#include <Shellapi.h>

//#define ASSERT _ASSERT

void RP1210Trace(_TCHAR *formatStr, ...);
void _DbgTrace(_TCHAR *formatStr, ...);
void _DbgPrintErrCode(DWORD errCode);

#ifdef _DEBUG
#define TRACE ::RP1210Trace
#else
#define TRACE 1 ? (void)0 : ::RP1210Trace
#endif

// TODO: reference additional headers your program requires here

//{{AFX_INSERT_LOCATION}}
// Microsoft eMbedded Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__174AF967_442C_4851_8B51_3AA4CEBE03C4__INCLUDED_)

#else //NOT _WIN32_WCE

#define _CRT_SECURE_NO_DEPRECATE 
#pragma comment(lib,"wsock32.lib") 

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <list>
#include <map>
#include <deque>
#include <vector>
using namespace std;

#include <windows.h>

#include <crtdbg.h>
#include <TCHAR.H>
#include <Winsock2.h>
#include <stdlib.h>
#include <direct.h>
#include <Shellapi.h>

#define ASSERT _ASSERT

void RP1210Trace(_TCHAR *formatStr, ...);
void _DbgTrace(_TCHAR *formatStr, ...);

#ifdef _DEBUG
#define TRACE ::RP1210Trace
#else
#define TRACE 1 ? (void)0 : ::RP1210Trace
#endif

#define _CRT_SECURE_NO_DEPRECATE 
//#define  _CRT_NON_CONFORMING_SWPRINTFS
#endif //_WIN32_WCE