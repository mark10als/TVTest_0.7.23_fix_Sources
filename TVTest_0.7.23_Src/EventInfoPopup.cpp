#include "stdafx.h"
#include "TVTest.h"
#include "EventInfoPopup.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




const LPCTSTR CEventInfoPopup::m_pszWindowClass=APP_NAME TEXT(" Event Info");
HINSTANCE CEventInfoPopup::m_hinst=NULL;


bool CEventInfoPopup::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CEventInfoPopup::CEventInfoPopup()
	: m_hwndEdit(NULL)
	, m_BackColor(::GetSysColor(COLOR_WINDOW))
	, m_TextColor(::GetSysColor(COLOR_WINDOWTEXT))
	, m_TitleBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,
						  RGB(255,255,255),RGB(228,228,240))
	, m_TitleTextColor(RGB(80,80,80))
	, m_TitleLineMargin(1)
	, m_TitleHeight(0)
	, m_ButtonSize(14)
	, m_ButtonMargin(3)
	, m_pEventHandler(NULL)
	, m_fDetailInfo(
#ifdef _DEBUG
		true
#else
		false
#endif
		)
{
	m_WindowPosition.Width=320;
	m_WindowPosition.Height=320;

	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
}


CEventInfoPopup::~CEventInfoPopup()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=NULL;
}


bool CEventInfoPopup::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,m_pszWindowClass,NULL,m_hinst);
}


void CEventInfoPopup::SetEventInfo(const CEventInfoData *pEventInfo)
{
	if (m_EventInfo==*pEventInfo)
		return;

	m_EventInfo=*pEventInfo;

	LPCTSTR pszVideo=TEXT("?"),pszAudio=TEXT("?");

	static const struct {
		BYTE ComponentType;
		LPCTSTR pszText;
	} VideoComponentTypeList[] = {
		{0x01,TEXT("480i[4:3]")},
		{0x03,TEXT("480i[16:9]")},
		{0x04,TEXT("480i[>16:9]")},
		{0xA1,TEXT("480p[4:3]")},
		{0xA3,TEXT("480p[16:9]")},
		{0xA4,TEXT("480p[>16:9]")},
		{0xB1,TEXT("1080i[4:3]")},
		{0xB3,TEXT("1080i[16:9]")},
		{0xB4,TEXT("1080i[>16:9]")},
		{0xC1,TEXT("720p[4:3]")},
		{0xC3,TEXT("720p[16:9]")},
		{0xC4,TEXT("720p[>16:9]")},
		{0xD1,TEXT("240p[4:3]")},
		{0xD3,TEXT("240p[16:9]")},
		{0xD4,TEXT("240p[>16:9]")},
	};
	for (int i=0;i<lengthof(VideoComponentTypeList);i++) {
		if (VideoComponentTypeList[i].ComponentType==m_EventInfo.m_VideoInfo.ComponentType) {
			pszVideo=VideoComponentTypeList[i].pszText;
			break;
		}
	}

	TCHAR szAudioComponent[64];
	szAudioComponent[0]=_T('\0');
	if (m_EventInfo.m_AudioList.size()>0) {
		static const struct {
			BYTE ComponentType;
			LPCTSTR pszText;
		} AudioComponentTypeList[] = {
			{0x01,TEXT("Mono")},
			{0x02,TEXT("Dual mono")},
			{0x03,TEXT("Stereo")},
			{0x07,TEXT("3+1")},
			{0x08,TEXT("3+2")},
			{0x09,TEXT("5.1ch")},
		};

		// TODO:複数音声対応
		const CEventInfoData::AudioInfo *pAudioInfo=m_EventInfo.GetMainAudioInfo();

		if (pAudioInfo->ComponentType==0x02
				&& pAudioInfo->bESMultiLingualFlag) {
			pszAudio=TEXT("Mono 2カ国語");
		} else {
			for (int i=0;i<lengthof(AudioComponentTypeList);i++) {
				if (AudioComponentTypeList[i].ComponentType==pAudioInfo->ComponentType) {
					pszAudio=AudioComponentTypeList[i].pszText;
					break;
				}
			}
		}

		LPCTSTR p=pAudioInfo->szText;
		if (*p!=_T('\0')) {
			szAudioComponent[0]=_T(' ');
			szAudioComponent[1]=_T('[');
			size_t i;
			for (i=2;*p!=_T('\0') && i<lengthof(szAudioComponent)-2;i++) {
				if (*p==_T('\r') || *p==_T('\n')) {
					szAudioComponent[i]=_T('/');
					p++;
					if (*p==_T('\n'))
						p++;
				} else {
					szAudioComponent[i]=*p++;
				}
			}
			szAudioComponent[i+0]=_T(']');
			szAudioComponent[i+1]=_T('\0');
		}
	}

	TCHAR szText[4096];
	CStaticStringFormatter Formatter(szText,lengthof(szText));
	Formatter.AppendFormat(TEXT("%s%s%s"),
						   NullToEmptyString(m_EventInfo.GetEventText()),
						   !IsStringEmpty(m_EventInfo.GetEventText())?TEXT("\r\n\r\n"):TEXT(""),
						   NullToEmptyString(m_EventInfo.GetEventExtText()));
	Formatter.RemoveTrailingWhitespace();
	if (*pszVideo!=_T('?') || *pszAudio!=_T('?')) {
		Formatter.AppendFormat(TEXT("%s(映像: %s / 音声: %s%s)"),
							   !Formatter.IsEmpty()?TEXT("\r\n\r\n"):TEXT(""),
							   pszVideo,
							   pszAudio,
							   szAudioComponent);
	}
	if (m_fDetailInfo) {
		Formatter.AppendFormat(TEXT("\r\nイベントID 0x%04X"),m_EventInfo.m_EventID);
		if (m_EventInfo.m_fCommonEvent)
			Formatter.AppendFormat(TEXT(" (イベント共有 サービスID 0x%04X / イベントID 0x%04X)"),
								   m_EventInfo.m_CommonEventInfo.ServiceID,
								   m_EventInfo.m_CommonEventInfo.EventID);
		if (m_EventInfo.m_ContentNibble.NibbleCount>0)
			Formatter.AppendFormat(TEXT("\r\nジャンル %d - %d"),
								   m_EventInfo.m_ContentNibble.NibbleList[0].ContentNibbleLevel1,
								   m_EventInfo.m_ContentNibble.NibbleList[0].ContentNibbleLevel2);
	}

	LOGFONT lf;
	CHARFORMAT cf;
	HDC hdc=::GetDC(m_hwndEdit);
	m_Font.GetLogFont(&lf);
	CRichEditUtil::LogFontToCharFormat(hdc,&lf,&cf);
	cf.dwMask|=CFM_COLOR;
	cf.crTextColor=m_TextColor;
	::ReleaseDC(m_hwndEdit,hdc);
	::SendMessage(m_hwndEdit,WM_SETREDRAW,FALSE,0);
	::SetWindowText(m_hwndEdit,NULL);
	CRichEditUtil::AppendText(m_hwndEdit,Formatter.GetString(),&cf);
	CRichEditUtil::DetectURL(m_hwndEdit,&cf);
	POINT pt={0,0};
	::SendMessage(m_hwndEdit,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&pt));
	::SendMessage(m_hwndEdit,WM_SETREDRAW,TRUE,0);
	::InvalidateRect(m_hwndEdit,NULL,TRUE);

	CalcTitleHeight();
	RECT rc;
	GetClientRect(&rc);
	::MoveWindow(m_hwndEdit,0,m_TitleHeight,rc.right,max(rc.bottom-m_TitleHeight,0),TRUE);
	Invalidate();
}


void CEventInfoPopup::CalcTitleHeight()
{
	TEXTMETRIC tm;
	RECT rc;

	HDC hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
	::GetTextMetrics(hdc,&tm);
	//FontHeight=tm.tmHeight-tm.tmInternalLeading;
	int FontHeight=tm.tmHeight;
	m_TitleLineHeight=FontHeight+m_TitleLineMargin;
	GetClientRect(&rc);
	rc.right-=m_ButtonSize+m_ButtonMargin*2;
	m_TitleHeight=(DrawUtil::CalcWrapTextLines(hdc,m_EventInfo.GetEventName(),rc.right)+1)*m_TitleLineHeight;
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


bool CEventInfoPopup::Show(const CEventInfoData *pEventInfo,const RECT *pPos)
{
	if (pEventInfo==NULL)
		return false;
	bool fExists=m_hwnd!=NULL;
	if (!fExists) {
		if (!Create(NULL,WS_POPUP | WS_CLIPCHILDREN | WS_THICKFRAME,WS_EX_TOPMOST | WS_EX_NOACTIVATE,0))
			return false;
	}
	if (pPos!=NULL) {
		if (!GetVisible())
			SetPosition(pPos);
	} else if (!IsVisible() || m_EventInfo!=*pEventInfo) {
		RECT rc;
		POINT pt;
		int Width,Height;

		GetPosition(&rc);
		Width=rc.right-rc.left;
		Height=rc.bottom-rc.top;
		::GetCursorPos(&pt);
		pt.y+=16;
		HMONITOR hMonitor=::MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST);
		if (hMonitor!=NULL) {
			MONITORINFO mi;

			mi.cbSize=sizeof(mi);
			if (::GetMonitorInfo(hMonitor,&mi)) {
				if (pt.x+Width>mi.rcMonitor.right)
					pt.x=mi.rcMonitor.right-Width;
				if (pt.y+Height>mi.rcMonitor.bottom) {
					pt.y=mi.rcMonitor.bottom-Height;
					if (pt.x+Width<mi.rcMonitor.right)
						pt.x+=min(16,mi.rcMonitor.right-(pt.x+Width));
				}
			}
		}
		::SetWindowPos(m_hwnd,HWND_TOPMOST,pt.x,pt.y,Width,Height,
					   SWP_NOACTIVATE);
	}
	SetEventInfo(pEventInfo);
	::ShowWindow(m_hwnd,SW_SHOWNA);
	return true;
}


bool CEventInfoPopup::Hide()
{
	if (m_hwnd!=NULL)
		::ShowWindow(m_hwnd,SW_HIDE);
	return true;
}


bool CEventInfoPopup::IsVisible()
{
	return m_hwnd!=NULL && GetVisible();
}


bool CEventInfoPopup::IsOwnWindow(HWND hwnd) const
{
	if (hwnd==NULL)
		return false;
	return hwnd==m_hwnd || hwnd==m_hwndEdit;
}


void CEventInfoPopup::GetSize(int *pWidth,int *pHeight)
{
	RECT rc;

	GetPosition(&rc);
	if (pWidth!=NULL)
		*pWidth=rc.right-rc.left;
	if (pHeight!=NULL)
		*pHeight=rc.bottom-rc.top;
}


bool CEventInfoPopup::SetSize(int Width,int Height)
{
	if (Width<0 || Height<0)
		return false;
	RECT rc;
	GetPosition(&rc);
	rc.right=rc.left+Width;
	rc.bottom=rc.top+Height;
	return SetPosition(&rc);
}


void CEventInfoPopup::SetColor(COLORREF BackColor,COLORREF TextColor)
{
	m_BackColor=BackColor;
	m_TextColor=TextColor;
	if (m_hwnd!=NULL) {
		::SendMessage(m_hwndEdit,EM_SETBKGNDCOLOR,0,m_BackColor);
		//::InvalidateRect(m_hwndEdit,NULL,TRUE);
	}
}


void CEventInfoPopup::SetTitleColor(Theme::GradientInfo *pBackGradient,COLORREF TextColor)
{
	m_TitleBackGradient=*pBackGradient;
	m_TitleTextColor=TextColor;
	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_TitleHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CEventInfoPopup::SetFont(const LOGFONT *pFont)
{
	LOGFONT lf=*pFont;

	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	if (m_hwnd!=NULL) {
		CalcTitleHeight();
		RECT rc;
		GetClientRect(&rc);
		::MoveWindow(m_hwndEdit,0,m_TitleHeight,rc.right,max(rc.bottom-m_TitleHeight,0),TRUE);
		Invalidate();

		SetWindowFont(m_hwndEdit,m_Font.GetHandle(),TRUE);
	}
	return true;
}


void CEventInfoPopup::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pPopup=this;
	m_pEventHandler=pEventHandler;
}


bool CEventInfoPopup::IsSelected() const
{
	return CRichEditUtil::IsSelected(m_hwndEdit);
}


LPTSTR CEventInfoPopup::GetSelectedText() const
{
	return CRichEditUtil::GetSelectedText(m_hwndEdit);
}


LRESULT CEventInfoPopup::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		m_RichEditUtil.LoadRichEditLib();
		m_hwndEdit=::CreateWindowEx(0,TEXT("RichEdit20W"),TEXT(""),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,
			0,0,0,0,hwnd,(HMENU)1,m_hinst,NULL);
		SetWindowFont(m_hwndEdit,m_Font.GetHandle(),FALSE);
		::SendMessage(m_hwndEdit,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS | ENM_LINK);
		::SendMessage(m_hwndEdit,EM_SETBKGNDCOLOR,0,m_BackColor);
		return 0;

	case WM_SIZE:
		CalcTitleHeight();
		::MoveWindow(m_hwndEdit,0,m_TitleHeight,
					 LOWORD(lParam),max(HIWORD(lParam)-m_TitleHeight,0),TRUE);
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			RECT rc;
			HFONT hfontOld;
			int OldBkMode;
			COLORREF OldTextColor;
			TCHAR szText[64];
			int Length;

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rc);
			rc.bottom=m_TitleHeight;
			Theme::FillGradient(ps.hdc,&rc,&m_TitleBackGradient);
			hfontOld=DrawUtil::SelectObject(ps.hdc,m_TitleFont);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			OldTextColor=::SetTextColor(ps.hdc,m_TitleTextColor);
			if (m_EventInfo.m_fValidStartTime) {
				Length=StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d/%d(%s) %d:%02d"),
					m_EventInfo.m_stStartTime.wYear,
					m_EventInfo.m_stStartTime.wMonth,
					m_EventInfo.m_stStartTime.wDay,
					GetDayOfWeekText(m_EventInfo.m_stStartTime.wDayOfWeek),
					m_EventInfo.m_stStartTime.wHour,
					m_EventInfo.m_stStartTime.wMinute);
				SYSTEMTIME stEnd;
				if (m_EventInfo.m_DurationSec>0
						&& m_EventInfo.GetEndTime(&stEnd))
					Length+=StdUtil::snprintf(szText+Length,lengthof(szText)-Length,
											  TEXT("～%d:%02d"),stEnd.wHour,stEnd.wMinute);
				::TextOut(ps.hdc,0,0,szText,Length);
			}
			rc.top+=m_TitleLineHeight;
			rc.right-=m_ButtonSize+m_ButtonMargin*2;
			DrawUtil::DrawWrapText(ps.hdc,m_EventInfo.GetEventName(),&rc,m_TitleLineHeight);
			::SelectObject(ps.hdc,hfontOld);
			::SetBkMode(ps.hdc,OldBkMode);
			::SetTextColor(ps.hdc,OldTextColor);
			GetCloseButtonRect(&rc);
			::DrawFrameControl(ps.hdc,&rc,DFC_CAPTION,DFCS_CAPTIONCLOSE | DFCS_MONO);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			Hide();
		}
		return 0;

	case WM_ACTIVATEAPP:
		if (wParam==0) {
			Hide();
		}
		return 0;

	case WM_NCHITTEST:
		{
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ScreenToClient(hwnd,&pt);
			GetCloseButtonRect(&rc);
			if (::PtInRect(&rc,pt))
				return HTCLOSE;
			::GetClientRect(hwnd,&rc);
			rc.bottom=m_TitleHeight;
			if (::PtInRect(&rc,pt))
				return HTCAPTION;
		}
		break;

	case WM_NCLBUTTONDOWN:
		if (wParam==HTCLOSE) {
			::SendMessage(hwnd,WM_CLOSE,0,0);
			return 0;
		}
		break;

	case WM_NCRBUTTONDOWN:
		if (wParam==HTCAPTION) {
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			HMENU hmenu=::CreatePopupMenu();

			::AppendMenu(hmenu,MF_STRING | MF_ENABLED,1,TEXT("番組名をコピー(&C)"));
			int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
			::DestroyMenu(hmenu);
			switch (Command) {
			case 1:
				CopyText(m_EventInfo.GetEventName());
				break;
			}
			return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		return ::SendMessage(m_hwndEdit,uMsg,wParam,lParam);

	case WM_NCMOUSEMOVE:
		{
			TRACKMOUSEEVENT tme;

			tme.cbSize=sizeof(TRACKMOUSEEVENT);
			tme.dwFlags=TME_LEAVE | TME_NONCLIENT;
			tme.hwndTrack=hwnd;
			::TrackMouseEvent(&tme);
		}
		return 0;

	case WM_NCMOUSELEAVE:
		{
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::GetWindowRect(hwnd,&rc);
			if (!::PtInRect(&rc,pt))
				Hide();
		}
		return 0;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONDOWN) {
				HMENU hmenu=::CreatePopupMenu();

				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,1,TEXT("コピー(&C)"));
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,2,TEXT("すべて選択(&A)"));
				::AppendMenu(hmenu,MF_STRING | MF_ENABLED,3,TEXT("番組名をコピー(&E)"));
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnMenuPopup(hmenu);
				POINT pt;
				::GetCursorPos(&pt);
				int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
				::DestroyMenu(hmenu);
				switch (Command) {
				case 1:
					if (::SendMessage(m_hwndEdit,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
						CRichEditUtil::CopyAllText(m_hwndEdit);
					} else {
						::SendMessage(m_hwndEdit,WM_COPY,0,0);
					}
					break;
				case 2:
					CRichEditUtil::SelectAll(m_hwndEdit);
					break;
				case 3:
					CopyText(m_EventInfo.GetEventName());
					break;
				default:
					if (Command>=CEventHandler::COMMAND_FIRST)
						m_pEventHandler->OnMenuSelected(Command);
					break;
				}
			}
			return 0;

		case EN_LINK:
			{
				ENLINK *penl=reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg==WM_LBUTTONUP)
					CRichEditUtil::HandleLinkClick(penl);
			}
			return 0;
		}
		break;

	case WM_CLOSE:
		Hide();
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CEventInfoPopup::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	GetClientRect(&rc);
	rc.right-=m_ButtonMargin;
	rc.left=rc.right-m_ButtonSize;
	rc.top=m_ButtonMargin;
	rc.bottom=rc.top+m_ButtonSize;
	*pRect=rc;
}


bool CEventInfoPopup::CopyText(LPCWSTR pszText) const
{
	if (pszText==NULL)
		return false;

	int Length=::lstrlenW(pszText);
	HGLOBAL hData=::GlobalAlloc(GMEM_MOVEABLE,(Length+1)*sizeof(WCHAR));
	if (hData==NULL)
		return false;

	bool fOK=false;
	if (::OpenClipboard(m_hwnd)) {
		LPWSTR pData=static_cast<LPWSTR>(::GlobalLock(hData));
		if (pData!=NULL) {
			::lstrcpyW(pData,pszText);
			::GlobalUnlock(hData);
			::EmptyClipboard();
			if (::SetClipboardData(CF_UNICODETEXT,hData)!=NULL)
				fOK=true;
		}
		::CloseClipboard();
	}
	if (!fOK)
		::GlobalFree(hData);
	return fOK;
}




CEventInfoPopup::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopup::CEventHandler::~CEventHandler()
{
	if (m_pPopup!=NULL)
		m_pPopup->m_pEventHandler=NULL;
}




const LPCTSTR CEventInfoPopupManager::m_pszPropName=TEXT("EventInfoPopup");


CEventInfoPopupManager::CEventInfoPopupManager(CEventInfoPopup *pPopup)
	: m_pPopup(pPopup)
	, m_fEnable(true)
	, m_hwnd(NULL)
	, m_pOldWndProc(NULL)
	, m_pEventHandler(NULL)
	, m_HitTestParam(-1)
{
}


CEventInfoPopupManager::~CEventInfoPopupManager()
{
	Finalize();
}


bool CEventInfoPopupManager::Initialize(HWND hwnd,CEventHandler *pEventHandler)
{
	if (hwnd==NULL)
		return false;
	m_hwnd=hwnd;
	m_pOldWndProc=(WNDPROC)::SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)HookWndProc);
	m_pEventHandler=pEventHandler;
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=m_pPopup;
	m_fTrackMouseEvent=false;
	::SetProp(hwnd,m_pszPropName,this);
	return true;
}


void CEventInfoPopupManager::Finalize()
{
	if (m_hwnd!=NULL) {
		m_pPopup->Hide();
		if (m_pOldWndProc!=NULL) {
			::SetWindowLongPtr(m_hwnd,GWLP_WNDPROC,(LONG_PTR)m_pOldWndProc);
			m_pOldWndProc=NULL;
		}
		::RemoveProp(m_hwnd,m_pszPropName);
		m_hwnd=NULL;
	}
}


bool CEventInfoPopupManager::SetEnable(bool fEnable)
{
	m_fEnable=fEnable;
	if (!fEnable)
		m_pPopup->Hide();
	return true;
}


bool CEventInfoPopupManager::Popup(int x,int y)
{
	if (m_pEventHandler==NULL)
		return false;
	m_HitTestParam=-1;
	if (m_pEventHandler->HitTest(x,y,&m_HitTestParam)) {
		const CEventInfoData *pEventInfo;

		if (m_pEventHandler->GetEventInfo(m_HitTestParam,&pEventInfo)
				&& m_pEventHandler->OnShow(pEventInfo)) {
			m_pPopup->Show(pEventInfo);
			return true;
		}
	}
	return false;
}


LRESULT CALLBACK CEventInfoPopupManager::HookWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CEventInfoPopupManager *pThis=static_cast<CEventInfoPopupManager*>(::GetProp(hwnd,m_pszPropName));

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_MOUSEMOVE:
		if (!pThis->m_fTrackMouseEvent) {
			TRACKMOUSEEVENT tme;

			tme.cbSize=sizeof(tme);
			tme.dwFlags=TME_HOVER | TME_LEAVE;
			tme.hwndTrack=hwnd;
			tme.dwHoverTime=1000;
			if (::TrackMouseEvent(&tme))
				pThis->m_fTrackMouseEvent=true;
		}
		if (pThis->m_pPopup->IsVisible() && pThis->m_pEventHandler!=NULL) {
			LPARAM Param;
			if (pThis->m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Param)) {
				if (Param!=pThis->m_HitTestParam) {
					const CEventInfoData *pEventInfo;

					pThis->m_HitTestParam=Param;
					if (pThis->m_pEventHandler->GetEventInfo(pThis->m_HitTestParam,&pEventInfo)
							&& pThis->m_pEventHandler->OnShow(pEventInfo))
						pThis->m_pPopup->Show(pEventInfo);
				}
			} else {
				pThis->m_pPopup->Hide();
			}
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_VSCROLL:
	case WM_HSCROLL:
		pThis->m_pPopup->Hide();
		break;

	case WM_MOUSELEAVE:
		if (pThis->m_pPopup->IsVisible()) {
			POINT pt;
			::GetCursorPos(&pt);
			HWND hwndCur=::WindowFromPoint(pt);
			if (!pThis->m_pPopup->IsOwnWindow(hwndCur))
				pThis->m_pPopup->Hide();
		}
		pThis->m_fTrackMouseEvent=false;
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			HWND hwndActive=reinterpret_cast<HWND>(lParam);
			if (!pThis->m_pPopup->IsOwnWindow(hwndActive))
				pThis->m_pPopup->Hide();
		}
		break;

	case WM_MOUSEHOVER:
		if (pThis->m_pEventHandler!=NULL && pThis->m_fEnable
				&& ::GetActiveWindow()==::GetForegroundWindow()) {
			bool fHit=false;
			pThis->m_HitTestParam=-1;
			if (pThis->m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&pThis->m_HitTestParam)) {
				const CEventInfoData *pEventInfo;

				if (pThis->m_pEventHandler->GetEventInfo(pThis->m_HitTestParam,&pEventInfo)
						&& pThis->m_pEventHandler->OnShow(pEventInfo)) {
					pThis->m_pPopup->Show(pEventInfo);
					fHit=true;
				}
			}
			if (!fHit)
				pThis->m_pPopup->Hide();
		}
		pThis->m_fTrackMouseEvent=false;
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			pThis->m_pPopup->Hide();
		return 0;

	case WM_DESTROY:
		::CallWindowProc(pThis->m_pOldWndProc,hwnd,uMsg,wParam,lParam);
		pThis->Finalize();
		return 0;
	}
	return ::CallWindowProc(pThis->m_pOldWndProc,hwnd,uMsg,wParam,lParam);
}




CEventInfoPopupManager::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopupManager::CEventHandler::~CEventHandler()
{
}
