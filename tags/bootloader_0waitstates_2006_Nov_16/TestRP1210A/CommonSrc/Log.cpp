#include "StdAfx.h"
#include "Log.h"

const COLORREF Log::Red		= 0x0000FF;
const COLORREF Log::Black	= 0x000000;
const COLORREF Log::Green	= 0x008000;
const COLORREF Log::Blue	= 0xA00000;

Log log;

Log::Log(void) : re(NULL)
{
}

Log::~Log(void)
{
}

/*****************/
/* Log::LogText */
/***************/
void Log::LogText (const CString &text, COLORREF clr) {
	if (re == NULL) {
		return;
	}

	re->SetSel(-1, -1);
	
	CHARFORMAT cf;
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = 0;
	cf.crTextColor = clr;
	re->SetSelectionCharFormat(cf);

	if (re->GetLineCount() < 1) {
		re->ReplaceSel(text , false);
	} else {
		re->ReplaceSel(_T("\r\n") + text , false);
	}
	
	int visLines = 20;
	int count = re->GetLineCount();
	int cur = re->GetFirstVisibleLine();
	if (count < visLines) { 
		re->LineScroll(-cur, 0);
	} else {
		re->LineScroll(count-visLines-cur, 0);
	}
	re->UpdateWindow();
}

/****************/
/* Log::LogDot */
/**************/
void Log::LogDot() {
	if (re == NULL) {
		return;
	}

	re->SetSel(-1, -1);
	
	re->ReplaceSel(_T("."), false);
	
	int visLines = 20;
	int count = re->GetLineCount();
	int cur = re->GetFirstVisibleLine();
	if (count < visLines) { 
		re->LineScroll(-cur, 0);
	} else {
		re->LineScroll(count-visLines-cur, 0);
	}
	re->UpdateWindow();
}
