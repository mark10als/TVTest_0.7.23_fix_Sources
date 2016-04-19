#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ChannelScan.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// 信号レベルを考慮する場合の閾値
#define SIGNAL_LEVEL_THRESHOLD	7.0f

// スキャンスレッドから送られるメッセージ
#define WM_APP_BEGINSCAN	(WM_APP+0)
#define WM_APP_CHANNELFOUND	(WM_APP+1)
#define WM_APP_ENDSCAN		(WM_APP+2)

enum ScanResult
{
	SCAN_RESULT_SUCCEEDED,
	SCAN_RESULT_CANCELLED,
	SCAN_RESULT_SET_CHANNEL_PARTIALLY_FAILED,
	SCAN_RESULT_SET_CHANNEL_TIMEOUT
};




class CChannelListSort {
	static int CALLBACK CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
public:
	enum SortType {
		SORT_NAME,
		SORT_CHANNEL,
		SORT_CHANNELNAME,
		SORT_SERVICEID,
		SORT_REMOTECONTROLKEYID
	};
	CChannelListSort();
	CChannelListSort(SortType Type,bool fDescending=false);
	void Sort(HWND hwndList);
	bool UpdateChannelList(HWND hwndList,CChannelList *pList);
	SortType m_Type;
	bool m_fDescending;
};


CChannelListSort::CChannelListSort()
	: m_Type(SORT_CHANNEL)
	, m_fDescending(false)
{
}


CChannelListSort::CChannelListSort(SortType Type,bool fDescending)
	: m_Type(Type)
	, m_fDescending(fDescending)
{
}


int CALLBACK CChannelListSort::CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CChannelListSort *pThis=reinterpret_cast<CChannelListSort*>(lParamSort);
	CChannelInfo *pChInfo1=reinterpret_cast<CChannelInfo*>(lParam1);
	CChannelInfo *pChInfo2=reinterpret_cast<CChannelInfo*>(lParam2);
	int Cmp;

	switch (pThis->m_Type) {
	case SORT_NAME:
		Cmp=::lstrcmpi(pChInfo1->GetName(),pChInfo2->GetName());
		if (Cmp==0)
			Cmp=::lstrcmp(pChInfo1->GetName(),pChInfo2->GetName());
		break;

	case SORT_CHANNEL:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		break;

	case SORT_CHANNELNAME:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		break;

	case SORT_SERVICEID:
		{
			NetworkType Ch1Network=GetNetworkType(pChInfo1->GetNetworkID());
			NetworkType Ch2Network=GetNetworkType(pChInfo2->GetNetworkID());
			if (Ch1Network!=Ch2Network) {
				Cmp=Ch1Network-Ch2Network;
			} else {
				Cmp=pChInfo1->GetServiceID()-pChInfo2->GetServiceID();
			}
		}
		break;

	case SORT_REMOTECONTROLKEYID:
		Cmp=pChInfo1->GetChannelNo()-pChInfo2->GetChannelNo();
		break;

	default:
		Cmp=0;
	}

	return pThis->m_fDescending?-Cmp:Cmp;
}


void CChannelListSort::Sort(HWND hwndList)
{
	ListView_SortItems(hwndList,CompareFunc,reinterpret_cast<LPARAM>(this));
}


bool CChannelListSort::UpdateChannelList(HWND hwndList,CChannelList *pList)
{
	int Count=ListView_GetItemCount(hwndList);
	LV_ITEM lvi;
	CChannelList ChannelList;
	int i;

	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;
	for (i=0;i<Count;i++) {
		lvi.iItem=i;
		ListView_GetItem(hwndList,&lvi);
		ChannelList.AddChannel(*reinterpret_cast<CChannelInfo*>(lvi.lParam));
	}
	*pList=ChannelList;
	for (i=0;i<Count;i++) {
		lvi.iItem=i;
		lvi.lParam=reinterpret_cast<LPARAM>(pList->GetChannelInfo(i));
		ListView_SetItem(hwndList,&lvi);
	}
	return true;
}




CChannelScan::CChannelScan(CCoreEngine *pCoreEngine)
	: m_pCoreEngine(pCoreEngine)
	, m_pOriginalTuningSpaceList(NULL)
	, m_fScanService(false)
	, m_fIgnoreSignalLevel(false)	// 信号レベルを無視
	, m_ScanWait(5000)				// チャンネル切り替え後の待ち時間(ms)
	, m_RetryCount(4)				// 情報取得の再試行回数
	, m_RetryInterval(1000)			// 再試行の間隔(ms)
	, m_hScanDlg(NULL)
	, m_hScanThread(NULL)
	, m_hCancelEvent(NULL)
	, m_fChanging(false)
{
}


CChannelScan::~CChannelScan()
{
	Destroy();
}


bool CChannelScan::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();

	if ((Flags&UPDATE_CHANNELLIST)!=0) {
		AppMain.UpdateChannelList(&m_TuningSpaceList);
	}

	if ((Flags&UPDATE_PREVIEW)!=0) {
		if (m_fRestorePreview)
			AppMain.GetUICore()->EnableViewer(true);
	}

	return true;
}


bool CChannelScan::ReadSettings(CSettings &Settings)
{
	Settings.Read(TEXT("ChannelScanIgnoreSignalLevel"),&m_fIgnoreSignalLevel);
	Settings.Read(TEXT("ChannelScanWait"),&m_ScanWait);
	Settings.Read(TEXT("ChannelScanRetry"),&m_RetryCount);
	return true;
}


bool CChannelScan::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("ChannelScanIgnoreSignalLevel"),m_fIgnoreSignalLevel);
	Settings.Write(TEXT("ChannelScanWait"),m_ScanWait);
	Settings.Write(TEXT("ChannelScanRetry"),m_RetryCount);
	return true;
}


bool CChannelScan::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_CHANNELSCAN));
}


bool CChannelScan::SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList)
{
	m_pOriginalTuningSpaceList=pTuningSpaceList;
	return true;
}


void CChannelScan::InsertChannelInfo(int Index,const CChannelInfo *pChInfo)
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST);
	LV_ITEM lvi;
	TCHAR szText[256];

	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.pszText=const_cast<LPTSTR>(pChInfo->GetName());
	lvi.lParam=reinterpret_cast<LPARAM>(pChInfo);
	lvi.iItem=ListView_InsertItem(hwndList,&lvi);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelIndex());
	lvi.pszText=szText;
	ListView_SetItem(hwndList,&lvi);
	lvi.iSubItem=2;
	LPCTSTR pszChannelName=m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetChannelName(pChInfo->GetSpace(),pChInfo->GetChannelIndex());
	::lstrcpyn(szText,!IsStringEmpty(pszChannelName)?pszChannelName:TEXT("\?\?\?"),lengthof(szText));
	ListView_SetItem(hwndList,&lvi);
	lvi.iSubItem=3;
	::wsprintf(szText,TEXT("%d"),pChInfo->GetServiceID());
	ListView_SetItem(hwndList,&lvi);
	if (pChInfo->GetChannelNo()>0) {
		lvi.iSubItem=4;
		::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelNo());
		lvi.pszText=szText;
		ListView_SetItem(hwndList,&lvi);
	}
	ListView_SetCheckState(hwndList,lvi.iItem,pChInfo->IsEnabled());
}


void CChannelScan::SetChannelList(int Space)
{
	const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);

	ListView_DeleteAllItems(::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST));
	if (pChannelList==NULL)
		return;
	m_fChanging=true;
	for (int i=0;i<pChannelList->NumChannels();i++)
		InsertChannelInfo(i,pChannelList->GetChannelInfo(i));
	m_fChanging=false;
}


CChannelInfo *CChannelScan::GetSelectedChannelInfo() const
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST);
	LV_ITEM lvi;

	lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (lvi.iItem>=0) {
		lvi.mask=LVIF_PARAM;
		lvi.iSubItem=0;
		if (ListView_GetItem(hwndList,&lvi))
			return reinterpret_cast<CChannelInfo*>(lvi.lParam);
	}
	return NULL;
}


bool CChannelScan::LoadPreset(LPCTSTR pszFileName,CChannelList *pChannelList,int Space,bool *pfCorrupted)
{
	TCHAR szPresetFileName[MAX_PATH];
	CTuningSpaceList PresetList;

	GetAppClass().GetAppDirectory(szPresetFileName);
	::PathAppend(szPresetFileName,pszFileName);
	if (!PresetList.LoadFromFile(szPresetFileName))
		return false;

	CBonSrcDecoder &BonSrcDecoder=m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder;
	std::vector<CDynamicString> BonDriverChannelList;
	LPCTSTR pszName;
	for (int i=0;(pszName=BonSrcDecoder.GetChannelName(Space,i))!=NULL;i++) {
		BonDriverChannelList.push_back(CDynamicString(pszName));
	}
	if (BonDriverChannelList.empty())
		return false;

	bool fCorrupted=false;
	CChannelList *pPresetChannelList=PresetList.GetAllChannelList();
	for (int i=0;i<pPresetChannelList->NumChannels();i++) {
		CChannelInfo ChannelInfo(*pPresetChannelList->GetChannelInfo(i));

		LPCTSTR pszChannelName=ChannelInfo.GetName(),pszDelimiter;
		if (pszChannelName[0]==_T('%')
				&& (pszDelimiter=::StrChr(pszChannelName+1,_T(' ')))!=NULL) {
			int TSNameLength=(int)(pszDelimiter-pszChannelName)-1;
			TCHAR szName[MAX_CHANNEL_NAME];
			::lstrcpyn(szName,pszChannelName+1,TSNameLength+1);
			bool fFound=false;
			for (size_t j=0;j<BonDriverChannelList.size();j++) {
				pszName=BonDriverChannelList[j].Get();
				LPCTSTR p=::StrStrI(pszName,szName);
				if (p!=NULL && !::IsCharAlphaNumeric(p[TSNameLength])) {
					ChannelInfo.SetChannelIndex((int)j);
					fFound=true;
					break;
				}
			}
			if (!fFound) {
				if (ChannelInfo.GetChannelIndex()>=(int)BonDriverChannelList.size())
					continue;
				fCorrupted=true;
			}
			::lstrcpy(szName,pszDelimiter+1);
			ChannelInfo.SetName(szName);
		} else {
			if (ChannelInfo.GetChannelIndex()>=(int)BonDriverChannelList.size()) {
				fCorrupted=true;
				continue;
			}
		}

		ChannelInfo.SetSpace(Space);
		pChannelList->AddChannel(ChannelInfo);
	}

	*pfCorrupted=fCorrupted;

	return true;
}


bool CChannelScan::SetPreset(bool fAuto)
{
	LPCTSTR pszName=m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(m_ScanSpace);
	if (pszName==NULL)
		return false;

	bool fBS=::StrStrI(pszName,TEXT("BS"))!=NULL;
	bool fCS=::StrStrI(pszName,TEXT("CS"))!=NULL;
	if (fBS==fCS)
		return false;

	CTuningSpaceInfo *pTuningSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo(m_ScanSpace);
	if (pTuningSpaceInfo==NULL)
		return false;

	CChannelList *pChannelList=pTuningSpaceInfo->GetChannelList();
	if (pChannelList==NULL)
		return false;

	bool fResult,fCorrupted;
	CChannelList NewChannelList;
	if (fBS)
		fResult=LoadPreset(TEXT("Preset_BS.ch2"),&NewChannelList,m_ScanSpace,&fCorrupted);
	else
		fResult=LoadPreset(TEXT("Preset_CS.ch2"),&NewChannelList,m_ScanSpace,&fCorrupted);
	if (!fResult)
		return false;

	if (fCorrupted) {
		if (fAuto)
			return false;
		if (::MessageBox(m_hDlg,
						 TEXT("この BonDriver はプリセットとチャンネルが異なっている可能性があります。\n")
						 TEXT("プリセットを使用せずに、スキャンを行うことをお勧めします。\n")
						 TEXT("プリセットを読み込みますか？"),
						 TEXT("プリセット読み込み"),
						 MB_YESNO | MB_DEFBUTTON2 | MB_ICONINFORMATION)!=IDYES)
			return false;
	}

	*pChannelList=NewChannelList;
	pTuningSpaceInfo->SetName(pszName);
	m_fUpdated=true;
	SetChannelList(m_ScanSpace);
	m_SortColumn=-1;
	SetListViewSortMark(::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST),-1);

	return true;
}


INT_PTR CChannelScan::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			int NumSpaces;
			LPCTSTR pszName;

			m_TuningSpaceList=*m_pOriginalTuningSpaceList;
			for (NumSpaces=0;(pszName=m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(NumSpaces))!=NULL;NumSpaces++) {
				::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_ADDSTRING,0,
									 reinterpret_cast<LPARAM>(pszName));
			}
			if (NumSpaces>0) {
				m_ScanSpace=0;
				::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_SETCURSEL,m_ScanSpace,0);
				m_TuningSpaceList.Reserve(NumSpaces);
			} else {
				m_ScanSpace=-1;
				EnableDlgItems(hDlg,IDC_CHANNELSCAN_SPACE,IDC_CHANNELSCAN_START,false);
			}

			m_fUpdated=false;
			m_fScaned=false;
			m_fRestorePreview=false;
			m_SortColumn=-1;

			HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES | LVS_EX_LABELTIP);

			LV_COLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=128;
			lvc.pszText=TEXT("名前");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.fmt=LVCFMT_RIGHT;
			lvc.cx=40;
			lvc.pszText=TEXT("番号");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvc.cx=72;
			lvc.pszText=TEXT("チャンネル");
			ListView_InsertColumn(hwndList,2,&lvc);
			lvc.cx=72;
			lvc.pszText=TEXT("サービスID");
			ListView_InsertColumn(hwndList,3,&lvc);
			lvc.cx=80;
			lvc.pszText=TEXT("リモコンキー");
			ListView_InsertColumn(hwndList,4,&lvc);
			if (NumSpaces>0) {
				//SetChannelList(hDlg,m_ScanSpace);
				::SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_CHANNELSCAN_SPACE,CBN_SELCHANGE),0);
				/*
				for (i=0;i<5;i++)
					ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
				*/
			}

			DlgCheckBox_Check(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL,m_fIgnoreSignalLevel);

			TCHAR szText[8];
			for (int i=1;i<=10;i++) {
				wsprintf(szText,TEXT("%d 秒"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELSCAN_SCANWAIT,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_CHANNELSCAN_SCANWAIT,m_ScanWait/1000-1);
			for (int i=0;i<=10;i++) {
				wsprintf(szText,TEXT("%d 秒"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELSCAN_RETRYCOUNT,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_CHANNELSCAN_RETRYCOUNT,m_RetryCount);

			if (GetAppClass().GetCoreEngine()->IsNetworkDriver())
				EnableDlgItems(hDlg,IDC_CHANNELSCAN_FIRST,IDC_CHANNELSCAN_LAST,false);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELSCAN_SPACE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				const int Space=(int)::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_GETCURSEL,0,0);
				if (Space<0)
					return TRUE;

				m_ScanSpace=Space;
				LPCTSTR pszName=m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(Space);
				bool fBS=false,fCS=false;
				if (pszName!=NULL) {
					fBS=::StrStrI(pszName,TEXT("BS"))!=NULL;
					fCS=::StrStrI(pszName,TEXT("CS"))!=NULL;
				}
				CTuningSpaceInfo *pTuningSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo(Space);
				CChannelList *pChannelList=NULL;
				if (pTuningSpaceInfo!=NULL)
					pChannelList=pTuningSpaceInfo->GetChannelList();
				if (fBS!=fCS && pChannelList!=NULL && pChannelList->NumChannels()==0) {
					SetPreset(true);
				} else {
					SetChannelList(Space);
				}
				DlgCheckBox_Check(hDlg,IDC_CHANNELSCAN_SCANSERVICE,
								  fBS || fCS || (pChannelList!=NULL && pChannelList->HasMultiService()));
				EnableDlgItem(hDlg,IDC_CHANNELSCAN_LOADPRESET,fBS || fCS);
				m_SortColumn=-1;
				SetListViewSortMark(::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST),-1);
			}
			return TRUE;

		case IDC_CHANNELSCAN_LOADPRESET:
			SetPreset(false);
			return TRUE;

		case IDC_CHANNELSCAN_START:
			{
				int Space=(int)::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_GETCURSEL,0,0);

				if (Space>=0) {
					HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);

					if (GetAppClass().GetUICore()->IsViewerEnabled()) {
						GetAppClass().GetUICore()->EnableViewer(false);
						m_fRestorePreview=true;
					}
					m_ScanSpace=Space;
					m_ScanChannel=0;
					m_fScanService=
						DlgCheckBox_IsChecked(hDlg,IDC_CHANNELSCAN_SCANSERVICE);
					m_fIgnoreSignalLevel=
						DlgCheckBox_IsChecked(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL);
					m_ScanWait=((unsigned int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_SCANWAIT)+1)*1000;
					m_RetryCount=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_RETRYCOUNT);
					ListView_DeleteAllItems(hwndList);
					if (::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_CHANNELSCAN),::GetParent(hDlg),
							ScanDlgProc,reinterpret_cast<LPARAM>(this))==IDOK) {
						if (ListView_GetItemCount(hwndList)>0) {
							// 元あったチャンネルで検出されなかったものがある場合、残すか問い合わせる
							CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);
							std::vector<CChannelInfo*> NotFoundChannels;
							for (int i=0;i<pChannelList->NumChannels();i++) {
								CChannelInfo *pOldChannel=pChannelList->GetChannelInfo(i);
								bool fFound=false;
								for (int j=0;j<m_ScanningChannelList.NumChannels();j++) {
									CChannelInfo *pNewChannel=m_ScanningChannelList.GetChannelInfo(j);
									if (/*pNewChannel->GetNetworkID()==pOldChannel->GetNetworkID()
											&& */pNewChannel->GetTransportStreamID()==pOldChannel->GetTransportStreamID()
											&& pNewChannel->GetServiceID()==pOldChannel->GetServiceID()) {
										fFound=true;
										break;
									}
								}
								if (!fFound)
									NotFoundChannels.push_back(pOldChannel);
							}
							if (NotFoundChannels.size()>0) {
								const int Channels=(int)NotFoundChannels.size();
								TCHAR szMessage[256+(MAX_CHANNEL_NAME+2)*10];
								CStaticStringFormatter Formatter(szMessage,lengthof(szMessage));

								Formatter.AppendFormat(
									TEXT("元あったチャンネルのうち、以下の%d%sのチャンネルが検出されませんでした。\n\n"),
									Channels,Channels<10?TEXT("つ"):TEXT(""));
								for (int i=0;i<Channels;i++) {
									if (i==10) {
										Formatter.Append(TEXT("...\n"));
										break;
									}
									Formatter.AppendFormat(TEXT("・%s\n"),NotFoundChannels[i]->GetName());
								}
								Formatter.Append(TEXT("\n検出されなかったチャンネルを残しますか？"));
								if (::MessageBox(hDlg,Formatter.GetString(),TEXT("問い合わせ"),
												 MB_YESNO | MB_ICONQUESTION)==IDYES) {
									for (size_t i=0;i<NotFoundChannels.size();i++) {
										CChannelInfo *pInfo=new CChannelInfo(*NotFoundChannels[i]);
										m_ScanningChannelList.AddChannel(pInfo);
										InsertChannelInfo(ListView_GetItemCount(hwndList),pInfo);
									}
								}
							}

							CChannelListSort::SortType SortType;
							if (m_ScanningChannelList.HasRemoteControlKeyID()
									&& m_ScanningChannelList.GetChannelInfo(0)->GetNetworkID()>10)
								SortType=CChannelListSort::SORT_REMOTECONTROLKEYID;
							else
								SortType=CChannelListSort::SORT_SERVICEID;
							CChannelListSort ListSort(SortType);
							ListSort.Sort(hwndList);
							ListSort.UpdateChannelList(hwndList,m_TuningSpaceList.GetChannelList(Space));
							if (IsStringEmpty(m_TuningSpaceList.GetTuningSpaceName(Space))) {
								m_TuningSpaceList.GetTuningSpaceInfo(Space)->SetName(
									m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(Space));
							}
							m_SortColumn=(int)SortType;
							m_fSortDescending=false;
							SetListViewSortMark(hwndList,m_SortColumn,!m_fSortDescending);
							ListView_EnsureVisible(hwndList,0,FALSE);
						} else {
							// チャンネルが検出できなかった
							TCHAR szText[1024];

							::lstrcpy(szText,TEXT("チャンネルが検出できませんでした。"));
							if ((m_fIgnoreSignalLevel
										&& m_MaxSignalLevel<1.0f)
									|| (!m_fIgnoreSignalLevel
										&& m_MaxSignalLevel<SIGNAL_LEVEL_THRESHOLD)) {
								::lstrcat(szText,TEXT("\n信号レベルが取得できないか、低すぎます。"));
								if (!m_fIgnoreSignalLevel
										&& m_MaxBitRate>8000000)
									::lstrcat(szText,TEXT("\n[信号レベルを無視する] をチェックしてスキャンしてみてください。"));
							} else if (m_MaxBitRate<8000000) {
								::lstrcat(szText,TEXT("\nストリームを受信できません。"));
							}
							::MessageBox(hDlg,szText,TEXT("スキャン結果"),MB_OK | MB_ICONEXCLAMATION);
						}
						m_fUpdated=true;
					} else {
						SetChannelList(Space);
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_PROPERTIES:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
				LV_ITEM lvi;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem>=0) {
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					ListView_GetItem(hwndList,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					if (::DialogBoxParam(GetAppClass().GetResourceInstance(),
										 MAKEINTRESOURCE(IDD_CHANNELPROPERTIES),
										 hDlg,ChannelPropDlgProc,
										 reinterpret_cast<LPARAM>(pChInfo))==IDOK) {
						lvi.mask=LVIF_TEXT;
						lvi.pszText=const_cast<LPTSTR>(pChInfo->GetName());
						ListView_SetItem(hwndList,&lvi);
						lvi.iSubItem=4;
						TCHAR szText[16];
						if (pChInfo->GetChannelNo()>0)
							::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelNo());
						else
							szText[0]='\0';
						lvi.pszText=szText;
						ListView_SetItem(hwndList,&lvi);
						m_fUpdated=true;
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_DELETE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
				LV_ITEM lvi;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem>=0) {
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					ListView_GetItem(hwndList,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					CChannelList *pList=m_TuningSpaceList.GetChannelList(m_ScanSpace);
					if (pList!=NULL) {
						int Index=pList->Find(pChInfo);
						if (Index>=0) {
							ListView_DeleteItem(hwndList,lvi.iItem);
							pList->DeleteChannel(Index);
							m_fUpdated=true;
						}
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_COLUMNCLICK:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (m_SortColumn==pnmlv->iSubItem) {
					m_fSortDescending=!m_fSortDescending;
				} else {
					m_SortColumn=pnmlv->iSubItem;
					m_fSortDescending=false;
				}
				CChannelListSort ListSort(
								(CChannelListSort::SortType)pnmlv->iSubItem,
								m_fSortDescending);
				ListSort.Sort(pnmlv->hdr.hwndFrom);
				ListSort.UpdateChannelList(pnmlv->hdr.hwndFrom,
					m_TuningSpaceList.GetChannelList(m_ScanSpace));
				SetListViewSortMark(pnmlv->hdr.hwndFrom,pnmlv->iSubItem,!m_fSortDescending);
			}
			return TRUE;

		case NM_RCLICK:
			if (((LPNMHDR)lParam)->hwndFrom==::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST)) {
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0) {
					HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),
											MAKEINTRESOURCE(IDM_CHANNELSCAN));
					POINT pt;

					::GetCursorPos(&pt);
					::TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,
														pt.x,pt.y,0,hDlg,NULL);
					::DestroyMenu(hmenu);
				}
			}
			break;

		case NM_DBLCLK:
			if (((LPNMHDR)lParam)->hwndFrom==::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST)) {
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0)
					::SendMessage(hDlg,WM_COMMAND,IDC_CHANNELSCAN_PROPERTIES,0);
			}
			break;

		case LVN_ENDLABELEDIT:
			{
				NMLVDISPINFO *plvdi=reinterpret_cast<NMLVDISPINFO*>(lParam);
				BOOL fResult;

				if (plvdi->item.pszText!=NULL && plvdi->item.pszText[0]!='\0') {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=plvdi->item.iItem;
					lvi.iSubItem=0;
					ListView_GetItem(plvdi->hdr.hwndFrom,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->SetName(plvdi->item.pszText);
					m_fUpdated=true;
					fResult=TRUE;
				} else {
					fResult=FALSE;
				}
				::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,fResult);
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			if (!m_fChanging) {
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (pnmlv->iItem>=0
						&& (pnmlv->uNewState&LVIS_STATEIMAGEMASK)!=
						   (pnmlv->uOldState&LVIS_STATEIMAGEMASK)) {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=pnmlv->iItem;
					lvi.iSubItem=0;
					ListView_GetItem(pnmlv->hdr.hwndFrom,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->Enable((pnmlv->uNewState&LVIS_STATEIMAGEMASK)==INDEXTOSTATEIMAGEMASK(2));
					m_fUpdated=true;
				}
			}
			return TRUE;

		case PSN_APPLY:
			{
				if (m_fUpdated) {
					m_TuningSpaceList.MakeAllChannelList();
					TCHAR szFileName[MAX_PATH];
					if (!GetAppClass().GetChannelManager()->GetChannelFileName(szFileName,lengthof(szFileName))
							|| ::lstrcmpi(::PathFindExtension(szFileName),CHANNEL_FILE_EXTENSION)!=0
							|| !::PathFileExists(szFileName)) {
						m_pCoreEngine->GetDriverPath(szFileName);
						::PathRenameExtension(szFileName,CHANNEL_FILE_EXTENSION);
					}
					m_TuningSpaceList.SaveToFile(szFileName);
					GetAppClass().AddLog(TEXT("チャンネルファイルを \"%s\" に保存しました。"),szFileName);
					SetUpdateFlag(UPDATE_CHANNELLIST);
				}

				if (m_fRestorePreview)
					SetUpdateFlag(UPDATE_PREVIEW);

				m_fIgnoreSignalLevel=
					DlgCheckBox_IsChecked(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL);
				m_ScanWait=((unsigned int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_SCANWAIT)+1)*1000;
				m_RetryCount=(int)DlgComboBox_GetCurSel(hDlg,IDC_CHANNELSCAN_RETRYCOUNT);

				m_fChanged=true;
			}
			return TRUE;

		case PSN_RESET:
			if (m_fRestorePreview)
				//SetUpdateFlag(UPDATE_PREVIEW);
				GetAppClass().GetUICore()->EnableViewer(true);
			return TRUE;
		}
		break;

	case WM_APP_CHANNELFOUND:
		{
			const CChannelInfo *pChInfo=reinterpret_cast<const CChannelInfo*>(lParam);
			HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
			int Index=ListView_GetItemCount(hwndList);

			m_fChanging=true;
			InsertChannelInfo(Index,pChInfo);
			ListView_EnsureVisible(hwndList,Index,FALSE);
			m_fChanging=false;
			::UpdateWindow(hwndList);
		}
		return TRUE;
	}

	return FALSE;
}


CChannelScan *CChannelScan::GetThis(HWND hDlg)
{
	return static_cast<CChannelScan*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CChannelScan::ScanDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CChannelScan *pThis=reinterpret_cast<CChannelScan*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hScanDlg=hDlg;

			CBonSrcDecoder &BonSrcDecoder=pThis->m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder;
			pThis->m_BonDriverChannelList.clear();
			if (pThis->m_ScanChannel>0)
				pThis->m_BonDriverChannelList.resize(pThis->m_ScanChannel);
			LPCTSTR pszName;
			for (int i=pThis->m_ScanChannel;(pszName=BonSrcDecoder.GetChannelName(pThis->m_ScanSpace,i))!=NULL;i++) {
				pThis->m_BonDriverChannelList.push_back(TVTest::String(pszName));
			}

			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETRANGE32,
								 pThis->m_ScanChannel,pThis->m_BonDriverChannelList.size());
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETPOS,
								 pThis->m_ScanChannel,0);

			GetAppClass().BeginChannelScan(pThis->m_ScanSpace);

			pThis->m_fCancelled=false;
			pThis->m_hCancelEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
			pThis->m_hScanThread=(HANDLE)::_beginthreadex(NULL,0,ScanProc,pThis,0,NULL);
			pThis->m_fScaned=true;

			::SetTimer(hDlg,1,1000,NULL);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			{
				CChannelScan *pThis=GetThis(hDlg);

				pThis->m_fCancelled=LOWORD(wParam)==IDCANCEL;
				::SetEvent(pThis->m_hCancelEvent);
				::EnableDlgItem(hDlg,IDOK,false);
				::EnableDlgItem(hDlg,IDCANCEL,false);
			}
			return TRUE;
		}
		return TRUE;

	case WM_TIMER:
		{
			const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
			unsigned int BitRate=pCoreEngine->GetBitRate()*100/(1024*1024);
			TCHAR szText[64];

			StdUtil::snprintf(szText,lengthof(szText),
							  TEXT("%.2f dB / %u.%02u Mbps"),
							  pCoreEngine->GetSignalLevel(),
							  BitRate/100,BitRate%100);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_LEVEL,szText);
		}
		return TRUE;

	case WM_APP_BEGINSCAN:
		{
			CChannelScan *pThis=GetThis(hDlg);
			const int CurChannel=(int)wParam;
			const int NumChannels=(int)pThis->m_BonDriverChannelList.size();
			unsigned int EstimateRemain=(NumChannels-CurChannel)*pThis->m_ScanWait/1000;
			TCHAR szText[80];

			if (pThis->m_fIgnoreSignalLevel)
				EstimateRemain+=(NumChannels-CurChannel)*pThis->m_RetryCount*pThis->m_RetryInterval/1000;
			StdUtil::snprintf(szText,lengthof(szText),
				TEXT("チャンネル %d/%d をスキャンしています... (残り時間 %u:%02u)"),
				CurChannel+1,NumChannels,
				EstimateRemain/60,EstimateRemain%60);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_INFO,szText);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_CHANNEL,pThis->m_BonDriverChannelList[CurChannel].c_str());
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETPOS,wParam,0);
			GetAppClass().SetProgress(CurChannel,NumChannels);
		}
		return TRUE;

	case WM_APP_CHANNELFOUND:
		{
			CChannelScan *pThis=GetThis(hDlg);
			//int ScanChannel=LOWORD(wParam),Service=HIWORD(wParam);
			//CChannelInfo *pChannelInfo=reinterpret_cast<CChannelInfo*>(lParam);

			::SendMessage(pThis->m_hDlg,WM_APP_CHANNELFOUND,0,lParam);
		}
		return TRUE;

	case WM_APP_ENDSCAN:
		{
			CChannelScan *pThis=GetThis(hDlg);
			ScanResult Result=(ScanResult)wParam;

			::WaitForSingleObject(pThis->m_hScanThread,INFINITE);

			//GetAppClass().EndChannelScan();
			GetAppClass().EndProgress();

			if (pThis->m_fCancelled) {
				::EndDialog(hDlg,IDCANCEL);
			} else {
				CMessageDialog Dialog;

				if (Result==SCAN_RESULT_SET_CHANNEL_PARTIALLY_FAILED) {
					const int ChannelCount=LOWORD(lParam);
					const int ErrorCount=HIWORD(lParam);
					TVTest::String Message;
					if (ErrorCount<ChannelCount) {
						TVTest::StringUtility::Format(Message,
							TEXT("%dチャンネルのうち、%d回のチャンネル変更が BonDriver に受け付けられませんでした。\n")
							TEXT("(受信できるチャンネルが全てスキャンできていれば問題はありません)"),
							ChannelCount,ErrorCount);
						Dialog.Show(hDlg,CMessageDialog::TYPE_INFO,Message.c_str(),NULL,NULL,TEXT("お知らせ"));
					} else {
						Dialog.Show(hDlg,CMessageDialog::TYPE_WARNING,
									TEXT("チャンネル変更の要求が BonDriver に受け付けられないため、スキャンを行えませんでした。"));
					}
				} else if (Result==SCAN_RESULT_SET_CHANNEL_TIMEOUT) {
					Dialog.Show(hDlg,CMessageDialog::TYPE_WARNING,
								TEXT("タイムアウトのためチャンネル変更ができません。"));
				}

				::EndDialog(hDlg,IDOK);
			}
		}
		return TRUE;

	case WM_DESTROY:
		{
			CChannelScan *pThis=GetThis(hDlg);

			if (pThis->m_hScanThread!=NULL) {
				::CloseHandle(pThis->m_hScanThread);
				pThis->m_hScanThread=NULL;
			}
			if (pThis->m_hCancelEvent!=NULL) {
				::CloseHandle(pThis->m_hCancelEvent);
				pThis->m_hCancelEvent=NULL;
			}
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


unsigned int __stdcall CChannelScan::ScanProc(LPVOID lpParameter)
{
	CChannelScan *pThis=static_cast<CChannelScan*>(lpParameter);
	CDtvEngine *pDtvEngine=&pThis->m_pCoreEngine->m_DtvEngine;
	CTsAnalyzer *pTsAnalyzer=&pDtvEngine->m_TsAnalyzer;

	ScanResult Result=SCAN_RESULT_CANCELLED;
	int SetChannelCount=0,SetChannelErrorCount=0;

	pThis->m_ScanningChannelList.Clear();
	pThis->m_MaxSignalLevel=0.0f;
	pThis->m_MaxBitRate=0;

	bool fPurgeStream=pDtvEngine->m_BonSrcDecoder.IsPurgeStreamOnChannelChange();
	if (!fPurgeStream)
		pDtvEngine->m_BonSrcDecoder.SetPurgeStreamOnChannelChange(true);

	for (;;pThis->m_ScanChannel++) {
		if (pThis->m_ScanChannel>=(int)pThis->m_BonDriverChannelList.size()) {
			if (Result==SCAN_RESULT_CANCELLED)
				Result=SCAN_RESULT_SUCCEEDED;
			break;
		}

		::PostMessage(pThis->m_hScanDlg,WM_APP_BEGINSCAN,pThis->m_ScanChannel,0);

		SetChannelCount++;
		if (!pDtvEngine->SetChannel(pThis->m_ScanSpace,pThis->m_ScanChannel)) {
			SetChannelErrorCount++;
			if (pDtvEngine->GetLastErrorCode()==CBonSrcDecoder::ERR_TIMEOUT) {
				Result=SCAN_RESULT_SET_CHANNEL_TIMEOUT;
				break;
			} else {
				Result=SCAN_RESULT_SET_CHANNEL_PARTIALLY_FAILED;
			}
			if (::WaitForSingleObject(pThis->m_hCancelEvent,0)!=WAIT_TIMEOUT)
				break;
			continue;
		}

		if (::WaitForSingleObject(pThis->m_hCancelEvent,
								  min(pThis->m_ScanWait,2000))!=WAIT_TIMEOUT)
			break;
		if (pThis->m_ScanWait>2000) {
			DWORD Wait=pThis->m_ScanWait-2000;
			while (true) {
				// 全てのサービスのPMTが来たら待ち時間終了
				int NumServices=pTsAnalyzer->GetServiceNum();
				if (NumServices>0) {
					int i;
					for (i=0;i<NumServices;i++) {
						if (!pTsAnalyzer->IsServiceUpdated(i))
							break;
					}
					if (i==NumServices)
						break;
				}
				if (::WaitForSingleObject(pThis->m_hCancelEvent,min(Wait,1000))!=WAIT_TIMEOUT)
					goto End;
				if (Wait<=1000)
					break;
				Wait-=1000;
			}
		}

		bool fScanService=pThis->m_fScanService;
		bool fFound=false;
		int NumServices;
		TCHAR szName[32];
		if (pTsAnalyzer->GetViewableServiceNum()>0
				|| pThis->m_fIgnoreSignalLevel
				|| pThis->GetSignalLevel()>=SIGNAL_LEVEL_THRESHOLD) {
			for (int i=0;i<=pThis->m_RetryCount;i++) {
				if (i>0) {
					if (::WaitForSingleObject(pThis->m_hCancelEvent,
											  pThis->m_RetryInterval)!=WAIT_TIMEOUT)
						goto End;
					pThis->GetSignalLevel();
				}
				NumServices=pTsAnalyzer->GetViewableServiceNum();
				if (NumServices>0) {
					WORD ServiceID;

					if (fScanService) {
						int j;

						for (j=0;j<NumServices;j++) {
							if (!pTsAnalyzer->GetViewableServiceID(j,&ServiceID)
									|| pTsAnalyzer->GetServiceName(
										pTsAnalyzer->GetServiceIndexByID(ServiceID),szName,lengthof(szName))==0)
								break;
						}
						if (j==NumServices)
							fFound=true;
					} else {
						if (pTsAnalyzer->GetFirstViewableServiceID(&ServiceID)
								&& pTsAnalyzer->GetServiceName(
									pTsAnalyzer->GetServiceIndexByID(ServiceID),szName,lengthof(szName))>0) {
							if (pTsAnalyzer->GetTsName(szName,lengthof(szName))>0) {
								fFound=true;
							} else {
								// BS/CS の場合はサービスの検索を有効にする
								WORD NetworkID=pTsAnalyzer->GetNetworkID();
								if (IsBSNetworkID(NetworkID) || IsCSNetworkID(NetworkID))
									fScanService=true;
							}
						}
					}
					if (fFound)
						break;
				}
			}
		}
		if (!fFound) {
			TRACE(TEXT("Channel scan [%2d] Service not found.\n"),
				  pThis->m_ScanChannel);
			continue;
		}

		WORD TransportStreamID=pTsAnalyzer->GetTransportStreamID();
		WORD NetworkID=pTsAnalyzer->GetNetworkID();
		if (NetworkID==0) {
			// ネットワークIDが取得できるまで待つ
			for (int i=20;i>0;i--) {
				if (::WaitForSingleObject(pThis->m_hCancelEvent,500)!=WAIT_TIMEOUT)
					goto End;
				NetworkID=pTsAnalyzer->GetNetworkID();
				if (NetworkID!=0)
					break;
			}
			if (NetworkID==0) {
				TRACE(TEXT("Channel scan [%2d] Can't acquire Network ID.\n"),
					  pThis->m_ScanChannel);
				continue;
			}
		}

		// 見付かったチャンネルを追加する
		if (fScanService) {
			for (int i=0;i<NumServices;i++) {
				WORD ServiceID=0;
				pTsAnalyzer->GetViewableServiceID(i,&ServiceID);
				const int ServiceIndex=pTsAnalyzer->GetServiceIndexByID(ServiceID);
				int RemoteControlKeyID=pTsAnalyzer->GetRemoteControlKeyID();
				if (RemoteControlKeyID==0) {
					// BSのリモコン番号を割り当てる
					if (IsBSNetworkID(NetworkID) && ServiceID<230) {
						if (ServiceID<=109)
							RemoteControlKeyID=ServiceID%10;
						else
							RemoteControlKeyID=ServiceID/10-10;
					} else if (ServiceID<1000) {
						RemoteControlKeyID=ServiceID;
					}
				}
				pTsAnalyzer->GetServiceName(ServiceIndex,szName,lengthof(szName));
				CChannelInfo *pChInfo=
					new CChannelInfo(pThis->m_ScanSpace,pThis->m_ScanChannel,
									 RemoteControlKeyID,szName);
				pChInfo->SetNetworkID(NetworkID);
				pChInfo->SetTransportStreamID(TransportStreamID);
				pChInfo->SetServiceID(ServiceID);
				const BYTE ServiceType=pTsAnalyzer->GetServiceType(ServiceIndex);
				if ((ServiceType!=SERVICE_TYPE_DIGITALTV
						&& ServiceType!=0xA5
#ifdef TVH264
						&& ServiceType!=SERVICE_TYPE_DATA
#endif
#ifdef TVTEST_RADIO_SUPPORT
						&& ServiceType!=SERVICE_TYPE_DIGITALAUDIO
#endif
						)
						// BSのサブチャンネルを無効にする
						|| (IsBSNetworkID(NetworkID) && ServiceID>=140 && ServiceID<190 && ServiceID%10>=2))
					pChInfo->Enable(false);
				TRACE(TEXT("Channel found : %s %d %d %d %d\n"),
					szName,pThis->m_ScanChannel,pChInfo->GetChannelNo(),NetworkID,ServiceID);
				pThis->m_ScanningChannelList.AddChannel(pChInfo);
				::PostMessage(pThis->m_hScanDlg,WM_APP_CHANNELFOUND,
							  MAKEWPARAM(pThis->m_ScanChannel,i),
							  reinterpret_cast<LPARAM>(pChInfo));
			}
		} else {
			CChannelInfo *pChInfo=
				new CChannelInfo(pThis->m_ScanSpace,pThis->m_ScanChannel,
								 pTsAnalyzer->GetRemoteControlKeyID(),szName);
			pChInfo->SetNetworkID(NetworkID);
			pChInfo->SetTransportStreamID(TransportStreamID);
			WORD ServiceID=0;
			if (pTsAnalyzer->GetFirstViewableServiceID(&ServiceID))
				pChInfo->SetServiceID(ServiceID);
			TRACE(TEXT("Channel found : %s %d %d %d %d\n"),
				szName,pThis->m_ScanChannel,pChInfo->GetChannelNo(),NetworkID,ServiceID);
			pThis->m_ScanningChannelList.AddChannel(pChInfo);
			::PostMessage(pThis->m_hScanDlg,WM_APP_CHANNELFOUND,
						  pThis->m_ScanChannel,
						  reinterpret_cast<LPARAM>(pChInfo));
		}
	}

End:
	if (!fPurgeStream)
		pDtvEngine->m_BonSrcDecoder.SetPurgeStreamOnChannelChange(false);

	::PostMessage(pThis->m_hScanDlg,WM_APP_ENDSCAN,
				  (WPARAM)Result,MAKELPARAM(SetChannelCount,SetChannelErrorCount));

	return 0;
}


float CChannelScan::GetSignalLevel()
{
	CBonSrcDecoder *pBonSrcDecoder=&m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder;
	float SignalLevel=pBonSrcDecoder->GetSignalLevel();
	if (SignalLevel>m_MaxSignalLevel)
		m_MaxSignalLevel=SignalLevel;
	DWORD BitRate=pBonSrcDecoder->GetBitRate();
	if (BitRate>m_MaxBitRate)
		m_MaxBitRate=BitRate;
	return SignalLevel;
}


INT_PTR CALLBACK CChannelScan::ChannelPropDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lParam);
			TCHAR szText[64];

			::SetProp(hDlg,TEXT("ChannelInfo"),pChInfo);
			::SendDlgItemMessage(hDlg,IDC_CHANNELPROP_NAME,EM_LIMITTEXT,MAX_CHANNEL_NAME-1,0);
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_NAME,pChInfo->GetName());
			for (int i=1;i<=12;i++) {
				TCHAR szText[4];

				::wsprintf(szText,TEXT("%d"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELPROP_CONTROLKEY,szText);
			}
			if (pChInfo->GetChannelNo()>0)
				::SetDlgItemInt(hDlg,IDC_CHANNELPROP_CONTROLKEY,pChInfo->GetChannelNo(),TRUE);
			::wsprintf(szText,TEXT("%d (%#04x)"),
					   pChInfo->GetTransportStreamID(),pChInfo->GetTransportStreamID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_TSID,szText);
			::wsprintf(szText,TEXT("%d (%#04x)"),
					   pChInfo->GetServiceID(),pChInfo->GetServiceID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_SERVICEID,szText);
			::SetDlgItemInt(hDlg,IDC_CHANNELPROP_TUNINGSPACE,pChInfo->GetSpace(),TRUE);
			::SetDlgItemInt(hDlg,IDC_CHANNELPROP_CHANNELINDEX,pChInfo->GetChannelIndex(),TRUE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				CChannelInfo *pChInfo=static_cast<CChannelInfo*>(::GetProp(hDlg,TEXT("ChannelInfo")));
				bool fModified=false;
				TCHAR szName[MAX_CHANNEL_NAME];
				int ControlKey;

				::GetDlgItemText(hDlg,IDC_CHANNELPROP_NAME,szName,lengthof(szName));
				if (szName[0]=='\0') {
					::MessageBox(hDlg,TEXT("名前を入力してください。"),TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
					::SetFocus(::GetDlgItem(hDlg,IDC_CHANNELPROP_NAME));
					return TRUE;
				}
				if (::lstrcmp(pChInfo->GetName(),szName)!=0) {
					pChInfo->SetName(szName);
					fModified=true;
				}
				ControlKey=::GetDlgItemInt(hDlg,IDC_CHANNELPROP_CONTROLKEY,NULL,TRUE);
				if (ControlKey!=pChInfo->GetChannelNo()) {
					pChInfo->SetChannelNo(ControlKey);
					fModified=true;
				}
				if (!fModified)
					wParam=IDCANCEL;
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("ChannelInfo"));
		return TRUE;
	}
	return FALSE;
}
