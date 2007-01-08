// TestRP1210_CEDlg.h : header file
//

#pragma once
#include "..\..\CommonSrc\inimgr.h"
#include <afxwin.h>

// CTestRP1210_CEDlg dialog
class CTestRP1210_CEDlg : public CDialog
{
// Construction
public:
	CTestRP1210_CEDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TESTRP1210_CE_DIALOG };


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	vector<INIMgr::Devices> dev;
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedTest1();

	CListBox m_clb;
	CComboBox m_dev1;
	CComboBox m_dev2;
	CEdit m_log;

public:
	afx_msg void OnBnClickedSelall();
public:
	afx_msg void OnBnClickedClearresults();
public:
	afx_msg void OnBnClickedDesel();
};
