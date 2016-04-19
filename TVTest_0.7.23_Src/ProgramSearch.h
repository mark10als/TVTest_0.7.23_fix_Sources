#ifndef PROGRAM_SEARCH_H
#define PROGRAM_SEARCH_H


#include <vector>
#include "Dialog.h"
#include "EpgProgramList.h"
#include "RichEditUtil.h"


class CProgramSearch : public CResizableDialog
{
public:
	enum {
		MAX_KEYWORD_LENGTH=256,
		MAX_KEYWORD_HISTORY=50,
		NUM_COLUMNS=3
	};

	class CEventHandler {
	protected:
		class CProgramSearch *m_pSearch;

		bool AddSearchResult(const CEventInfoData *pEventInfo,LPCTSTR pszChannelName);
		bool CompareKeyword(LPCTSTR pszString,LPCTSTR pKeyword,int KeywordLength) const;
		bool MatchKeyword(const CEventInfoData *pEventInfo,LPCTSTR pszKeyword) const;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnClose() { return true; }
		virtual bool Search(LPCTSTR pszKeyword)=0;
		friend class CProgramSearch;
	};
	friend class CEventHandler;

	CProgramSearch();
	~CProgramSearch();
	bool Create(HWND hwndOwner);
	bool SetEventHandler(CEventHandler *pHandler);
	bool SetKeywordHistory(const LPTSTR *pKeywordList,int NumKeywords);
	int GetKeywordHistoryCount() const;
	LPCTSTR GetKeywordHistory(int Index) const;
	int GetColumnWidth(int Index) const;
	bool SetColumnWidth(int Index,int Width);
	bool Search(LPTSTR pszKeyword);

private:
	CEventHandler *m_pEventHandler;
	WNDPROC m_pOldEditProc;
	TCHAR m_szKeyword[MAX_KEYWORD_LENGTH];
	int m_MaxHistory;
	std::vector<CDynamicString> m_History;
	int m_SortColumn;
	bool m_fSortDescending;
	int m_ColumnWidth[NUM_COLUMNS];
	CRichEditUtil m_RichEditUtil;
	CHARFORMAT m_InfoTextFormat;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool AddSearchResult(const CEventInfoData *pEventInfo,LPCTSTR pszChannelName);
	void ClearSearchResult();
	void SortSearchResult();
	int FormatEventTimeText(const CEventInfoData *pEventInfo,LPTSTR pszText,int MaxLength) const;
	int FormatEventInfoText(const CEventInfoData *pEventInfo,LPTSTR pszText,int MaxLength) const;
	void HighlightKeyword();
	bool SearchNextKeyword(LPCTSTR *ppszText,LPCTSTR pKeyword,int KeywordLength,int *pLength) const;
	static const LPCTSTR m_pszPropName;
	static LRESULT CALLBACK EditProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static int CALLBACK ResultCompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
};


#endif
