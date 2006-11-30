// TestAppMFCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TestAppMFC.h"
#include "TestAppMFCDlg.h"
#include "..\..\qbrdge32dll_eVC\QBRDGE32.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#pragma warning(disable:4996)


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


// CTestAppMFCDlg dialog




CTestAppMFCDlg::CTestAppMFCDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTestAppMFCDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTestAppMFCDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_editbox);
	DDX_Control(pDX, DBGEB, m_debugeb);
}

BEGIN_MESSAGE_MAP(CTestAppMFCDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, &CTestAppMFCDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CTestAppMFCDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CTestAppMFCDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CTestAppMFCDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CTestAppMFCDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CTestAppMFCDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CTestAppMFCDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_CREATECON4_BTN, &CTestAppMFCDlg::OnBnClickedCreatecon4Btn)
	ON_BN_CLICKED(IDC_SEND_COM4BTN, &CTestAppMFCDlg::OnBnClickedSendCom4btn)
	ON_BN_CLICKED(IDC_READCOM4_BTN, &CTestAppMFCDlg::OnBnClickedReadcom4Btn)
	ON_BN_CLICKED(IDC_BUTTON8, &CTestAppMFCDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN, &CTestAppMFCDlg::OnBnClickedSendresetcmdBtn)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN2, &CTestAppMFCDlg::OnBnClickedSendresetcmdBtn2)
	ON_BN_CLICKED(IDC_SENDRESETCMD_BTN3, &CTestAppMFCDlg::OnBnClickedSendresetcmdBtn3)
	ON_BN_CLICKED(IDC_BUTTON9, &CTestAppMFCDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_sendj1939msgbtn, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn)
	ON_BN_CLICKED(IDC_SETJ1708FILTERBTN, &CTestAppMFCDlg::OnBnClickedSetj1708filterbtn)
	ON_BN_CLICKED(IDC_READCOM4_BTN2, &CTestAppMFCDlg::OnBnClickedReadcom4Btn2)
	ON_BN_CLICKED(IDC_BUTTON10, &CTestAppMFCDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON11, &CTestAppMFCDlg::OnBnClickedButton11)
	ON_BN_CLICKED(IDC_BUTTON12, &CTestAppMFCDlg::OnBnClickedButton12)
	ON_BN_CLICKED(IDC_BUTTON13, &CTestAppMFCDlg::OnBnClickedButton13)
	ON_BN_CLICKED(IDC_BUTTON14, &CTestAppMFCDlg::OnBnClickedButton14)
	ON_BN_CLICKED(IDC_sendj1939msgbtn2, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn2)
	ON_BN_CLICKED(IDC_sendj1939msgbtn3, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn3)
	ON_BN_CLICKED(IDC_sendj1939msgbtn4, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn4)
	ON_BN_CLICKED(IDC_sendj1939msgbtn5, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn5)
	ON_BN_CLICKED(IDC_sendj1939msgbtn6, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn6)
	ON_BN_CLICKED(IDC_READCOM4_BTN3, &CTestAppMFCDlg::OnBnClickedReadcom4Btn3)
	ON_BN_CLICKED(IDC_sendj1939msgbtn7, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn7)
	ON_BN_CLICKED(IDC_AddrClaimCom3, &CTestAppMFCDlg::OnBnClickedAddrclaimcom3)
	ON_BN_CLICKED(IDC_AddrClaimCom4, &CTestAppMFCDlg::OnBnClickedAddrclaimcom4)
	ON_BN_CLICKED(IDC_sendj1939msgbtn8, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn8)
	ON_BN_CLICKED(IDC_BUTTON16, &CTestAppMFCDlg::OnBnClickedButton16)
	ON_BN_CLICKED(IDC_BUTTON15, &CTestAppMFCDlg::OnBnClickedButton15)
	ON_BN_CLICKED(IDC_sendj1939msgbtn9, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn9)
	ON_BN_CLICKED(IDC_sendj1939msgbtn10, &CTestAppMFCDlg::OnBnClickedsendj1939msgbtn10)
	ON_BN_CLICKED(IDC_READCOM4_BTN4, &CTestAppMFCDlg::OnBnClickedReadcom4Btn4)
	ON_BN_CLICKED(IDC_AddrClaimCom5, &CTestAppMFCDlg::OnBnClickedAddrclaimcom5)
END_MESSAGE_MAP()


// CTestAppMFCDlg message handlers

HMODULE mod;
BOOL CTestAppMFCDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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
	mod = LoadLibrary(_T("QBRDGE32.dll"));
 
	return TRUE;  // return TRUE  unless you set the focus to a control
}


void CTestAppMFCDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTestAppMFCDlg::OnPaint()
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
HCURSOR CTestAppMFCDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

short lastCom3Client;
short lastCom4Client;

void CTestAppMFCDlg::OnBnClickedButton1()
{
	/*typedef void (WINAPI* TestMe) ();
	LPCSTR procName = "TestMe";
	TestMe tfunc = (TestMe) GetProcAddress(mod, procName);
	tfunc();
*/
	typedef void (WINAPI* fp_RP1210_GetStatusInfo) (TCHAR * buf, int size);
	fp_RP1210_GetStatusInfo cfunc = (fp_RP1210_GetStatusInfo) GetProcAddress(mod, "RP1210_GetStatusInfo");
	TCHAR buf[128];

	cfunc(buf, 128);
}

void CTestAppMFCDlg::OnBnClickedButton2()
{
	mod = LoadLibrary(_T("QBRDGE32.dll"));
}

void CTestAppMFCDlg::OnBnClickedButton3()
{
	FreeLibrary(mod);
}

//DISCONNECT ALL CLIENTS
void CTestAppMFCDlg::OnBnClickedButton5()
{
	typedef short (WINAPI* fp_RP1210_ClientDisconnect) (
		short nClientID
		);
	fp_RP1210_ClientDisconnect cfunc = (fp_RP1210_ClientDisconnect) GetProcAddress(mod, "RP1210_ClientDisconnect");
	
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

	fp_RP1210_ClientConnect cfunc = (fp_RP1210_ClientConnect) GetProcAddress(mod, "RP1210_ClientConnect");
		
	HWND hwndClient = ::GetForegroundWindow();
	long lTxBufferSize = 8000;
	long lRcvBufferSize = 8000;
	short nIsAppPacketizingIncomingMsgs = 0;
	
	comClient = cfunc(hwndClient, nDeviceID, fpchProtocol, 0, 0, 0);
}

void CTestAppMFCDlg::rp1210GetHardwareStatus(short nClientID) {
	typedef short (WINAPI* fp_RP1210_GetHardwareStatus) (
		short nClientID,
		char far* fpchClientInfo,
		short nInfoSize,
		short nBlockOnRequest
		);
	fp_RP1210_GetHardwareStatus cfunc = (fp_RP1210_GetHardwareStatus) GetProcAddress(mod, "RP1210_GetHardwareStatus");

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");

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

void CTestAppMFCDlg::rp1210SendCustomCommand(short nCommandNumber, short nClientID, char far* fpchMsg, short msgSize) {

	typedef short (WINAPI* fp_RP1210_SendCommand) (
		short nCommandNumber,
		short nClientID,
		char far* fpchClientCommand,
		short nMessageSize
		);
	fp_RP1210_SendCommand cfunc = (fp_RP1210_SendCommand) GetProcAddress(mod, "RP1210_SendCommand");

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");

	short nRet;
	nRet = cfunc(nCommandNumber, nClientID, fpchMsg, msgSize);
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

void CTestAppMFCDlg::rp1210SendCommand(short nCommandNumber, short nClientID) {
	typedef short (WINAPI* fp_RP1210_SendCommand) (
		short nCommandNumber,
		short nClientID,
		char far* fpchClientCommand,
		short nMessageSize
		);
	fp_RP1210_SendCommand cfunc = (fp_RP1210_SendCommand) GetProcAddress(mod, "RP1210_SendCommand");

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");

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
	
	if (nCommandNumber == 7) {
		fpchMessage[0] = 114;
		msgLen = 1;
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
void CTestAppMFCDlg::rp1210SendCustomMsg(short comClient, char far* fpchMsg, short msgSize, 
										 short nNotifyStatusOnTx, short nBlockOnSend) {
	typedef short (WINAPI* fp_RP1210_SendMessage) (
		short nClientID,
		char far* fpchClientMessage,
		short nMessageSize,
		short nNotifyStatusOnTx,
		short nBlockOnSend
		);
	fp_RP1210_SendMessage cfunc = (fp_RP1210_SendMessage) GetProcAddress(mod, "RP1210_SendMessage");

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);
	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");

	short nRet;
	nRet = cfunc(comClient, fpchMsg, msgSize, nNotifyStatusOnTx, nBlockOnSend);
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
		CString c("Msg Sent");
		m_editbox.SetWindowTextW(c);
	}
}

void CTestAppMFCDlg::rp1210SendMessage(short comClient, short nNotifyStatusOnTx, short nBlockOnSend) {		
	char far fpchMessage[128];
	//char far* fpchMessage = "1212233223233232144324322341234123443214324321";
	fpchMessage[0] = 1;
	//fpchMessage[0] = 0x53; //invalid priority
	fpchMessage[1] = 'T'; //mid code 0
	fpchMessage[2] = 'A'; 
	fpchMessage[3] = 'B';
	fpchMessage[4] = 'C';
	fpchMessage[5] = 'D';
	fpchMessage[6] = 'E';
	fpchMessage[7] = 'F';
	fpchMessage[8] = 'G';
	fpchMessage[20] = 'Z';
	rp1210SendCustomMsg(comClient, fpchMessage, 21, nNotifyStatusOnTx, nBlockOnSend);
	//rp1210SendCustomMsg(comClient, fpchMessage, 0, nNotifyStatusOnTx, nBlockOnSend);
}

void CTestAppMFCDlg::rp1210ReadMessage(short comClient, short nBlockOnRead, bool isJ1939) {	
	// TODO: Add your control notification handler code here
	typedef short (WINAPI* fp_RP1210_ReadMessage) (
		short nClientID,
		char far* fpchAPIMessage,
		short nBufferSize,
		short nBlockOnRead
		);
	fp_RP1210_ReadMessage cfunc = (fp_RP1210_ReadMessage) GetProcAddress(mod, "RP1210_ReadMessage");

	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");


	char far fpchMessage[2200];

	short nRet = (cfunc(comClient, fpchMessage, 2200, nBlockOnRead));
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
		CString c(fpchMessage);
		m_editbox.SetWindowTextW(c);
		
		if (isJ1939) {
			CString dbg;
			char buffer[65];
			dbg.Append(CString("RECV: \r\n"));
			dbg.Append(CString("TIMESTAMP: "));	
			dbg.Append(CString(itoa((UINT8)fpchMessage[0],buffer,10)));
			dbg.Append(CString(","));
			dbg.Append(CString(itoa((UINT8)fpchMessage[1],buffer,10)));
			dbg.Append(CString(","));
			dbg.Append(CString(itoa((UINT8)fpchMessage[2],buffer,10)));
			dbg.Append(CString(","));
			dbg.Append(CString(itoa((UINT8)fpchMessage[3],buffer,10)));
			dbg.Append(CString("\r\n"));
			dbg.Append(CString("PGN: "));	
			dbg.Append(CString(itoa((UINT8)fpchMessage[4],buffer,10)));
			dbg.Append(CString(","));
			dbg.Append(CString(itoa((UINT8)fpchMessage[5],buffer,10)));
			dbg.Append(CString(","));
			dbg.Append(CString(itoa((UINT8)fpchMessage[6],buffer,10)));
			dbg.Append(CString("\r\n"));
			dbg.Append(CString("How Priority: "));
			dbg.Append(CString(itoa((UINT8)fpchMessage[7],buffer,10)));
			dbg.Append(CString("\r\n"));
			dbg.Append(CString("Source Address: "));
			dbg.Append(CString(itoa((UINT8)fpchMessage[8],buffer,10)));
			dbg.Append(CString("\r\n"));
			dbg.Append(CString("Dest Address: "));
			dbg.Append(CString(itoa((UINT8)fpchMessage[9],buffer,10)));
			dbg.Append(CString("\r\n"));
			dbg.Append(CString("Message Data: "));
			for (int i = 10; i < nRet; i++) {
				dbg.Append(CString(itoa((UINT8)fpchMessage[i],buffer,10)));
				dbg.Append(CString(","));
			}
			dbg.Append(CString("\r\n"));
			m_debugeb.SetWindowTextW(dbg);
		}
	}
	else if (nRet == 0) {
		CString c("Return 0, no messages for client");
		m_editbox.SetWindowTextW(c);
	}
}

void CTestAppMFCDlg::OnBnClickedButton4()
{
	char far* fpchProtocol = "J1708";
	rp1210ClientConnect(3, fpchProtocol, lastCom3Client);
	rp1210SendCommand(3, lastCom3Client); //set all filter states to pass	
}
void CTestAppMFCDlg::OnBnClickedCreatecon4Btn()
{
	char far* fpchProtocol = "J1708";
	rp1210ClientConnect(4, fpchProtocol, lastCom4Client);
	rp1210SendCommand(3, lastCom4Client); //set all filter states to pass
}

void CTestAppMFCDlg::OnBnClickedSendCom4btn()
{
	//for (int i = 0; i < 400; i++) {
	//	rp1210SendMessage(lastCom4Client, 1, 0);
	//}
	rp1210SendMessage(lastCom4Client, 0, 1);
	return;
	typedef short (WINAPI* fp_RP1210_ClientDisconnect) (
		short nClientID
		);
	fp_RP1210_ClientDisconnect cfunc = (fp_RP1210_ClientDisconnect) GetProcAddress(mod, "RP1210_ClientDisconnect");
	
	for (short i = 0; i < 128; i++) {
		short errcode = cfunc(i);
	}
}
void CTestAppMFCDlg::OnBnClickedButton6()
{
	rp1210SendMessage(lastCom3Client, 1, 0);
	//rp1210SendMessage(lastCom3Client, 0, 1);
	//rp1210SendMessage(lastCom3Client, 0, 1);
	//rp1210SendMessage(lastCom3Client, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedReadcom4Btn()
{
	rp1210ReadMessage(lastCom4Client, 0, false);
}

void CTestAppMFCDlg::OnBnClickedButton7()
{
	rp1210ReadMessage(lastCom3Client, 0, false);
}

//DISCONNECT THREAD
static DWORD __stdcall DisconnectFunc(void * args) {
	::Sleep(1000);
	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");

	typedef short (WINAPI* fp_RP1210_ClientDisconnect) (
		short nClientID
		);
	fp_RP1210_ClientDisconnect cfunc = (fp_RP1210_ClientDisconnect) GetProcAddress(mod, "RP1210_ClientDisconnect");
	
	for (short i = 0; i < 128; i++) {
		short nRet = cfunc(i);
		if (nRet < 0) {
			char pchBuf[256];
			char far fpchDescription[80];

			if (!efunc(-nRet, fpchDescription))
				sprintf(pchBuf, "RdErr#: %d. %s", nRet, fpchDescription);
			else
				sprintf(pchBuf, "RdErr#: %d. No description available.", nRet);	

			CString c(pchBuf);			
		}
	}	


	return 0;
}

void CTestAppMFCDlg::rp1210Disconnect(short nClient) {
	typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
		short ErrorCode,
		char far* fpchDescription
		);	
	fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");

	typedef short (WINAPI* fp_RP1210_ClientDisconnect) (
		short nClientID
		);
	fp_RP1210_ClientDisconnect cfunc = (fp_RP1210_ClientDisconnect) GetProcAddress(mod, "RP1210_ClientDisconnect");
	
		short nRet = cfunc(nClient);
		if (nRet < 0) {
			char pchBuf[256];
			char far fpchDescription[80];

			if (!efunc(-nRet, fpchDescription))
				sprintf(pchBuf, "RdErr#: %d. %s", nRet, fpchDescription);
			else
				sprintf(pchBuf, "RdErr#: %d. No description available.", nRet);	

			CString c(pchBuf);			
		}
}

Thread dThread;

//READ COM3 BLOCK
void CTestAppMFCDlg::OnBnClickedButton8()
{	
	void * arg = NULL;
	dThread.SetupThread(DisconnectFunc, arg);
	dThread.Resume();

	rp1210ReadMessage(lastCom3Client, 1, false);
}

void CTestAppMFCDlg::OnBnClickedSendresetcmdBtn()
{
	short cid;
	rp1210ClientConnect(3, "J1708", cid);
	rp1210SendCommand(0, cid);
}

void CTestAppMFCDlg::OnBnClickedSendresetcmdBtn2()
{
	//upgrad firmware cmd.
	rp1210SendCommand(256, lastCom3Client);
}

void CTestAppMFCDlg::OnBnClickedSendresetcmdBtn3()
{
	rp1210GetHardwareStatus(lastCom3Client);
}

void CTestAppMFCDlg::OnBnClickedButton9()
{
	char far* fpchProtocol = "J1939";
	rp1210ClientConnect(3, fpchProtocol, lastCom3Client);
	rp1210SendCommand(3, lastCom3Client); //set all filter states to pass
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn()
{
	char far* msg = "\x03" "\xF0" "\x00" "\x03" "\x06" "\x00" "\xFF" "\xFE" "\x26"
		"\x01" "\xFF" "\xFF" "\xFF" "\xFF";
	short msgLen = 14;
	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedSetj1708filterbtn()
{
	rp1210SendCommand(7, lastCom3Client);
	rp1210SendCustomCommand(7, lastCom3Client, "abcd", 4);
}

void CTestAppMFCDlg::OnBnClickedReadcom4Btn2()
{
	rp1210ReadMessage(0, 0, false);
}


void CTestAppMFCDlg::OnBnClickedButton10()
{
	short cid;
	rp1210ClientConnect(3, "J1708", cid);
	rp1210SendCustomMsg(cid, "falfdkjslajijel", 15, 0, 1);
	rp1210Disconnect(cid);
	rp1210ClientConnect(3, "J1708", cid);
	rp1210SendCustomMsg(cid, "", 0, 0, 1);
	rp1210Disconnect(cid);
}

void CTestAppMFCDlg::OnBnClickedButton11()
{	
        short cl1 = 0;
		rp1210ClientConnect(3, "J1708", cl1);
        short cl2 = 0;
		rp1210ClientConnect(3, "J1708", cl2);

        char multiSendFilter = 114;
		rp1210SendCommand(7, cl1);
		//rp1210SendCommand(3, cl1);

        char msg[] = { 4, 114, 2 };
		rp1210SendCustomMsg(cl2, msg, sizeof(msg), false, true);

		rp1210ReadMessage(cl1, true, false);
        return;
}

void CTestAppMFCDlg::OnBnClickedButton12()
{	
        CString testName("Zero Length message");
        char *data = NULL;
        int len = 0;

        short errClient;
		rp1210ClientConnect(3, "J1708", errClient);

		rp1210SendCustomMsg(errClient, data, 0, false, true);

		rp1210Disconnect(errClient);
}

/****************************/
/* CustomResetSecondThread */
/**************************/
UINT __cdecl CustomResetSecondThread ( LPVOID pParam ) {
        while (true) {
                DWORD time = GetTickCount();
                char basicJ1708TxBuf [] = { 4, 112, 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', ' ', '-', '-', '-', '-' };

				typedef short (WINAPI* fp_RP1210_SendMessage) (
					short nClientID,
					char far* fpchClientMessage,
					short nMessageSize,
					short nNotifyStatusOnTx,
					short nBlockOnSend
					);
				fp_RP1210_SendMessage cfunc = (fp_RP1210_SendMessage) GetProcAddress(mod, "RP1210_SendMessage");

				typedef short (WINAPI* fp_RP1210_GetErrorMsg) (
					short ErrorCode,
					char far* fpchDescription
					);
				
				fp_RP1210_GetErrorMsg efunc = (fp_RP1210_GetErrorMsg) GetProcAddress(mod, "RP1210_GetErrorMsg");
				
				rp1210ClientConnect(4, "J1708", lastCom4Client);
				
				short nRet;
				nRet = cfunc(lastCom4Client, basicJ1708TxBuf, sizeof(basicJ1708TxBuf), false, true);
				if (nRet > 127) {
					char pchBuf[256];
					char fpchDescription[80];

					if (!efunc(nRet, fpchDescription))
						sprintf(pchBuf, "Error #: %d. %s", nRet, fpchDescription);
					else
						sprintf(pchBuf, "Error #: %d. No description available.", nRet);	

					CString c(pchBuf);
					break;
					
					//m_editbox.SetWindowTextW(c);
				}
				else
				{
					CString c("Msg Sent");
					//m_editbox.SetWindowTextW(c);
				}
  
			while (GetTickCount() < time+500) { Sleep(10); }
        }
        return 0;
}


void CTestAppMFCDlg::OnBnClickedButton13()
{
	rp1210ClientConnect(3, "J1708", lastCom3Client);
	AfxBeginThread(CustomResetSecondThread, NULL, 0, 0);
	::Sleep(500);

	rp1210SendCommand(0, lastCom3Client);

	rp1210ClientConnect(3, "J1708", lastCom3Client);
	rp1210Disconnect(lastCom3Client);
}

void CTestAppMFCDlg::OnBnClickedButton14()
{
	char far* fpchProtocol = "J1939";
	rp1210ClientConnect(4, fpchProtocol, lastCom4Client);
	rp1210SendCommand(3, lastCom4Client); //set all filter states to pass
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn2()
{
	char far* msg = "\x03" "\xF0" "\x00" "\x83" "\x06" "\xFF" "\xFF" "\xFE" "\x26"
		"\x01" "A" "Aafdsdafds" "Adfdsf" "Afsd" "ABDADVB";
	short msgLen = 24;
	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn3()
{
	char far msg[1791];
	short msgLen = 1791;
	for (int i = 0; i < 1791; i++) {
		msg[i] = 'A';
	}
	msg[0] = 0x03;
	msg[1] = (byte)0xF0;
	msg[2] = 0x00;
	msg[3] = (byte)0x83;
	msg[4] = 0x06;
	msg[5] = (byte)0xFF;
	msg[6] = 'Z';
	msg[1789] = 'Y';
	msg[1790] = 'Z';
	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn4()
{
	// TODO: Add your control notification handler code here
	char far msg[1792];
	short msgLen = 1792;
	for (int i = 0; i < 1791; i++) {
		msg[i] = 'A';
	}
	msg[0] = 0x03;
	msg[1] = (byte)0xF0;
	msg[2] = 0x00;
	msg[3] = (byte)0x83;
	msg[4] = 0x06;
	msg[5] = (byte)0xFF;
	msg[6] = 'Z';
	msg[1790] = 'Y';
	msg[1791] = 'Z';
	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn5()
{
	char far msg[1790];
	short msgLen = 1790;
	for (int i = 0; i < 1790; i++) {
		msg[i] = 'A';
	}
	msg[0] = 0x03;
	msg[1] = (byte)0xF0;
	msg[2] = 0x00;
	msg[3] = (byte)0x83;
	msg[4] = 0x06;
	msg[5] = (byte)0xFF;
	msg[6] = 'Z';
	msg[1788] = 'Y';
	msg[1789] = 'Z';
	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 1, 0);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn6()
{
	char far* msg = "\x03" "\xF0" "\x00" "\x03" "\x06" "\x08" "\xFF" "\xFE" "\x26"
		"\x01" "A" "Aafdsdafds" "Adfdsf" "Afsd" "ABDADVB";
	short msgLen = 24;
	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 0, 1);

	CString dbg;
	char buffer[65];
	dbg.Append(CString("SENT: \r\n"));
	dbg.Append(CString("PGN: "));	
	dbg.Append(CString(itoa((UINT8)msg[0],buffer,10)));
	dbg.Append(CString(","));
	dbg.Append(CString(itoa((UINT8)msg[1],buffer,10)));
	dbg.Append(CString(","));
	dbg.Append(CString(itoa((UINT8)msg[2],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("How Priority: "));
	dbg.Append(CString(itoa((UINT8)msg[3],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Source Address: "));
	dbg.Append(CString(itoa((UINT8)msg[4],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Dest Address: "));
	dbg.Append(CString(itoa((UINT8)msg[5],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Message Data: "));
	for (int i = 6; i < msgLen; i++) {
		dbg.Append(CString(itoa((UINT8)msg[i],buffer,10)));
		dbg.Append(CString(","));
	}
	dbg.Append(CString("\r\n"));
	m_debugeb.SetWindowTextW(dbg);
}

void CTestAppMFCDlg::OnBnClickedReadcom4Btn3()
{
	rp1210ReadMessage(lastCom4Client, 0, true);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn7()
{
	char far msg[1791];
	short msgLen = 1791;
	for (int i = 0; i < 1791; i++) {
		msg[i] = 'A';
	}
	msg[0] = 0x03;
	msg[1] = (byte)0xF0;
	msg[2] = 0x00;
	msg[3] = (byte)0x03;
	msg[4] = 0x06;
	msg[5] = (byte)0x08;
	msg[6] = 'Z';
	msg[1789] = 'Y';
	msg[1790] = 'Z';

	rp1210SendCustomMsg(lastCom3Client, msg, msgLen, 0, 1);

	CString dbg;
	char buffer[65];
	dbg.Append(CString("SENT: \r\n"));
	dbg.Append(CString("PGN: "));	
	dbg.Append(CString(itoa((UINT8)msg[0],buffer,10)));
	dbg.Append(CString(","));
	dbg.Append(CString(itoa((UINT8)msg[1],buffer,10)));
	dbg.Append(CString(","));
	dbg.Append(CString(itoa((UINT8)msg[2],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("How Priority: "));
	dbg.Append(CString(itoa((UINT8)msg[3],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Source Address: "));
	dbg.Append(CString(itoa((UINT8)msg[4],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Dest Address: "));
	dbg.Append(CString(itoa((UINT8)msg[5],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Message Data: "));
	for (int i = 6; i < msgLen; i++) {
		dbg.Append(CString(itoa((UINT8)msg[i],buffer,10)));
		dbg.Append(CString(","));
	}
	dbg.Append(CString("\r\n"));
	m_debugeb.SetWindowTextW(dbg);
}


void CTestAppMFCDlg::OnBnClickedAddrclaimcom3()
{
	char far msg[10];
	short msgLen = 10;
	for (int i = 0; i < msgLen; i++)
	{
		msg[i] = (char)0xFF;
	}
	msg[0] = 6;
	msg[9] = 0; //block until done
	rp1210SendCustomCommand(19, lastCom3Client, msg, msgLen);
}

void CTestAppMFCDlg::OnBnClickedAddrclaimcom4()
{
	char far msg[10];
	short msgLen = 10;
	for (int i = 0; i < msgLen; i++)
	{
		msg[i] = 0x01;
	}
	msg[0] = 6;
	msg[9] = 0; //block until done
	rp1210SendCustomCommand(19, lastCom4Client, msg, msgLen);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn8()
{
	char far* msg = "\x03" "\xF0" "\x00" "\x03" "\x08" "\x06" "\xFF" "\xFE" "\x26"
		"\x01" "A" "Aafdsdafds" "Adfdsf" "Afsd" "ABDADVB";
	short msgLen = 24;
	rp1210SendCustomMsg(lastCom4Client, msg, msgLen, 0, 1);

	CString dbg;
	char buffer[65];
	dbg.Append(CString("SENT: \r\n"));
	dbg.Append(CString("PGN: "));	
	dbg.Append(CString(itoa((UINT8)msg[0],buffer,10)));
	dbg.Append(CString(","));
	dbg.Append(CString(itoa((UINT8)msg[1],buffer,10)));
	dbg.Append(CString(","));
	dbg.Append(CString(itoa((UINT8)msg[2],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("How Priority: "));
	dbg.Append(CString(itoa((UINT8)msg[3],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Source Address: "));
	dbg.Append(CString(itoa((UINT8)msg[4],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Dest Address: "));
	dbg.Append(CString(itoa((UINT8)msg[5],buffer,10)));
	dbg.Append(CString("\r\n"));
	dbg.Append(CString("Message Data: "));
	for (int i = 6; i < msgLen; i++) {
		dbg.Append(CString(itoa((UINT8)msg[i],buffer,10)));
		dbg.Append(CString(","));
	}
	dbg.Append(CString("\r\n"));
	m_debugeb.SetWindowTextW(dbg);
}

void CTestAppMFCDlg::OnBnClickedButton16()
{
	rp1210SendCustomCommand(17, lastCom4Client, "a", 1);
}

void CTestAppMFCDlg::OnBnClickedButton15()
{
	char far msg[7];
	short msglen = 7;
	msg[0] = 1;
	msg[1] = 0xFF;
	msg[2] = 0xFF;
	msg[3] = 0x01;
	rp1210SendCustomCommand(4, lastCom3Client, msg, msglen);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn9()
{
	char far* msg = "\x03" "\xF0" "\x00" "\x03" "\x06" "\x00" "\xFF" "\xFE" "\x26"
		"\x01" "\xFF" "\xFF" "\xFF" "\xFF";
	short msgLen = 14;
	rp1210SendCustomMsg(lastCom4Client, msg, msgLen, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedsendj1939msgbtn10()
{
	char far* msg = "\xFF" "\xFF" "\x01" "\x03" "\x06" "\x00" "\xFF" "\xFE" "\x26"
		"\x01" "\xFF" "\xFF" "\xFF" "\xFF";
	short msgLen = 14;
	rp1210SendCustomMsg(lastCom4Client, msg, msgLen, 0, 1);
}

void CTestAppMFCDlg::OnBnClickedReadcom4Btn4()
{
	rp1210ReadMessage(lastCom3Client, 0, true);
}

void CTestAppMFCDlg::OnBnClickedAddrclaimcom5()
{
	char far msg[10];
	short msgLen = 10;
	for (int i = 0; i < msgLen; i++)
	{
		msg[i] = 0x01;
	}
	msg[0] = 8;
	msg[9] = 0; //block until done
	rp1210SendCustomCommand(19, lastCom4Client, msg, msgLen);
}
