// TestAppMFCTreqDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestAppMFCTreq.h"
#include "TestAppMFCTreqDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTestAppMFCTreqDlg dialog

CTestAppMFCTreqDlg::CTestAppMFCTreqDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestAppMFCTreqDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestAppMFCTreqDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_editbox);
}

BEGIN_MESSAGE_MAP(CTestAppMFCTreqDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CTestAppMFCTreqDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_CREATECON4_BTN, &CTestAppMFCTreqDlg::OnBnClickedCreatecon4Btn)
	ON_BN_CLICKED(IDC_BUTTON4, &CTestAppMFCTreqDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON6, &CTestAppMFCTreqDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CTestAppMFCTreqDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CTestAppMFCTreqDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN, &CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN2, &CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn2)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN3, &CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn3)
	ON_BN_CLICKED(IDC_BUTTON2, &CTestAppMFCTreqDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CTestAppMFCTreqDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_SEND_COM4BTN, &CTestAppMFCTreqDlg::OnBnClickedSendCom4btn)
	ON_BN_CLICKED(IDC_READCOM4_BTN, &CTestAppMFCTreqDlg::OnBnClickedReadcom4Btn)
	ON_BN_CLICKED(IDC_BUTTON5, &CTestAppMFCTreqDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON9, &CTestAppMFCTreqDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON10, &CTestAppMFCTreqDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON11, &CTestAppMFCTreqDlg::OnBnClickedButton11)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN4, &CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn4)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN5, &CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn5)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN6, &CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn6)
END_MESSAGE_MAP()


// CTestAppMFCTreqDlg message handlers

HMODULE mod;
BOOL CTestAppMFCTreqDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	mod = LoadLibrary(_T("QBRDGE32.dll"));
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CTestAppMFCTreqDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/)
{
	DRA::RelayoutDialog(
		AfxGetInstanceHandle(), 
		this->m_hWnd, 
		DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_TESTAPPMFCTREQ_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_TESTAPPMFCTREQ_DIALOG));
}
#endif

short lastCom2Client;
short lastCom3Client;
short lastCom4Client;

void CTestAppMFCTreqDlg::OnBnClickedButton1()
{
	/*typedef void (WINAPI* TestMe) ();
	LPCSTR procName = "TestMe";
	TestMe tfunc = (TestMe) GetProcAddress(mod, procName);
	tfunc();
*/
	typedef void (WINAPI* fp_RP1210_GetStatusInfo) (TCHAR * buf, int size);
	fp_RP1210_GetStatusInfo cfunc = (fp_RP1210_GetStatusInfo) GetProcAddress(mod, _T("RP1210_GetStatusInfo"));
	TCHAR buf[128];

	cfunc(buf, 128);
}

void CTestAppMFCTreqDlg::OnBnClickedButton2()
{
	mod = LoadLibrary(_T("QBRDGE32.dll"));
}

void CTestAppMFCTreqDlg::OnBnClickedButton3()
{
	FreeLibrary(mod);
}

void CTestAppMFCTreqDlg::OnBnClickedButton5()
{
	typedef short (WINAPI* fp_RP1210_ClientDisconnect) (
		short nClientID
		);
	fp_RP1210_ClientDisconnect cfunc = (fp_RP1210_ClientDisconnect) GetProcAddress(mod, _T("RP1210_ClientDisconnect"));
	
	for (short i = 0; i < 128; i++) {
		short errcode = cfunc(i);
	}
}

void rp1210ClientConnect(short nDeviceID, char far* fpchProtocol, short &comClient)
{
	typedef short (WINAPI* fp_RP1210_ClientConnect) (
		HWND hwndClient,
		short nDeviceID,
		char far* fpchProtocol,
		long lTxBufferSize,
		long lRcvBufferSize,
		short nIsAppPacketizingIncomingMsgs
	);

	fp_RP1210_ClientConnect cfunc = (fp_RP1210_ClientConnect) GetProcAddress(mod, _T("RP1210_ClientConnect"));
		
	HWND hwndClient = 0;
	long lTxBufferSize = 8000;
	long lRcvBufferSize = 8000;
	short nIsAppPacketizingIncomingMsgs = 0;
	
	comClient = cfunc(hwndClient, nDeviceID, fpchProtocol, 0, 0, 0);
}

void CTestAppMFCTreqDlg::rp1210GetHardwareStatus(short nClientID) {
	typedef short (WINAPI* fp_RP1210_GetHardwareStatus) (
		short nClientID,
		char far* fpchClientInfo,
		short nInfoSize,
		short nBlockOnRequest
		);
	fp_RP1210_GetHardwareStatus cfunc = (fp_RP1210_GetHardwareStatus) GetProcAddress(mod, _T("RP1210_GetHardwareStatus"));

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, _T("RP1210_GetErrorMsg"));

	char far fpchClientInfo[16];
	int msgLen = 16;


	short nRet;
	nRet = cfunc(nClientID, fpchClientInfo, msgLen, 0);
	if (nRet > 127) {
		char pchBuf[256];
		char fpchDescription[80];

		if (!efunc(nRet, fpchDescription))
			sprintf(pchBuf, "Error #: %d. %s", nRet, fpchDescription);
		else
			sprintf(pchBuf, "Error #: %d. No description available.", nRet);	

		CString c(pchBuf);
		
		m_editbox.SetWindowTextW(c);
	}
	else
	{
 		CString c("Success GetHardwareInfo");
		m_editbox.SetWindowTextW(c);
	}
}

void CTestAppMFCTreqDlg::rp1210SendCommand(short nCommandNumber, short nClientID) {
	typedef short (WINAPI* fp_RP1210_SendCommand) (
		short nCommandNumber,
		short nClientID,
		char far* fpchClientCommand,
		short nMessageSize
		);
	fp_RP1210_SendCommand cfunc = (fp_RP1210_SendCommand) GetProcAddress(mod, _T("RP1210_SendCommand"));

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, _T("RP1210_GetErrorMsg"));

	char far fpchMessage[128];
	//char far* fpchMessage = "1212233223233232144324322341234123443214324321";
	fpchMessage[0] = 0x01;
	fpchMessage[1] = 0x41;
	fpchMessage[2] = 0x42;
	fpchMessage[3] = 0x43;
	fpchMessage[4] = 0x44;
	fpchMessage[20] = 0x5a;
	int msgLen = 0;

	if (nCommandNumber == 256) // upgrade fw
	{
		fpchMessage[0] = 'q';
		fpchMessage[1] = 'b';
		fpchMessage[2] = 'r';
		fpchMessage[3] = 'i';
		fpchMessage[4] = 'd';
		fpchMessage[5] = 'g';
		fpchMessage[6] = 'e';
		fpchMessage[7] = '.';
		fpchMessage[8] = 's';
		fpchMessage[9] = 'r';
		fpchMessage[10] = 'e';
		fpchMessage[11] = 'c';
		fpchMessage[12] = 0x00;
		msgLen = 12;
	}

	short nRet;
	nRet = cfunc(nCommandNumber, nClientID, fpchMessage, msgLen);
	if (nRet > 127) {
		char pchBuf[256];
		char fpchDescription[80];

		if (!efunc(nRet, fpchDescription))
			sprintf(pchBuf, "Error #: %d. %s", nRet, fpchDescription);
		else
			sprintf(pchBuf, "Error #: %d. No description available.", nRet);	

		CString c(pchBuf);
		
		m_editbox.SetWindowTextW(c);
	}
	else
	{
		CString c("Success SendCommand");
		m_editbox.SetWindowTextW(c);
	}
}
void CTestAppMFCTreqDlg::rp1210SendMessage(short comClient, short nBlockOnSend) {	
	// TODO: Add your control notification handler code here
	typedef short (WINAPI* fp_RP1210_SendMessage) (
		short nClientID,
		char far* fpchClientMessage,
		short nMessageSize,
		short nNotifyStatusOnTx,
		short nBlockOnSend
		);
	fp_RP1210_SendMessage cfunc = (fp_RP1210_SendMessage) GetProcAddress(mod, _T("RP1210_SendMessage"));

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, _T("RP1210_GetErrorMsg"));



	char far fpchMessage[128];
	//char far* fpchMessage = "1212233223233232144324322341234123443214324321";
	fpchMessage[0] = 0x01;
	fpchMessage[1] = 0x41;
	fpchMessage[2] = 0x42;
	fpchMessage[3] = 0x43;
	fpchMessage[4] = 0x44;
	fpchMessage[20] = 0x5a;

	short nRet;
	nRet = cfunc(comClient, fpchMessage, 21, 0, nBlockOnSend);
	if (nRet > 127) {
		char pchBuf[256];
		char fpchDescription[80];

		if (!efunc(nRet, fpchDescription))
			sprintf(pchBuf, "Error #: %d. %s", nRet, fpchDescription);
		else
			sprintf(pchBuf, "Error #: %d. No description available.", nRet);	

		CString c(pchBuf);
		
		m_editbox.SetWindowTextW(c);
	}
	else
	{

	}
}

void CTestAppMFCTreqDlg::rp1210ReadMessage(short comClient, short nBlockOnRead) {	
	// TODO: Add your control notification handler code here
	typedef short (WINAPI* fp_RP1210_ReadMessage) (
		short nClientID,
		char far* fpchAPIMessage,
		short nBufferSize,
		short nBlockOnRead
		);
	fp_RP1210_ReadMessage cfunc = (fp_RP1210_ReadMessage) GetProcAddress(mod, _T("RP1210_ReadMessage"));

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, _T("RP1210_GetErrorMsg"));


	char far fpchMessage[512];

	short nRet = (cfunc(comClient, fpchMessage, 512, nBlockOnRead));
	if (nRet < 0) {
		char pchBuf[256];
		char far fpchDescription[80];

		if (!efunc(-nRet, fpchDescription))
			sprintf(pchBuf, "RdErr#: %d. %s", nRet, fpchDescription);
		else
			sprintf(pchBuf, "RdErr#: %d. No description available.", nRet);	

		CString c(pchBuf);
		
		m_editbox.SetWindowTextW(c);
	}
	else if (nRet > 0) {
		CString c("");
		char pchBuf[512];
		sprintf(pchBuf, "read msg: %d, %s", nRet, fpchMessage);
		CString cstr(pchBuf);
		m_editbox.SetWindowTextW(cstr);
	}
	else if (nRet == 0) {
		CString c("Return 0, no messages for client");
		m_editbox.SetWindowTextW(c);
	}
}

void CTestAppMFCTreqDlg::OnBnClickedButton4()
{
	char far* fpchProtocol = "J1708";
	rp1210ClientConnect(3, fpchProtocol, lastCom3Client);
}
void CTestAppMFCTreqDlg::OnBnClickedCreatecon4Btn()
{
	char far* fpchProtocol = "J1708";
	rp1210ClientConnect(4, fpchProtocol, lastCom4Client);
	CString cstr("");	
	cstr.Format(_T("new client id: %d"), lastCom4Client);
	m_editbox.SetWindowTextW(cstr);	
}

void CTestAppMFCTreqDlg::OnBnClickedSendCom4btn()
{	
	rp1210SendCommand(3, lastCom4Client);
	//for (int i = 0; i < 6; i++) {
		rp1210SendMessage(lastCom4Client, 1);
	//}
}
void CTestAppMFCTreqDlg::OnBnClickedButton6()
{
	rp1210SendMessage(lastCom3Client, 1);
}

void CTestAppMFCTreqDlg::OnBnClickedReadcom4Btn()
{
	rp1210ReadMessage(lastCom4Client, 0);
}

void CTestAppMFCTreqDlg::OnBnClickedButton7()
{
	rp1210ReadMessage(lastCom3Client, 0);
}
void CTestAppMFCTreqDlg::OnBnClickedButton8()
{
	rp1210ReadMessage(lastCom3Client, 1);
}

void CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn()
{
	rp1210SendCommand(0, lastCom3Client);
}

void CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn2()
{
	//upgrad firmware cmd.
	rp1210SendCommand(256, lastCom3Client);
}

void CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn3()
{
	rp1210GetHardwareStatus(lastCom3Client);
}

void CTestAppMFCTreqDlg::OnBnClickedButton9()
{
	char far* fpchProtocol = "J1708";
	rp1210ClientConnect(2, fpchProtocol, lastCom2Client);
}

void CTestAppMFCTreqDlg::OnBnClickedButton10()
{
	rp1210SendMessage(lastCom2Client, 1);
}

void CTestAppMFCTreqDlg::OnBnClickedButton11()
{
	rp1210ReadMessage(lastCom2Client, 0);
}

void CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn4()
{
	rp1210SendCommand(0, lastCom2Client);
}

void CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn5()
{
	//upgrad firmware cmd.
	rp1210SendCommand(256, lastCom2Client);
}

void CTestAppMFCTreqDlg::OnBnClickedSendresetcmdBtn6()
{
	rp1210GetHardwareStatus(lastCom2Client);
}
