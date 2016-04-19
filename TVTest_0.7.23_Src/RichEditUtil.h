#ifndef RICH_EDIT_UTIL_H
#define RICH_EDIT_UTIL_H


#include <richedit.h>


class CRichEditUtil
{
	HMODULE m_hLib;

	static bool SearchNextURL(LPCTSTR *ppszText,int *pLength);

public:
	CRichEditUtil();
	~CRichEditUtil();
	bool LoadRichEditLib();
	void UnloadRichEditLib();
	bool IsRichEditLibLoaded() const { return m_hLib!=NULL; }
	static bool LogFontToCharFormat(HDC hdc,const LOGFONT *plf,CHARFORMAT *pcf);
	static bool LogFontToCharFormat2(HDC hdc,const LOGFONT *plf,CHARFORMAT2 *pcf);
	static void CharFormatToCharFormat2(const CHARFORMAT *pcf,CHARFORMAT2 *pcf2);
	static bool AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT *pcf);
	static bool AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT2 *pcf);
	static bool CopyAllText(HWND hwndEdit);
	static void SelectAll(HWND hwndEdit);
	static bool IsSelected(HWND hwndEdit);
	static LPTSTR GetSelectedText(HWND hwndEdit);
	static int GetMaxLineWidth(HWND hwndEdit);
	static bool DetectURL(HWND hwndEdit,const CHARFORMAT *pcf,int FirstLine=0,int LastLine=-1);
	static bool HandleLinkClick(const ENLINK *penl);
};


#endif
