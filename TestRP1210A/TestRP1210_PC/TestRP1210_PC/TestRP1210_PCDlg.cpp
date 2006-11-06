// TestRP1210_PCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestRP1210_PC.h"
#include "TestRP1210_PCDlg.h"
#include "..\..\commonSrc\TestRP1210.h"
#include "..\..\commonSrc\log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CTestRP1210_PCDlg dialog




CTestRP1210_PCDlg::CTestRP1210_PCDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestRP1210_PCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestRP1210_PCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, CTRL_D1, m_dev1);
	DDX_Control(pDX, CTRL_D2, m_dev2);
	DDX_Control(pDX, CTRL_RE, m_log);
}

BEGIN_MESSAGE_MAP(CTestRP1210_PCDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(CTRL_TEST1, &CTestRP1210_PCDlg::OnBnClickedTest1)
END_MESSAGE_MAP()


// CTestRP1210_PCDlg message handlers

/************************************/
/* CTestRP1210_PCDlg::OnInitDialog */
/**********************************/
BOOL CTestRP1210_PCDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	log.SetRichEdit(&m_log);

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	dev = INIMgr::GetDevices();
	for (size_t i = 0; i < dev.size(); i++) {
		m_dev1.AddString(dev[i].deviceName +  _T(" (") + dev[i].deviceDesc + _T(")"));
		m_dev2.AddString(dev[i].deviceName +  _T(" (") + dev[i].deviceDesc + _T(")"));
		/*log.LogText(_T("Device: ") + dev[i].deviceName, Log::Blue);
		CString fmt;
		fmt.Format(_T("%d"), dev[i].deviceID);
		log.LogText(_T("    ID: ") + fmt);
		log.LogText(_T("    DLL: ") + dev[i].dll);
		log.LogText(_T("    Desc: ") + dev[i].deviceDesc);
		fmt = "";
		CString pids, pdescs, pstrgs;
		for (size_t j = 0; j < dev[i].protocolIDs.size(); j++) {
			pids += dev[i].protocolIDs[j] + _T(", ");
			pdescs += dev[i].protocolDescs[j] + _T(", ");
			pstrgs += dev[i].protocolStrings[j] + _T(", ");
		}
		log.LogText(_T("    Protocol IDs: ") + pids);
		log.LogText(_T("    Protocol Strings: ") + pstrgs);
		log.LogText(_T("    Protocol Desciptions: ") + pdescs);*/
	}

	m_dev1.SetCurSel(AfxGetApp()->GetProfileInt(_T("Default"), _T("dev1"), -1));
	m_dev2.SetCurSel(AfxGetApp()->GetProfileInt(_T("Default"), _T("dev2"), -1));
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTestRP1210_PCDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTestRP1210_PCDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTestRP1210_PCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


/****************************************/
/* CTestRP1210_PCDlg::OnBnClickedTest1 */
/**************************************/
void CTestRP1210_PCDlg::OnBnClickedTest1()
{
	static bool isTesting = false;
	if (isTesting) { return; }
	isTesting = true;

	AfxGetApp()->WriteProfileInt(_T("Default"), _T("dev1"), m_dev1.GetCurSel());
	AfxGetApp()->WriteProfileInt(_T("Default"), _T("dev2"), m_dev2.GetCurSel());
		
	TestRP1210 rtst;
	rtst.Test (dev, m_dev1.GetCurSel(), m_dev2.GetCurSel());

	isTesting = false;

}
