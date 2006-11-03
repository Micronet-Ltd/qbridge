#pragma once

class Log
{
public:
	Log(void);
	~Log(void);

	static const COLORREF Red;
	static const COLORREF Black;
	static const COLORREF Green;
	static const COLORREF Blue;

	void SetRichEdit(CRichEditCtrl *inRe) { re = inRe; re->SetBackgroundColor(false, 0xFFFFFF); }
	void LogText (const CString &text, COLORREF clr = Black);
private:
	CRichEditCtrl *re;
};

extern Log log;