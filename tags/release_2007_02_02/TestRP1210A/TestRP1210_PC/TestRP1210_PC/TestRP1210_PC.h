// TestRP1210_PC.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CTestRP1210_PCApp:
// See TestRP1210_PC.cpp for the implementation of this class
//

class CTestRP1210_PCApp : public CWinApp
{
public:
	CTestRP1210_PCApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CTestRP1210_PCApp theApp;