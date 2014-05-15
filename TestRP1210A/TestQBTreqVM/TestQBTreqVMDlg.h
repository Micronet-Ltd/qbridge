// TestQBTreqVMDlg.h : header file
//

#pragma once
#include "afxwin.h"

// CTestQBTreqVMDlg dialog
class CTestQBTreqVMDlg : public CDialog
{
// Construction
public:
	CTestQBTreqVMDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TESTQBTREQVM_DIALOG };


	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	//constants
	enum ProtocolType { _J1708, _J1939 };
	enum RP1210CommandNumber { 
SET_RESET_DEVICE						=0,
SET_ALL_FILTER_STATES_TO_PASS					=3,
SET_MSG_FILTER_J1939=4,
SET_CAN_FILTERING					=5,
SET_MSG_FILTER_J1708                 =7,
SET_GENERIC_DRIVER_CMD                  =14,
SET_J1708_MODE                      =15,
SET_ECHO_TRANSMITTED_MSGS           =16,
SET_ALL_FILTER_DISCARD                 =17,
SET_MESSAGE_RECEIVE                 =18,
SET_PROTECT_J1939_ADDRESS               =19,
SET_UPGRADE_FIRMWARE                    =256};
	enum TimerTypeID { _SEND_ID = 1, _RECV_ID = 2};

	//variables
	HICON m_hIcon;
	HMODULE qBModule;
	short clientID;
	ProtocolType protocol;
	bool isConnected;
	int readMsgCnt;
	int sendMsgCnt;
	
	//functions
	short RP1210ClientConnect();
	short RP1210ClientDisconnect();
	short RP1210SendCommand(short nCommandNumber);
	short RP1210SendCommand(short nCommandNumber, char * msg, int msgLen);
	short RP1210SendMessage(short nBlockOnSend);
	short RP1210ReadMessage(short nBlockOnRead);
	void ShowMsg(CString line);
	void ShowRP1210Error(short errorCode);
	void OnConnectionChange();

	// Generated message map functions
	virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedConnectbtn();
	CButton connectBtn;
	CStatic sendCountTxt;
	CButton sendChk;
	CButton blockSendChk;
	CEdit msgEdit;
	CStatic recvCountTxt;
	CButton blockRecvChk;
	CButton recvChk;
	CButton j1939RadioBtn;
	CButton j1708RadioBtn;
	CComboBox portCombo;
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCleartextbtn();
	afx_msg void OnBnClickedClearmidbtn();
	afx_msg void OnBnClickedSetgoodmidbtn();
	afx_msg void OnBnClickedSetothermidbtn();
	afx_msg void OnBnClickedDiscardbtn();
	afx_msg void OnBnClickedSetFilterPGN();
};

class TxJ1939Buffer {
public:
	TxJ1939Buffer (int pgn, bool how, int priority, int srcAddr, int dstAddr, const char *inData, int dataLen);
	TxJ1939Buffer (const TxJ1939Buffer & txb) : len(txb.len) { Copy(txb); }
	~TxJ1939Buffer () { Free(); }
	TxJ1939Buffer & operator = (const TxJ1939Buffer &txb) { if (&txb != this) { Free(); Copy(txb); }  return *this; }

	operator char *() { return data; }
	operator int () { return len; }
	operator short() { return len; }

private:
	int len;
	char *data;
	void Free() { delete[] data; data = NULL; len = 0; }
	void Copy (const TxJ1939Buffer &txb) { data = new char[txb.len]; memcpy(data, txb.data, len); }
};