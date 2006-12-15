// TestRP1210_PCDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "..\..\CommonSrc\inimgr.h"


// CTestRP1210_PCDlg dialog
class CTestRP1210_PCDlg : public CDialog
{
// Construction
public:
	CTestRP1210_PCDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TESTRP1210_PC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	vector<INIMgr::Devices> dev;
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CCheckListBox m_clb;
	CComboBox m_dev1;
	CComboBox m_dev2;
	CRichEditCtrl m_log;
	afx_msg void OnBnClickedTest1();
	afx_msg void OnBnClickedBtn2();
public:
	afx_msg void OnBnClickedOk();
};
