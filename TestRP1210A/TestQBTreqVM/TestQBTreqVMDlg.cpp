// TestQBTreqVMDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestQBTreqVM.h"
#include "TestQBTreqVMDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTestQBTreqVMDlg dialog

CTestQBTreqVMDlg::CTestQBTreqVMDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestQBTreqVMDlg::IDD, pParent),
	protocol(_J1708),
	readMsgCnt(0),
	sendMsgCnt(0),
	isConnected(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestQBTreqVMDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CONNECTBTN, connectBtn);
	DDX_Control(pDX, IDC_SENDCOUNT, sendCountTxt);
	DDX_Control(pDX, IDC_SENDCHK, sendChk);
	DDX_Control(pDX, IDC_SENDBLOCKCHK, blockSendChk);
	DDX_Control(pDX, IDC_MSGEDIT, msgEdit);
	DDX_Control(pDX, IDC_RECVCOUNT, recvCountTxt);
	DDX_Control(pDX, IDC_BLOCKRECVCHK, blockRecvChk);
	DDX_Control(pDX, IDC_RECVCHK, recvChk);
	DDX_Control(pDX, IDC_J1708RADIO, j1708RadioBtn);
	DDX_Control(pDX, IDC_J1939RADIO, j1939RadioBtn);
	DDX_Control(pDX, IDC_PORTCOMBO, portCombo);
}

BEGIN_MESSAGE_MAP(CTestQBTreqVMDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CONNECTBTN, &CTestQBTreqVMDlg::OnBnClickedConnectbtn)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CLEARTEXTBTN, &CTestQBTreqVMDlg::OnBnClickedCleartextbtn)
	ON_BN_CLICKED(IDC_CLEARMIDBTN, &CTestQBTreqVMDlg::OnBnClickedClearmidbtn)
	ON_BN_CLICKED(IDC_SETGOODMIDBTN, &CTestQBTreqVMDlg::OnBnClickedSetgoodmidbtn)
	ON_BN_CLICKED(IDC_SETOTHERMIDBTN, &CTestQBTreqVMDlg::OnBnClickedSetothermidbtn)
	ON_BN_CLICKED(IDC_DISCARDBTN, &CTestQBTreqVMDlg::OnBnClickedDiscardbtn)
	ON_BN_CLICKED(IDC_DISCARDBTN2, &CTestQBTreqVMDlg::OnBnClickedSetFilterPGN)
END_MESSAGE_MAP()

/***********************************/
/* CTestQBTreqVMDlg::OnInitDialog */
/*********************************/
BOOL CTestQBTreqVMDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon   
   
    CeSetThreadPriority(GetCurrentThread(), 249);

    //open registry and set RP1210 priority
    HKEY hKey;
    DWORD dwDisp = 0;
    LPDWORD lpdwDisp = &dwDisp;
    CString strKey = _T("SOFTWARE\\RP1210");
    CString strDword = _T("Priority");
    DWORD dwVal = 251;
    dwVal = 249;

    /*LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, strKey, 0, KEY_READ, &hKey);
    DWORD dVal;
    if (lRes == ERROR_SUCCESS) {
        unsigned long type=REG_DWORD, size=1024;
        lRes == RegQueryValueEx(hKey, strDword, NULL, &type, (LPBYTE)&dVal, &size);
        if (lRes == ERROR_SUCCESS) {

        }
    }*/

    LONG iSuccess = RegCreateKeyEx(HKEY_LOCAL_MACHINE, strKey, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey,lpdwDisp);
    if(iSuccess == ERROR_SUCCESS)
    {
        RegSetValueEx (hKey, strDword, 0L, REG_DWORD,(CONST BYTE*) &dwVal, sizeof(DWORD));
    }

	qBModule = LoadLibrary(_T("QBRDGE32.dll"));
	if (protocol == _J1708) {
		j1708RadioBtn.SetCheck(BST_CHECKED);
	}
	else {
		j1939RadioBtn.SetCheck(BST_CHECKED);
	}
	portCombo.SetCurSel(2);
	SetTimer(_RECV_ID, 20, NULL);
	SetTimer(_SEND_ID, 2000, NULL); 
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CTestQBTreqVMDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	if (AfxIsDRAEnabled())
	{
		DRA::RelayoutDialog(
			AfxGetResourceHandle(), 
			this->m_hWnd, 
			DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_TESTQBTREQVM_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_TESTQBTREQVM_DIALOG));
	}
}
#endif

/********************************************/
/* CTestQBTreqVMDlg::OnBnClickedConnectbtn */
/******************************************/
void CTestQBTreqVMDlg::OnBnClickedConnectbtn()
{
	protocol = _J1708;
	if (j1939RadioBtn.GetCheck() == BST_CHECKED) {
		protocol = _J1939;
	}

	if (isConnected == false) {
		//connect pressed
		clientID = RP1210ClientConnect();		
		if (clientID >= 0 && clientID <= 127) {
			isConnected = true;

			CString txt;
			txt.Format(_T("Connected id,%d"), clientID);
			ShowMsg(txt);
		}
	}	
	else {
		//disconnect pressed
		RP1210ClientDisconnect();
		isConnected = false;		
		CString txt;
		txt.Format(_T("Disconnected id,%d"), clientID);
		ShowMsg(txt);
	}

	OnConnectionChange();
}

/*****************************************/
/* CTestQBTreqVMDlg::OnConnectionChange */
/***************************************/
void CTestQBTreqVMDlg::OnConnectionChange()
{
	if (isConnected == true) {
		connectBtn.SetWindowText(_T("Disconnect"));
		sendChk.EnableWindow(TRUE);
		blockSendChk.EnableWindow(TRUE);
		recvChk.EnableWindow(TRUE);
		blockRecvChk.EnableWindow(TRUE);

		j1939RadioBtn.EnableWindow(FALSE);
		j1708RadioBtn.EnableWindow(FALSE);
		
		RP1210SendCommand(SET_ALL_FILTER_STATES_TO_PASS);
		CString txt(_T("All filters set to pass id"));
		ShowMsg(txt);
	}
	else {
		connectBtn.SetWindowText(_T("Connect"));
		sendChk.EnableWindow(FALSE);
		sendChk.SetCheck(BST_UNCHECKED);
		blockSendChk.EnableWindow(FALSE);
		blockSendChk.SetCheck(BST_UNCHECKED);
		recvChk.EnableWindow(FALSE);
		recvChk.SetCheck(BST_UNCHECKED);
		blockRecvChk.EnableWindow(FALSE);
		blockRecvChk.SetCheck(BST_UNCHECKED);

		j1939RadioBtn.EnableWindow(TRUE);
		j1708RadioBtn.EnableWindow(TRUE);
	}	
}

/******************************************/
/* CTestQBTreqVMDlg::RP1210ClientConnect */
/****************************************/
short CTestQBTreqVMDlg::RP1210ClientConnect()
{
	typedef short (WINAPI* fp_RP1210_ClientConnect) (
		HWND hwndClient,
		short nDeviceID,
		char far* fpchProtocol,
		long lTxBufferSize,
		long lRcvBufferSize,
		short nIsAppPacketizingIncomingMsgs
		);

	fp_RP1210_ClientConnect cfunc = 
		(fp_RP1210_ClientConnect) GetProcAddress(qBModule, _T("RP1210_ClientConnect"));

	HWND hwndClient = 0;
	long lTxBufferSize = 8000;
	long lRcvBufferSize = 8000;
	short nIsAppPacketizingIncomingMsgs = 0;
	short nDeviceID = portCombo.GetCurSel() + 1;
	char far* fpchProtocol = "J1708";
	if (protocol == _J1939) {
		fpchProtocol = "J1939";
	}

	short retVal = cfunc(hwndClient, nDeviceID, fpchProtocol, lTxBufferSize, 
		lRcvBufferSize, nIsAppPacketizingIncomingMsgs);

	if (retVal > 127) {
		ShowRP1210Error(retVal);
	}
	return retVal;
}

/*********************************************/
/* CTestQBTreqVMDlg::RP1210ClientDisconnect */
/*******************************************/
short CTestQBTreqVMDlg::RP1210ClientDisconnect()
{
	typedef short (WINAPI* fp_RP1210_ClientDisconnect) (
		short nClientID
		);
	fp_RP1210_ClientDisconnect cfunc = 
		(fp_RP1210_ClientDisconnect) GetProcAddress(qBModule, _T("RP1210_ClientDisconnect"));

	short retVal = cfunc(clientID);

	if (retVal > 127) {
		ShowRP1210Error(retVal);
	}
	return retVal;
}

/****************************************/
/* CTestQBTreqVMDlg::RP1210SendCommand */
/**************************************/
short CTestQBTreqVMDlg::RP1210SendCommand(short nCommandNumber)
{
	char fpchMessage[1];
	int msgLen = 0;
	return RP1210SendCommand(nCommandNumber, fpchMessage, 0);
}

short CTestQBTreqVMDlg::RP1210SendCommand(short nCommandNumber, char * msg, int msgLen)
{
	typedef short (WINAPI* fp_RP1210_SendCommand) (
		short nCommandNumber,
		short nClientID,
		char far* fpchClientCommand,
		short nMessageSize
		);
	fp_RP1210_SendCommand cfunc = (fp_RP1210_SendCommand) GetProcAddress(qBModule, _T("RP1210_SendCommand"));

	//char far fpchMessage[128];
	//int msgLen = 0;
	short retVal = cfunc(nCommandNumber, clientID, msg, msgLen);

	if (retVal > 127) {
		ShowRP1210Error(retVal);
	}
	return retVal;
}

/****************************************/
/* CTestQBTreqVMDlg::RP1210SendMessage */
/**************************************/
short CTestQBTreqVMDlg::RP1210SendMessage(short nBlockOnSend)
{	
	typedef short (WINAPI* fp_RP1210_SendMessage) (
		short nClientID,
		char far* fpchClientMessage,
		short nMessageSize,
		short nNotifyStatusOnTx,
		short nBlockOnSend
		);
	fp_RP1210_SendMessage cfunc = (fp_RP1210_SendMessage) GetProcAddress(qBModule, _T("RP1210_SendMessage"));

	short retVal;
	if (protocol == _J1708) {
		char far *fpchMessage = "\x01" "123456789Y123456789Z";
		retVal = cfunc(clientID, fpchMessage, 21, 0, nBlockOnSend);
	}
	else if (protocol == _J1939) {
		TxJ1939Buffer txBuf(0x014444, false, 4, 100, 102, "1234567890", 8);
		retVal = cfunc(clientID, txBuf, txBuf, 0, nBlockOnSend);
	}

	if (retVal > 127) {
		ShowRP1210Error(retVal);
	}
	else
	{
		CString cStr;
		sendMsgCnt++;
		cStr.Format(_T("%d"), sendMsgCnt);
		sendCountTxt.SetWindowText(cStr);
	}
	return retVal;
}

/****************************************/
/* CTestQBTreqVMDlg::RP1210ReadMessage */
/**************************************/
short CTestQBTreqVMDlg::RP1210ReadMessage(short nBlockOnRead) 
{	
	typedef short (WINAPI* fp_RP1210_ReadMessage) (
		short nClientID,
		char far* fpchAPIMessage,
		short nBufferSize,
		short nBlockOnRead
		);
	fp_RP1210_ReadMessage cfunc = (fp_RP1210_ReadMessage) GetProcAddress(qBModule, _T("RP1210_ReadMessage"));

	char far fpchMessage[512];
	short nRet = cfunc(clientID, fpchMessage, 512, nBlockOnRead);

	if (nRet < 0) {
		ShowRP1210Error(-nRet);
	}
	else {
		if (nRet > 0) {
			char pchBuf[512];
			if (j1708RadioBtn.GetCheck() == BST_CHECKED) {
				sprintf(pchBuf, "Read msg: %d, Count-4: %d, %s", fpchMessage[4], nRet-4, fpchMessage+4);
			}
			else {
				sprintf(pchBuf, "Read msg: %d, %s", nRet-3, fpchMessage+2);
			}
			CString cStr(pchBuf);
			
			ShowMsg(cStr);

            readMsgCnt++;            
            cStr.Format(_T("%d"), readMsgCnt);
            recvCountTxt.SetWindowText(cStr);
		}

	}
	return nRet;
}

/******************************/
/* CTestQBTreqVMDlg::ShowMsg */
/****************************/
void CTestQBTreqVMDlg::ShowMsg(CString line)
{
	CString txt;
	msgEdit.GetWindowText(txt);
	if (txt.GetLength() > 500) {
		txt = txt.Left(200);
	}
	line.Append(_T("\r\n"));
	txt.Insert(0, line);
	msgEdit.SetWindowText(txt);
}

/**************************************/
/* CTestQBTreqVMDlg::ShowRP1210Error */
/************************************/
void CTestQBTreqVMDlg::ShowRP1210Error(short errorCode)
{
	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);	

	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(qBModule, _T("RP1210_GetErrorMsg"));

	char pchBuf[256];
	char far fpchDescription[80];
	if (!efunc(errorCode, fpchDescription))
		sprintf(pchBuf, "Err#: %d. %s", errorCode, fpchDescription);
	else
		sprintf(pchBuf, "Err#: %d. No description available.", errorCode);	

	CString txt(pchBuf);
	ShowMsg(txt);
}

/******************************/
/* CTestQBTreqVMDlg::OnTimer */
/****************************/
void CTestQBTreqVMDlg::OnTimer(UINT_PTR nIDEvent)
{
    CeSetThreadPriority(GetCurrentThread(), 249);
	if (nIDEvent == _RECV_ID) {
		//receive timer
		if (recvChk.GetCheck() == BST_CHECKED) 
		{
			if(blockRecvChk.GetCheck() == BST_CHECKED)
			{
				RP1210ReadMessage(true);
			}
			else
			{
				for (int i = 0; i < 50; i++) 
				{
					RP1210ReadMessage(false);
				}
			}
		}
	}
	else if (nIDEvent == _SEND_ID) {
		//send timer
		if (sendChk.GetCheck() == BST_CHECKED) {
			RP1210SendMessage(blockSendChk.GetCheck() == BST_CHECKED);
		}
	}

	CDialog::OnTimer(nIDEvent);
}

/**********************************************/
/* CTestQBTreqVMDlg::OnBnClickedCleartextbtn */
/********************************************/
void CTestQBTreqVMDlg::OnBnClickedCleartextbtn()
{
	msgEdit.SetWindowText(_T(""));
}

/*********************************/
/* TxJ1939Buffer::TxJ1939Buffer */
/*******************************/
TxJ1939Buffer::TxJ1939Buffer(int pgn, bool how, int priority, int srcAddr, int dstAddr, const char *inData, int dataLen) {
	len = 6+dataLen;
	data = new char[len];
	memcpy(data, &pgn, 3);
	data[3] = (how ? 0x80 : 0x00) | (priority & 0x07);
	data[4] = srcAddr;
	data[5] = dstAddr;
	memcpy(data+6, inData, dataLen);
}


/******************************************/
/* TxJ1939Buffer::OnBnClickedClearmidbtn */
/****************************************/
void CTestQBTreqVMDlg::OnBnClickedClearmidbtn()
{
	RP1210SendCommand(SET_ALL_FILTER_STATES_TO_PASS);
}

void CTestQBTreqVMDlg::OnBnClickedSetgoodmidbtn()
{
	char data[128];
	int dataLen = 2;
	data[0] = 'A';
	data[1] = 'B';
	RP1210SendCommand(SET_MSG_FILTER_J1708, data, dataLen);
}

void CTestQBTreqVMDlg::OnBnClickedSetothermidbtn()
{
	char data[128];
	int dataLen = 2;
	data[0] = 'Z';
	data[1] = 'Z';
	RP1210SendCommand(SET_MSG_FILTER_J1708, data, dataLen);
}

void CTestQBTreqVMDlg::OnBnClickedDiscardbtn()
{
	RP1210SendCommand(SET_ALL_FILTER_DISCARD);
}

void CTestQBTreqVMDlg::OnBnClickedSetFilterPGN()
{
    OnBnClickedDiscardbtn();

    // filter pgn
	char data[128];
	int dataLen = 1;
	data[0] = (char)0x01;
    data[1] = (char)0x00;
    data[2] = (char)0xFE;
    data[3] = (char)0xE9;
    data[4] = (char)0x00;
	RP1210SendCommand(SET_MSG_FILTER_J1939, data, dataLen);
}
