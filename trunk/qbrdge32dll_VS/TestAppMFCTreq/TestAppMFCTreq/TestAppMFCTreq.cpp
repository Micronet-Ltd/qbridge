// TestAppMFCTreq.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TestAppMFCTreq.h"
#include "TestAppMFCTreqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestAppMFCTreqApp

BEGIN_MESSAGE_MAP(CTestAppMFCTreqApp, CWinApp)
END_MESSAGE_MAP()


// CTestAppMFCTreqApp construction
CTestAppMFCTreqApp::CTestAppMFCTreqApp()
	: CWinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CTestAppMFCTreqApp object
CTestAppMFCTreqApp theApp;

// CTestAppMFCTreqApp initialization

BOOL CTestAppMFCTreqApp::InitInstance()
{

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CTestAppMFCTreqDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
