#pragma once

#include "..\TestRP1210_PC\TestRP1210_PC\TestRP1210_PC.h"
#include "INIMgr.h"

class TestRP1210;


class DlgTestWinNotify : public CDialog
{
	DECLARE_DYNAMIC(DlgTestWinNotify)

public:
	DlgTestWinNotify(TestRP1210 *inTest, INIMgr::Devices &inDev, CWnd* pParent = NULL);   // standard constructor
	virtual ~DlgTestWinNotify();

// Dialog Data
	enum { IDD = DLG_TESTWINNOTIFY };

protected:
	TestRP1210 *test;
	INIMgr::Devices &dev;
	UINT notifyMsg;
	UINT errMsg;

	set<int> sent;
	int recvCount;

	void FlushMessages();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void PerformTest();

	DECLARE_MESSAGE_MAP()
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
public:
	virtual BOOL OnInitDialog();
};
