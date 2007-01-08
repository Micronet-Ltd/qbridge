// TestAppMFCTreqDlg.h : header file
//

#pragma once
#include "c:\program files\microsoft visual studio 8\vc\atlmfc\include\afxwin.h"

// CTestAppMFCTreqDlg dialog
class CTestAppMFCTreqDlg : public CDialog
{
// Construction
public:
	CTestAppMFCTreqDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TESTAPPMFCTREQ_DIALOG };


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void rp1210SendMessage(short comClient, short nBlockOnSend);
	void rp1210ReadMessage(short comClient, short nBlockOnRead);
	void rp1210SendCommand(short nCommandNumber, short nClientID);
	void rp1210GetHardwareStatus(short nClientID);

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
public:
	afx_msg void OnBnClickedCreatecon4Btn();
public:
	afx_msg void OnBnClickedButton4();
public:
	afx_msg void OnBnClickedButton6();
public:
	afx_msg void OnBnClickedButton7();
public:
	afx_msg void OnBnClickedButton8();
public:
	afx_msg void OnBnClickedSendresetcmdBtn();
public:
	afx_msg void OnBnClickedSendresetcmdBtn2();
public:
	afx_msg void OnBnClickedSendresetcmdBtn3();
public:
	afx_msg void OnBnClickedButton2();
public:
	afx_msg void OnBnClickedButton3();
public:
	afx_msg void OnBnClickedSendCom4btn();
public:
	afx_msg void OnBnClickedReadcom4Btn();
public:
	afx_msg void OnBnClickedButton5();
public:
	CEdit m_editbox;
public:
	afx_msg void OnBnClickedButton9();
public:
	afx_msg void OnBnClickedButton10();
public:
	afx_msg void OnBnClickedButton11();
public:
	afx_msg void OnBnClickedSendresetcmdBtn4();
public:
	afx_msg void OnBnClickedSendresetcmdBtn5();
public:
	afx_msg void OnBnClickedSendresetcmdBtn6();
};
