// TestRP1210_CEDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestRP1210_CE.h"
#include "TestRP1210_CEDlg.h"

#include "..\..\commonSrc\TestRP1210.h"
#include "..\..\commonSrc\log.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTestRP1210_CEDlg dialog

/*****************************************/
/* CTestRP1210_CEDlg::CTestRP1210_CEDlg */
/***************************************/
CTestRP1210_CEDlg::CTestRP1210_CEDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestRP1210_CEDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

/**************************************/
/* CTestRP1210_CEDlg::DoDataExchange */
/************************************/
void CTestRP1210_CEDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, CTRL_TESTLIST, m_clb);
	DDX_Control(pDX, CTRL_D1, m_dev1);
	DDX_Control(pDX, CTRL_D2, m_dev2);
	DDX_Control(pDX, CTRL_RE, m_log);
}

BEGIN_MESSAGE_MAP(CTestRP1210_CEDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(CTRL_TEST1, &CTestRP1210_CEDlg::OnBnClickedTest1)
	ON_BN_CLICKED(CTRL_SELALL, &CTestRP1210_CEDlg::OnBnClickedSelall)
	ON_BN_CLICKED(CTRL_CLEARRESULTS, &CTestRP1210_CEDlg::OnBnClickedClearresults)
	ON_BN_CLICKED(CTRL_DESEL, &CTestRP1210_CEDlg::OnBnClickedDesel)
END_MESSAGE_MAP()


// CTestRP1210_CEDlg message handlers

/************************************/
/* CTestRP1210_CEDlg::OnInitDialog */
/**********************************/
BOOL CTestRP1210_CEDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	log.SetEdit(&m_log);


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
	
	m_clb.AddString (_T("Read Version"));
	m_clb.AddString (_T("Connect"));
	m_clb.AddString (_T("Multiconnect"));
	m_clb.AddString (_T("J1708 Basic Read"));
	m_clb.AddString (_T("J1708 Advanced Read"));
	m_clb.AddString (_T("J1708 Multi Read"));
	m_clb.AddString (_T("J1708 Basic Send"));
	m_clb.AddString (_T("J1708 Advanced Send"));
	m_clb.AddString (_T("J1708 Window Notify"));
	m_clb.AddString (_T("J1708 Filter States/On Off message"));
	m_clb.AddString (_T("J1708 Filters"));
	m_clb.AddString (_T("J1939 Address Claim"));
	m_clb.AddString (_T("J1939 Basic Read"));
	m_clb.AddString (_T("J1939 Advanced Read"));
	m_clb.AddString (_T("J1939 Basic Send"));
	m_clb.AddString (_T("J1939 Window Notify"));
	m_clb.AddString (_T("J1939 Filters"));

	for (int i = 0; i < m_clb.GetCount(); i++) {
		CString curTestName;
		m_clb.GetText(i, curTestName);
		m_clb.SetSel(i, AfxGetApp()->GetProfileInt(_T("Tests"), curTestName, 1));
	}


	return TRUE;  // return TRUE  unless you set the focus to a control
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CTestRP1210_CEDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	DRA::RelayoutDialog(
		AfxGetInstanceHandle(), 
		this->m_hWnd, 
		DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_TESTRP1210_CE_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_TESTRP1210_CE_DIALOG));
}
#endif


/****************************************/
/* CTestRP1210_CEDlg::OnBnClickedTest1 */
/**************************************/
void CTestRP1210_CEDlg::OnBnClickedTest1()
{
	static bool isTesting = false;
	if (isTesting) { return; }
	isTesting = true;

	AfxGetApp()->WriteProfileInt(_T("Default"), _T("dev1"), m_dev1.GetCurSel());
	AfxGetApp()->WriteProfileInt(_T("Default"), _T("dev2"), m_dev2.GetCurSel());

	set <CString> testList;
	int *sel = (int *)alloca(sizeof(int) * m_clb.GetCount());
	int selectedCount = m_clb.GetSelItems(m_clb.GetCount(), sel);
	set<int> selectedSet;
	for (int i = 0; i < selectedCount; i++) {
		selectedSet.insert(sel[i]);
	}

	for (int i = 0; i < m_clb.GetCount(); i++) {
		CString curTestName;
		m_clb.GetText(i, curTestName);
		
		int result = (selectedSet.find(i) != selectedSet.end());
		AfxGetApp()->WriteProfileInt(_T("Tests"), curTestName, result);
		if (result) {
			testList.insert(curTestName);
		}
	}

		
	TestRP1210 rtst;
	rtst.Test (testList, dev, m_dev1.GetCurSel(), m_dev2.GetCurSel());

	isTesting = false;
}

/*****************************************/
/* CTestRP1210_CEDlg::OnBnClickedSelall */
/***************************************/
void CTestRP1210_CEDlg::OnBnClickedSelall()
{
	int i;
	for (i = 0; i < m_clb.GetCount(); i++) {
		m_clb.SetSel(i, true);
	}
}

/***********************************************/
/* CTestRP1210_CEDlg::OnBnClickedClearresults */
/*********************************************/
void CTestRP1210_CEDlg::OnBnClickedClearresults()
{
	m_log.SetWindowText(_T(""));
}

/****************************************/
/* CTestRP1210_CEDlg::OnBnClickedDesel */
/**************************************/
void CTestRP1210_CEDlg::OnBnClickedDesel()
{
	int i;
	for (i = 0; i < m_clb.GetCount(); i++) {
		m_clb.SetSel(i, false);
	}
}
