// TestAppMFCDlg.h : header file
//

#pragma once
#include "afxwin.h"

class Thread
{
public:
	Thread() : valid(false) {}
	bool SetupThread ( DWORD (WINAPI * pFun) (void* arg), void* pArg)
    {
		if (valid) {
			return false;
		}
		valid = true;
        _handle = CreateThread (
            NULL, // Security attributes
            0, // Stack size
            pFun,
            pArg,
            CREATE_SUSPENDED,
            &_tid);
		return true;
    }
	~Thread () { } //CloseHandle (_handle);
    void Resume () { ResumeThread (_handle); }
    void WaitForDeath ()
    {
        WaitForSingleObject (_handle, 2000);
    }

	bool valid;
private:
    HANDLE _handle;
    DWORD  _tid;     // thread id
};

extern Thread dThread;

// CTestAppMFCDlg dialog
class CTestAppMFCDlg : public CDialog
{
// Construction
public:
	CTestAppMFCDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TESTAPPMFC_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void rp1210SendMessage(short comClient, short nBlockOnSend);
	void rp1210ReadMessage(short comClient, short nBlockOnRead);
	void rp1210SendCommand(short nCommandNumber, short nClientID);
	void rp1210GetHardwareStatus(short nClientID);
	
	void rp1210SendCustomMsg(short comClient, short nBlockOnSend, char far* fpchMsg, short msgSize);
	
public:
// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
public:
	afx_msg void OnBnClickedButton2();
public:
	afx_msg void OnBnClickedButton3();
public:
	afx_msg void OnBnClickedButton4();
public:
	CEdit m_editbox;
public:
	afx_msg void OnBnClickedButton5();
public:
	afx_msg void OnBnClickedButton6();
public:
	afx_msg void OnBnClickedButton7();
public:
	afx_msg void OnBnClickedCreatecon4Btn();
public:
	afx_msg void OnBnClickedSendCom4btn();
public:
	afx_msg void OnBnClickedReadcom4Btn();
public:
	afx_msg void OnBnClickedButton8();
public:
	afx_msg void OnBnClickedSendresetcmdBtn();
public:
	afx_msg void OnBnClickedSendresetcmdBtn2();
public:
	afx_msg void OnBnClickedSendresetcmdBtn3();
public:
	afx_msg void OnBnClickedButton9();
public:
	afx_msg void OnBnClickedsendj1939msgbtn();
};

static DWORD __stdcall DisconnectFunc(void* args);
