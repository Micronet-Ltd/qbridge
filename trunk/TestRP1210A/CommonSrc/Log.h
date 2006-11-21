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

#ifdef WINCE
	void SetEdit (CEdit *inRe) { re = inRe; }
#else
	void SetRichEdit(CRichEditCtrl *inRe) { re = inRe; re->SetBackgroundColor(false, 0xFFFFFF); }
#endif
	void LogText (const CString &text, COLORREF clr = Black);
	void LogDot();
private:

#ifdef WINCE
	CEdit *re;
#else
	CRichEditCtrl *re;
#endif
};

extern Log log;