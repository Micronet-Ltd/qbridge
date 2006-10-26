// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

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
