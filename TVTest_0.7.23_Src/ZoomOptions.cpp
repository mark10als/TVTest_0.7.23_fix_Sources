#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ZoomOptions.h"
#include "DialogUtil.h"
#include "HelperClass/StdUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PACK_LPARAM(index,rate,factor) \
	((LPARAM)(((index)<<24)|((rate)<<12)|(factor)))
#define GET_LPARAM_INDEX(lparam)	((int)((lparam)>>24))
#define GET_LPARAM_RATE(lparam)		((int)(((lparam)>>12)&0xFFF))
#define GET_LPARAM_FACTOR(lparam)	((int)((lparam)&0xFFF))


#ifdef TVH264_FOR_1SEG
#define f1Seg true
#else
#define f1Seg false
#endif

CZoomOptions::ZoomInfo CZoomOptions::m_ZoomList[NUM_ZOOM] = {
	{CM_ZOOM_20,		{  1,   5},	!f1Seg},
	{CM_ZOOM_25,		{  1,   4},	!f1Seg},
	{CM_ZOOM_33,		{  1,   3},	!f1Seg},
	{CM_ZOOM_50,		{  1,   2},	true},
	{CM_ZOOM_66,		{  2,   3},	true},
	{CM_ZOOM_75,		{  3,   4},	true},
	{CM_ZOOM_100,		{  1,   1},	true},
	{CM_ZOOM_150,		{  3,   2},	true},
	{CM_ZOOM_200,		{  2,   1},	true},
	{CM_ZOOM_250,		{  5,   2},	f1Seg},
	{CM_ZOOM_300,		{  3,   1},	f1Seg},
	{CM_CUSTOMZOOM_1,	{100, 100},	false},
	{CM_CUSTOMZOOM_2,	{100, 100},	false},
	{CM_CUSTOMZOOM_3,	{100, 100},	false},
	{CM_CUSTOMZOOM_4,	{100, 100},	false},
	{CM_CUSTOMZOOM_5,	{100, 100},	false},
};


CZoomOptions::CZoomOptions(const CCommandList *pCommandList)
	: CCommandCustomizer(CM_CUSTOMZOOM_FIRST,CM_CUSTOMZOOM_LAST)
	, m_pCommandList(pCommandList)
{
	for (int i=0;i<NUM_ZOOM;i++)
		m_Order[i]=i;
}


CZoomOptions::~CZoomOptions()
{
}


bool CZoomOptions::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_ZOOMOPTIONS))==IDOK;
}


bool CZoomOptions::ReadSettings(CSettings &Settings)
{
	TCHAR szText[256];

	int j=CM_ZOOM_LAST-CM_ZOOM_FIRST+1;
	for (int i=0;i<=CM_CUSTOMZOOM_LAST-CM_CUSTOMZOOM_FIRST;i++,j++) {
		int Rate;

		::wsprintf(szText,TEXT("CustomZoom%d"),i+1);
		if (Settings.Read(szText,&Rate) && Rate>0 && Rate<=MAX_RATE)
			m_ZoomList[j].Zoom.Rate=Rate;
	}

	int ListCount;
	if (Settings.Read(TEXT("ZoomListCount"),&ListCount) && ListCount>0) {
		if (ListCount>NUM_ZOOM)
			ListCount=NUM_ZOOM;
		int Count=0;
		for (int i=0;i<ListCount;i++) {
			TCHAR szName[32];

			::wsprintf(szName,TEXT("ZoomList%d"),i);
			if (Settings.Read(szName,szText,lengthof(szText))) {
				LPTSTR p=szText;
				while (*p!=_T('\0') && *p!=_T(','))
					p++;
				if (*p==_T(','))
					*p++=_T('\0');
				int Command=m_pCommandList->ParseText(szText);
				if (Command!=0) {
					for (j=0;j<NUM_ZOOM;j++) {
						if (m_ZoomList[j].Command==Command)
							break;
					}
					if (j<NUM_ZOOM) {
						int k;
						for (k=0;k<Count;k++) {
							if (m_Order[k]==j)
								break;
						}
						if (k==Count) {
							m_Order[Count]=j;
							m_ZoomList[j].fVisible=*p!=_T('0');
							Count++;
						}
					}
				}
			}
		}
		if (Count<NUM_ZOOM) {
			for (int i=0;i<NUM_ZOOM;i++) {
				for (j=0;j<Count;j++) {
					if (m_Order[j]==i)
						break;
				}
				if (j==Count)
					m_Order[Count++]=i;
			}
		}
	}
	return true;
}


bool CZoomOptions::WriteSettings(CSettings &Settings)
{
	TCHAR szText[256];

	int j=CM_ZOOM_LAST-CM_ZOOM_FIRST+1;
	for (int i=0;i<=CM_CUSTOMZOOM_LAST-CM_CUSTOMZOOM_FIRST;i++,j++) {
		::wsprintf(szText,TEXT("CustomZoom%d"),i+1);
		Settings.Write(szText,m_ZoomList[j].Zoom.Rate);
	}

	Settings.Write(TEXT("ZoomListCount"),NUM_ZOOM);
	for (int i=0;i<NUM_ZOOM;i++) {
		const ZoomInfo *pInfo=&m_ZoomList[m_Order[i]];
		TCHAR szName[32];

		::wsprintf(szName,TEXT("ZoomList%d"),i);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%s,%d"),
						  m_pCommandList->GetCommandTextByID(pInfo->Command),pInfo->fVisible?1:0);
		Settings.Write(szName,szText);
	}
	return true;
}


bool CZoomOptions::SetMenu(HMENU hmenu,const ZoomRate *pCurRate) const
{
	for (int i=::GetMenuItemCount(hmenu)-3;i>=0;i--)
		::DeleteMenu(hmenu,i,MF_BYPOSITION);
	int Pos=0;
	bool fCheck=false;
	for (int i=0;i<NUM_ZOOM;i++) {
		const ZoomInfo *pInfo=&m_ZoomList[m_Order[i]];
		if (pInfo->fVisible) {
			TCHAR szText[64];
			UINT Flags=MF_BYPOSITION | MF_STRING | MF_ENABLED;

			if (!fCheck && pCurRate!=NULL
					&& pCurRate->GetPercentage()==pInfo->Zoom.GetPercentage()) {
				Flags|=MF_CHECKED;
				fCheck=true;
			}
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%d%%"),
							  pInfo->Zoom.GetPercentage());
			::InsertMenu(hmenu,Pos++,Flags,pInfo->Command,szText);
		}
	}
	return true;
}


bool CZoomOptions::GetZoomRateByCommand(int Command,ZoomRate *pRate) const
{
	for (int i=0;i<NUM_ZOOM;i++) {
		if (m_ZoomList[i].Command==Command) {
			*pRate=m_ZoomList[i].Zoom;
			return true;
		}
	}
	return false;
}


void CZoomOptions::SetItemState(HWND hDlg)
{
	HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
	int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

	if (Sel>=0) {
		LVITEM lvi;

		lvi.mask=LVIF_PARAM;
		lvi.iItem=Sel;
		lvi.iSubItem=0;
		ListView_GetItem(hwndList,&lvi);
		::SetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_RATE,
			GET_LPARAM_RATE(lvi.lParam)*100/GET_LPARAM_FACTOR(lvi.lParam),TRUE);
		int Index=GET_LPARAM_INDEX(lvi.lParam);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_RATE,
						m_ZoomList[Index].Command>=CM_CUSTOMZOOM_FIRST
							&& m_ZoomList[Index].Command<=CM_CUSTOMZOOM_LAST);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_UP,Sel>0);
		::EnableDlgItem(hDlg,IDC_ZOOMOPTIONS_DOWN,Sel+1<NUM_ZOOM);
	} else {
		::SetDlgItemText(hDlg,IDC_ZOOMOPTIONS_RATE,TEXT(""));
		::EnableDlgItems(hDlg,IDC_ZOOMOPTIONS_RATE,IDC_ZOOMOPTIONS_DOWN,FALSE);
	}
}

INT_PTR CZoomOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			m_fChanging=true;
			HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
			ListView_SetExtendedListViewStyle(hwndList,
											  LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
			RECT rc;
			GetClientRect(hwndList,&rc);
			rc.right-=GetSystemMetrics(SM_CXHSCROLL);
			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=rc.right;
			lvc.pszText=TEXT("");
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);
			LVITEM lvi;
			TCHAR szText[64];
			lvi.mask=LVIF_TEXT | LVIF_PARAM;
			lvi.iSubItem=0;
			lvi.pszText=szText;
			for (int i=0;i<NUM_ZOOM;i++) {
				const ZoomInfo *pInfo=&m_ZoomList[m_Order[i]];
				lvi.iItem=i;
				lvi.lParam=PACK_LPARAM(m_Order[i],pInfo->Zoom.Rate,pInfo->Zoom.Factor);
				m_pCommandList->GetCommandNameByID(pInfo->Command,szText,lengthof(szText));
				ListView_InsertItem(hwndList,&lvi);
				ListView_SetCheckState(hwndList,i,pInfo->fVisible);
			}
			SetItemState(hDlg);
			m_fChanging=false;
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ZOOMOPTIONS_RATE:
			if (HIWORD(wParam)==EN_CHANGE) {
				int Rate=::GetDlgItemInt(hDlg,IDC_ZOOMOPTIONS_RATE,NULL,TRUE);

				if (Rate>0 && Rate<=MAX_RATE) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
					int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

					if (Sel>=0) {
						LVITEM lvi;
						lvi.mask=LVIF_PARAM;
						lvi.iItem=Sel;
						lvi.iSubItem=0;
						ListView_GetItem(hwndList,&lvi);
						int Index=GET_LPARAM_INDEX(lvi.lParam);
						if (m_ZoomList[Index].Command>=CM_CUSTOMZOOM_FIRST
								&& m_ZoomList[Index].Command<=CM_CUSTOMZOOM_LAST) {
							TCHAR szText[64];

							m_fChanging=true;
							::LoadString(GetAppClass().GetResourceInstance(),m_ZoomList[Index].Command,szText,lengthof(szText));
							::wsprintf(szText+::lstrlen(szText),TEXT(" : %d%%"),Rate);
							lvi.mask=LVIF_TEXT | LVIF_PARAM;
							lvi.pszText=szText;
							lvi.lParam=PACK_LPARAM(Index,Rate,100);
							ListView_SetItem(hwndList,&lvi);
							m_fChanging=false;
						}
					}
				}
			}
			return TRUE;

		case IDC_ZOOMOPTIONS_UP:
		case IDC_ZOOMOPTIONS_DOWN:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
				int From=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED),To;

				if (From>=0) {
					m_fChanging=true;
					if (LOWORD(wParam)==IDC_ZOOMOPTIONS_UP) {
						if (From<1)
							break;
						To=From-1;
					} else {
						if (From+1>=NUM_ZOOM)
							break;
						To=From+1;
					}
					LVITEM lvi;
					TCHAR szText[64];
					lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_PARAM;
					lvi.iItem=From;
					lvi.iSubItem=0;
					lvi.stateMask=(UINT)-1;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					BOOL fChecked=ListView_GetCheckState(hwndList,From);
					ListView_DeleteItem(hwndList,From);
					lvi.iItem=To;
					ListView_InsertItem(hwndList,&lvi);
					ListView_SetCheckState(hwndList,To,fChecked);
					SetItemState(hDlg);
					m_fChanging=false;
				}
			}
			return TRUE;

		case IDOK:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_ZOOMOPTIONS_LIST);
				LVITEM lvi;

				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (int i=0;i<NUM_ZOOM;i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					int Index=GET_LPARAM_INDEX(lvi.lParam);
					m_Order[i]=Index;
					if (m_ZoomList[Index].Command>=CM_CUSTOMZOOM_FIRST
							&& m_ZoomList[Index].Command<=CM_CUSTOMZOOM_LAST) {
						m_ZoomList[Index].Zoom.Rate=GET_LPARAM_RATE(lvi.lParam);
						m_ZoomList[Index].Zoom.Factor=GET_LPARAM_FACTOR(lvi.lParam);
					}
					m_ZoomList[Index].fVisible=ListView_GetCheckState(hwndList,i)!=FALSE;
				}
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			if (!m_fChanging)
				SetItemState(hDlg);
			return TRUE;
		}
		break;
	}

	return FALSE;
}


bool CZoomOptions::GetCommandName(int Command,LPTSTR pszName,int MaxLength)
{
	ZoomRate Zoom;
	if (!GetZoomRateByCommand(Command,&Zoom))
		return false;
	int Length=::LoadString(GetAppClass().GetResourceInstance(),Command,pszName,MaxLength);
	StdUtil::snprintf(pszName+Length,MaxLength-Length,TEXT(" : %d%%"),Zoom.GetPercentage());
	return true;
}
