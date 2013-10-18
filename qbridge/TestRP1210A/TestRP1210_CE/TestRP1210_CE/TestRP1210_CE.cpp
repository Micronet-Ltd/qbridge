// TestRP1210_CE.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TestRP1210_CE.h"
#include "TestRP1210_CEDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTestRP1210_CEApp

BEGIN_MESSAGE_MAP(CTestRP1210_CEApp, CWinApp)
END_MESSAGE_MAP()


// CTestRP1210_CEApp construction
CTestRP1210_CEApp::CTestRP1210_CEApp()
	: CWinApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CTestRP1210_CEApp object
CTestRP1210_CEApp theApp;

// CTestRP1210_CEApp initialization

BOOL CTestRP1210_CEApp::InitInstance()
{

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CTestRP1210_CEDlg dlg;
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
