// TestQBTreqVM.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#ifdef STANDARDSHELL_UI_MODEL
#include "resource.h"
#endif

// CTestQBTreqVMApp:
// See TestQBTreqVM.cpp for the implementation of this class
//

class CTestQBTreqVMApp : public CWinApp
{
public:
	CTestQBTreqVMApp();
	
// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CTestQBTreqVMApp theApp;
