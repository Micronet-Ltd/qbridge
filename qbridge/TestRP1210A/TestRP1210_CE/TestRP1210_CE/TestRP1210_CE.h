// TestRP1210_CE.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#ifdef STANDARDSHELL_UI_MODEL
#include "resource.h"
#endif

// CTestRP1210_CEApp:
// See TestRP1210_CE.cpp for the implementation of this class
//

class CTestRP1210_CEApp : public CWinApp
{
public:
	CTestRP1210_CEApp();
	
// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CTestRP1210_CEApp theApp;
