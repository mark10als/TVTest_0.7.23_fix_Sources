#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramSearch.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef LVN_GETEMPTYMARKUP
#include <pshpack1.h>
#define LVN_GETEMPTYMARKUP	(LVN_FIRST-87)
#define EMF_CENTERED		0x00000001
typedef struct tagNMLVEMPTYMARKUP {
	NMHDR hdr;
	DWORD dwFlags;
	WCHAR szMarkup[L_MAX_URL_LENGTH];
} NMLVEMPTYMARKUP;
#include <poppack.h>
#endif




class CSearchEventInfo : public CEventInfoData
{
public:
	LPTSTR m_pszChannelName;
	CSearchEventInfo(const CEventInfoData &EventInfo,LPCTSTR pszChannelName)
		: CEventInfoData(EventInfo)
		, m_pszChannelName(DuplicateString(pszChannelName)) {
	}
	~CSearchEventInfo() {
		delete [] m_pszChannelName;
	}
};




const LPCTSTR CProgramSearch::m_pszPropName=TEXT("ProgramSearch");


CProgramSearch::CProgramSearch()
	: m_pEventHandler(NULL)
	, m_pOldEditProc(NULL)
	, m_MaxHistory(40)
{
	m_ColumnWidth[0]=100;
	m_ColumnWidth[1]=136;
	m_ColumnWidth[2]=240;
}


CProgramSearch::~CProgramSearch()
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pSearch=NULL;
}


bool CProgramSearch::Create(HWND hwndOwner)
{
	m_RichEditUtil.LoadRichEditLib();
	return CreateDialogWindow(hwndOwner,GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_PROGRAMSEARCH));
}


bool CProgramSearch::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pSearch=NULL;
	if (pHandler!=NULL)
		pHandler->m_pSearch=this;
	m_pEventHandler=pHandler;
	return true;
}


bool CProgramSearch::SetKeywordHistory(const LPTSTR *pKeywordList,int NumKeywords)
{
	if (pKeywordList==NULL)
		return false;
	m_History.clear();
	for (int i=0;i<NumKeywords;i++)
		m_History.push_back(CDynamicString(pKeywordList[i]));
	return true;
}


int CProgramSearch::GetKeywordHistoryCount() const
{
	return (int)m_History.size();
}


LPCTSTR CProgramSearch::GetKeywordHistory(int Index) const
{
	if (Index<0 || (size_t)Index>=m_History.size())
		return NULL;
	return m_History[Index].Get();
}


int CProgramSearch::GetColumnWidth(int Index) const
{
	if (Index<0 || Index>=NUM_COLUMNS)
		return 0;
	return m_ColumnWidth[Index];
}


bool CProgramSearch::SetColumnWidth(int Index,int Width)
{
	if (Index<0 || Index>=NUM_COLUMNS)
		return false;
	m_ColumnWidth[Index]=max(Width,0);
	return true;
}


bool CProgramSearch::Search(LPTSTR pszKeyword)
{
	if (m_hDlg==NULL || pszKeyword==NULL)
		return false;
	::SetDlgItemText(m_hDlg,IDC_PROGRAMSEARCH_KEYWORD,pszKeyword);
	::UpdateWindow(::GetDlgItem(m_hDlg,IDC_PROGRAMSEARCH_KEYWORD));
	::SendMessage(m_hDlg,WM_COMMAND,IDC_PROGRAMSEARCH_SEARCH,0);
	return true;
}


INT_PTR CProgramSearch::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	INT_PTR Result=CResizableDialog::DlgProc(hDlg,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_INITDIALOG:
		AddControl(IDC_PROGRAMSEARCH_KEYWORD,ALIGN_HORZ);
		AddControl(IDC_PROGRAMSEARCH_SEARCH,ALIGN_RIGHT);
		AddControl(IDC_PROGRAMSEARCH_STATUS,ALIGN_HORZ);
		AddControl(IDC_PROGRAMSEARCH_RESULT,ALIGN_ALL);
		AddControl(IDC_PROGRAMSEARCH_INFO,ALIGN_HORZ_BOTTOM);

		DlgComboBox_LimitText(hDlg,IDC_PROGRAMSEARCH_KEYWORD,MAX_KEYWORD_LENGTH-1);
		for (size_t i=0;i<m_History.size();i++)
			DlgComboBox_AddString(hDlg,IDC_PROGRAMSEARCH_KEYWORD,m_History[i].Get());
		{
			HWND hwndEdit=::GetTopWindow(::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_KEYWORD));
			m_pOldEditProc=SubclassWindow(hwndEdit,EditProc);
			::SetProp(hwndEdit,m_pszPropName,m_pOldEditProc);
		}

		{
			HWND hwndList=GetDlgItem(hDlg,IDC_PROGRAMSEARCH_RESULT);
			LV_COLUMN lvc;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=m_ColumnWidth[0];
			lvc.pszText=TEXT("チャンネル");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.cx=m_ColumnWidth[1];
			lvc.pszText=TEXT("日時");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvc.cx=m_ColumnWidth[2];
			lvc.pszText=TEXT("番組名");
			ListView_InsertColumn(hwndList,2,&lvc);

			m_SortColumn=-1;
			m_fSortDescending=false;
		}

		{
			LOGFONT lf;
			HDC hdc;

			::GetObject(GetWindowFont(hDlg),sizeof(LOGFONT),&lf);
			hdc=::GetDC(hDlg);
			CRichEditUtil::LogFontToCharFormat(hdc,&lf,&m_InfoTextFormat);
			::ReleaseDC(hDlg,hdc);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMSEARCH_INFO,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS | ENM_LINK);
		}

		ApplyPosition();
		return TRUE;

	case WM_SIZE:
		InvalidateDlgItem(hDlg,IDC_PROGRAMSEARCH_STATUS);
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMSEARCH_SEARCH:
			if (m_pEventHandler!=NULL) {
				TCHAR szKeyword[MAX_KEYWORD_LENGTH];

				::GetDlgItemText(hDlg,IDC_PROGRAMSEARCH_KEYWORD,szKeyword,lengthof(szKeyword));
				ClearSearchResult();
				if (szKeyword[0]!='\0') {
					// 履歴に追加する
					HWND hwndComboBox=::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_KEYWORD);
					int i;
					i=ComboBox_FindStringExact(hwndComboBox,-1,szKeyword);
					if (i==CB_ERR) {
						ComboBox_InsertString(hwndComboBox,0,szKeyword);
						if (ComboBox_GetCount(hwndComboBox)>m_MaxHistory)
							ComboBox_DeleteString(hwndComboBox,m_MaxHistory);
					} else if (i!=0) {
						ComboBox_DeleteString(hwndComboBox,i);
						ComboBox_InsertString(hwndComboBox,0,szKeyword);
					}
					::SetWindowText(hwndComboBox,szKeyword);

					HCURSOR hcurOld=::LoadCursor(NULL,IDC_WAIT);
					DWORD StartTime=::GetTickCount();
					m_pEventHandler->Search(szKeyword);
					DWORD SearchTime=TickTimeSpan(StartTime,::GetTickCount());
					if (m_SortColumn>=0)
						SortSearchResult();
					::lstrcpy(m_szKeyword,szKeyword);
					::SetCursor(hcurOld);

					TCHAR szStatus[MAX_KEYWORD_LENGTH+64];
					StdUtil::snprintf(szStatus,lengthof(szStatus),
									  TEXT("%s に一致する番組 %d 件 (%d.%02d 秒)"),
									  szKeyword,ListView_GetItemCount(::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_RESULT)),
									  SearchTime/1000,SearchTime/10%100);
					::SetDlgItemText(hDlg,IDC_PROGRAMSEARCH_STATUS,szStatus);
				}
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			if (m_pEventHandler==NULL || m_pEventHandler->OnClose())
				::DestroyWindow(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_STATUS)) {
			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc,::GetSysColor(COLOR_WINDOWTEXT));
			::SetBkColor(hdc,::GetSysColor(COLOR_WINDOW));
			return reinterpret_cast<INT_PTR>(::GetSysColorBrush(COLOR_WINDOW));
		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_COLUMNCLICK:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (pnmlv->iSubItem==m_SortColumn) {
					m_fSortDescending=!m_fSortDescending;
				} else {
					m_SortColumn=pnmlv->iSubItem;
					m_fSortDescending=false;
				}
				SortSearchResult();
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);
				int Sel=ListView_GetNextItem(pnmlv->hdr.hwndFrom,-1,LVNI_SELECTED);
				HWND hwndInfo=::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_INFO);

				::SetWindowText(hwndInfo,TEXT(""));
				if (Sel>=0) {
					LV_ITEM lvi;
					TCHAR szText[2048];

					lvi.mask=LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					ListView_GetItem(pnmlv->hdr.hwndFrom,&lvi);
					::SendMessage(hwndInfo,WM_SETREDRAW,FALSE,0);
					const CSearchEventInfo *pEventInfo=reinterpret_cast<const CSearchEventInfo*>(lvi.lParam);
					FormatEventTimeText(pEventInfo,szText,lengthof(szText));
					CRichEditUtil::AppendText(hwndInfo,szText,&m_InfoTextFormat);
					FormatEventInfoText(pEventInfo,szText,lengthof(szText));
					CRichEditUtil::AppendText(hwndInfo,szText,&m_InfoTextFormat);
					HighlightKeyword();
					CRichEditUtil::DetectURL(hwndInfo,&m_InfoTextFormat,1);
					POINT pt={0,0};
					::SendMessage(hwndInfo,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&pt));
					::SendMessage(hwndInfo,WM_SETREDRAW,TRUE,0);
					::InvalidateRect(hwndInfo,NULL,TRUE);
				}
			}
			return TRUE;

		case LVN_GETEMPTYMARKUP:
			{
				NMLVEMPTYMARKUP *pnmMarkup=reinterpret_cast<NMLVEMPTYMARKUP*>(lParam);

				pnmMarkup->dwFlags=EMF_CENTERED;
				::lstrcpyW(pnmMarkup->szMarkup,L"検索された番組はありません");
				::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,TRUE);
			}
			return TRUE;

		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONDOWN) {
				HWND hwndInfo=::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_INFO);
				HMENU hmenu=::CreatePopupMenu();
				POINT pt;

				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,2,TEXT("すべて選択(&A)"));
				::GetCursorPos(&pt);
				switch (::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hDlg,NULL)) {
				case 1:
					if (::SendMessage(hwndInfo,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
						CRichEditUtil::CopyAllText(hwndInfo);
					} else {
						::SendMessage(hwndInfo,WM_COPY,0,0);
					}
					break;
				case 2:
					CRichEditUtil::SelectAll(hwndInfo);
					break;
				}
				::DestroyMenu(hmenu);
			}
			return TRUE;

		case EN_LINK:
			{
				ENLINK *penl=reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg==WM_LBUTTONUP) {
					CRichEditUtil::HandleLinkClick(penl);
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			HWND hwndComboBox=::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_KEYWORD);
			int Count=ComboBox_GetCount(hwndComboBox);

			m_History.clear();
			for (int i=0;i<Count;i++) {
				TCHAR szKeyword[MAX_KEYWORD_LENGTH];

				ComboBox_GetLBText(hwndComboBox,i,szKeyword);
				m_History.push_back(CDynamicString(szKeyword));
			}
		}
		{
			HWND hwndList=::GetDlgItem(hDlg,IDC_PROGRAMSEARCH_RESULT);
			for (int i=0;i<lengthof(m_ColumnWidth);i++)
				m_ColumnWidth[i]=ListView_GetColumnWidth(hwndList,i);
		}
		ClearSearchResult();
		return TRUE;
	}
	return Result;
}


int CALLBACK CProgramSearch::ResultCompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	const CSearchEventInfo *pInfo1=reinterpret_cast<const CSearchEventInfo*>(lParam1);
	const CSearchEventInfo *pInfo2=reinterpret_cast<const CSearchEventInfo*>(lParam2);
	int Cmp;

	switch (LOWORD(lParamSort)) {
	case 0:	// チャンネル
		Cmp=::lstrcmpi(pInfo1->m_pszChannelName,pInfo2->m_pszChannelName);
		break;
	case 1:	// 時刻
		Cmp=CompareSystemTime(&pInfo1->m_stStartTime,&pInfo2->m_stStartTime);
		break;
	case 2:	// 番組名
		if (pInfo1->GetEventName()==NULL) {
			if (pInfo2->GetEventName()==NULL)
				Cmp=0;
			else
				Cmp=-1;
		} else {
			if (pInfo2->GetEventName()==NULL)
				Cmp=1;
			else
				Cmp=::lstrcmpi(pInfo1->GetEventName(),pInfo2->GetEventName());
		}
		break;
	}
	return HIWORD(lParamSort)==0?Cmp:-Cmp;
}


bool CProgramSearch::AddSearchResult(const CEventInfoData *pEventInfo,LPCTSTR pszChannelName)
{
	if (m_hDlg==NULL || pEventInfo==NULL)
		return false;

	HWND hwndList=::GetDlgItem(m_hDlg,IDC_PROGRAMSEARCH_RESULT);
	LV_ITEM lvi;
	TCHAR szText[256];

	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=ListView_GetItemCount(hwndList);
	lvi.iSubItem=0;
	::lstrcpyn(szText,NullToEmptyString(pszChannelName),lengthof(szText));
	lvi.pszText=szText;
	lvi.lParam=reinterpret_cast<LPARAM>(new CSearchEventInfo(*pEventInfo,pszChannelName));
	ListView_InsertItem(hwndList,&lvi);
	SYSTEMTIME stEnd;
	pEventInfo->GetEndTime(&stEnd);
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%02d/%02d(%s) %02d:%02d～%02d:%02d"),
					  pEventInfo->m_stStartTime.wMonth,pEventInfo->m_stStartTime.wDay,
					  GetDayOfWeekText(pEventInfo->m_stStartTime.wDayOfWeek),
					  pEventInfo->m_stStartTime.wHour,pEventInfo->m_stStartTime.wMinute,
					  stEnd.wHour,stEnd.wMinute);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	//lvi.pszText=szText;
	ListView_SetItem(hwndList,&lvi);
	//lvi.mask=LVIF_TEXT;
	lvi.iSubItem=2;
	::lstrcpyn(szText,NullToEmptyString(pEventInfo->GetEventName()),lengthof(szText));
	//lvi.pszText=szText;
	ListView_SetItem(hwndList,&lvi);

	return true;
}


void CProgramSearch::ClearSearchResult()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_PROGRAMSEARCH_RESULT);
	int Items=ListView_GetItemCount(hwndList);
	LV_ITEM lvi;

	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;
	for (int i=0;i<Items;i++) {
		lvi.iItem=i;
		ListView_GetItem(hwndList,&lvi);
		delete reinterpret_cast<CSearchEventInfo*>(lvi.lParam);
	}
	ListView_DeleteAllItems(hwndList);
	::SetDlgItemText(m_hDlg,IDC_PROGRAMSEARCH_INFO,TEXT(""));
}


void CProgramSearch::SortSearchResult()
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_PROGRAMSEARCH_RESULT);

	ListView_SortItems(hwndList,ResultCompareFunc,
					   MAKELPARAM(m_SortColumn,m_fSortDescending));
	SetListViewSortMark(hwndList,m_SortColumn,!m_fSortDescending);
}


int CProgramSearch::FormatEventTimeText(const CEventInfoData *pEventInfo,LPTSTR pszText,int MaxLength) const
{
	if (pEventInfo==NULL) {
		pszText[0]='\0';
		return 0;
	}

	TCHAR szEndTime[16];
	SYSTEMTIME stEnd;
	if (pEventInfo->m_DurationSec>0 && pEventInfo->GetEndTime(&stEnd))
		StdUtil::snprintf(szEndTime,lengthof(szEndTime),
						  TEXT("～%d:%02d"),stEnd.wHour,stEnd.wMinute);
	else
		szEndTime[0]='\0';
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%d/%d/%d(%s) %d:%02d%s\r\n"),
							 pEventInfo->m_stStartTime.wYear,
							 pEventInfo->m_stStartTime.wMonth,
							 pEventInfo->m_stStartTime.wDay,
							 GetDayOfWeekText(pEventInfo->m_stStartTime.wDayOfWeek),
							 pEventInfo->m_stStartTime.wHour,
							 pEventInfo->m_stStartTime.wMinute,
							 szEndTime);
}


int CProgramSearch::FormatEventInfoText(const CEventInfoData *pEventInfo,LPTSTR pszText,int MaxLength) const
{
	if (pEventInfo==NULL) {
		pszText[0]='\0';
		return 0;
	}

	return StdUtil::snprintf(
		pszText,MaxLength,
		TEXT("%s\r\n\r\n%s%s%s%s"),
		NullToEmptyString(pEventInfo->GetEventName()),
		NullToEmptyString(pEventInfo->GetEventText()),
		pEventInfo->GetEventText()!=NULL?TEXT("\r\n\r\n"):TEXT(""),
		NullToEmptyString(pEventInfo->GetEventExtText()),
		pEventInfo->GetEventExtText()!=NULL?TEXT("\r\n\r\n"):TEXT(""));
}


void CProgramSearch::HighlightKeyword()
{
	const HWND hwndInfo=::GetDlgItem(m_hDlg,IDC_PROGRAMSEARCH_INFO);
	const int LineCount=(int)::SendMessage(hwndInfo,EM_GETLINECOUNT,0,0);
	CHARFORMAT2 cfHighlight;
	CRichEditUtil::CharFormatToCharFormat2(&m_InfoTextFormat,&cfHighlight);
	cfHighlight.dwMask|=CFM_BOLD | CFM_BACKCOLOR;
	cfHighlight.dwEffects|=CFE_BOLD;
	cfHighlight.crBackColor=RGB(255,255,0);
	CHARRANGE cr,crOld;

	::SendMessage(hwndInfo,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	for (int i=1;i<LineCount;) {
		const int LineIndex=(int)::SendMessage(hwndInfo,EM_LINEINDEX,i,0);
		TCHAR szText[2048],*q;
		int TotalLength=0,Length;

		q=szText;
		while (i<LineCount) {
#ifdef UNICODE
			q[0]=(WORD)(lengthof(szText)-2-TotalLength);
#else
			*(WORD*)q=(WORD)(sizeof(szText)-sizeof(WORD)-1-TotalLength);
#endif
			Length=(int)::SendMessage(hwndInfo,EM_GETLINE,i,reinterpret_cast<LPARAM>(q));
			i++;
			if (Length<1)
				break;
			q+=Length;
			TotalLength+=Length;
			if (*(q-1)=='\r' || *(q-1)=='\n')
				break;
		}
		if (TotalLength>0) {
			szText[TotalLength]='\0';

			LPCTSTR p=m_szKeyword;
			while (*p!='\0') {
				TCHAR szWord[MAX_KEYWORD_LENGTH],Delimiter;
				bool fMinus=false;

				while (*p==' ')
					p++;
				if (*p=='-') {
					fMinus=true;
					p++;
				}
				if (*p=='"') {
					p++;
					Delimiter='"';
				} else {
					Delimiter=' ';
				}
				int KeywordLength;
				for (KeywordLength=0;*p!=Delimiter && *p!='|' && *p!='\0';KeywordLength++)
					szWord[KeywordLength]=*p++;
				if (*p==Delimiter)
					p++;
				if (!fMinus && KeywordLength>0) {
					LPCTSTR q=szText;
					while (SearchNextKeyword(&q,szWord,KeywordLength,&Length)) {
						cr.cpMin=LineIndex+(LONG)(q-szText);
						cr.cpMax=cr.cpMin+Length;
						::SendMessage(hwndInfo,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
						::SendMessage(hwndInfo,EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&cfHighlight));
						q+=Length;
					}
				}
				while (*p==' ')
					p++;
				if (*p=='|')
					p++;
			}
		}
	}
	::SendMessage(hwndInfo,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&crOld));
}


bool CProgramSearch::SearchNextKeyword(LPCTSTR *ppszText,LPCTSTR pKeyword,int KeywordLength,int *pLength) const
{
	LPCTSTR p=*ppszText;
	const int StringLength=::lstrlen(p);

	if (StringLength<KeywordLength)
		return false;

	for (int i=0;i<=StringLength-KeywordLength;i++) {
		if (::CompareString(LOCALE_USER_DEFAULT,NORM_IGNORECASE | NORM_IGNOREWIDTH,
							&p[i],KeywordLength,pKeyword,KeywordLength)==CSTR_EQUAL) {
			*ppszText=p+i;
			*pLength=KeywordLength;
			return true;
		}
	}
	return false;
}


LRESULT CALLBACK CProgramSearch::EditProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	WNDPROC pOldWndProc=static_cast<WNDPROC>(::GetProp(hwnd,m_pszPropName));

	if (pOldWndProc==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_KEYDOWN:
		if (wParam==VK_RETURN) {
			::SendMessage(::GetParent(::GetParent(hwnd)),WM_COMMAND,IDC_PROGRAMSEARCH_SEARCH,0);
			return 0;
		}
		break;

	case WM_CHAR:
		if (wParam=='\r' || wParam=='\n')
			return 0;
		break;

	case WM_NCDESTROY:
		::RemoveProp(hwnd,m_pszPropName);
		break;
	}
	return ::CallWindowProc(pOldWndProc,hwnd,uMsg,wParam,lParam);
}




CProgramSearch::CEventHandler::CEventHandler()
	: m_pSearch(NULL)
{
}


CProgramSearch::CEventHandler::~CEventHandler()
{
	if (m_pSearch!=NULL)
		m_pSearch->m_pEventHandler=NULL;
}


bool CProgramSearch::CEventHandler::AddSearchResult(const CEventInfoData *pEventInfo,LPCTSTR pszChannelName)
{
	if (pEventInfo==NULL || m_pSearch==NULL)
		return false;
	return m_pSearch->AddSearchResult(pEventInfo,pszChannelName);
}


bool CProgramSearch::CEventHandler::CompareKeyword(LPCTSTR pszString,LPCTSTR pKeyword,int KeywordLength) const
{
	const int StringLength=::lstrlen(pszString);

	if (StringLength<KeywordLength)
		return false;

	for (int i=0;i<=StringLength-KeywordLength;i++) {
		if (::CompareString(LOCALE_USER_DEFAULT,NORM_IGNORECASE | NORM_IGNOREWIDTH,
							&pszString[i],KeywordLength,pKeyword,KeywordLength)==CSTR_EQUAL)
			return true;
	}
	return false;
}


bool CProgramSearch::CEventHandler::MatchKeyword(const CEventInfoData *pEventInfo,LPCTSTR pszKeyword) const
{
	bool fMatch=false;
	bool fOr=false,fPrevOr=false,fOrMatch;
	LPCTSTR p=pszKeyword;

	while (*p!='\0') {
		TCHAR szWord[CProgramSearch::MAX_KEYWORD_LENGTH],Delimiter;
		bool fMinus=false;

		while (*p==' ')
			p++;
		if (*p=='-') {
			fMinus=true;
			p++;
		}
		if (*p=='"') {
			p++;
			Delimiter='"';
		} else {
			Delimiter=' ';
		}
		int i;
		for (i=0;*p!=Delimiter && *p!='|' && *p!='\0';i++) {
			szWord[i]=*p++;
		}
		if (*p==Delimiter)
			p++;
		while (*p==' ')
			p++;
		if (*p=='|') {
			if (!fOr) {
				fOr=true;
				fOrMatch=false;
			}
			p++;
		} else {
			fOr=false;
		}
		if (i>0) {
			if ((pEventInfo->GetEventName()!=NULL
					&& CompareKeyword(pEventInfo->GetEventName(),szWord,i))
				|| (pEventInfo->GetEventText()!=NULL
					&& CompareKeyword(pEventInfo->GetEventText(),szWord,i))
				|| (pEventInfo->GetEventExtText()!=NULL
					&& CompareKeyword(pEventInfo->GetEventExtText(),szWord,i))) {
				if (fMinus)
					return false;
				fMatch=true;
				if (fOr)
					fOrMatch=true;
			} else {
				if (!fMinus && !fOr && (!fPrevOr || !fOrMatch))
					return false;
			}
		}
		fPrevOr=fOr;
	}
	return fMatch;
}
