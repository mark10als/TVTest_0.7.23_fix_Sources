#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuide.h"
#include "DialogUtil.h"
#include "LogoManager.h"
#include "EpgChannelSettings.h"
#include "Help/HelpID.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TITLE_TEXT TEXT("EPG番組表")

// 現在時刻を表す線の幅
#define CUR_TIME_LINE_WIDTH 2

// 現在時刻を更新するタイマのID
#define TIMER_ID_UPDATECURTIME	1

#define HEADER_MARGIN_LEFT		4
#define HEADER_MARGIN_RIGHT		4
#define HEADER_MARGIN_TOP		4
#define HEADER_MARGIN_BOTTOM	4

#define CHEVRON_WIDTH	10
#define CHEVRON_HEIGHT	10

#define PROGRAM_GUIDE_ICON_MARGIN_LEFT		1
#define PROGRAM_GUIDE_ICON_MARGIN_RIGHT		0
#define PROGRAM_GUIDE_ICON_MARGIN_TOP		1
#define PROGRAM_GUIDE_ICON_MARGIN			1

#define HEADER_SHADOW_HEIGHT	8
#define TIMEBAR_SHADOW_WIDTH	6

// メニューの位置
#define MENU_DATE	0
#define MENU_DRIVER 1

static const LPCTSTR DayText[] = {
	TEXT("今日"), TEXT("明日"), TEXT("明後日"), TEXT("3日後"),
	TEXT("4日後"), TEXT("5日後"), TEXT("6日後"), TEXT("7日後"),
};




namespace ProgramGuide
{


class CEventItem
{
	const CEventInfoData *m_pEventInfo;
	const CEventInfoData *m_pCommonEventInfo;
	SYSTEMTIME m_StartTime;
	SYSTEMTIME m_EndTime;
	DWORD m_Duration;
	int m_TitleLines;
	//int m_TextLines;
	int m_ItemPos;
	int m_ItemLines;

	int GetTitleText(LPTSTR pszText,int MaxLength) const;
	LPCTSTR GetEventText() const;

public:
	CEventItem(const CEventInfoData *pInfo);
	CEventItem(const SYSTEMTIME &StartTime,DWORD Duration);
	~CEventItem();
	const CEventInfoData *GetEventInfo() const { return m_pEventInfo; }
	const CEventInfoData *GetCommonEventInfo() const { return m_pCommonEventInfo; }
	const SYSTEMTIME &GetStartTime() const { return m_StartTime; }
	bool SetStartTime(const SYSTEMTIME &Time);
	const SYSTEMTIME &GetEndTime() const { return m_EndTime; }
	bool SetEndTime(const SYSTEMTIME &Time);
	int GetGenre(int Level=0) const;
	bool SetCommonEvent(const CEventInfoData *pEvent);
	int GetTitleLines() const { return m_TitleLines; }
	//int GetTextLines() const { return m_TextLines; }
	//int GetLines() const { return m_TitleLines+m_TextLines; }
	//int CalcLines(HDC hdc,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth);
	void CalcLines(HDC hdc,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth);
	void DrawTitle(HDC hdc,const RECT *pRect,int LineHeight) const;
	void DrawText(HDC hdc,const RECT *pRect,int LineHeight) const;
	int GetItemPos() const { return m_ItemPos; }
	void SetItemPos(int Pos) { m_ItemPos=Pos; }
	int GetItemLines() const { return m_ItemLines; }
	void SetItemLines(int Lines) { m_ItemLines=Lines; }
	bool IsNullItem() const { return m_pEventInfo==NULL; }
};


CEventItem::CEventItem(const CEventInfoData *pInfo)
	: m_pEventInfo(pInfo)
	, m_pCommonEventInfo(NULL)
	, m_TitleLines(0)
	//, m_TextLines(0)
	, m_ItemPos(-1)
	, m_ItemLines(0)
{
	m_pEventInfo->GetStartTime(&m_StartTime);
	m_StartTime.wSecond=0;
	m_pEventInfo->GetEndTime(&m_EndTime);
	m_EndTime.wSecond=0;
}


CEventItem::CEventItem(const SYSTEMTIME &StartTime,DWORD Duration)
	: m_pEventInfo(NULL)
	, m_pCommonEventInfo(NULL)
	, m_StartTime(StartTime)
	, m_Duration(Duration)
	, m_TitleLines(0)
	//, m_TextLines(0)
	, m_ItemPos(-1)
	, m_ItemLines(0)
{
	m_EndTime=StartTime;
	OffsetSystemTime(&m_EndTime,Duration*1000);
}


CEventItem::~CEventItem()
{
}


bool CEventItem::SetStartTime(const SYSTEMTIME &Time)
{
	if (CompareSystemTime(&Time,&m_EndTime)>=0)
		return false;
	m_StartTime=Time;
	return true;
}


bool CEventItem::SetEndTime(const SYSTEMTIME &Time)
{
	if (CompareSystemTime(&m_StartTime,&Time)>=0)
		return false;
	m_EndTime=Time;
	return true;
}


int CEventItem::GetGenre(int Level) const
{
	if (m_pEventInfo!=NULL) {
		const CEventInfoData::ContentNibble *pContentNibble;

		if (m_pEventInfo->m_ContentNibble.NibbleCount>0) {
			pContentNibble=&m_pEventInfo->m_ContentNibble;
		} else if (m_pCommonEventInfo!=NULL
				&& m_pCommonEventInfo->m_ContentNibble.NibbleCount>0) {
			pContentNibble=&m_pCommonEventInfo->m_ContentNibble;
		} else {
			return -1;
		}
		for (int i=0;i<pContentNibble->NibbleCount;i++) {
			if (pContentNibble->NibbleList[i].ContentNibbleLevel1!=0xE) {
				int Nibble=Level==0?pContentNibble->NibbleList[i].ContentNibbleLevel1:
									pContentNibble->NibbleList[i].ContentNibbleLevel2;
				if (Nibble<=CEventInfoData::CONTENT_LAST)
					return Nibble;
				break;
			}
		}
	}
	return -1;
}


bool CEventItem::SetCommonEvent(const CEventInfoData *pEvent)
{
	if (m_pEventInfo==NULL || !m_pEventInfo->m_fCommonEvent || pEvent==NULL)
		return false;
	m_pCommonEventInfo=pEvent;
	/*
	if (m_pEventInfo->GetEventName()==NULL)
		m_TitleLines=pItem->m_TitleLines;	// とりあえず
	*/
	return true;
}


int CEventItem::GetTitleText(LPTSTR pszText,int MaxLength) const
{
	int Length;

#ifndef _DEBUG
	Length=StdUtil::snprintf(pszText,MaxLength,TEXT("%d:%02d"),
							 m_StartTime.wHour,m_StartTime.wMinute);
#else
	Length=StdUtil::snprintf(pszText,MaxLength,TEXT("%d:%02d.%02d-%d:%02d.%02d"),
							 m_StartTime.wHour,m_StartTime.wMinute,m_StartTime.wSecond,
							 m_EndTime.wHour,m_EndTime.wMinute,m_EndTime.wSecond);
#endif
	if (m_pEventInfo!=NULL) {
		LPCTSTR pszEventName=m_pEventInfo->GetEventName();
		if (pszEventName==NULL && m_pCommonEventInfo!=NULL)
			pszEventName=m_pCommonEventInfo->GetEventName();
		if (pszEventName!=NULL)
			Length+=StdUtil::snprintf(pszText+Length,MaxLength-Length,TEXT(" %s"),pszEventName);
	}
	return Length;
}


LPCTSTR CEventItem::GetEventText() const
{
	if (m_pEventInfo==NULL)
		return NULL;

	LPCTSTR pszEventText,p;

	pszEventText=m_pEventInfo->GetEventText();
	//if (pszEventText==NULL && m_pCommonEventInfo!=NULL)
	//	pszEventText=m_pCommonEventInfo->GetEventText();
	if (pszEventText!=NULL) {
		p=pszEventText;
		while (*p!='\0') {
			if (*p<=0x20) {
				p++;
				continue;
			}
			return p;
		}
	}
	pszEventText=m_pEventInfo->GetEventExtText();
	//if (pszEventText==NULL && m_pCommonEventInfo!=NULL)
	//	pszEventText=m_pCommonEventInfo->GetEventExtText();
	if (pszEventText!=NULL) {
		p=pszEventText;
		if (memcmp(p,TEXT("番組内容"),8)==0)
			p+=4*(3-sizeof(TCHAR));
		while (*p!='\0') {
			if (*p<=0x20) {
				p++;
				continue;
			}
			return p;
		}
	}
	return NULL;
}


/*
int CEventItem::CalcLines(HDC hdc,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth)
{
	TCHAR szText[256];
	GetTitleText(szText,lengthof(szText));
	SelectFont(hdc,hfontTitle);
	m_TitleLines=DrawUtil::CalcWrapTextLines(hdc,szText,TitleWidth);

	LPCTSTR pszEventText=GetEventText();
	if (!IsStringEmpty(pszEventText)) {
		SelectFont(hdc,hfontText);
		m_TextLines=DrawUtil::CalcWrapTextLines(hdc,pszEventText,TextWidth);
	} else {
		m_TextLines=0;
	}

	return m_TitleLines+m_TextLines;
}
*/

void CEventItem::CalcLines(HDC hdc,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth)
{
	TCHAR szText[256];
	GetTitleText(szText,lengthof(szText));
	SelectFont(hdc,hfontTitle);
	m_TitleLines=DrawUtil::CalcWrapTextLines(hdc,szText,TitleWidth);
}


void CEventItem::DrawTitle(HDC hdc,const RECT *pRect,int LineHeight) const
{
	TCHAR szText[1024];

	GetTitleText(szText,lengthof(szText));
	DrawUtil::DrawWrapText(hdc,szText,pRect,LineHeight);
}


void CEventItem::DrawText(HDC hdc,const RECT *pRect,int LineHeight) const
{
	LPCTSTR pszEventText=GetEventText();
	if (!IsStringEmpty(pszEventText))
		DrawUtil::DrawWrapText(hdc,pszEventText,pRect,LineHeight);
}




class CEventLayout
{
	const CServiceInfo *m_pServiceInfo;
	std::vector<CEventItem*> m_EventList;

public:
	CEventLayout(const CServiceInfo *pServiceInfo);
	~CEventLayout();
	const CServiceInfo *GetServiceInfo() const { return m_pServiceInfo; }
	void Clear();
	size_t NumItems() const { return m_EventList.size(); }
	void AddItem(CEventItem *pItem) { m_EventList.push_back(pItem); }
	bool InsertItem(size_t Index,CEventItem *pItem);
	CEventItem *GetItem(size_t Index);
	const CEventItem *GetItem(size_t Index) const;
	void InsertNullItems(const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime);
};


CEventLayout::CEventLayout(const CServiceInfo *pServiceInfo)
	: m_pServiceInfo(pServiceInfo)
{
}


CEventLayout::~CEventLayout()
{
	Clear();
}


void CEventLayout::Clear()
{
	for (size_t i=0;i<m_EventList.size();i++)
		delete m_EventList[i];
	m_EventList.clear();
}


bool CEventLayout::InsertItem(size_t Index,CEventItem *pItem)
{
	if (Index>m_EventList.size())
		return false;
	m_EventList.insert(m_EventList.begin()+Index,pItem);
	return true;
}


CEventItem *CEventLayout::GetItem(size_t Index)
{
	if (Index>=m_EventList.size())
		return NULL;
	return m_EventList[Index];
}


const CEventItem *CEventLayout::GetItem(size_t Index) const
{
	if (Index>=m_EventList.size())
		return NULL;
	return m_EventList[Index];
}


void CEventLayout::InsertNullItems(const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime)
{
	int FirstItem,LastItem;
	int i;
	CEventItem *pItem,*pPrevItem;
	SYSTEMTIME stPrev,stStart,stEnd;
	int EmptyCount;

	FirstItem=-1;
	LastItem=-1;
	EmptyCount=0;
	stPrev=FirstTime;
	for (i=0;i<(int)m_EventList.size();i++) {
		pItem=m_EventList[i];
		stStart=pItem->GetStartTime();
		stEnd=pItem->GetEndTime();
		if (CompareSystemTime(&stStart,&LastTime)<0
				&& CompareSystemTime(&stEnd,&FirstTime)>0) {
			if (FirstItem<0) {
				FirstItem=i;
				LastItem=i+1;
			} else if (LastItem<i+1) {
				LastItem=i+1;
			}
			if (CompareSystemTime(&stPrev,&stStart)<0)
				EmptyCount++;
		}
		if (CompareSystemTime(&stEnd,&LastTime)>=0)
			break;
		stPrev=stEnd;
	}
	if (EmptyCount>0) {
		pPrevItem=NULL;
		stPrev=FirstTime;
		for (i=FirstItem;i<LastItem;i++) {
			pItem=m_EventList[i];
			stStart=pItem->GetStartTime();
			int Cmp=CompareSystemTime(&stPrev,&stStart);
			if (Cmp>0) {
				if (pPrevItem)
					pPrevItem->SetEndTime(stStart);
			} else if (Cmp<0) {
				LONGLONG Diff=DiffSystemTime(&stPrev,&stStart);

				if (Diff<60*1000) {
					if (pPrevItem)
						pPrevItem->SetEndTime(stStart);
				} else {
					InsertItem(i,new CEventItem(stPrev,(DWORD)(Diff/1000)));
					i++;
					LastItem++;
				}
			}
			stPrev=pItem->GetEndTime();
			pPrevItem=pItem;
		}
	}
}




CEventLayoutList::~CEventLayoutList()
{
	Clear();
}


void CEventLayoutList::Clear()
{
	for (size_t i=0;i<m_LayoutList.size();i++)
		delete m_LayoutList[i];
	m_LayoutList.clear();
}


void CEventLayoutList::Add(CEventLayout *pLayout)
{
	m_LayoutList.push_back(pLayout);
}


const CEventLayout *CEventLayoutList::operator[](size_t Index) const
{
	if (Index>=m_LayoutList.size())
		return NULL;
	return m_LayoutList[Index];
}




class CServiceInfo
{
	CServiceInfoData m_ServiceData;
	LPTSTR m_pszServiceName;
	HBITMAP m_hbmLogo;
	DrawUtil::CBitmap m_StretchedLogo;
	std::vector<CEventInfoData*> m_EventList;
	typedef std::map<WORD,CEventInfoData*> EventIDMap;
	EventIDMap m_EventIDMap;

	void SortSub(CEventInfoData **ppFirst,CEventInfoData **ppLast);

public:
	CServiceInfo(const CChannelInfo *pChannelInfo);
	CServiceInfo(const CChannelInfo *pChannelInfo,const CEpgServiceInfo &Info);
	~CServiceInfo();
	const CServiceInfoData *GetServiceInfoData() const { return &m_ServiceData; }
	WORD GetNetworkID() const { return m_ServiceData.m_NetworkID; }
	WORD GetTSID() const { return m_ServiceData.m_TSID; }
	WORD GetServiceID() const { return m_ServiceData.m_ServiceID; }
	LPCTSTR GetServiceName() const { return m_pszServiceName; }
	void SetLogo(HBITMAP hbm) { m_hbmLogo=hbm; }
	HBITMAP GetLogo() const { return m_hbmLogo; }
	HBITMAP GetStretchedLogo(int Width,int Height);
	int NumEvents() const { return (int)m_EventList.size(); }
	CEventInfoData *GetEvent(int Index);
	const CEventInfoData *GetEvent(int Index) const;
	CEventInfoData *GetEventByEventID(WORD EventID);
	const CEventInfoData *GetEventByEventID(WORD EventID) const;
	bool AddEvent(CEventInfoData *pEvent);
	void ClearEvents();
	void SortEvents();
	void CalcLayout(CEventLayout *pEventList,const CServiceList *pServiceList,
		HDC hdc,const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime,int LinesPerHour,
		HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth);
	bool SaveiEpgFile(const CEventInfoData *pEventInfo,LPCTSTR pszFileName,bool fVersion2) const;
};


CServiceInfo::CServiceInfo(const CChannelInfo *pChannelInfo)
	: m_ServiceData(pChannelInfo->GetNetworkID(),
					pChannelInfo->GetTransportStreamID(),
					pChannelInfo->GetServiceID())
	, m_pszServiceName(DuplicateString(pChannelInfo->GetName()))
	, m_hbmLogo(NULL)
{
}


CServiceInfo::CServiceInfo(const CChannelInfo *pChannelInfo,const CEpgServiceInfo &Info)
	: m_ServiceData(Info.m_ServiceData)
	, m_pszServiceName(DuplicateString(pChannelInfo->GetName()))
	, m_hbmLogo(NULL)
{
}


CServiceInfo::~CServiceInfo()
{
	delete [] m_pszServiceName;
	ClearEvents();
}


HBITMAP CServiceInfo::GetStretchedLogo(int Width,int Height)
{
	if (m_hbmLogo==NULL)
		return NULL;
	// AlphaBlendでリサイズすると汚いので、予めリサイズした画像を作成しておく
	if (m_StretchedLogo.IsCreated()) {
		if (m_StretchedLogo.GetWidth()!=Width || m_StretchedLogo.GetHeight()!=Height)
			m_StretchedLogo.Destroy();
	}
	if (!m_StretchedLogo.IsCreated()) {
		HBITMAP hbm=DrawUtil::ResizeBitmap(m_hbmLogo,Width,Height);
		if (hbm!=NULL)
			m_StretchedLogo.Attach(hbm);
	}
	return m_StretchedLogo.GetHandle();
}


CEventInfoData *CServiceInfo::GetEvent(int Index)
{
	if (Index<0 || (size_t)Index>=m_EventList.size()) {
		TRACE(TEXT("CServiceInfo::GetEvent() : Out of range %d\n"),Index);
		return NULL;
	}
	return m_EventList[Index];
}


const CEventInfoData *CServiceInfo::GetEvent(int Index) const
{
	if (Index<0 || (size_t)Index>=m_EventList.size()) {
		TRACE(TEXT("CServiceInfo::GetEvent() const : Out of range %d\n"),Index);
		return NULL;
	}
	return m_EventList[Index];
}


CEventInfoData *CServiceInfo::GetEventByEventID(WORD EventID)
{
	EventIDMap::iterator itr=m_EventIDMap.find(EventID);
	if (itr==m_EventIDMap.end())
		return NULL;
	return itr->second;
}


const CEventInfoData *CServiceInfo::GetEventByEventID(WORD EventID) const
{
	EventIDMap::const_iterator itr=m_EventIDMap.find(EventID);
	if (itr==m_EventIDMap.end())
		return NULL;
	return itr->second;
}


bool CServiceInfo::AddEvent(CEventInfoData *pEvent)
{
	m_EventList.push_back(pEvent);
	m_EventIDMap[pEvent->m_EventID]=pEvent;
	return true;
}


void CServiceInfo::ClearEvents()
{
	for (size_t i=0;i<m_EventList.size();i++)
		delete m_EventList[i];
	m_EventList.clear();
	m_EventIDMap.clear();
}


void CServiceInfo::SortSub(CEventInfoData **ppFirst,CEventInfoData **ppLast)
{
	SYSTEMTIME stKey=ppFirst[(ppLast-ppFirst)/2]->m_stStartTime;
	CEventInfoData **p,**q;

	p=ppFirst;
	q=ppLast;
	while (p<=q) {
		while (CompareSystemTime(&(*p)->m_stStartTime,&stKey)<0)
			p++;
		while (CompareSystemTime(&(*q)->m_stStartTime,&stKey)>0)
			q--;
		if (p<=q) {
			CEventInfoData *pTemp;

			pTemp=*p;
			*p=*q;
			*q=pTemp;
			p++;
			q--;
		}
	}
	if (q>ppFirst)
		SortSub(ppFirst,q);
	if (p<ppLast)
		SortSub(p,ppLast);
}


void CServiceInfo::SortEvents()
{
	if (m_EventList.size()>1)
		SortSub(&m_EventList[0],&m_EventList[m_EventList.size()-1]);
}


void CServiceInfo::CalcLayout(CEventLayout *pEventList,const CServiceList *pServiceList,
	HDC hdc,const SYSTEMTIME &FirstTime,const SYSTEMTIME &LastTime,
	int LinesPerHour,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth)
{
	pEventList->Clear();

	int FirstItem=-1,LastItem=-1;
	for (int i=0;i<(int)m_EventList.size();i++) {
		CEventInfoData *pEvent=m_EventList[i];
		SYSTEMTIME StartTime,EndTime;
		pEvent->GetStartTime(&StartTime);
		pEvent->GetEndTime(&EndTime);

		if (CompareSystemTime(&StartTime,&LastTime)<0
				&& CompareSystemTime(&EndTime,&FirstTime)>0) {
			if (FirstItem<0) {
				FirstItem=i;
				LastItem=i+1;
			} else if (LastItem<i+1) {
				LastItem=i+1;
			}

			CEventItem *pItem=new CEventItem(pEvent);
			if (pEvent->m_fCommonEvent) {
				const CEventInfoData *pCommonEvent=
					pServiceList->FindEvent(m_ServiceData.m_TSID,
											pEvent->m_CommonEventInfo.ServiceID,
											pEvent->m_CommonEventInfo.EventID);
				if (pCommonEvent!=NULL)
					pItem->SetCommonEvent(pCommonEvent);
			}
			pItem->CalcLines(hdc,hfontTitle,TitleWidth,hfontText,TextWidth);
			pItem->SetItemPos(-1);
			pItem->SetItemLines(0);
			pEventList->AddItem(pItem);
		}
		if (CompareSystemTime(&EndTime,&LastTime)>=0)
			break;
	}
	if (FirstItem<0)
		return;

	pEventList->InsertNullItems(FirstTime,LastTime);

	const size_t NumItems=pEventList->NumItems();
	SYSTEMTIME stFirst,stLast;
	CEventItem *pItem;
	int ItemPos=0;

	stFirst=FirstTime;
	for (size_t i=0;i<NumItems;) {
		if (CompareSystemTime(&stFirst,&LastTime)>=0)
			break;
		stLast=stFirst;
		OffsetSystemTime(&stLast,60*60*1000);
		do {
			if (CompareSystemTime(&pEventList->GetItem(i)->GetEndTime(),&stFirst)>0)
				break;
			i++;
		} while (i<NumItems);
		if (i==NumItems)
			break;
		int ProgramsPerHour=0;
		do {
			if (CompareSystemTime(&pEventList->GetItem(i+ProgramsPerHour)->GetStartTime(),&stLast)>=0)
				break;
			ProgramsPerHour++;
		} while (i+ProgramsPerHour<NumItems);
		if (ProgramsPerHour>0) {
			int Lines=LinesPerHour,Offset=0;

			const SYSTEMTIME &stStart=pEventList->GetItem(i)->GetStartTime();
			if (CompareSystemTime(&stStart,&stFirst)>0) {
				Offset=(int)(DiffSystemTime(&stFirst,&stStart)*LinesPerHour/(60*60*1000));
				Lines-=Offset;
			}
			if (Lines>ProgramsPerHour) {
				const SYSTEMTIME &stEnd=pEventList->GetItem(i+ProgramsPerHour-1)->GetEndTime();
				if (CompareSystemTime(&stEnd,&stLast)<0) {
					Lines-=(int)(DiffSystemTime(&stEnd,&stLast)*LinesPerHour/(60*60*1000));
					if (Lines<ProgramsPerHour)
						Lines=ProgramsPerHour;
				}
			}
			if (ProgramsPerHour==1) {
				pItem=pEventList->GetItem(i);
				pItem->SetItemLines(pItem->GetItemLines()+Lines);
				if (pItem->GetItemPos()<0)
					pItem->SetItemPos(ItemPos+Offset);
			} else {
				int *pItemLines=new int[ProgramsPerHour];

				for (int j=0;j<ProgramsPerHour;j++)
					pItemLines[j]=j<Lines?1:0;
				if (Lines>ProgramsPerHour) {
					int LineCount=ProgramsPerHour;

					do {
						DWORD Time,MaxTime;
						int MaxItem;

						MaxTime=0;
						for (int j=0;j<ProgramsPerHour;j++) {
							pItem=pEventList->GetItem(i+j);
							SYSTEMTIME stStart=pItem->GetStartTime();
							if (CompareSystemTime(&stStart,&stFirst)<0)
								stStart=stFirst;
							SYSTEMTIME stEnd=pItem->GetEndTime();
							if (CompareSystemTime(&stEnd,&stLast)>0)
								stEnd=stLast;
							Time=(DWORD)(DiffSystemTime(&stStart,&stEnd)/pItemLines[j]);
							if (Time>MaxTime) {
								MaxTime=Time;
								MaxItem=j;
							}
						}
						if (MaxTime==0)
							break;
						pItemLines[MaxItem]++;
						LineCount++;
					} while (LineCount<Lines);
				}
				int Pos=ItemPos+Offset;
				for (int j=0;j<min(ProgramsPerHour,Lines);j++) {
					pItem=pEventList->GetItem(i+j);
					if (pItem->GetItemPos()<0)
						pItem->SetItemPos(Pos);
					pItem->SetItemLines(pItem->GetItemLines()+pItemLines[j]);
					Pos+=pItemLines[j];
				}
				delete [] pItemLines;
				i+=ProgramsPerHour-1;
			}
		}
		ItemPos+=LinesPerHour;
		stFirst=stLast;
	}
}


bool CServiceInfo::SaveiEpgFile(const CEventInfoData *pEventInfo,LPCTSTR pszFileName,bool fVersion2) const
{
	if (pEventInfo==NULL)
		return false;

	HANDLE hFile;
	char szText[2048],szServiceName[64],szEventName[256];
	SYSTEMTIME stStart,stEnd;
	DWORD Length,Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
					   CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (!IsStringEmpty(m_pszServiceName))
		::WideCharToMultiByte(CP_ACP,0,m_pszServiceName,-1,
							  szServiceName,sizeof(szServiceName),NULL,NULL);
	else
		szServiceName[0]='\0';
	pEventInfo->GetStartTime(&stStart);
	pEventInfo->GetEndTime(&stEnd);
	if (!IsStringEmpty(pEventInfo->GetEventName()))
		::WideCharToMultiByte(CP_ACP,0,pEventInfo->GetEventName(),-1,
							  szEventName,sizeof(szEventName),NULL,NULL);
	else
		szEventName[0]='\0';
	if (fVersion2) {
		char szStation[16];
		StdUtil::snprintf(szStation,lengthof(szText),
						  m_ServiceData.IsBS()?"BSDT%03d":
						  m_ServiceData.IsCS()?"CSDT%03d":"DFS%05x",
						  m_ServiceData.m_ServiceID);
		Length=::wnsprintfA(szText,sizeof(szText),
			"Content-type: application/x-tv-program-digital-info; charset=shift_jis\r\n"
			"version: 2\r\n"
			"station: %s\r\n"
			"station-name: %s\r\n"
			"year: %d\r\n"
			"month: %02d\r\n"
			"date: %02d\r\n"
			"start: %02d:%02d\r\n"
			"end: %02d:%02d\r\n"
			"program-title: %s\r\n"
			"program-id: %d\r\n",
			szStation,szServiceName,
			stStart.wYear,stStart.wMonth,stStart.wDay,
			stStart.wHour,stStart.wMinute,
			stEnd.wHour,stEnd.wMinute,
			szEventName,pEventInfo->m_EventID);
	} else {
		Length=::wnsprintfA(szText,sizeof(szText),
			"Content-type: application/x-tv-program-info; charset=shift_jis\r\n"
			"version: 1\r\n"
			"station: %s\r\n"
			"year: %d\r\n"
			"month: %02d\r\n"
			"date: %02d\r\n"
			"start: %02d:%02d\r\n"
			"end: %02d:%02d\r\n"
			"program-title: %s\r\n",
			szServiceName,
			stStart.wYear,stStart.wMonth,stStart.wDay,
			stStart.wHour,stStart.wMinute,
			stEnd.wHour,stEnd.wMinute,
			szEventName);
	}
	bool fOK=::WriteFile(hFile,szText,Length,&Write,NULL) && Write==Length;
	::FlushFileBuffers(hFile);
	::CloseHandle(hFile);
	return fOK;
}




CServiceList::~CServiceList()
{
	Clear();
}


CServiceInfo *CServiceList::GetItem(size_t Index)
{
	if (Index>=m_ServiceList.size())
		return NULL;
	return m_ServiceList[Index];
}


const CServiceInfo *CServiceList::GetItem(size_t Index) const
{
	if (Index>=m_ServiceList.size())
		return NULL;
	return m_ServiceList[Index];
}


CServiceInfo *CServiceList::FindItem(WORD TransportStreamID,WORD ServiceID)
{
	for (size_t i=0;i<m_ServiceList.size();i++) {
		CServiceInfo *pInfo=m_ServiceList[i];

		if (pInfo->GetTSID()==TransportStreamID
				&& pInfo->GetServiceID()==ServiceID)
			return pInfo;
	}
	return NULL;
}


const CServiceInfo *CServiceList::FindItem(WORD TransportStreamID,WORD ServiceID) const
{
	for (size_t i=0;i<m_ServiceList.size();i++) {
		const CServiceInfo *pInfo=m_ServiceList[i];

		if (pInfo->GetTSID()==TransportStreamID
				&& pInfo->GetServiceID()==ServiceID)
			return pInfo;
	}
	return NULL;
}


CEventInfoData *CServiceList::FindEvent(WORD TransportStreamID,WORD ServiceID,WORD EventID)
{
	CServiceInfo *pService=FindItem(TransportStreamID,ServiceID);
	if (pService==NULL)
		return NULL;
	return pService->GetEventByEventID(EventID);
}


const CEventInfoData *CServiceList::FindEvent(WORD TransportStreamID,WORD ServiceID,WORD EventID) const
{
	const CServiceInfo *pService=FindItem(TransportStreamID,ServiceID);
	if (pService==NULL)
		return NULL;
	return pService->GetEventByEventID(EventID);
}


void CServiceList::Add(CServiceInfo *pInfo)
{
	m_ServiceList.push_back(pInfo);
}


void CServiceList::Clear()
{
	for (size_t i=0;i<m_ServiceList.size();i++)
		delete m_ServiceList[i];
	m_ServiceList.clear();
}


}	// namespace ProgramGuide




const LPCTSTR CProgramGuide::m_pszWindowClass=APP_NAME TEXT(" Program Guide");
HINSTANCE CProgramGuide::m_hinst=NULL;


bool CProgramGuide::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuide::CProgramGuide()
	: m_pProgramList(NULL)
	, m_ListMode(LIST_SERVICES)
	, m_WeekListService(-1)
	, m_LinesPerHour(12)
	, m_LineMargin(1)
	, m_ItemWidth(140)
	, m_ItemMargin(4)
	, m_TextLeftMargin(CEpgIcons::ICON_WIDTH+
					   PROGRAM_GUIDE_ICON_MARGIN_LEFT+PROGRAM_GUIDE_ICON_MARGIN_RIGHT)
	, m_fDragScroll(false)
	, m_hDragCursor1(NULL)
	, m_hDragCursor2(NULL)
	, m_VisibleEventIcons(((1<<(CEpgIcons::ICON_LAST+1))-1)^CEpgIcons::IconFlag(CEpgIcons::ICON_PAY))
	, m_fBarShadow(false)
	, m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_fShowToolTip(true)
	, m_CurrentTuningSpace(-2)
	, m_fExcludeNoEventServices(true)
	, m_pDriverManager(NULL)
	, m_BeginHour(-1)
	, m_fUpdating(false)
	, m_pEventHandler(NULL)
	, m_pFrame(NULL)
	, m_pProgramCustomizer(NULL)
	, m_WheelScrollLines(0)
	, m_ProgramSearchEventHandler(this)
	, m_Filter(0)
{
	m_WindowPosition.Left=0;
	m_WindowPosition.Top=0;
	m_WindowPosition.Width=640;
	m_WindowPosition.Height=480;

	LOGFONT lf;
	DrawUtil::GetDefaultUIFont(&lf);
	SetFont(&lf);

	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_OldScrollPos=m_ScrollPos;
	m_szDriverFileName[0]='\0';

	m_ColorList[COLOR_BACK]=::GetSysColor(COLOR_WINDOW);
	m_ColorList[COLOR_TEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_CHANNELNAMETEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_TIMETEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_TIMELINE]=m_ColorList[COLOR_TIMETEXT];
	m_ColorList[COLOR_CURTIMELINE]=RGB(255,64,0);
	for (int i=COLOR_CONTENT_FIRST;i<=COLOR_CONTENT_LAST;i++)
		m_ColorList[i]=RGB(240,240,240);
	m_ColorList[COLOR_CONTENT_NEWS]=RGB(255,255,224);
	m_ColorList[COLOR_CONTENT_SPORTS]=RGB(255,255,224);
	//m_ColorList[COLOR_CONTENT_INFORMATION]=RGB(255,255,224);
	m_ColorList[COLOR_CONTENT_DRAMA]=RGB(255,224,224);
	m_ColorList[COLOR_CONTENT_MUSIC]=RGB(224,255,224);
	m_ColorList[COLOR_CONTENT_VARIETY]=RGB(224,224,255);
	m_ColorList[COLOR_CONTENT_MOVIE]=RGB(224,255,255);
	m_ColorList[COLOR_CONTENT_ANIME]=RGB(255,224,255);
	m_ColorList[COLOR_CONTENT_DOCUMENTARY]=RGB(255,255,224);
	m_ColorList[COLOR_CONTENT_THEATER]=RGB(224,255,255);
	m_ChannelNameBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_ChannelNameBackGradient.Direction=Theme::DIRECTION_VERT;
	m_ChannelNameBackGradient.Color1=::GetSysColor(COLOR_3DFACE);
	m_ChannelNameBackGradient.Color2=m_ChannelNameBackGradient.Color1;
	m_CurChannelNameBackGradient=m_ChannelNameBackGradient;
	m_TimeBarMarginGradient.Type=Theme::GRADIENT_NORMAL;
	m_TimeBarMarginGradient.Direction=Theme::DIRECTION_HORZ;
	m_TimeBarMarginGradient.Color1=::GetSysColor(COLOR_3DFACE);
	m_TimeBarMarginGradient.Color2=m_TimeBarMarginGradient.Color1;
	for (int i=0;i<TIME_BAR_BACK_COLORS;i++) {
		m_TimeBarBackGradient[i].Type=Theme::GRADIENT_NORMAL;
		m_TimeBarBackGradient[i].Direction=Theme::DIRECTION_HORZ;
		m_TimeBarBackGradient[i].Color1=m_TimeBarMarginGradient.Color1;
		m_TimeBarBackGradient[i].Color2=m_TimeBarMarginGradient.Color2;
	}

	m_EventInfoPopup.SetEventHandler(&m_EventInfoPopupHandler);
}


CProgramGuide::~CProgramGuide()
{
	Destroy();

	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pProgramGuide=NULL;
	if (m_pFrame!=NULL)
		m_pFrame->m_pProgramGuide=NULL;
	if (m_pProgramCustomizer!=NULL)
		m_pProgramCustomizer->m_pProgramGuide=NULL;
}


bool CProgramGuide::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


void CProgramGuide::Clear()
{
	m_ServiceList.Clear();
	m_EventLayoutList.Clear();
	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_OldScrollPos=m_ScrollPos;
	m_ChannelList.Clear();
	m_TuningSpaceList.Clear();
	m_CurrentTuningSpace=-2;
	//m_CurrentChannel.Clear();
	m_szDriverFileName[0]='\0';
	if (m_hwnd!=NULL) {
		SetCaption();
		Invalidate();
	}
}


bool CProgramGuide::UpdateProgramGuide(bool fUpdateList)
{
	if (m_hwnd!=NULL) {
		HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		if (m_pFrame!=NULL)
			m_pFrame->SetCaption(TITLE_TEXT TEXT(" - 番組表を作成しています..."));
		if (UpdateList(fUpdateList)) {
			CalcLayout();
			SetScrollBar();
			GetCurrentJST(&m_stCurTime);
			Invalidate();
		}
		SetCaption();
		if (m_pFrame!=NULL) {
			m_pFrame->OnDateChanged();
			m_pFrame->OnSpaceChanged();
		}

		::SetCursor(hcurOld);
	}
	return true;
}


bool CProgramGuide::UpdateList(bool fUpdateList)
{
	if (m_pProgramList==NULL)
		return false;

	CServiceInfoData CurServiceInfo;
	if (m_ListMode==LIST_WEEK) {
		ProgramGuide::CServiceInfo *pCurService=m_ServiceList.GetItem(m_WeekListService);
		if (pCurService!=NULL)
			CurServiceInfo=*pCurService->GetServiceInfoData();
		m_WeekListService=-1;
	}

	m_ServiceList.Clear();
	for (int i=0;i<m_ChannelList.NumChannels();i++) {
		const CChannelInfo *pChannelInfo=m_ChannelList.GetChannelInfo(i);

		if (IsExcludeService(pChannelInfo->GetNetworkID(),
							 pChannelInfo->GetTransportStreamID(),
							 pChannelInfo->GetServiceID()))
			continue;

		if (fUpdateList)
			m_pProgramList->UpdateService(
				pChannelInfo->GetNetworkID(),
				pChannelInfo->GetTransportStreamID(),
				pChannelInfo->GetServiceID());
		CEpgServiceInfo *pServiceInfo=m_pProgramList->GetServiceInfo(
			pChannelInfo->GetNetworkID(),
			pChannelInfo->GetTransportStreamID(),
			pChannelInfo->GetServiceID());

		ProgramGuide::CServiceInfo *pService;

		if (pServiceInfo!=NULL) {
			pService=new ProgramGuide::CServiceInfo(pChannelInfo,*pServiceInfo);
			CEventInfoList::EventIterator itrEvent;
			for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
					itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
					itrEvent++) {
				pService->AddEvent(new CEventInfoData(itrEvent->second));
			}
			pService->SortEvents();
		} else {
			if (m_fExcludeNoEventServices)
				continue;
			pService=new ProgramGuide::CServiceInfo(pChannelInfo);
		}

		HBITMAP hbmLogo=GetAppClass().GetLogoManager()->GetAssociatedLogoBitmap(
			pService->GetNetworkID(),pService->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo!=NULL)
			pService->SetLogo(hbmLogo);

		if (m_ListMode==LIST_WEEK && pServiceInfo->m_ServiceData==CurServiceInfo)
			m_WeekListService=(int)m_ServiceList.NumServices();

		m_ServiceList.Add(pService);
	}

	if (m_ListMode==LIST_WEEK && m_WeekListService<0) {
		m_ListMode=LIST_SERVICES;
		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();
	}

	return true;
}


bool CProgramGuide::UpdateService(ProgramGuide::CServiceInfo *pService,bool fUpdateEpg)
{
	if (fUpdateEpg)
		m_pProgramList->UpdateService(pService->GetNetworkID(),pService->GetTSID(),pService->GetServiceID());

	CEpgServiceInfo *pServiceInfo=
		m_pProgramList->GetServiceInfo(pService->GetNetworkID(),pService->GetTSID(),pService->GetServiceID());
	if (pServiceInfo==NULL)
		return false;

	pService->ClearEvents();
	CEventInfoList::EventIterator itrEvent;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
			itrEvent++) {
		pService->AddEvent(new CEventInfoData(itrEvent->second));
	}
	pService->SortEvents();
	return true;
}


void CProgramGuide::CalcLayout()
{
	HDC hdc=::GetDC(m_hwnd);
	HFONT hfontOld=static_cast<HFONT>(GetCurrentObject(hdc,OBJ_FONT));

	SYSTEMTIME stFirst,stLast;
	GetCurrentTimeRange(&stFirst,&stLast);

	m_EventLayoutList.Clear();
	if (m_ListMode==LIST_SERVICES) {
		for (size_t i=0;i<m_ServiceList.NumServices();i++) {
			ProgramGuide::CServiceInfo *pService=m_ServiceList.GetItem(i);
			ProgramGuide::CEventLayout *pLayout=new ProgramGuide::CEventLayout(pService);

			pService->CalcLayout(
				pLayout,&m_ServiceList,
				hdc,stFirst,stLast,m_LinesPerHour,
				m_TitleFont.GetHandle(),m_ItemWidth,
				m_Font.GetHandle(),m_ItemWidth-m_TextLeftMargin);
			m_EventLayoutList.Add(pLayout);
		}
	} else if (m_ListMode==LIST_WEEK) {
		ProgramGuide::CServiceInfo *pCurService=m_ServiceList.GetItem(m_WeekListService);

		if (pCurService!=NULL) {
			for (int i=0;i<7;i++) {
				ProgramGuide::CEventLayout *pLayout=new ProgramGuide::CEventLayout(pCurService);

				pCurService->CalcLayout(
					pLayout,&m_ServiceList,
					hdc,stFirst,stLast,m_LinesPerHour,
					m_TitleFont.GetHandle(),m_ItemWidth,
					m_Font.GetHandle(),m_ItemWidth-m_TextLeftMargin);
				m_EventLayoutList.Add(pLayout);
				OffsetSystemTime(&stFirst,24*60*60*1000);
				OffsetSystemTime(&stLast,24*60*60*1000);
			}
		}
	}
	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);

	SetTooltip();
}


void CProgramGuide::DrawEventList(const ProgramGuide::CEventLayout *pLayout,
								  HDC hdc,const RECT &Rect,const RECT &PaintRect) const
{
	const int LineHeight=m_FontHeight+m_LineMargin;
	const int CurTimePos=Rect.top+GetCurTimeLinePos();

	HFONT hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	COLORREF OldTextColor=::GetTextColor(hdc);

	HDC hdcIcons=::CreateCompatibleDC(hdc);
	HBITMAP hbmOld=DrawUtil::SelectObject(hdcIcons,m_EpgIcons);

	RECT rcItem;
	rcItem.left=Rect.left;
	rcItem.right=Rect.right;
	for (size_t i=0;i<pLayout->NumItems();i++) {
		const ProgramGuide::CEventItem *pItem=pLayout->GetItem(i);

		if (!pItem->IsNullItem() && pItem->GetItemLines()>0) {
			rcItem.top=Rect.top+pItem->GetItemPos()*LineHeight;
			if (rcItem.top>=PaintRect.bottom)
				break;
			rcItem.bottom=rcItem.top+pItem->GetItemLines()*LineHeight;
			if (rcItem.bottom<=PaintRect.top)
				continue;

			const CEventInfoData *pEventInfo=pItem->GetEventInfo();
			const bool fCommonEvent=pEventInfo->m_fCommonEvent;
			if (fCommonEvent && pItem->GetCommonEventInfo()!=NULL)
				pEventInfo=pItem->GetCommonEventInfo();
			const int Genre1=pItem->GetGenre(0);
			const int Genre2=pItem->GetGenre(1);
			int ColorType=Genre1>=0?COLOR_CONTENT_FIRST+Genre1:COLOR_CONTENT_OTHER;
			COLORREF BackColor=m_ColorList[ColorType];
			COLORREF TextColor=m_ColorList[COLOR_TEXT];

			bool fFilter=false;
			if ((m_Filter&FILTER_FREE)!=0
					&& pEventInfo->m_FreeCaMode!=CEventInfoData::FREE_CA_MODE_UNSCRAMBLED
					&& pEventInfo->m_NetworkID>=4 && pEventInfo->m_NetworkID<=10) {
				fFilter=true;
			} else if ((m_Filter&FILTER_NEWPROGRAM)!=0
					&& (pEventInfo->GetEventName()==NULL
						|| ::StrStr(pEventInfo->GetEventName(),TEXT("[新]"))==NULL)) {
				fFilter=true;
			} else if ((m_Filter&FILTER_ORIGINAL)!=0
					&& pEventInfo->GetEventName()!=NULL
					&& ::StrStr(pEventInfo->GetEventName(),TEXT("[再]"))!=NULL) {
				fFilter=true;
			} else if ((m_Filter&FILTER_RERUN)!=0
					&& (pEventInfo->GetEventName()==NULL
						|| ::StrStr(pEventInfo->GetEventName(),TEXT("[再]"))==NULL)) {
				fFilter=true;
			} else if ((m_Filter&FILTER_NOT_SHOPPING)!=0
					&& Genre1==2 && Genre2==4) {
				fFilter=true;
			} else if ((m_Filter&FILTER_GENRE_MASK)!=0) {
				if (Genre1<0 || (m_Filter&(FILTER_GENRE_FIRST<<Genre1))==0)
					fFilter=true;
				// 映画ジャンルのアニメ
				if ((m_Filter&FILTER_ANIME)!=0 && Genre1==6 && Genre2==2)
					fFilter=false;
			}

			if (fFilter) {
				BackColor=MixColor(BackColor,m_ColorList[COLOR_BACK],96);
				TextColor=MixColor(TextColor,m_ColorList[COLOR_BACK],96);
			} else {
				if (fCommonEvent)
					BackColor=MixColor(BackColor,m_ColorList[COLOR_BACK],192);
			}
			::SetTextColor(hdc,TextColor);
			DrawUtil::Fill(hdc,&rcItem,BackColor);
			HPEN hpen=::CreatePen(PS_SOLID,1,
				MixColor(BackColor,RGB(0,0,0),pItem->GetStartTime().wMinute==0?192:224));
			HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,hpen));
			::MoveToEx(hdc,rcItem.left,rcItem.top,NULL);
			::LineTo(hdc,rcItem.right,rcItem.top);
			::SelectObject(hdc,hpenOld);
			::DeleteObject(hpen);

			if (((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_ListMode==LIST_WEEK)
					&& CurTimePos>=rcItem.top && CurTimePos<rcItem.bottom) {
				LOGBRUSH lb;

				lb.lbStyle=BS_SOLID;
				lb.lbColor=MixColor(m_ColorList[COLOR_CURTIMELINE],BackColor,64);
				hpen=::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
									CUR_TIME_LINE_WIDTH,&lb,0,NULL);
				::SelectObject(hdc,hpen);
				::MoveToEx(hdc,rcItem.left,CurTimePos,NULL);
				::LineTo(hdc,rcItem.right,CurTimePos);
				::SelectObject(hdc,hpenOld);
				::DeleteObject(hpen);
			}

			RECT rcTitle,rcText;
			rcTitle=rcItem;
			rcTitle.bottom=min(rcItem.bottom,rcItem.top+pItem->GetTitleLines()*LineHeight);
			rcText.left=rcItem.left+m_TextLeftMargin;
			rcText.top=rcTitle.bottom;
			rcText.right=rcItem.right;
			rcText.bottom=rcItem.bottom;

			if (m_pProgramCustomizer!=NULL)
				m_pProgramCustomizer->DrawBackground(*pEventInfo,hdc,rcItem,rcTitle,rcText,BackColor);

			DrawUtil::SelectObject(hdc,m_TitleFont);
			pItem->DrawTitle(hdc,&rcTitle,LineHeight);

			if (rcText.bottom>rcTitle.bottom) {
				DrawUtil::SelectObject(hdc,m_Font);
				pItem->DrawText(hdc,&rcText,LineHeight);

				const unsigned int ShowIcons=
					CEpgIcons::GetEventIcons(pEventInfo) & m_VisibleEventIcons;
				if (ShowIcons!=0) {
					int y=rcText.top+PROGRAM_GUIDE_ICON_MARGIN_TOP;
					int Icon=0;
					for (unsigned int Flag=ShowIcons;Flag!=0;Flag>>=1) {
						if (y>=rcText.bottom)
							break;
						if ((Flag&1)!=0) {
							CEpgIcons::Draw(hdc,rcItem.left+PROGRAM_GUIDE_ICON_MARGIN_LEFT,y,
											hdcIcons,Icon,
											CEpgIcons::ICON_WIDTH,
											min(CEpgIcons::ICON_HEIGHT,rcText.bottom-y),
											(fCommonEvent || fFilter)?128:255);
							y+=CEpgIcons::ICON_HEIGHT+PROGRAM_GUIDE_ICON_MARGIN;
						}
						Icon++;
					}
				}
			}
		}
	}

	::SelectObject(hdcIcons,hbmOld);
	::DeleteDC(hdcIcons);

	::SetTextColor(hdc,OldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawHeaderBackground(HDC hdc,const RECT &Rect,bool fCur) const
{
	const Theme::GradientInfo &Gradient=
		fCur?m_CurChannelNameBackGradient:m_ChannelNameBackGradient;
	RECT rc;

	rc=Rect;
	rc.left++;
	rc.right--;
	Theme::FillGradient(hdc,&rc,&Gradient);

	Theme::GradientInfo Border;
	Border.Type=Gradient.Type;
	Border.Direction=Theme::DIRECTION_VERT;
	Border.Color1=MixColor(Gradient.Color1,RGB(255,255,255),192);
	Border.Color2=MixColor(Gradient.Color2,RGB(255,255,255),192);
	rc=Rect;
	rc.right=rc.left+1;
	Theme::FillGradient(hdc,&rc,&Border);

	Border.Color1=MixColor(Gradient.Color1,RGB(0,0,0),192);
	Border.Color2=MixColor(Gradient.Color2,RGB(0,0,0),192);
	rc=Rect;
	rc.left=rc.right-1;
	Theme::FillGradient(hdc,&rc,&Border);
}


void CProgramGuide::DrawServiceHeader(ProgramGuide::CServiceInfo *pServiceInfo,
									  HDC hdc,const RECT &Rect,int Chevron,
									  bool fLeftAlign)
{
	bool fCur=pServiceInfo->GetNetworkID()==m_CurrentChannel.NetworkID
		//&& pServiceInfo->GetTransportStreamID()==m_CurrentChannel.TransportStreamID
		&& pServiceInfo->GetServiceID()==m_CurrentChannel.ServiceID;
	RECT rc;

	DrawHeaderBackground(hdc,Rect,fCur);

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
	COLORREF TextColor=m_ColorList[fCur?COLOR_CURCHANNELNAMETEXT:COLOR_CHANNELNAMETEXT];
	COLORREF OldTextColor=::SetTextColor(hdc,TextColor);
	rc=Rect;
	rc.left+=HEADER_MARGIN_LEFT;
	rc.right-=HEADER_MARGIN_RIGHT;

	HBITMAP hbmLogo=pServiceInfo->GetLogo();
	if (hbmLogo!=NULL) {
		int LogoWidth,LogoHeight;
		LogoHeight=min(rc.bottom-rc.top-4,14);
		LogoWidth=LogoHeight*16/9;
		HBITMAP hbmStretched=pServiceInfo->GetStretchedLogo(LogoWidth,LogoHeight);
		DrawUtil::DrawBitmap(hdc,rc.left,rc.top+(rc.bottom-rc.top-LogoHeight)/2,
							 LogoWidth,LogoHeight,
							 hbmStretched!=NULL?hbmStretched:hbmLogo,NULL,192);
		rc.left+=LogoWidth+HEADER_MARGIN_LEFT;
	}

	rc.right-=CHEVRON_WIDTH;
	m_Chevron.Draw(hdc,rc.right,rc.top+((rc.bottom-rc.top)-CHEVRON_HEIGHT)/2,
				   TextColor,Chevron*CHEVRON_WIDTH,0,CHEVRON_WIDTH,CHEVRON_HEIGHT);
	rc.right-=HEADER_MARGIN_RIGHT;

	::DrawText(hdc,pServiceInfo->GetServiceName(),-1,&rc,
			   (fLeftAlign || hbmLogo!=NULL?DT_LEFT:DT_CENTER) |
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	::SetTextColor(hdc,OldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawDayHeader(int Day,HDC hdc,const RECT &Rect) const
{
	SYSTEMTIME st;
	GetCurrentTimeRange(&st,NULL);
	if (Day>0)
		OffsetSystemTime(&st,Day*(24*60*60*1000));

	DrawHeaderBackground(hdc,Rect,false);

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_TitleFont);
	COLORREF OldTextColor=::SetTextColor(hdc,m_ColorList[COLOR_CHANNELNAMETEXT]);
	TCHAR szText[64];
	StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d(%s)"),
					  st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek));
	RECT rc=Rect;
	rc.left+=HEADER_MARGIN_LEFT;
	rc.right-=HEADER_MARGIN_RIGHT;
	::DrawText(hdc,szText,-1,&rc,
			   DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	::SetTextColor(hdc,OldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawTimeBar(HDC hdc,const RECT &Rect,bool fRight)
{
	const int PixelsPerHour=(m_FontHeight+m_LineMargin)*m_LinesPerHour;
	const int CurTimePos=Rect.top+GetCurTimeLinePos();
	HFONT hfontOld;
	COLORREF crOldTextColor;
	HPEN hpen,hpenOld;
	RECT rc;

	hfontOld=DrawUtil::SelectObject(hdc,m_TimeFont);
	crOldTextColor=::SetTextColor(hdc,m_ColorList[COLOR_TIMETEXT]);
	hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMETEXT]);
	hpenOld=SelectPen(hdc,hpen);
	rc.left=Rect.left;
	rc.top=Rect.top;
	rc.right=Rect.right;
	for (int i=0;i<m_Hours;i++) {
		int Hour;

		if ((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_BeginHour<0)
			Hour=(m_stFirstTime.wHour+i)%24;
		else
			Hour=(m_BeginHour+i)%24;
		rc.bottom=rc.top+PixelsPerHour;
		Theme::FillGradient(hdc,&rc,&m_TimeBarBackGradient[Hour/3]);
		::MoveToEx(hdc,rc.left,rc.top,NULL);
		::LineTo(hdc,rc.right,rc.top);
		if (((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_ListMode==LIST_WEEK)
				&& CurTimePos>=rc.top && CurTimePos<rc.bottom) {
			const int TriangleHeight=m_FontHeight*2/3;
			const int TriangleWidth=TriangleHeight*8/10;
			POINT ptTriangle[3];
			HBRUSH hbr,hbrOld;

			hbr=::CreateSolidBrush(m_ColorList[COLOR_CURTIMELINE]);
			hbrOld=SelectBrush(hdc,hbr);
			SelectObject(hdc,::CreatePen(PS_SOLID,1,m_ColorList[COLOR_CURTIMELINE]));
			if (fRight) {
				ptTriangle[0].x=rc.left;
				ptTriangle[0].y=CurTimePos;
				ptTriangle[1].x=ptTriangle[0].x+TriangleWidth;
				ptTriangle[1].y=ptTriangle[0].y-TriangleHeight/2;
				ptTriangle[2].x=ptTriangle[0].x+TriangleWidth;
				ptTriangle[2].y=ptTriangle[1].y+TriangleHeight;
			} else {
				ptTriangle[0].x=rc.right-1;
				ptTriangle[0].y=CurTimePos;
				ptTriangle[1].x=ptTriangle[0].x-TriangleWidth;
				ptTriangle[1].y=ptTriangle[0].y-TriangleHeight/2;
				ptTriangle[2].x=ptTriangle[0].x-TriangleWidth;
				ptTriangle[2].y=ptTriangle[1].y+TriangleHeight;
			}
			::Polygon(hdc,ptTriangle,3);
			::SelectObject(hdc,hbrOld);
			::DeleteObject(hbr);
			::DeleteObject(::SelectObject(hdc,hpen));
		}
		TCHAR szText[64];
		if (m_ListMode==LIST_SERVICES && (i==0 || Hour==0)) {
			SYSTEMTIME st;
			GetCurrentTimeRange(&st,NULL);
			if (i>0)
				OffsetSystemTime(&st,(LONGLONG)i*(1000*60*60));
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d(%s) %d時"),
							  st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),Hour);
		} else {
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%d"),Hour);
		}
		::TextOut(hdc,rc.right-4,rc.top+4,szText,lstrlen(szText));
		rc.top=rc.bottom;
	}

	if (m_ListMode==LIST_SERVICES && m_Day<DAY_LAST) {
		// ▼
		RECT rcClient;

		GetClientRect(&rcClient);
		if (rc.top-m_TimeBarWidth<rcClient.bottom) {
			const int TriangleWidth=m_FontHeight*2/3;
			const int TriangleHeight=TriangleWidth*8/10;
			POINT ptTriangle[3];
			HBRUSH hbr,hbrOld;

			hbr=::CreateSolidBrush(m_ColorList[COLOR_TIMETEXT]);
			hbrOld=SelectBrush(hdc,hbr);
			ptTriangle[0].x=m_TimeBarWidth/2;
			ptTriangle[0].y=rc.top-(m_TimeBarWidth-TriangleHeight)/2;
			ptTriangle[1].x=ptTriangle[0].x-TriangleWidth/2;
			ptTriangle[1].y=ptTriangle[0].y-TriangleHeight;
			ptTriangle[2].x=ptTriangle[0].x+TriangleWidth/2;
			ptTriangle[2].y=ptTriangle[1].y;
			::Polygon(hdc,ptTriangle,3);
			for (int i=0;i<3;i++)
				ptTriangle[i].x+=rcClient.right-m_TimeBarWidth;
			::Polygon(hdc,ptTriangle,3);
			::SelectObject(hdc,hbrOld);
			::DeleteObject(hbr);
		}
	}

	::SelectObject(hdc,hpenOld);
	::DeleteObject(hpen);
	::SetTextColor(hdc,crOldTextColor);
	SelectFont(hdc,hfontOld);
}


void CProgramGuide::Draw(HDC hdc,const RECT &PaintRect)
{
	RECT rcClient,rcGuide,rc;
	HRGN hrgn;

	::GetClientRect(m_hwnd,&rcClient);
	GetProgramGuideRect(&rcGuide);

	if (::IntersectRect(&rc,&rcGuide,&PaintRect)) {
		HBRUSH hbr=::CreateSolidBrush(m_ColorList[COLOR_BACK]);
		::FillRect(hdc,&rc,hbr);
		::DeleteObject(hbr);
	}

	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	int HeaderHeight=m_HeaderHeight;
	if (m_ListMode==LIST_WEEK)
		HeaderHeight+=m_HeaderHeight;

	if (m_ServiceList.NumServices()>0) {
		if (PaintRect.top<HeaderHeight) {
			rc.left=rcClient.left+m_TimeBarWidth;
			rc.top=0;
			rc.right=rcClient.right-m_TimeBarWidth;
			rc.bottom=HeaderHeight;
			hrgn=::CreateRectRgnIndirect(&rc);
			::SelectClipRgn(hdc,hrgn);
			if (m_ListMode==LIST_SERVICES) {
				rc.left=m_TimeBarWidth-m_ScrollPos.x;
				for (size_t i=0;i<m_ServiceList.NumServices();i++) {
					rc.right=rc.left+(m_ItemWidth+m_ItemMargin*2);
					if (rc.left<PaintRect.right && rc.right>PaintRect.left)
						DrawServiceHeader(m_ServiceList.GetItem(i),hdc,rc,2);
					rc.left=rc.right;
				}
			} else if (m_ListMode==LIST_WEEK) {
				rc.bottom=m_HeaderHeight;
				DrawServiceHeader(m_ServiceList.GetItem(m_WeekListService),hdc,rc,3,true);
				rc.left=m_TimeBarWidth-m_ScrollPos.x;
				rc.top=rc.bottom;
				rc.bottom+=m_HeaderHeight;
				for (int i=0;i<7;i++) {
					rc.right=rc.left+(m_ItemWidth+m_ItemMargin*2);
					if (rc.left<PaintRect.right && rc.right>PaintRect.left)
						DrawDayHeader(i,hdc,rc);
					rc.left=rc.right;
				}
			}
			if (rc.left<PaintRect.right) {
				rc.right=PaintRect.right;
				Theme::FillGradient(hdc,&rc,&m_ChannelNameBackGradient);
			}
			::SelectClipRgn(hdc,NULL);
			::DeleteObject(hrgn);
		}

		hrgn=::CreateRectRgnIndirect(&rcGuide);
		::SelectClipRgn(hdc,hrgn);
		rc.top=HeaderHeight-m_ScrollPos.y*(m_FontHeight+m_LineMargin);
		rc.left=m_TimeBarWidth+m_ItemMargin-m_ScrollPos.x;
		HPEN hpen,hpenOld;
		hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMELINE]);
		hpenOld=SelectPen(hdc,hpen);
		int PixelsPerHour=(m_FontHeight+m_LineMargin)*m_LinesPerHour;
		int CurTimePos=rc.top+GetCurTimeLinePos();
		for (size_t i=0;i<m_EventLayoutList.Length();i++) {
			rc.right=rc.left+m_ItemWidth;
			if (rc.top<PaintRect.bottom) {
				for (int j=0;j<m_Hours;j++) {
					int y=rc.top+j*PixelsPerHour;
					if (y>=PaintRect.top && y<PaintRect.bottom) {
						/*
						::MoveToEx(hdc,rc.left-m_ItemMargin,y,NULL);
						::LineTo(hdc,rc.left,y);
						::MoveToEx(hdc,rc.right,y,NULL);
						::LineTo(hdc,rc.right+m_ItemMargin,y);
						*/
						::MoveToEx(hdc,rc.left-m_ItemMargin,y,NULL);
						::LineTo(hdc,rc.right+m_ItemMargin,y);
					}
					if (((m_ListMode==LIST_SERVICES && m_Day==DAY_TODAY) || m_ListMode==LIST_WEEK)
							&& CurTimePos>=y && CurTimePos<y+PixelsPerHour) {
						HPEN hpenCurTime;
						LOGBRUSH lb;

						lb.lbStyle=BS_SOLID;
						lb.lbColor=m_ColorList[COLOR_CURTIMELINE];
						hpenCurTime=::ExtCreatePen(
							PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
							CUR_TIME_LINE_WIDTH,&lb,0,NULL);
						::SelectObject(hdc,hpenCurTime);
						::MoveToEx(hdc,rc.left-m_ItemMargin,CurTimePos,NULL);
						::LineTo(hdc,rc.right+m_ItemMargin,CurTimePos);
						::SelectObject(hdc,hpen);
						::DeleteObject(hpenCurTime);
					}
				}
				if (rc.left<PaintRect.right && rc.right>PaintRect.left)
					DrawEventList(m_EventLayoutList[i],hdc,rc,PaintRect);
			}
			rc.left=rc.right+m_ItemMargin*2;
		}
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hpen);
		::SelectClipRgn(hdc,NULL);
		::DeleteObject(hrgn);
	} else {
		if (PaintRect.top<m_HeaderHeight) {
			rc.left=max(PaintRect.left,m_TimeBarWidth);
			rc.right=min(PaintRect.right,rcClient.right-m_TimeBarWidth);
			if (rc.left<rc.right) {
				rc.top=0;
				rc.bottom=m_HeaderHeight;
				Theme::FillGradient(hdc,&rc,&m_ChannelNameBackGradient);
			}
		}
	}

	rc.left=0;
	rc.top=HeaderHeight;
	rc.right=rcClient.right;
	rc.bottom=rcClient.bottom;
	hrgn=::CreateRectRgnIndirect(&rc);
	::SelectClipRgn(hdc,hrgn);
	rc.top=HeaderHeight-m_ScrollPos.y*(m_FontHeight+m_LineMargin);
		rc.bottom=rc.top+(m_FontHeight+m_LineMargin)*m_LinesPerHour*m_Hours;
	if (PaintRect.left<m_TimeBarWidth) {
		rc.left=0;
		rc.right=m_TimeBarWidth;
		DrawTimeBar(hdc,rc,false);
	}
	rc.left=rcClient.right-m_TimeBarWidth;
	if (rc.left<PaintRect.right) {
		rc.right=rcClient.right;
		DrawTimeBar(hdc,rc,true);
	}
	::SelectClipRgn(hdc,NULL);
	::DeleteObject(hrgn);

	if (rc.bottom<PaintRect.bottom) {
		::SetRect(&rc,0,rc.bottom,m_TimeBarWidth,rcClient.bottom);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
		::OffsetRect(&rc,rcGuide.right,0);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
	}
	if (PaintRect.top<HeaderHeight) {
		::SetRect(&rc,0,0,m_TimeBarWidth,HeaderHeight);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
		::OffsetRect(&rc,rcGuide.right,0);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
	}

	if (m_ListMode==LIST_SERVICES && m_Day!=DAY_TODAY
			&& PaintRect.top<m_HeaderHeight) {
		// ▲
		const int TriangleWidth=m_FontHeight*2/3;
		const int TriangleHeight=TriangleWidth*8/10;
		POINT ptTriangle[3];
		HPEN hpen,hpenOld;
		HBRUSH hbr,hbrOld;

		hbr=::CreateSolidBrush(m_ColorList[COLOR_TIMETEXT]);
		hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMETEXT]);
		hbrOld=SelectBrush(hdc,hbr);
		hpenOld=SelectPen(hdc,hpen);
		ptTriangle[0].x=m_TimeBarWidth/2;
		ptTriangle[0].y=(m_HeaderHeight-TriangleHeight)/2;
		ptTriangle[1].x=ptTriangle[0].x-TriangleWidth/2;
		ptTriangle[1].y=ptTriangle[0].y+TriangleHeight;
		ptTriangle[2].x=ptTriangle[0].x+TriangleWidth/2;
		ptTriangle[2].y=ptTriangle[1].y;
		::Polygon(hdc,ptTriangle,3);
		for (int i=0;i<3;i++)
			ptTriangle[i].x+=rcClient.right-m_TimeBarWidth;
		::Polygon(hdc,ptTriangle,3);
		::SelectObject(hdc,hbrOld);
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hbr);
		::DeleteObject(hpen);
	}

	::SetBkMode(hdc,OldBkMode);

	if (m_fBarShadow) {
		rc.left=rcGuide.left;
		rc.top=rcGuide.top;
		rc.right=rcGuide.right;
		rc.bottom=min(rc.top+HEADER_SHADOW_HEIGHT,rcGuide.bottom);
		DrawUtil::FillGradient(hdc,&rc,DrawUtil::RGBA(0,0,0,80),DrawUtil::RGBA(0,0,0,0),
							   DrawUtil::DIRECTION_VERT);
		rc.top=rcGuide.top;
		rc.bottom=rcGuide.bottom;
		rc.left=rcGuide.left;
		rc.right=min(rc.left+TIMEBAR_SHADOW_WIDTH,rcGuide.right);
		DrawUtil::FillGradient(hdc,&rc,DrawUtil::RGBA(0,0,0,64),DrawUtil::RGBA(0,0,0,0),
							   DrawUtil::DIRECTION_HORZ);
		rc.right=rcGuide.right;
		rc.left=max(rc.right-TIMEBAR_SHADOW_WIDTH,rcGuide.left);
		DrawUtil::FillGradient(hdc,&rc,DrawUtil::RGBA(0,0,0,0),DrawUtil::RGBA(0,0,0,48),
							   DrawUtil::DIRECTION_HORZ);
	}
}


int CProgramGuide::GetCurTimeLinePos() const
{
	SYSTEMTIME stFirst;
	LONGLONG Span;

	GetCurrentTimeRange(&stFirst,NULL);
	Span=DiffSystemTime(&stFirst,&m_stCurTime)%(24LL*60*60*1000);
	if (Span<0)
		Span+=24*60*60*1000;
	return (int)(Span*(LONGLONG)((m_FontHeight+m_LineMargin)*m_LinesPerHour)/(60*60*1000));
}


void CProgramGuide::GetProgramGuideRect(RECT *pRect) const
{
	GetClientRect(pRect);
	pRect->left+=m_TimeBarWidth;
	pRect->right-=m_TimeBarWidth;
	pRect->top+=m_HeaderHeight;
	if (m_ListMode==LIST_WEEK)
		pRect->top+=m_HeaderHeight;
}


void CProgramGuide::Scroll(int XScroll,int YScroll)
{
	POINT Pos=m_ScrollPos;
	RECT rcGuide;
	SCROLLINFO si;
	int XScrollSize=0,YScrollSize=0;

	GetProgramGuideRect(&rcGuide);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_POS;
	if (XScroll!=0) {
		int TotalWidth=(int)m_EventLayoutList.Length()*(m_ItemWidth+m_ItemMargin*2);
		int XPage=max(rcGuide.right-rcGuide.left,0);

		Pos.x=m_ScrollPos.x+XScroll;
		if (Pos.x<0)
			Pos.x=0;
		else if (Pos.x>max(TotalWidth-XPage,0))
			Pos.x=max(TotalWidth-XPage,0);
		si.nPos=Pos.x;
		::SetScrollInfo(m_hwnd,SB_HORZ,&si,TRUE);
		XScrollSize=m_ScrollPos.x-Pos.x;
	}
	if (YScroll!=0) {
		int TotalLines=m_Hours*m_LinesPerHour;
		int YPage=(rcGuide.bottom-rcGuide.top)/(m_FontHeight+m_LineMargin);

		Pos.y=m_ScrollPos.y+YScroll;
		if (Pos.y<0)
			Pos.y=0;
		else if (Pos.y>max(TotalLines-YPage,0))
			Pos.y=max(TotalLines-YPage,0);
		si.nPos=Pos.y;
		::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
		YScrollSize=(m_ScrollPos.y-Pos.y)*(m_FontHeight+m_LineMargin);
	}

	m_ScrollPos=Pos;

	if (!CBufferedPaint::IsSupported()) {
		RECT rcClip=rcGuide;
		if (m_fBarShadow) {
			rcClip.top+=HEADER_SHADOW_HEIGHT;
			rcClip.left+=TIMEBAR_SHADOW_WIDTH;
			rcClip.right-=TIMEBAR_SHADOW_WIDTH;
		}
		if (rcClip.right>rcClip.left && rcClip.bottom>rcClip.top
				&& abs(YScrollSize)<rcClip.bottom-rcClip.top
				&& abs(XScrollSize)<rcClip.right-rcClip.left) {
			::ScrollWindowEx(m_hwnd,XScrollSize,YScrollSize,&rcGuide,&rcClip,
							 NULL,NULL,SW_INVALIDATE);

			if (m_fBarShadow) {
				RECT rc;
				rc.left=rcGuide.left;
				rc.top=rcGuide.top;
				rc.right=rcGuide.right;
				rc.bottom=rcClip.top;
				::InvalidateRect(m_hwnd,&rc,FALSE);
				rc.top=rcGuide.top;
				rc.bottom=rcGuide.bottom;
				rc.left=rcGuide.left;
				rc.right=rcClip.left;
				::InvalidateRect(m_hwnd,&rc,FALSE);
				rc.left=rcClip.right;
				rc.right=rcGuide.right;
				::InvalidateRect(m_hwnd,&rc,FALSE);
			}

			if (XScrollSize!=0) {
				RECT rcHeader;

				::SetRect(&rcHeader,rcGuide.left,0,rcGuide.right,rcGuide.top);
				if (m_ListMode==LIST_WEEK)
					rcHeader.top+=m_HeaderHeight;
				::ScrollWindowEx(m_hwnd,XScrollSize,0,&rcHeader,&rcHeader,NULL,NULL,SW_INVALIDATE);
			}

			if (YScrollSize!=0) {
				RECT rcTime;

				::SetRect(&rcTime,0,rcGuide.top,rcGuide.left,rcGuide.bottom);
				::ScrollWindowEx(m_hwnd,0,YScrollSize,&rcTime,&rcTime,NULL,NULL,SW_INVALIDATE);
				rcTime.left=rcGuide.right;
				rcTime.right=rcTime.left+m_TimeBarWidth;
				::ScrollWindowEx(m_hwnd,0,YScrollSize,&rcTime,&rcTime,NULL,NULL,SW_INVALIDATE);
			}
		} else {
			Invalidate();
		}
	} else {
		Invalidate();
	}

	SetTooltip();
}


void CProgramGuide::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	GetProgramGuideRect(&rc);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_Hours*m_LinesPerHour-1;
	si.nPage=(rc.bottom-rc.top)/(m_FontHeight+m_LineMargin);
	si.nPos=m_ScrollPos.y;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
	si.nMax=(int)m_EventLayoutList.Length()*(m_ItemWidth+m_ItemMargin*2)-1;
	si.nPage=rc.right-rc.left;
	si.nPos=m_ScrollPos.x;
	::SetScrollInfo(m_hwnd,SB_HORZ,&si,TRUE);
}


void CProgramGuide::SetCaption()
{
	if (m_hwnd!=NULL && m_pFrame!=NULL) {
		if (m_pProgramList!=NULL) {
			if (m_fUpdating) {
				m_pFrame->SetCaption(TITLE_TEXT TEXT(" - 番組表の取得中..."));
			} else {
				TCHAR szText[256];
				SYSTEMTIME stFirst,stLast;

				GetCurrentTimeRange(&stFirst,&stLast);
				if (m_ListMode==LIST_SERVICES) {
					OffsetSystemTime(&stLast,-60*60*1000);
					StdUtil::snprintf(szText,lengthof(szText),
						TITLE_TEXT TEXT(" - %s %d/%d(%s) %d時 ～ %d/%d(%s) %d時"),
						DayText[m_Day],stFirst.wMonth,stFirst.wDay,
						GetDayOfWeekText(stFirst.wDayOfWeek),stFirst.wHour,
						stLast.wMonth,stLast.wDay,
						GetDayOfWeekText(stLast.wDayOfWeek),stLast.wHour);
				} else {
					stLast=stFirst;
					OffsetSystemTime(&stLast,6LL*24*60*60*1000);
					StdUtil::snprintf(szText,lengthof(szText),
						TITLE_TEXT TEXT(" - %s %d/%d(%s) ～ %d/%d(%s)"),
						m_ServiceList.GetItem(m_WeekListService)->GetServiceName(),
						stFirst.wMonth,stFirst.wDay,
						GetDayOfWeekText(stFirst.wDayOfWeek),
						stLast.wMonth,stLast.wDay,
						GetDayOfWeekText(stLast.wDayOfWeek));
				}
				m_pFrame->SetCaption(szText);
			}
		} else {
			m_pFrame->SetCaption(TITLE_TEXT);
		}
	}
}


void CProgramGuide::SetTooltip()
{
	RECT rc;

	if (m_ListMode==LIST_SERVICES) {
		int NumTools=m_Tooltip.NumTools();
		int NumServices=(int)m_ServiceList.NumServices();

		RECT rcClient,rcHeader;
		GetClientRect(&rcClient);
		rcHeader.left=m_TimeBarWidth;
		rcHeader.right=rcClient.right-m_TimeBarWidth;
		rcHeader.top=0;
		rcHeader.bottom=m_HeaderHeight;

		rc.left=m_TimeBarWidth-m_ScrollPos.x;
		rc.top=0;
		rc.bottom=m_HeaderHeight;
		int ToolCount=0;
		for (int i=0;i<NumServices;i++) {
			rc.right=rc.left+(m_ItemWidth+m_ItemMargin*2);
			rc.left=rc.right-(HEADER_MARGIN_RIGHT+CHEVRON_WIDTH);
			if (rc.left>=rcHeader.right)
				break;
			if (rc.right>rcHeader.left) {
				RECT rcTool;

				::IntersectRect(&rcTool,&rc,&rcHeader);
				if (ToolCount<NumTools) {
					m_Tooltip.SetText(ToolCount,TEXT("1週間表示"));
					m_Tooltip.SetToolRect(ToolCount,rcTool);
				} else {
					m_Tooltip.AddTool(ToolCount,rcTool,TEXT("1週間表示"));
				}
				ToolCount++;
			}
			rc.left=rc.right;
		}

		while (NumTools>ToolCount) {
			m_Tooltip.DeleteTool(--NumTools);
		}

		if (m_Day>DAY_TODAY) {
			rc.top=0;
			rc.bottom=m_HeaderHeight;
			rc.left=0;
			rc.right=m_TimeBarWidth;
			m_Tooltip.AddTool(ToolCount++,rc,TEXT("一日前へ"));
			rc.left=rcClient.right-m_TimeBarWidth;
			rc.right=rcClient.right;
			m_Tooltip.AddTool(ToolCount++,rc,TEXT("一日前へ"));
		}
		if (m_Day<DAY_LAST) {
			int y=m_HeaderHeight+
				(m_Hours*m_LinesPerHour-m_ScrollPos.y)*(m_FontHeight+m_LineMargin);
			rc.top=y-m_TimeBarWidth;
			rc.bottom=y;
			rc.left=0;
			rc.right=m_TimeBarWidth;
			m_Tooltip.AddTool(ToolCount++,rc,TEXT("一日後へ"));
			rc.left=rcClient.right-m_TimeBarWidth;
			rc.right=rcClient.right;
			m_Tooltip.AddTool(ToolCount,rc,TEXT("一日後へ"));
		}
	} else if (m_ListMode==LIST_WEEK) {
		m_Tooltip.DeleteAllTools();
		GetClientRect(&rc);
		rc.left+=m_TimeBarWidth;
		rc.right-=m_TimeBarWidth;
		rc.bottom=m_HeaderHeight;
		m_Tooltip.AddTool(0,rc,TEXT("チャンネル一覧表示へ"));
	}
}


bool CProgramGuide::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
}


bool CProgramGuide::SetTuningSpaceList(LPCTSTR pszDriverFileName,const CTuningSpaceList *pList,int Space)
{
	m_TuningSpaceList=*pList;
	SetTuningSpace(Space);
	::lstrcpy(m_szDriverFileName,pszDriverFileName);
	return true;
}


bool CProgramGuide::SetTuningSpace(int Space)
{
	const CChannelList *pList;

	if (Space<0)
		pList=m_TuningSpaceList.GetAllChannelList();
	else
		pList=m_TuningSpaceList.GetChannelList(Space);

	int OldTSID,OldNumChannels=m_ChannelList.NumChannels();
	if (OldNumChannels>0)
		OldTSID=m_ChannelList.GetChannelInfo(0)->GetTransportStreamID();
	else
		OldTSID=0;
	m_ChannelList.Clear();
	if (pList!=NULL && pList->NumChannels()>0) {
		for (int i=0;i<pList->NumChannels();i++) {
			const CChannelInfo *pChannelInfo=pList->GetChannelInfo(i);

			if (pChannelInfo->IsEnabled())
				m_ChannelList.AddChannel(*pChannelInfo);
		}
		if (OldTSID!=pList->GetChannelInfo(0)->GetTransportStreamID()
				|| OldNumChannels!=pList->NumChannels()) {
			m_ScrollPos.x=0;
			m_ScrollPos.y=0;
			m_OldScrollPos=m_ScrollPos;
		}
	} else {
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
		m_OldScrollPos=m_ScrollPos;
	}
	m_CurrentTuningSpace=Space;
	return true;
}


bool CProgramGuide::UpdateChannelList()
{
	return SetTuningSpace(m_CurrentTuningSpace);
}


bool CProgramGuide::SetDriverList(const CDriverManager *pDriverManager)
{
	m_pDriverManager=pDriverManager;
	return true;
}


void CProgramGuide::SetCurrentService(WORD NetworkID,WORD TSID,WORD ServiceID)
{
	m_CurrentChannel.NetworkID=NetworkID;
	m_CurrentChannel.TransportStreamID=TSID;
	m_CurrentChannel.ServiceID=ServiceID;
	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_HeaderHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CProgramGuide::GetTuningSpaceName(int Space,LPTSTR pszName,int MaxName) const
{
	if (MaxName<1)
		return false;
	if (Space==-1) {
		::lstrcpyn(pszName,TEXT("すべてのチャンネル"),MaxName);
	} else {
		if (Space<0 || Space>=m_TuningSpaceList.NumSpaces()) {
			pszName[0]='\0';
			return false;
		}
		LPCTSTR p=m_TuningSpaceList.GetTuningSpaceName(Space);
		if (p!=NULL) {
			::lstrcpyn(pszName,p,MaxName);
		} else {
			StdUtil::snprintf(pszName,MaxName,TEXT("チューニング空間 %d"),Space+1);
		}
	}
	return true;
}


bool CProgramGuide::EnumDriver(int *pIndex,LPTSTR pszName,int MaxName) const
{
	if (m_pDriverManager==NULL)
		return false;
	(*pIndex)++;
	if (*pIndex<0 || *pIndex>=m_pDriverManager->NumDrivers())
		return false;
	const CDriverInfo *pDriverInfo=m_pDriverManager->GetDriverInfo(*pIndex);
	if (pDriverInfo==NULL)
		return false;
	if (CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName()))
		return EnumDriver(pIndex,pszName,MaxName);
	::lstrcpyn(pszName,pDriverInfo->GetFileName(),MaxName);
	return true;
}


bool CProgramGuide::SetExcludeNoEventServices(bool fExclude)
{
	m_fExcludeNoEventServices=fExclude;
	return true;
}


bool CProgramGuide::SetExcludeServiceList(const ServiceInfoList &List)
{
	m_ExcludeServiceList=List;
	return true;
}


bool CProgramGuide::GetExcludeServiceList(ServiceInfoList *pList) const
{
	if (pList==NULL)
		return false;
	*pList=m_ExcludeServiceList;
	return true;
}


bool CProgramGuide::IsExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const
{
	for (auto i=m_ExcludeServiceList.begin();i!=m_ExcludeServiceList.end();i++) {
		if ((NetworkID==0 || i->NetworkID==NetworkID)
				&& (TransportStreamID==0 || i->TransportStreamID==TransportStreamID)
				&& i->ServiceID==ServiceID)
			return true;
	}
	return false;
}


bool CProgramGuide::SetExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fExclude)
{
	for (auto i=m_ExcludeServiceList.begin();i!=m_ExcludeServiceList.end();i++) {
		if (i->NetworkID==NetworkID
				&& i->TransportStreamID==TransportStreamID
				&& i->ServiceID==ServiceID) {
			if (!fExclude)
				m_ExcludeServiceList.erase(i);
			return true;
		}
	}
	if (fExclude)
		m_ExcludeServiceList.push_back(ServiceInfo(NetworkID,TransportStreamID,ServiceID));
	return true;
}


bool CProgramGuide::SetServiceListMode()
{
	if (m_ListMode!=LIST_SERVICES) {
		m_ListMode=LIST_SERVICES;
		m_WeekListService=-1;

		HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		m_ScrollPos=m_OldScrollPos;
		CalcLayout();
		SetScrollBar();
		SetCaption();
		Invalidate();

		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();

		::SetCursor(hcurOld);
	}
	return true;
}


bool CProgramGuide::SetWeekListMode(int Service)
{
	ProgramGuide::CServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);

	if (pServiceInfo==NULL)
		return false;
	if (m_ListMode!=LIST_WEEK || m_WeekListService!=Service) {
		m_ListMode=LIST_WEEK;
		m_WeekListService=Service;

		HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));

		UpdateService(pServiceInfo,true);
		m_OldScrollPos=m_ScrollPos;
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
		CalcLayout();
		SetScrollBar();
		SetCaption();
		Invalidate();

		if (m_pFrame!=NULL)
			m_pFrame->OnListModeChanged();

		::SetCursor(hcurOld);
	}
	return true;
}


bool CProgramGuide::SetBeginHour(int Hour)
{
	if (Hour<-1 || Hour>=24)
		return false;
	m_BeginHour=Hour;
	return true;
}


bool CProgramGuide::SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime)
{
	FILETIME ftFirst,ftLast;

	if (CompareSystemTime(pFirstTime,pLastTime)>=0)
		return false;

	m_stFirstTime=*pFirstTime;
	m_stFirstTime.wMinute=0;
	m_stFirstTime.wSecond=0;
	m_stFirstTime.wMilliseconds=0;
	m_stLastTime=*pLastTime;
	m_stLastTime.wMinute=0;
	m_stLastTime.wSecond=0;
	m_stLastTime.wMilliseconds=0;
	::SystemTimeToFileTime(&m_stFirstTime,&ftFirst);
	::SystemTimeToFileTime(&m_stLastTime,&ftLast);
	m_Hours=(int)((ftLast-ftFirst)/(FILETIME_SECOND*60*60));

	if (m_pFrame!=NULL)
		m_pFrame->OnTimeRangeChanged();

	return true;
}


bool CProgramGuide::GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	if (pFirstTime!=NULL)
		*pFirstTime=m_stFirstTime;
	if (pLastTime!=NULL)
		*pLastTime=m_stLastTime;
	return true;
}


bool CProgramGuide::GetCurrentTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	if (m_ListMode==LIST_WEEK) {
		GetDayTimeRange(DAY_TOMORROW,pFirstTime,pLastTime);
		if (pFirstTime!=NULL)
			OffsetSystemTime(pFirstTime,-24*60*60*1000);
		if (pLastTime!=NULL)
			OffsetSystemTime(pLastTime,-24*60*60*1000);
		return true;
	}

	return GetDayTimeRange(m_Day,pFirstTime,pLastTime);
}


bool CProgramGuide::GetDayTimeRange(int Day,SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	SYSTEMTIME stFirst=m_stFirstTime,stLast=m_stLastTime;

	if (Day!=DAY_TODAY) {
		LONGLONG Offset;

		if (m_BeginHour<0) {
			Offset=Day*24;
		} else {
			Offset=Day*24-stFirst.wHour;
			if (stFirst.wHour>=m_BeginHour)
				Offset+=m_BeginHour;
			else
				Offset-=24-m_BeginHour;
		}
		Offset*=60*60*1000;
		OffsetSystemTime(&stFirst,Offset);
		OffsetSystemTime(&stLast,Offset);
	}
	if (pFirstTime!=NULL)
		*pFirstTime=stFirst;
	if (pLastTime!=NULL)
		*pLastTime=stLast;
	return true;
}


bool CProgramGuide::SetViewDay(int Day)
{
	if (Day<DAY_TODAY || Day>DAY_LAST)
		return false;
	m_Day=Day;
	return true;
}


bool CProgramGuide::SetUIOptions(int LinesPerHour,int ItemWidth,int LineMargin)
{
	if (LinesPerHour<MIN_LINES_PER_HOUR || LinesPerHour>MAX_LINES_PER_HOUR
			|| ItemWidth<MIN_ITEM_WIDTH || ItemWidth>MAX_ITEM_WIDTH)
		return false;
	if (m_LinesPerHour!=LinesPerHour
			|| m_ItemWidth!=ItemWidth
			|| m_LineMargin!=LineMargin) {
		m_LinesPerHour=LinesPerHour;
		m_ItemWidth=ItemWidth;
		m_LineMargin=LineMargin;
		if (m_hwnd!=NULL) {
			m_ScrollPos.x=0;
			m_ScrollPos.y=0;
			m_OldScrollPos=m_ScrollPos;
			CalcLayout();
			SetScrollBar();
			Invalidate();
		}
	}
	return true;
}


bool CProgramGuide::SetColor(int Type,COLORREF Color)
{
	if (Type<0 || Type>COLOR_LAST)
		return false;
	m_ColorList[Type]=Color;
	return true;
}


void CProgramGuide::SetBackColors(const Theme::GradientInfo *pChannelBackGradient,
								  const Theme::GradientInfo *pCurChannelBackGradient,
								  const Theme::GradientInfo *pTimeBarMarginGradient,
								  const Theme::GradientInfo *pTimeBarBackGradient)
{
	m_ChannelNameBackGradient=*pChannelBackGradient;
	m_CurChannelNameBackGradient=*pCurChannelBackGradient;
	m_TimeBarMarginGradient=*pTimeBarMarginGradient;
	for (int i=0;i<TIME_BAR_BACK_COLORS;i++)
		m_TimeBarBackGradient[i]=pTimeBarBackGradient[i];
	if (m_hwnd!=NULL)
		Invalidate();
}


bool CProgramGuide::SetFont(const LOGFONT *pFont)
{
	LOGFONT lf;

	if (!m_Font.Create(pFont))
		return false;
	lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	lf.lfWeight=FW_NORMAL;
	lf.lfEscapement=lf.lfOrientation=2700;
	m_TimeFont.Create(&lf);
	if (m_hwnd!=NULL) {
		HDC hdc=::GetDC(m_hwnd);
		m_FontHeight=m_Font.GetHeight(hdc);
		::ReleaseDC(m_hwnd,hdc);
	} else {
		m_FontHeight=m_Font.GetHeight();
	}
	m_HeaderHeight=m_FontHeight+(HEADER_MARGIN_TOP+HEADER_MARGIN_BOTTOM);
	m_TimeBarWidth=m_FontHeight+8;
	if (m_hwnd!=NULL) {
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
		m_OldScrollPos=m_ScrollPos;
		CalcLayout();
		SetScrollBar();
		Invalidate();
	}
	return true;
}


bool CProgramGuide::GetFont(LOGFONT *pFont) const
{
	return m_Font.GetLogFont(pFont);
}


bool CProgramGuide::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


bool CProgramGuide::SetShowToolTip(bool fShow)
{
	if (m_fShowToolTip!=fShow) {
		m_fShowToolTip=fShow;
		m_EventInfoPopupManager.SetEnable(fShow);
	}
	return true;
}


void CProgramGuide::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler)
		m_pEventHandler->m_pProgramGuide=NULL;
	if (pEventHandler)
		pEventHandler->m_pProgramGuide=this;
	m_pEventHandler=pEventHandler;
}


void CProgramGuide::SetFrame(CFrame *pFrame)
{
	if (m_pFrame)
		m_pFrame->m_pProgramGuide=NULL;
	if (pFrame)
		pFrame->m_pProgramGuide=this;
	m_pFrame=pFrame;
}


void CProgramGuide::SetProgramCustomizer(CProgramCustomizer *pProgramCustomizer)
{
	if (m_pProgramCustomizer)
		m_pProgramCustomizer->m_pProgramGuide=NULL;
	if (pProgramCustomizer) {
		pProgramCustomizer->m_pProgramGuide=this;
		if (m_hwnd!=NULL)
			pProgramCustomizer->Initialize();
	}
	m_pProgramCustomizer=pProgramCustomizer;
}


bool CProgramGuide::SetDragScroll(bool fDragScroll)
{
	if (m_fDragScroll!=fDragScroll) {
		m_fDragScroll=fDragScroll;
		if (m_hwnd!=NULL) {
			POINT pt;

			::GetCursorPos(&pt);
			SendMessage(WM_SETCURSOR,(WPARAM)m_hwnd,
						MAKELPARAM(SendMessage(WM_NCHITTEST,0,MAKELPARAM(pt.x,pt.y)),WM_MOUSEMOVE));
		}
	}
	return true;
}


bool CProgramGuide::SetFilter(unsigned int Filter)
{
	if ((Filter&(FILTER_ORIGINAL | FILTER_RERUN))==(FILTER_ORIGINAL | FILTER_RERUN))
		Filter&=~(FILTER_ORIGINAL | FILTER_RERUN);
	if (m_Filter!=Filter) {
		m_Filter=Filter;
		if (m_hwnd!=NULL) {
			Invalidate();
		}
	}
	return true;
}


void CProgramGuide::SetVisibleEventIcons(UINT VisibleIcons)
{
	if (m_VisibleEventIcons!=VisibleIcons) {
		m_VisibleEventIcons=VisibleIcons;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


bool CProgramGuide::OnClose()
{
	if (m_pEventHandler!=NULL && !m_pEventHandler->OnClose())
		return false;
	if (m_fUpdating) {
		if (m_pEventHandler)
			m_pEventHandler->OnEndUpdate();
		m_fUpdating=false;
	}
	return true;
}


bool CProgramGuide::EventHitTest(int x,int y,int *pListIndex,int *pEventIndex,RECT *pItemRect) const
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetProgramGuideRect(&rc);
	if (::PtInRect(&rc,pt)) {
		const int XPos=x-rc.left+m_ScrollPos.x;
		const int ServiceWidth=m_ItemWidth+m_ItemMargin*2;

		if (XPos%ServiceWidth<m_ItemMargin
				|| XPos%ServiceWidth>=m_ItemMargin+m_ItemWidth)
			return false;
		int List=XPos/ServiceWidth;
		if (List<(int)m_EventLayoutList.Length()) {
			const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[List];
			int LineHeight=m_FontHeight+m_LineMargin;
			int YOrigin=rc.top-m_ScrollPos.y*LineHeight;

			y-=YOrigin;
			for (size_t i=0;i<pLayout->NumItems();i++) {
				const ProgramGuide::CEventItem *pItem=pLayout->GetItem(i);

				if (!pItem->IsNullItem() && pItem->GetItemLines()>0) {
					rc.top=pItem->GetItemPos()*LineHeight;
					rc.bottom=rc.top+pItem->GetItemLines()*LineHeight;
					if (y>=rc.top && y<rc.bottom) {
						if (pListIndex!=NULL)
							*pListIndex=List;
						if (pEventIndex!=NULL)
							*pEventIndex=(int)i;
						if (pItemRect!=NULL) {
							pItemRect->top=rc.top+YOrigin;
							pItemRect->bottom=pItemRect->top+(rc.bottom-rc.top);
							pItemRect->left=List*ServiceWidth-m_ScrollPos.x+m_ItemMargin;
							pItemRect->right=pItemRect->left+m_ItemWidth;
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}


LRESULT CProgramGuide::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			if (m_hDragCursor1==NULL)
				m_hDragCursor1=::LoadCursor(m_hinst,MAKEINTRESOURCE(IDC_GRAB1));
			if (m_hDragCursor2==NULL)
				m_hDragCursor2=::LoadCursor(m_hinst,MAKEINTRESOURCE(IDC_GRAB2));
			m_Chevron.Load(m_hinst,IDB_CHEVRON);
			m_EpgIcons.Load();
			m_EventInfoPopupManager.Initialize(hwnd,&m_EventInfoPopupHandler);
			m_Tooltip.Create(hwnd);
			if (m_pProgramCustomizer!=NULL)
				m_pProgramCustomizer->Initialize();

			m_fBarShadow=CBufferedPaint::IsSupported();

			GetCurrentJST(&m_stCurTime);
			::SetTimer(hwnd,TIMER_ID_UPDATECURTIME,1000,NULL);
		}
		return 0;

	case WM_PAINT:
#if 0
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
#else
		OnPaint(hwnd);
#endif
		return 0;

	case WM_SIZE:
		{
			RECT rc;
			POINT Pos;
			int TotalWidth=(int)m_EventLayoutList.Length()*(m_ItemWidth+m_ItemMargin*2);
			int TotalLines=m_Hours*m_LinesPerHour;
			int XPage,YPage;

			GetProgramGuideRect(&rc);
			Pos=m_ScrollPos;
			XPage=max(rc.right-rc.left,0);
			if (Pos.x>max(TotalWidth-XPage,0))
				Pos.x=max(TotalWidth-XPage,0);
			YPage=(rc.bottom-rc.top)/(m_FontHeight+m_LineMargin);
			if (Pos.y>max(TotalLines-YPage,0))
				Pos.y=max(TotalLines-YPage,0);
			SetScrollBar();
			if (Pos.x!=m_ScrollPos.x || Pos.y!=m_ScrollPos.y)
				Scroll(Pos.x-m_ScrollPos.x,Pos.y-m_ScrollPos.y);
		}
		return 0;

	case WM_VSCROLL:
	case WM_MOUSEWHEEL:
		{
			int Pos;
			RECT rc;
			int Page;
			int TotalLines=m_Hours*m_LinesPerHour;

			Pos=m_ScrollPos.y;
			GetProgramGuideRect(&rc);
			Page=(rc.bottom-rc.top)/(m_FontHeight+m_LineMargin);
			if (uMsg==WM_VSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINEUP:		Pos--;				break;
				case SB_LINEDOWN:	Pos++;				break;
				case SB_PAGEUP:		Pos-=Page;			break;
				case SB_PAGEDOWN:	Pos+=Page;			break;
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
				case SB_TOP:		Pos=0;				break;
				case SB_BOTTOM:		Pos=max(TotalLines-Page,0);	break;
				default:	return 0;
				}
			} else {
				int Lines;

				if (m_WheelScrollLines==0) {
					UINT SysLines;

					if (::SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&SysLines,0))
						Lines=SysLines;
					else
						Lines=2;
				} else {
					Lines=m_WheelScrollLines;
				}
				Pos-=GET_WHEEL_DELTA_WPARAM(wParam)*Lines/WHEEL_DELTA;
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(TotalLines-Page,0))
				Pos=max(TotalLines-Page,0);
			if (Pos!=m_ScrollPos.y)
				Scroll(0,Pos-m_ScrollPos.y);
		}
		return 0;

	case WM_HSCROLL:
	case WM_MOUSEHWHEEL:
		{
			int Pos;
			RECT rc;
			int Page;
			int TotalWidth=(int)m_EventLayoutList.Length()*(m_ItemWidth+m_ItemMargin*2);

			Pos=m_ScrollPos.x;
			GetProgramGuideRect(&rc);
			Page=rc.right-rc.left;
			if (uMsg==WM_HSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINELEFT:	Pos-=m_FontHeight;			break;
				case SB_LINERIGHT:	Pos+=m_FontHeight;			break;
				case SB_PAGELEFT:	Pos-=Page;					break;
				case SB_PAGERIGHT:	Pos+=Page;					break;
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
				case SB_LEFT:		Pos=0;						break;
				case SB_RIGHT:		Pos=max(TotalWidth-Page,0);	break;
				default:	return 0;
				}
			} else {
				Pos+=GET_WHEEL_DELTA_WPARAM(wParam)*m_FontHeight/WHEEL_DELTA;
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(TotalWidth-Page,0))
				Pos=max(TotalWidth-Page,0);
			if (Pos!=m_ScrollPos.x)
				Scroll(Pos-m_ScrollPos.x,0);
		}
		return uMsg==WM_MOUSEHWHEEL;

	case WM_LBUTTONDOWN:
		{
			POINT pt;
			RECT rc;

			::SetFocus(hwnd);
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::GetClientRect(hwnd,&rc);
			if (pt.y<m_HeaderHeight
					&& pt.x>=m_TimeBarWidth && pt.x<rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					const int HeaderWidth=m_ItemWidth+m_ItemMargin*2;

					if (pt.x-m_TimeBarWidth<
							(int)m_ServiceList.NumServices()*HeaderWidth-m_ScrollPos.x) {
						int x=pt.x+m_ScrollPos.x-m_TimeBarWidth;
						int Service=x/HeaderWidth;

						if (x%HeaderWidth<HeaderWidth-(HEADER_MARGIN_RIGHT+CHEVRON_WIDTH)) {
							if (m_pEventHandler) {
								ProgramGuide::CServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);

								if (pServiceInfo!=NULL)
									m_pEventHandler->OnServiceTitleLButtonDown(
													m_szDriverFileName,
													pServiceInfo->GetServiceInfoData());
							}
						} else {
							SetWeekListMode(Service);
						}
					}
				} else if (m_ListMode==LIST_WEEK) {
					SetServiceListMode();
				}
			} else if (pt.x<m_TimeBarWidth || pt.x>=rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					if (m_Day>DAY_TODAY && pt.y<m_HeaderHeight) {
						::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_TODAY+(m_Day-1),0);
					} else if (m_Day<DAY_LAST) {
						int y=(m_Hours*m_LinesPerHour-m_ScrollPos.y)*(m_FontHeight+m_LineMargin);
						if (pt.y-m_HeaderHeight>=y-m_TimeBarWidth
								&& pt.y-m_HeaderHeight<y) {
							::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_TODAY+(m_Day+1),0);
						}
					}
				}
			} else if (m_fDragScroll) {
				m_DragInfo.StartCursorPos=pt;
				m_DragInfo.StartScrollPos=m_ScrollPos;
				::SetCursor(m_hDragCursor2);
				::SetCapture(hwnd);
			} else {
				m_EventInfoPopupManager.Popup(pt.x,pt.y);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			::ReleaseCapture();
			::SetCursor(m_hDragCursor1);
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		if (m_pProgramCustomizer!=NULL) {
			POINT pt={GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};
			RECT ItemRect;

			m_CurItem.fValid=
				EventHitTest(pt.x,pt.y,&m_CurItem.ListIndex,&m_CurItem.EventIndex,&ItemRect);
			if (m_CurItem.fValid) {
				m_pProgramCustomizer->OnLButtonDoubleClick(
					*m_EventLayoutList[m_CurItem.ListIndex]->GetItem(m_CurItem.EventIndex)->GetEventInfo(),
					pt,ItemRect);
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			POINT pt;
			RECT ItemRect;
			HMENU hmenu,hmenuPopup;
			TCHAR szText[256],szMenu[64];

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			m_CurItem.fValid=
				EventHitTest(pt.x,pt.y,&m_CurItem.ListIndex,&m_CurItem.EventIndex,&ItemRect);
			::SetFocus(hwnd);
			hmenu=::LoadMenu(m_hinst,MAKEINTRESOURCE(IDM_PROGRAMGUIDE));
			hmenuPopup=::GetSubMenu(hmenu,0);
			::CheckMenuRadioItem(hmenu,CM_PROGRAMGUIDE_TODAY,
								 CM_PROGRAMGUIDE_TODAY+DAY_LAST,
								 CM_PROGRAMGUIDE_TODAY+m_Day,MF_BYCOMMAND);
			MENUITEMINFO mii;
			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_STRING;
			mii.dwTypeData=szText;
			for (int i=CM_PROGRAMGUIDE_TODAY;i<=CM_PROGRAMGUIDE_TODAY+DAY_LAST;i++) {
				SYSTEMTIME st;

				GetDayTimeRange(i-CM_PROGRAMGUIDE_TODAY,&st,NULL);
				mii.cch=lengthof(szText);
				::GetMenuItemInfo(hmenu,i,FALSE,&mii);
				int Length=::lstrlen(szText);
				StdUtil::snprintf(szText+Length,lengthof(szText)-Length,TEXT(" %d/%d(%s) %d時～"),
								  st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),st.wHour);
				::SetMenuItemInfo(hmenu,i,FALSE,&mii);
			}

			HMENU hmenuDriver=::GetSubMenu(hmenuPopup,MENU_DRIVER);
			ClearMenu(hmenuDriver);
			if (m_TuningSpaceList.GetAllChannelList()->NumChannels()>0) {
				::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
							 CM_PROGRAMGUIDE_TUNINGSPACE_ALL,TEXT("すべて"));
			}
			for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
				GetTuningSpaceName(i,szText,lengthof(szText));
				CopyToMenuText(szText,szMenu,lengthof(szMenu));
				::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
							 CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+i,szMenu);
			}
			if (::GetMenuItemCount(hmenuDriver)>0) {
				::CheckMenuRadioItem(hmenuDriver,CM_PROGRAMGUIDE_TUNINGSPACE_ALL,
									 CM_PROGRAMGUIDE_TUNINGSPACE_ALL+m_TuningSpaceList.NumSpaces(),
									 CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+m_CurrentTuningSpace,
									 MF_BYCOMMAND);
			}
			if (m_pDriverManager!=NULL) {
				int NumDrivers=m_pDriverManager->NumDrivers();

				if (NumDrivers>0) {
					::AppendMenu(hmenuDriver,MFT_SEPARATOR,0,NULL);
					int CurDriver=-1;
					int i=-1;
					while (EnumDriver(&i,szText,lengthof(szText))) {
						if (::lstrcmpi(m_szDriverFileName,szText)==0)
							CurDriver=i;
						::PathRemoveExtension(szText);
						CopyToMenuText(szText,szMenu,lengthof(szMenu));
						::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
									 CM_PROGRAMGUIDE_DRIVER_FIRST+i,szMenu);
					}
					if (CurDriver>=0)
						::CheckMenuRadioItem(hmenuDriver,CM_PROGRAMGUIDE_DRIVER_FIRST,
											 CM_PROGRAMGUIDE_DRIVER_FIRST+NumDrivers-1,
											 CM_PROGRAMGUIDE_DRIVER_FIRST+CurDriver,
											 MF_BYCOMMAND);
				}
			}
			::EnableMenuItem(hmenuPopup,MENU_DRIVER,
							 MF_BYPOSITION | (::GetMenuItemCount(hmenuDriver)>0?MFS_ENABLED:MFS_GRAYED));

			for (int i=0;(m_Filter>>i)!=0;i++) {
				if (((m_Filter>>i)&1)!=0)
					::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_FILTER_FIRST+i,MF_BYCOMMAND | MF_CHECKED);
			}

			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_UPDATE,
				MF_BYCOMMAND | (m_fUpdating?MFS_GRAYED:MFS_ENABLED));
			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_ENDUPDATE,
				MF_BYCOMMAND | (m_fUpdating?MFS_ENABLED:MFS_GRAYED));
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_ALWAYSONTOP,
				MF_BYCOMMAND | (m_pFrame->GetAlwaysOnTop()?MFS_CHECKED:MFS_UNCHECKED));
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_DRAGSCROLL,
				MF_BYCOMMAND | (m_fDragScroll?MFS_CHECKED:MFS_UNCHECKED));
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_POPUPEVENTINFO,
				MF_BYCOMMAND | (m_fShowToolTip?MFS_CHECKED:MFS_UNCHECKED));
			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_IEPGASSOCIATE,
				MF_BYCOMMAND | (m_CurItem.fValid?MFS_ENABLED:MFS_GRAYED));

			if (m_CurItem.fValid) {
				if (m_pProgramCustomizer!=NULL) {
					m_pProgramCustomizer->InitializeMenu(
						*m_EventLayoutList[m_CurItem.ListIndex]->GetItem(m_CurItem.EventIndex)->GetEventInfo(),
						::GetSubMenu(hmenu,0),CM_PROGRAMGUIDE_CUSTOM_FIRST,
						pt,ItemRect);
				}

				if (m_ToolList.NumTools()>0) {
					::AppendMenu(hmenuPopup,MFT_SEPARATOR | MFS_ENABLED,0,NULL);
					for (size_t i=0;i<m_ToolList.NumTools();i++) {
						const CProgramGuideTool *pTool=m_ToolList.GetTool(i);

						CopyToMenuText(pTool->GetName(),szText,lengthof(szText));
						::AppendMenu(hmenuPopup,MF_STRING | MF_ENABLED,
									 CM_PROGRAMGUIDETOOL_FIRST+i,szText);
					}
				}
			} else {
				if (m_pEventHandler!=NULL) {
					m_pEventHandler->OnMenuInitialize(::GetSubMenu(hmenu,0),CM_PROGRAMGUIDE_CUSTOM_FIRST);
				}
			}

			::ClientToScreen(hwnd,&pt);
			::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,
							 0,hwnd,NULL);
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_MOUSEMOVE:
		if (::GetCapture()==hwnd) {
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int XScroll,YScroll;

			XScroll=(m_DragInfo.StartScrollPos.x+(m_DragInfo.StartCursorPos.x-x))-m_ScrollPos.x;
			YScroll=(m_DragInfo.StartScrollPos.y+(m_DragInfo.StartCursorPos.y-y)/(m_FontHeight+m_LineMargin))-m_ScrollPos.y;
			if (XScroll!=0 || YScroll!=0)
				Scroll(XScroll,YScroll);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			::GetClientRect(hwnd,&rc);
			if (pt.y<m_HeaderHeight
					&& pt.x>=m_TimeBarWidth
					&& pt.x<rc.right-m_TimeBarWidth
					&& (m_ListMode==LIST_WEEK
						|| pt.x-m_TimeBarWidth<
							(int)m_EventLayoutList.Length()*(m_ItemWidth+m_ItemMargin*2)-m_ScrollPos.x)) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			} else if (pt.x<m_TimeBarWidth
					|| pt.x>=rc.right-m_TimeBarWidth) {
				if (m_ListMode==LIST_SERVICES) {
					if (m_Day>DAY_TODAY
							&& pt.y<m_HeaderHeight) {
						::SetCursor(::LoadCursor(NULL,IDC_HAND));
						return TRUE;
					}
					int y=(m_Hours*m_LinesPerHour-m_ScrollPos.y)*
						(m_FontHeight+m_LineMargin);
					if (m_Day<DAY_LAST
							&& pt.y-m_HeaderHeight>=y-m_TimeBarWidth
							&& pt.y-m_HeaderHeight<y) {
						::SetCursor(::LoadCursor(NULL,IDC_HAND));
						return TRUE;
					}
				}
			} else if (m_fDragScroll) {
				::SetCursor(m_hDragCursor1);
				return TRUE;
			}
		}
		break;

	case WM_KEYDOWN:
		{
			static const struct {
				WORD KeyCode;
				WORD Message;
				WORD Request;
			} KeyMap[] = {
				{VK_PRIOR,	WM_VSCROLL,	SB_PAGEUP},
				{VK_NEXT,	WM_VSCROLL,	SB_PAGEDOWN},
				{VK_UP,		WM_VSCROLL,	SB_LINEUP},
				{VK_DOWN,	WM_VSCROLL,	SB_LINEDOWN},
				{VK_LEFT,	WM_HSCROLL,	SB_LINEUP},
				{VK_RIGHT,	WM_HSCROLL,	SB_LINEDOWN},
				{VK_HOME,	WM_VSCROLL,	SB_TOP},
				{VK_END,	WM_VSCROLL,	SB_BOTTOM},
			};

			for (int i=0;i<lengthof(KeyMap);i++) {
				if (wParam==(WPARAM)KeyMap[i].KeyCode) {
					::SendMessage(hwnd,KeyMap[i].Message,KeyMap[i].Request,0);
					return 0;
				}
			}

			if (m_pEventHandler!=NULL
					&& m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_TIMER:
		if (wParam==TIMER_ID_UPDATECURTIME) {
			if (m_Day==DAY_TODAY) {
				SYSTEMTIME st;

				GetCurrentJST(&st);
				if (m_stCurTime.wMinute!=st.wMinute
						|| m_stCurTime.wHour!=st.wHour
						|| m_stCurTime.wDay!=st.wDay
						|| m_stCurTime.wMonth!=st.wMonth
						|| m_stCurTime.wYear!=st.wYear) {
					int OldTimeLinePos=GetCurTimeLinePos(),NewTimeLinePos;

					m_stCurTime=st;
					NewTimeLinePos=GetCurTimeLinePos();
					if (NewTimeLinePos!=OldTimeLinePos) {
						RECT rc,rcGuide;
						int Offset;

						::GetClientRect(hwnd,&rc);
						GetProgramGuideRect(&rcGuide);
						Offset=rcGuide.top-m_ScrollPos.y*(m_FontHeight+m_LineMargin);
						rc.top=Offset+OldTimeLinePos-m_FontHeight/2;
						rc.bottom=Offset+NewTimeLinePos+m_FontHeight/2;
						::InvalidateRect(hwnd,&rc,FALSE);
					}
				}
			}
		}
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wParam));
		return 0;

	case WM_DESTROY:
		m_ProgramSearch.Destroy();
		if (m_pProgramCustomizer!=NULL)
			m_pProgramCustomizer->Finalize();
		m_Tooltip.Destroy();
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnDestroy();
		m_Chevron.Destroy();
		m_EpgIcons.Destroy();
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CProgramGuide::OnCommand(int id)
{
	switch (id) {
	case CM_PROGRAMGUIDE_UPDATE:
		if (!m_fUpdating) {
			if (m_pEventHandler!=NULL
					&& m_pEventHandler->OnBeginUpdate()) {
				m_fUpdating=true;
				SetCaption();
			}
		}
		return;

	case CM_PROGRAMGUIDE_ENDUPDATE:
		if (m_fUpdating) {
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnEndUpdate();
			m_fUpdating=false;
			SetCaption();
		}
		return;

	case CM_PROGRAMGUIDE_REFRESH:
		if (m_pEventHandler==NULL
				|| m_pEventHandler->OnRefresh()) {
			UpdateProgramGuide(true);
		}
		return;

	case CM_PROGRAMGUIDE_TODAY:
	case CM_PROGRAMGUIDE_TOMORROW:
	case CM_PROGRAMGUIDE_DAYAFTERTOMORROW:
	case CM_PROGRAMGUIDE_2DAYSAFTERTOMORROW:
	case CM_PROGRAMGUIDE_3DAYSAFTERTOMORROW:
	case CM_PROGRAMGUIDE_4DAYSAFTERTOMORROW:
	case CM_PROGRAMGUIDE_5DAYSAFTERTOMORROW:
		{
			int Day=id-CM_PROGRAMGUIDE_TODAY;

			if (m_Day!=Day || m_ListMode!=LIST_SERVICES) {
				m_Day=Day;
				if (m_pProgramList!=NULL) {
					HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
					if (m_ListMode!=LIST_SERVICES) {
						m_ListMode=LIST_SERVICES;
						m_WeekListService=-1;
						m_ScrollPos.x=m_OldScrollPos.x;
						if (m_pFrame!=NULL)
							m_pFrame->OnListModeChanged();
					}
					m_ScrollPos.y=0;
					CalcLayout();
					SetScrollBar();
					SetCaption();
					GetCurrentJST(&m_stCurTime);
					Invalidate();
					::SetCursor(hcurOld);
					if (m_pFrame!=NULL)
						m_pFrame->OnDateChanged();
				}
			}
		}
		return;

	case CM_PROGRAMGUIDE_IEPGASSOCIATE:
		if (m_CurItem.fValid) {
			const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[m_CurItem.ListIndex];

			if (pLayout!=NULL) {
				const ProgramGuide::CEventItem *pEvent=pLayout->GetItem(m_CurItem.EventIndex);

				if (pEvent!=NULL) {
					TCHAR szFileName[MAX_PATH];

					GetAppClass().GetAppDirectory(szFileName);
					::PathAppend(szFileName,TEXT("iepg.tvpid"));
					if (!pLayout->GetServiceInfo()->SaveiEpgFile(pEvent->GetEventInfo(),szFileName,true))
						return;
					::ShellExecute(NULL,NULL,szFileName,NULL,NULL,SW_SHOWNORMAL);
				}
			}
		}
		return;

	case CM_PROGRAMGUIDE_SEARCH:
		if (!m_ProgramSearch.IsCreated()) {
			RECT rc;

			m_ProgramSearch.SetEventHandler(&m_ProgramSearchEventHandler);
			m_ProgramSearch.GetPosition(&rc);
			if (rc.left==rc.right || rc.top==rc.bottom) {
				POINT pt={0,0};
				::ClientToScreen(m_hwnd,&pt);
				m_ProgramSearch.SetPosition(pt.x,pt.y,0,0);
			}
			m_ProgramSearch.Create(m_hwnd);
		} else {
			m_ProgramSearch.Destroy();
		}
		return;

	case CM_PROGRAMGUIDE_CHANNELSETTINGS:
		{
			CEpgChannelSettings Settings(this);

			POINT pt={0,0};
			::ClientToScreen(m_hwnd,&pt);
			Settings.SetPosition(pt.x,pt.y,0,0);

			if (Settings.Show(m_hwnd))
				UpdateProgramGuide(false);
		}
		return;

	case CM_PROGRAMGUIDE_ALWAYSONTOP:
		if (m_pFrame!=NULL)
			m_pFrame->SetAlwaysOnTop(!m_pFrame->GetAlwaysOnTop());
		return;

	case CM_PROGRAMGUIDE_DRAGSCROLL:
		SetDragScroll(!m_fDragScroll);
		return;

	case CM_PROGRAMGUIDE_POPUPEVENTINFO:
		SetShowToolTip(!m_fShowToolTip);
		return;

	default:
		if (id>=CM_PROGRAMGUIDE_FILTER_FIRST
				&& id<=CM_PROGRAMGUIDE_FILTER_LAST) {
			unsigned int Filter=m_Filter^(1<<(id-CM_PROGRAMGUIDE_FILTER_FIRST));

			if (id==CM_PROGRAMGUIDE_FILTER_ORIGINAL) {
				if ((Filter&FILTER_ORIGINAL)!=0)
					Filter&=~FILTER_RERUN;
			} else if (id==CM_PROGRAMGUIDE_FILTER_RERUN) {
				if ((Filter&FILTER_RERUN)!=0)
					Filter&=~FILTER_ORIGINAL;
			}
			SetFilter(Filter);
			return;
		}

		if (id>=CM_PROGRAMGUIDE_TUNINGSPACE_ALL
				&& id<=CM_PROGRAMGUIDE_TUNINGSPACE_LAST) {
			if (m_fUpdating)
				OnCommand(CM_PROGRAMGUIDE_ENDUPDATE);
			SetTuningSpace(id-CM_PROGRAMGUIDE_TUNINGSPACE_FIRST);
			if (m_ListMode!=LIST_SERVICES) {
				m_ListMode=LIST_SERVICES;
				m_WeekListService=-1;
				if (m_pFrame!=NULL)
					m_pFrame->OnListModeChanged();
			}
			UpdateProgramGuide();
			return;
		}

		if (id>=CM_PROGRAMGUIDE_DRIVER_FIRST
				&& id<=CM_PROGRAMGUIDE_DRIVER_LAST) {
			if (m_fUpdating)
				OnCommand(CM_PROGRAMGUIDE_ENDUPDATE);
			if (m_pDriverManager!=NULL) {
				const CDriverInfo *pDriverInfo=
					m_pDriverManager->GetDriverInfo(id-CM_PROGRAMGUIDE_DRIVER_FIRST);

				if (pDriverInfo!=NULL) {
					CDriverInfo DriverInfo(pDriverInfo->GetFileName());

					if (DriverInfo.LoadTuningSpaceList()) {
						SetTuningSpaceList(DriverInfo.GetFileName(),
										   DriverInfo.GetTuningSpaceList(),-1);
						if (m_ListMode!=LIST_SERVICES) {
							m_ListMode=LIST_SERVICES;
							m_WeekListService=-1;
							if (m_pFrame!=NULL)
								m_pFrame->OnListModeChanged();
						}
						UpdateProgramGuide();
					}
				}
			}
			return;
		}

		if (id>=CM_PROGRAMGUIDE_CUSTOM_FIRST
				&& id<=CM_PROGRAMGUIDE_CUSTOM_LAST) {
			if (m_CurItem.fValid) {
				if (m_pProgramCustomizer!=NULL) {
					const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[m_CurItem.ListIndex];

					if (pLayout!=NULL) {
						const ProgramGuide::CEventItem *pEvent=pLayout->GetItem(m_CurItem.EventIndex);

						if (pEvent!=NULL)
							m_pProgramCustomizer->ProcessMenu(*pEvent->GetEventInfo(),id);
					}
				}
			} else {
				if (m_pEventHandler!=NULL)
					m_pEventHandler->OnMenuSelected(id);
			}
			return;
		}

		if (id>=CM_PROGRAMGUIDETOOL_FIRST
				&& id<=CM_PROGRAMGUIDETOOL_LAST) {
			if (m_CurItem.fValid) {
				const ProgramGuide::CEventLayout *pLayout=m_EventLayoutList[m_CurItem.ListIndex];

				if (pLayout!=NULL) {
					const ProgramGuide::CEventItem *pEvent=pLayout->GetItem(m_CurItem.EventIndex);

					if (pEvent!=NULL) {
						CProgramGuideTool *pTool=m_ToolList.GetTool(id-CM_PROGRAMGUIDETOOL_FIRST);

						if (pTool!=NULL)
							pTool->Execute(pLayout->GetServiceInfo(),pEvent->GetEventInfo(),
										   ::GetAncestor(m_hwnd,GA_ROOT));
					}
				}
			}
			return;
		}

		if (id>=CM_CHANNEL_FIRST && id<=CM_CHANNEL_LAST) {
			SetWeekListMode(id-CM_CHANNEL_FIRST);
			return;
		}
	}
}




CProgramGuide::CEventHandler::CEventHandler()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CEventHandler::~CEventHandler()
{
	if (m_pProgramGuide!=NULL)
		m_pProgramGuide->SetEventHandler(NULL);
}


CProgramGuide::CFrame::CFrame()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CFrame::~CFrame()
{
	if (m_pProgramGuide!=NULL)
		m_pProgramGuide->SetFrame(NULL);
}


CProgramGuide::CProgramCustomizer::CProgramCustomizer()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CProgramCustomizer::~CProgramCustomizer()
{
	if (m_pProgramGuide!=NULL)
		m_pProgramGuide->SetEventHandler(NULL);
}


CProgramGuide::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	/*if (m_pProgramGuide->m_fShowToolTip)*/ {
		int List,Event;

		if (m_pProgramGuide->EventHitTest(x,y,&List,&Event)) {
			*pParam=MAKELONG(List,Event);
			return true;
		}
	}
	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	int List=LOWORD(Param),Event=HIWORD(Param);
	const ProgramGuide::CEventLayout *pLayout=m_pProgramGuide->m_EventLayoutList[List];
	if (pLayout!=NULL) {
		const ProgramGuide::CEventItem *pItem=pLayout->GetItem(Event);
		if (pItem!=NULL) {
			const CEventInfoData *pEventInfo;
			if (pItem->GetCommonEventInfo()!=NULL)
				pEventInfo=pItem->GetCommonEventInfo();
			else
				pEventInfo=pItem->GetEventInfo();
			/*
			if (pEventInfo->GetEventName()==NULL && pEventInfo->m_fCommonEvent) {
				const CProgramGuideItem *pCommonItem=m_pProgramGuide->m_ServiceList.FindEvent(
					pServiceInfo->GetTSID(),
					pEventInfo->m_CommonEventInfo.ServiceID,
					pEventInfo->m_CommonEventInfo.EventID);
				if (pCommonItem!=NULL)
					pEventInfo=&pCommonItem->GetEventInfo();
			}
			*/
			*ppInfo=pEventInfo;
			return true;
		}
	}
	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::OnShow(const CEventInfoData *pInfo)
{
	int Genre=-1;
	for (int i=0;i<pInfo->m_ContentNibble.NibbleCount;i++) {
		if (pInfo->m_ContentNibble.NibbleList[i].ContentNibbleLevel1!=0xE) {
			Genre=pInfo->m_ContentNibble.NibbleList[i].ContentNibbleLevel1;
			break;
		}
	}

	COLORREF Color;
	int Red,Green,Blue;
	Theme::GradientInfo BackGradient;

	if (Genre>=0 && Genre<=CEventInfoData::CONTENT_LAST)
		Color=m_pProgramGuide->m_ColorList[COLOR_CONTENT_FIRST+Genre];
	else
		Color=m_pProgramGuide->m_ColorList[COLOR_CONTENT_OTHER];
	BackGradient.Type=Theme::GRADIENT_NORMAL;
	BackGradient.Direction=Theme::DIRECTION_VERT;
	Red=GetRValue(Color);
	Green=GetGValue(Color);
	Blue=GetBValue(Color);
	BackGradient.Color1=RGB(Red+(255-Red)/2,Green+(255-Green)/2,Blue+(255-Blue)/2);
	BackGradient.Color2=Color;
	CEventInfoPopupManager::CEventHandler::m_pPopup->SetTitleColor(&BackGradient,m_pProgramGuide->m_ColorList[COLOR_TEXT]);
	return true;
}


bool CProgramGuide::CEventInfoPopupHandler::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	::AppendMenu(hmenu,MFT_STRING | (CEventInfoPopup::CEventHandler::m_pPopup->IsSelected()?MFS_ENABLED:MFS_GRAYED),
				 COMMAND_FIRST,TEXT("選択文字列を検索(&S)"));
	return true;
}


void CProgramGuide::CEventInfoPopupHandler::OnMenuSelected(int Command)
{
	LPTSTR pszText=CEventInfoPopup::CEventHandler::m_pPopup->GetSelectedText();
	if (pszText!=NULL) {
		if (!m_pProgramGuide->m_ProgramSearch.IsCreated())
			m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_SEARCH,0);
		m_pProgramGuide->m_ProgramSearch.Search(pszText);
		delete [] pszText;
	}
}




CProgramGuide::CProgramSearchEventHandler::CProgramSearchEventHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CProgramSearchEventHandler::Search(LPCTSTR pszKeyword)
{
	for (size_t i=0;i<m_pProgramGuide->m_ServiceList.NumServices();i++) {
		const ProgramGuide::CServiceInfo *pServiceInfo=m_pProgramGuide->m_ServiceList.GetItem(i);

		for (int j=0;j<pServiceInfo->NumEvents();j++) {
			const CEventInfoData *pEventInfo=pServiceInfo->GetEvent(j);

			if (MatchKeyword(pEventInfo,pszKeyword))
				AddSearchResult(pEventInfo,pServiceInfo->GetServiceName());
		}
	}
	return true;
}




#include "Menu.h"

enum {
	STATUS_ITEM_TUNER,
	STATUS_ITEM_DATE,
	STATUS_ITEM_DATEPREV,
	STATUS_ITEM_DATENEXT
};


class CProgramGuideTunerStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

public:
	CProgramGuideTunerStatusItem::CProgramGuideTunerStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_TUNER,160)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("チューナー"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		TCHAR szText[256];

		m_pProgramGuide->GetTuningSpaceName(m_pProgramGuide->GetCurrentTuningSpace(),szText,lengthof(szText));
		DrawText(hdc,pRect,szText);
	}

	void OnFocus(bool fFocus)
	{
		if (fFocus) {
			int i;
			TCHAR szText[256];

			m_Menu.Clear();
			for (i=-1;m_pProgramGuide->GetTuningSpaceName(i,szText,lengthof(szText));i++)
				m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+i,szText));
			m_Menu.AppendSeparator();
			i=-1;
			while (m_pProgramGuide->EnumDriver(&i,szText,lengthof(szText))) {
				::PathRemoveExtension(szText);
				m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_DRIVER_FIRST+i,szText));
			}
			RECT rc;
			POINT pt;
			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			m_Menu.Show(GetParent(m_pStatus->GetHandle()),m_pProgramGuide->GetHandle(),&pt,
						CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+m_pProgramGuide->GetCurrentTuningSpace());
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc,pt))
				m_Menu.Hide();
		}
	}
};

class CListSelectStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

	class CServiceMenuItem : public CDropDownMenu::CItem
	{
		static const int m_LogoWidth=16;
		static const int m_LogoHeight=9;
		static const int m_LogoMargin=2;

		HBITMAP m_hbmLogo;

		int GetWidth(HDC hdc) override
		{
			if (m_Width==0) {
				m_Width=CItem::GetWidth(hdc)+(m_LogoWidth+m_LogoMargin);
			}
			return m_Width;
		}

		void Draw(HDC hdc,const RECT *pRect) override
		{
			RECT rc=*pRect;

			if (m_hbmLogo!=NULL) {
				DrawUtil::DrawBitmap(hdc,
									 rc.left,rc.top+(rc.bottom-rc.top-m_LogoHeight)/2,
									 m_LogoWidth,m_LogoHeight,m_hbmLogo);
			}
			rc.left+=m_LogoWidth+m_LogoMargin;
			CItem::Draw(hdc,&rc);
		}

	public:
		CServiceMenuItem(int Command,LPCTSTR pszText,HBITMAP hbmLogo)
			: CItem(Command,pszText)
			, m_hbmLogo(hbmLogo)
		{
		}
	};

public:
	CListSelectStatusItem::CListSelectStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATE,160)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("日時"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
			SYSTEMTIME stFirst;
			TCHAR szText[256];

			m_pProgramGuide->GetCurrentTimeRange(&stFirst,NULL);
			StdUtil::snprintf(szText,lengthof(szText),TEXT("%s %d/%d(%s) %d時～"),
							  DayText[m_pProgramGuide->GetViewDay()],
							  stFirst.wMonth,stFirst.wDay,
							  GetDayOfWeekText(stFirst.wDayOfWeek),stFirst.wHour);
			DrawText(hdc,pRect,szText);
		} else {
			const ProgramGuide::CServiceInfo *pService=
				m_pProgramGuide->GetServiceList().GetItem(m_pProgramGuide->GetWeekListService());
			if (pService!=NULL)
				DrawText(hdc,pRect,pService->GetServiceName());
		}
	}

	void OnFocus(bool fFocus)
	{
		if (fFocus) {
			int CurItem;

			m_Menu.Clear();
			if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
				for (int i=CProgramGuide::DAY_TODAY;i<=CProgramGuide::DAY_LAST;i++) {
					SYSTEMTIME st;
					TCHAR szText[256];

					m_pProgramGuide->GetDayTimeRange(i,&st,NULL);
					StdUtil::snprintf(szText,lengthof(szText),TEXT("%s %d/%d(%s) %d時～"),
									  DayText[i],
									  st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),st.wHour);
					m_Menu.AppendItem(new CDropDownMenu::CItem(CM_PROGRAMGUIDE_TODAY+i,szText));
				}
				CurItem=CM_PROGRAMGUIDE_TODAY+m_pProgramGuide->GetViewDay();
			} else {
				const ProgramGuide::CServiceList &ServiceList=m_pProgramGuide->GetServiceList();

				for (size_t i=0;i<ServiceList.NumServices();i++) {
					const ProgramGuide::CServiceInfo *pService=ServiceList.GetItem(i);

					m_Menu.AppendItem(
						new CServiceMenuItem(CM_CHANNEL_FIRST+(int)i,
											 pService->GetServiceName(),
											 pService->GetLogo()));
				}
				CurItem=CM_CHANNEL_FIRST+m_pProgramGuide->GetWeekListService();
			}
			RECT rc;
			POINT pt;
			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			m_Menu.Show(GetParent(m_pStatus->GetHandle()),m_pProgramGuide->GetHandle(),&pt,
						CurItem);
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc,pt))
				m_Menu.Hide();
		}
	}
};

class CListPrevStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;

public:
	CListPrevStatusItem::CListPrevStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATEPREV,16)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("前へ"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		bool fEnabled;

		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
			fEnabled=m_pProgramGuide->GetViewDay()>CProgramGuide::DAY_TODAY;
		else
			fEnabled=m_pProgramGuide->GetWeekListService()>0;

		COLORREF OldTextColor;
		if (!fEnabled)
			OldTextColor=::SetTextColor(hdc,MixColor(::GetTextColor(hdc),GetBkColor(hdc)));
		DrawText(hdc,pRect,TEXT("▲"),DRAWTEXT_HCENTER);
		if (!fEnabled)
			::SetTextColor(hdc,OldTextColor);
	}

	void OnLButtonDown(int x,int y)
	{
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
			int Day=m_pProgramGuide->GetViewDay();
			if (Day>CProgramGuide::DAY_TODAY)
				m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_TODAY+Day-1,0);
		} else {
			m_pProgramGuide->SetWeekListMode(m_pProgramGuide->GetWeekListService()-1);
		}
	}
};

class CListNextStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;

public:
	CListNextStatusItem::CListNextStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATENEXT,16)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("次へ"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		bool fEnabled;

		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
			fEnabled=m_pProgramGuide->GetViewDay()<CProgramGuide::DAY_LAST;
		else
			fEnabled=m_pProgramGuide->GetWeekListService()+1<
				(int)m_pProgramGuide->GetServiceList().NumServices();

		COLORREF OldTextColor;
		if (!fEnabled)
			OldTextColor=::SetTextColor(hdc,MixColor(::GetTextColor(hdc),GetBkColor(hdc)));
		DrawText(hdc,pRect,TEXT("▼"),DRAWTEXT_HCENTER);
		if (!fEnabled)
			::SetTextColor(hdc,OldTextColor);
	}

	void OnLButtonDown(int x,int y)
	{
		if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES) {
			int Day=m_pProgramGuide->GetViewDay();
			if (Day<CProgramGuide::DAY_LAST)
				m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_TODAY+Day+1,0);
		} else {
			m_pProgramGuide->SetWeekListMode(m_pProgramGuide->GetWeekListService()+1);
		}
	}
};




#define CALENDARBAR_FLAG_NOW	0x80000000


CCalendarBar::CCalendarBar()
	: m_fUseBufferedPaint(false)
{
}


CCalendarBar::~CCalendarBar()
{
	Destroy();
}


bool CCalendarBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (!CreateBasicWindow(hwndParent,
						   Style | TBSTYLE_CUSTOMERASE | TBSTYLE_LIST | TBSTYLE_FLAT
						   | CCS_NODIVIDER | CCS_NORESIZE | CCS_NOPARENTALIGN,
						   ExStyle,ID,
						   TOOLBARCLASSNAME,NULL,GetAppClass().GetInstance()))
		return false;

	::SendMessage(m_hwnd,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
	::SendMessage(m_hwnd,TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_MIXEDBUTTONS);
	::SendMessage(m_hwnd,TB_SETPADDING,0,MAKELPARAM(4,4));

	return true;
}


bool CCalendarBar::SetButtons(const SYSTEMTIME *pDateList,int Days,int FirstCommand)
{
	if (m_hwnd==NULL)
		return false;

	int ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0);
	for (int i=ButtonCount-1;i>=0;i--)
		::SendMessage(m_hwnd,TB_DELETEBUTTON,i,0);

	TBBUTTON tbb;
	tbb.iBitmap=I_IMAGENONE;
	tbb.fsState=TBSTATE_ENABLED;
	tbb.fsStyle=BTNS_CHECKGROUP | BTNS_SHOWTEXT;
	for (int i=0;i<Days;i++) {
		const SYSTEMTIME &Date=pDateList[i];
		TCHAR szText[32];

		tbb.idCommand=FirstCommand+i;
		StdUtil::snprintf(szText,lengthof(szText),
						  TEXT("%02d/%02d(%s)"),	// %02d にしているのは幅を揃えるため
						  Date.wMonth,Date.wDay,GetDayOfWeekText(Date.wDayOfWeek));
		tbb.iString=reinterpret_cast<INT_PTR>(szText);
		tbb.dwData=((DWORD)Date.wMonth<<16) | ((DWORD)Date.wDay<<8) | Date.wDayOfWeek;
		if (i==0 && Days>1 && Date.wDay==pDateList[i+1].wDay)
			tbb.dwData|=CALENDARBAR_FLAG_NOW;
		::SendMessage(m_hwnd,TB_ADDBUTTONS,1,reinterpret_cast<LPARAM>(&tbb));
	}

#if 0
	RECT rcButton,rcToolbar;
	::SendMessage(m_hwnd,TB_GETRECT,FirstCommand+Days-1,reinterpret_cast<LPARAM>(&rcButton));
	GetPosition(&rcToolbar);
	SetPosition(rcToolbar.left,rcToolbar.top,rcButton.right,rcButton.bottom);
#else
	RECT rcToolbar;
	GetPosition(&rcToolbar);
	SIZE sz;
	::SendMessage(m_hwnd,TB_GETMAXSIZE,0,reinterpret_cast<LPARAM>(&sz));
	SetPosition(rcToolbar.left,rcToolbar.top,sz.cx,sz.cy);
#endif

	return true;
}


void CCalendarBar::SelectButton(int Button)
{
	if (m_hwnd!=NULL)
		::SendMessage(m_hwnd,TB_CHECKBUTTON,Button,TRUE);
}


void CCalendarBar::UnselectButton()
{
	if (m_hwnd!=NULL) {
		const int ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0);

		for (int i=0;i<ButtonCount;i++) {
			TBBUTTON tbb;

			::SendMessage(m_hwnd,TB_GETBUTTON,i,reinterpret_cast<LPARAM>(&tbb));
			::SendMessage(m_hwnd,TB_CHECKBUTTON,tbb.idCommand,FALSE);
		}
	}
}


void CCalendarBar::GetBarSize(SIZE *pSize)
{
	int ButtonCount;

	if (m_hwnd!=NULL && (ButtonCount=(int)::SendMessage(m_hwnd,TB_BUTTONCOUNT,0,0))>0) {
#if 0
		TBBUTTON tbb;
		RECT rc;

		::SendMessage(m_hwnd,TB_GETBUTTON,ButtonCount-1,reinterpret_cast<LPARAM>(&tbb));
		::SendMessage(m_hwnd,TB_GETRECT,tbb.idCommand,reinterpret_cast<LPARAM>(&rc));
		pSize->cx=rc.right;
		pSize->cy=rc.bottom;
#else
		::SendMessage(m_hwnd,TB_GETMAXSIZE,0,reinterpret_cast<LPARAM>(pSize));
#endif
	} else {
		pSize->cx=0;
		pSize->cy=0;
	}
}


bool CCalendarBar::OnNotify(LPARAM lParam,LRESULT *pResult)
{
	NMTBCUSTOMDRAW *pnmtb=reinterpret_cast<NMTBCUSTOMDRAW*>(lParam);

	if (pnmtb->nmcd.hdr.hwndFrom!=m_hwnd
			|| pnmtb->nmcd.hdr.code!=NM_CUSTOMDRAW)
		return false;

	switch (pnmtb->nmcd.dwDrawStage) {
	case CDDS_PREERASE:
		*pResult=CDRF_SKIPDEFAULT;
		break;

	case CDDS_PREPAINT:
		*pResult=CDRF_NOTIFYITEMDRAW;
		break;

	case CDDS_ITEMPREPAINT:
		{
			HDC hdcBuffer,hdc;
			hdc=pnmtb->nmcd.hdc;
			if (m_fUseBufferedPaint) {
				hdcBuffer=m_BufferedPaint.Begin(pnmtb->nmcd.hdc,&pnmtb->nmcd.rc);
				if (hdcBuffer!=NULL)
					hdc=hdcBuffer;
			}

			Theme::Style Style;
			Style.Gradient.Type=Theme::GRADIENT_NORMAL;
			Style.Gradient.Direction=Theme::DIRECTION_VERT;
			if ((pnmtb->nmcd.uItemState&(CDIS_CHECKED | CDIS_HOT))!=0) {
				Style.Gradient.Color1=RGB(220,220,220);
				Style.Gradient.Color2=RGB(255,255,255);
			} else {
				Style.Gradient.Color1=RGB(255,255,255);
				Style.Gradient.Color2=RGB(220,220,220);
			}
			if ((pnmtb->nmcd.uItemState&CDIS_CHECKED)!=0) {
				Style.Border.Type=Theme::BORDER_SUNKEN;
				Style.Border.Color=RGB(220,220,220);
			} else {
				Style.Border.Type=Theme::BORDER_RAISED;
				Style.Border.Color=RGB(255,255,255);
			}
			Theme::DrawStyleBackground(hdc,&pnmtb->nmcd.rc,&Style);

			int DayOfWeek=(int)(pnmtb->nmcd.lItemlParam&0xFF);

			HFONT hfont=reinterpret_cast<HFONT>(::SendMessage(m_hwnd,WM_GETFONT,0,0));
			HGDIOBJ hOldFont=::SelectObject(hdc,hfont);
			int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
			COLORREF OldTextColor=
				::SetTextColor(hdc,DayOfWeek==0?RGB(255,32,0):DayOfWeek==6?RGB(0,32,255):RGB(0,0,0));

			TCHAR szText[32];
			if ((pnmtb->nmcd.lItemlParam&CALENDARBAR_FLAG_NOW)!=0) {
				::lstrcpy(szText,TEXT("現在"));
			} else {
				StdUtil::snprintf(szText,lengthof(szText),TEXT("%d/%d(%s)"),
								  (int)(pnmtb->nmcd.lItemlParam>>16),
								  (int)((pnmtb->nmcd.lItemlParam>>8)&0xFF),
								  GetDayOfWeekText(DayOfWeek));
			}
			::DrawText(hdc,szText,-1,&pnmtb->nmcd.rc,
					   DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

			::SetTextColor(hdc,OldTextColor);
			::SetBkMode(hdc,OldBkMode);
			::SelectObject(hdc,hOldFont);

			if (hdcBuffer!=NULL) {
				//m_BufferedPaint.SetAlpha(224);
				m_BufferedPaint.SetOpaque();
				m_BufferedPaint.End();
			}
		}
		*pResult=CDRF_SKIPDEFAULT;
		break;

	default:
		*pResult=CDRF_DODEFAULT;
		break;
	}

	return true;
}




CProgramGuideFrameBase::CProgramGuideFrameBase(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
	m_StatusMargin.left=0;
	m_StatusMargin.top=0;
	m_StatusMargin.right=0;
	m_StatusMargin.bottom=5;
	m_StatusItemMargin=6;
	m_StatusView[0].AddItem(new CProgramGuideTunerStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CListSelectStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CListPrevStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CListNextStatusItem(m_pProgramGuide));
}


CProgramGuideFrameBase::~CProgramGuideFrameBase()
{
}


void CProgramGuideFrameBase::OnDateChanged()
{
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATE);
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATEPREV);
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATENEXT);

	if (m_pProgramGuide->GetListMode()==CProgramGuide::LIST_SERVICES)
		m_CalendarBar.SelectButton(CM_PROGRAMGUIDE_TODAY+m_pProgramGuide->GetViewDay());
	else
		m_CalendarBar.UnselectButton();
}


void CProgramGuideFrameBase::OnSpaceChanged()
{
	m_StatusView[0].UpdateItem(STATUS_ITEM_TUNER);
}


void CProgramGuideFrameBase::OnListModeChanged()
{
	OnDateChanged();
}


void CProgramGuideFrameBase::OnTimeRangeChanged()
{
	SYSTEMTIME DateList[7];

	for (int i=0;i<7;i++)
		m_pProgramGuide->GetDayTimeRange(i,&DateList[i],NULL);
	m_CalendarBar.SetButtons(DateList,7,CM_PROGRAMGUIDE_TODAY);
	m_CalendarBar.SelectButton(CM_PROGRAMGUIDE_TODAY+m_pProgramGuide->GetViewDay());
}


void CProgramGuideFrameBase::OnWindowCreate(HWND hwnd,bool fBufferedPaint)
{
	m_pProgramGuide->SetFrame(this);
	m_pProgramGuide->Create(hwnd,WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL);

	for (int i=0;i<lengthof(m_StatusView);i++) {
		m_StatusView[i].Create(hwnd,WS_CHILD | WS_VISIBLE);
		m_StatusView[i].EnableBufferedPaint(fBufferedPaint);
	}

	m_CalendarBar.Create(hwnd,WS_CHILD | WS_VISIBLE);
	m_CalendarBar.EnableBufferedPaint(fBufferedPaint);
}


void CProgramGuideFrameBase::OnWindowDestroy()
{
	m_pProgramGuide->SetFrame(NULL);
	m_CalendarBar.Destroy();
}


void CProgramGuideFrameBase::OnSizeChanged(int Width,int Height)
{
	int x=m_StatusMargin.left;
	int y=m_StatusMargin.top;
	for (int i=0;i<lengthof(m_StatusView);i++) {
		int w=m_StatusView[i].GetIntegralWidth();
		m_StatusView[i].SetPosition(x,y,w,m_StatusView[i].GetHeight());
		x+=w+m_StatusItemMargin;
	}

	SIZE sz;
	m_CalendarBar.GetBarSize(&sz);
	if (x+sz.cx>Width-m_StatusMargin.right)
		sz.cx=max(Width-m_StatusMargin.right-x,0);
	m_CalendarBar.SetPosition(x,y,sz.cx,sz.cy);

	int BarHeight=m_StatusView[0].GetHeight();
	if (sz.cy>Height)
		BarHeight=sz.cy;
	y+=BarHeight;
	y+=m_StatusMargin.bottom;
	m_pProgramGuide->SetPosition(0,y,Width,Height-y);
}


void CProgramGuideFrameBase::SetStatusTheme(const CStatusView::ThemeInfo *pTheme)
{
	for (int i=0;i<lengthof(m_StatusView);i++)
		m_StatusView[i].SetTheme(pTheme);
}


LRESULT CProgramGuideFrameBase::DefaultMessageHandler(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_SIZE:
		OnSizeChanged(LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			::SendMessage(hwnd,WM_CLOSE,0,0);
			return 0;
		}
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		return m_pProgramGuide->SendMessage(uMsg,wParam,lParam);

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam)==m_CalendarBar.GetHandle()) {
			::SetCursor(::LoadCursor(NULL,IDC_HAND));
			return TRUE;
		}
		break;

	case WM_NOTIFY:
		{
			LRESULT Result;

			if (m_CalendarBar.OnNotify(lParam,&Result))
				return Result;
		}
		break;

	case WM_COMMAND:
		if (LOWORD(wParam)>=CM_PROGRAMGUIDE_TODAY
				&& LOWORD(wParam)<=CM_PROGRAMGUIDE_TODAY+CProgramGuide::DAY_LAST){
			return m_pProgramGuide->SendMessage(WM_COMMAND,wParam,lParam);
		}
		break;

	case WM_DESTROY:
		OnWindowDestroy();
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




const LPCTSTR CProgramGuideFrame::m_pszWindowClass=APP_NAME TEXT(" Program Guide Frame");
HINSTANCE CProgramGuideFrame::m_hinst=NULL;


bool CProgramGuideFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=::LoadIcon(hinst,MAKEINTRESOURCE(IDI_PROGRAMGUIDE));
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=::CreateSolidBrush(0xFF000000);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuideFrame::CProgramGuideFrame(CProgramGuide *pProgramGuide)
	: CProgramGuideFrameBase(pProgramGuide)
	, m_fAlwaysOnTop(false)
{
	m_WindowPosition.Width=640;
	m_WindowPosition.Height=480;
}


CProgramGuideFrame::~CProgramGuideFrame()
{
	Destroy();
}


bool CProgramGuideFrame::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_WindowPosition.fMaximized)
		Style|=WS_MAXIMIZE;
	if (m_fAlwaysOnTop)
		ExStyle|=WS_EX_TOPMOST;
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,TITLE_TEXT,m_hinst);
}


bool CProgramGuideFrame::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop!=fTop) {
		m_fAlwaysOnTop=fTop;
		if (m_hwnd!=NULL)
			::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	}
	return true;
}


void CProgramGuideFrame::SetCaption(LPCTSTR pszFileName)
{
	::SetWindowText(m_hwnd,pszFileName);
}


LRESULT CProgramGuideFrame::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		OnWindowCreate(hwnd,true);
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			break;
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
	case WM_DWMCOMPOSITIONCHANGED:
		{
			RECT rc={0,0,0,0};

			rc.top=m_StatusView[0].GetHeight()+m_StatusMargin.top+m_StatusMargin.bottom;
			m_AeroGlass.ApplyAeroGlass(hwnd,&rc);
		}
		return 0;

	case WM_ERASEBKGND:
		if (!m_AeroGlass.IsEnabled()) {
			RECT rc;
			::GetClientRect(hwnd,&rc);
			::FillRect(reinterpret_cast<HDC>(wParam),&rc,
					   reinterpret_cast<HBRUSH>(COLOR_3DFACE+1));
			return 1;
		}
		break;

	case WM_CLOSE:
		if (!m_pProgramGuide->OnClose())
			return 0;
		break;
	}
	return DefaultMessageHandler(hwnd,uMsg,wParam,lParam);
}




const LPCTSTR CProgramGuideDisplay::m_pszWindowClass=APP_NAME TEXT(" Program Guide Display");
HINSTANCE CProgramGuideDisplay::m_hinst=NULL;


bool CProgramGuideDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=::CreateSolidBrush(RGB(0,0,0));
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuideDisplay::CProgramGuideDisplay(CProgramGuide *pProgramGuide)
	: CProgramGuideFrameBase(pProgramGuide)
	, m_pEventHandler(NULL)
{
	m_StatusMargin.left=6;
	m_StatusMargin.top=m_StatusMargin.bottom;
}


CProgramGuideDisplay::~CProgramGuideDisplay()
{
	Destroy();
}


bool CProgramGuideDisplay::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
}


void CProgramGuideDisplay::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pProgramGuideDisplay=NULL;
	if (pHandler!=NULL)
		pHandler->m_pProgramGuideDisplay=this;
	m_pEventHandler=pHandler;
}


bool CProgramGuideDisplay::OnVisibleChange(bool fVisible)
{
	if (!fVisible && m_pEventHandler!=NULL)
		return m_pEventHandler->OnHide();
	return true;
}


bool CProgramGuideDisplay::SetAlwaysOnTop(bool fTop)
{
	if (m_pEventHandler==NULL)
		return false;
	return m_pEventHandler->SetAlwaysOnTop(fTop);
}


bool CProgramGuideDisplay::GetAlwaysOnTop() const
{
	if (m_pEventHandler==NULL)
		return false;
	return m_pEventHandler->GetAlwaysOnTop();
}


LRESULT CProgramGuideDisplay::OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		OnWindowCreate(hwnd,false);
		return 0;

	case WM_SIZE:
		{
			RECT rc;

			GetCloseButtonRect(&rc);
			m_StatusMargin.right=(LOWORD(lParam)-rc.left)+m_StatusMargin.left;
		}
		break;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			DrawCloseButton(ps.hdc);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (CloseButtonHitTest(x,y))
				SetVisible(false);
			else
				::SetFocus(hwnd);
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnLButtonDoubleClick(
				GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnRButtonDown(
				GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			SetVisible(false);
			return 0;
		}
		break;
	}
	return DefaultMessageHandler(hwnd,uMsg,wParam,lParam);
}


CProgramGuideDisplay::CEventHandler::CEventHandler()
	: m_pProgramGuideDisplay(NULL)
{
}


CProgramGuideDisplay::CEventHandler::~CEventHandler()
{
}




CProgramGuideTool::CProgramGuideTool()
	: m_hIcon(NULL)
{
	m_szName[0]='\0';
	m_szCommand[0]='\0';
}


CProgramGuideTool::CProgramGuideTool(const CProgramGuideTool &Tool)
	: m_hIcon(NULL)
{
	*this=Tool;
}


CProgramGuideTool::CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand)
	: m_hIcon(NULL)
{
	::lstrcpy(m_szName,pszName);
	::lstrcpy(m_szCommand,pszCommand);
}


CProgramGuideTool::~CProgramGuideTool()
{
	if (m_hIcon!=NULL)
		::DestroyIcon(m_hIcon);
}


CProgramGuideTool &CProgramGuideTool::operator=(const CProgramGuideTool &Tool)
{
	if (&Tool!=this) {
		::lstrcpy(m_szName,Tool.m_szName);
		::lstrcpy(m_szCommand,Tool.m_szCommand);
		if (m_hIcon!=NULL)
			::DestroyIcon(m_hIcon);
		if (Tool.m_hIcon!=NULL)
			m_hIcon=::CopyIcon(Tool.m_hIcon);
		else
			m_hIcon=NULL;
	}
	return *this;
}


bool CProgramGuideTool::GetPath(LPTSTR pszPath,int MaxLength) const
{
	LPCTSTR p=m_szCommand;

	return GetCommandFileName(&p,pszPath,MaxLength);
}


HICON CProgramGuideTool::GetIcon()
{
	if (m_hIcon==NULL && m_szCommand[0]!='\0') {
		TCHAR szFileName[MAX_PATH];
		LPCTSTR p=m_szCommand;

		if (GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
			SHFILEINFO shfi;
			if (::SHGetFileInfo(szFileName,0,&shfi,sizeof(shfi),
								SHGFI_ICON | SHGFI_SMALLICON))
				m_hIcon=shfi.hIcon;
		}
	}
	return m_hIcon;
}


bool CProgramGuideTool::Execute(const ProgramGuide::CServiceInfo *pServiceInfo,
								const CEventInfoData *pEventInfo,HWND hwnd)
{
	if (pServiceInfo==NULL || pEventInfo==NULL)
		return false;

	SYSTEMTIME stStart,stEnd;
	TCHAR szFileName[MAX_PATH],szCommand[2048];
	LPCTSTR p;
	LPTSTR q,pEnd;

	pEventInfo->GetStartTime(&stStart);
	pEventInfo->GetEndTime(&stEnd);
	p=m_szCommand;
	if (!GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
		::MessageBox(hwnd,TEXT("パスが長すぎます。"),NULL,MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	while (*p==_T(' '))
		p++;
	q=szCommand;
	pEnd=q+lengthof(szCommand);
	while (*p!=_T('\0')) {
		if (q>=pEnd) {
			::MessageBox(hwnd,TEXT("コマンドが長すぎます。"),NULL,MB_OK | MB_ICONEXCLAMATION);
			return false;
		}

		if (*p==_T('%')) {
			p++;
			if (*p==_T('%')) {
				*q++=_T('%');
				p++;
			} else {
				TCHAR szKeyword[32];
				int i;

				for (i=0;*p!=_T('%') && *p!=_T('\0');i++) {
					if (i<lengthof(szKeyword)-1)
						szKeyword[i]=*p;
					p++;
				}
				if (*p==_T('%'))
					p++;
				szKeyword[i]=_T('\0');
				if (::lstrcmpi(szKeyword,TEXT("tvpid"))==0) {
					TCHAR sziEpgFileName[MAX_PATH+11];

					GetAppClass().GetAppDirectory(sziEpgFileName);
					::PathAppend(sziEpgFileName,TEXT("iepg.tvpid"));
					if (!pServiceInfo->SaveiEpgFile(pEventInfo,sziEpgFileName,true)) {
						::MessageBox(hwnd,TEXT("iEPGファイルの書き出しができません。"),NULL,
									 MB_OK | MB_ICONEXCLAMATION);
						return false;
					}
					size_t Length=::lstrlen(sziEpgFileName);
					if (Length>=(size_t)(pEnd-q)) {
						::MessageBox(hwnd,TEXT("コマンドが長すぎます。"),NULL,
									 MB_OK | MB_ICONEXCLAMATION);
						return false;
					}
					::lstrcpy(q,sziEpgFileName);
					q+=Length;
				} else if (::lstrcmpi(szKeyword,TEXT("eid"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),pEventInfo->m_EventID);
				} else if (::lstrcmpi(szKeyword,TEXT("nid"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),pServiceInfo->GetNetworkID());
				} else if (::lstrcmpi(szKeyword,TEXT("tsid"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),pServiceInfo->GetTSID());
				} else if (::lstrcmpi(szKeyword,TEXT("sid"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),pServiceInfo->GetServiceID());
				} else if (::lstrcmpi(szKeyword,TEXT("start-year"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stStart.wYear);
				} else if (::lstrcmpi(szKeyword,TEXT("start-month"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stStart.wMonth);
				} else if (::lstrcmpi(szKeyword,TEXT("start-day"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stStart.wDay);
				} else if (::lstrcmpi(szKeyword,TEXT("start-hour"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stStart.wHour);
				} else if (::lstrcmpi(szKeyword,TEXT("start-minute"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%02d"),stStart.wMinute);
				} else if (::lstrcmpi(szKeyword,TEXT("end-year"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stEnd.wYear);
				} else if (::lstrcmpi(szKeyword,TEXT("end-month"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stEnd.wMonth);
				} else if (::lstrcmpi(szKeyword,TEXT("end-day"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stEnd.wDay);
				} else if (::lstrcmpi(szKeyword,TEXT("end-hour"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),stEnd.wHour);
				} else if (::lstrcmpi(szKeyword,TEXT("end-minute"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%02d"),stEnd.wMinute);
				} else if (::lstrcmpi(szKeyword,TEXT("duration-sec"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),pEventInfo->m_DurationSec);
				} else if (::lstrcmpi(szKeyword,TEXT("duration-min"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%d"),(pEventInfo->m_DurationSec+59)/60);
				} else if (::lstrcmpi(szKeyword,TEXT("event-name"))==0) {
					if (pEventInfo->GetEventName()!=NULL)
						q+=StdUtil::snprintf(q,pEnd-q,TEXT("%s"),pEventInfo->GetEventName());
				} else if (::lstrcmpi(szKeyword,TEXT("service-name"))==0) {
					q+=StdUtil::snprintf(q,pEnd-q,TEXT("%s"),pServiceInfo->GetServiceName());
				}
			}
		} else {
#ifndef UNICODE
			if (::IsDBCSLeadByteEx(CP_ACP,*p) && *(p+1)!='\0')
				*q++=*p++;
#endif
			*q++=*p++;
		}
	}
	*q=_T('\0');
	TRACE(TEXT("外部ツール実行 : %s, %s\n"),szFileName,szCommand);
	return ::ShellExecute(NULL,NULL,szFileName,szCommand,NULL,SW_SHOWNORMAL)>=(HINSTANCE)32;
}


bool CProgramGuideTool::ShowDialog(HWND hwndOwner)
{
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_PROGRAMGUIDETOOL),
							hwndOwner,DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


CProgramGuideTool *CProgramGuideTool::GetThis(HWND hDlg)
{
	return static_cast<CProgramGuideTool*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CProgramGuideTool::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CProgramGuideTool *pThis=reinterpret_cast<CProgramGuideTool*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
								 EM_LIMITTEXT,MAX_NAME-1,0);
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,pThis->m_szName);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,
								 EM_LIMITTEXT,MAX_COMMAND-1,0);
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,pThis->m_szCommand);
			if (pThis->m_szName[0]!='\0' && pThis->m_szCommand[0]!='\0')
				EnableDlgItem(hDlg,IDOK,true);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDETOOL_NAME:
		case IDC_PROGRAMGUIDETOOL_COMMAND:
			if (HIWORD(wParam)==EN_CHANGE)
				EnableDlgItem(hDlg,IDOK,
					GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)>0
					&& GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND)>0);
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szCommand[MAX_COMMAND];
				TCHAR szFileName[MAX_PATH],szDirectory[MAX_PATH];

				GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szCommand,MAX_COMMAND);
				if (szCommand[0]!='\0') {
					LPCTSTR p;

					p=szCommand;
					GetCommandFileName(&p,szFileName,lengthof(szFileName));
				} else
					szFileName[0]='\0';
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("実行ファイル(*.exe;*.bat)\0*.exe;*.bat\0")
								TEXT("すべてのファイル\0*.*\0");
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=MAX_PATH;
				if (szFileName[0]!='\0') {
					CFilePath Path(szFileName);
					Path.GetDirectory(szDirectory);
					ofn.lpstrInitialDir=szDirectory;
					::lstrcpy(szFileName,Path.GetFileName());
				}
				ofn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::PathQuoteSpaces(szFileName);
					//::lstrcat(szFileName,TEXT(" \"%tvpid%\""));
					::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szFileName);
					if (GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)==0) {
						*::PathFindExtension(szFileName)='\0';
						::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
										 ::PathFindFileName(szFileName));
					}
				}
				return TRUE;

			case IDC_PROGRAMGUIDETOOL_PARAMETER:
				{
					static const struct {
						LPCTSTR pszParameter;
						LPCTSTR pszText;
					} ParameterList[] = {
						{TEXT("%tvpid%"),			TEXT("iEPGファイル")},
						{TEXT("%eid%"),				TEXT("イベントID")},
						{TEXT("%nid%"),				TEXT("ネットワークID")},
						{TEXT("%tsid%"),			TEXT("ストリームID")},
						{TEXT("%sid%"),				TEXT("サービスID")},
						{TEXT("%start-year%"),		TEXT("開始年")},
						{TEXT("%start-month%"),		TEXT("開始月")},
						{TEXT("%start-day%"),		TEXT("開始日")},
						{TEXT("%start-hour%"),		TEXT("開始時間")},
						{TEXT("%start-minute%"),	TEXT("開始分")},
						{TEXT("%end-year%"),		TEXT("終了年")},
						{TEXT("%end-month%"),		TEXT("終了月")},
						{TEXT("%end-day%"),			TEXT("終了日")},
						{TEXT("%end-hour%"),		TEXT("終了時間")},
						{TEXT("%end-minute%"),		TEXT("終了分")},
						{TEXT("%duration-sec%"),	TEXT("秒単位の長さ")},
						{TEXT("%duration-min%"),	TEXT("分単位の長さ")},
						{TEXT("%event-name%"),		TEXT("イベント名")},
						{TEXT("%service-name%"),	TEXT("サービス名")},
					};
					HMENU hmenu=::CreatePopupMenu();
					RECT rc;
					int Command;

					for (int i=0;i<lengthof(ParameterList);i++) {
						TCHAR szText[128];

						StdUtil::snprintf(szText,lengthof(szText),TEXT("%s (%s)"),
										  ParameterList[i].pszParameter,
										  ParameterList[i].pszText);
						::AppendMenu(hmenu,MF_STRING | MF_ENABLED,i+1,szText);
					}
					::GetWindowRect(::GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_PARAMETER),&rc);
					Command=::TrackPopupMenu(hmenu,TPM_RETURNCMD,
											 rc.left,rc.bottom,0,hDlg,NULL);
					::DestroyMenu(hmenu);
					if (Command>0) {
						DWORD Start,End;

						::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,EM_GETSEL,
							reinterpret_cast<LPARAM>(&Start),
							reinterpret_cast<LPARAM>(&End));
						::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,EM_REPLACESEL,
							TRUE,reinterpret_cast<LPARAM>(ParameterList[Command-1].pszParameter));
						if (End<Start)
							Start=End;
						::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,EM_SETSEL,
							Start,Start+::lstrlen(ParameterList[Command-1].pszParameter));
					}
				}
				return TRUE;
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_HELP:
			GetAppClass().ShowHelpContent(HELP_ID_PROGRAMGUIDETOOL);
			return TRUE;

		case IDOK:
			{
				CProgramGuideTool *pThis=GetThis(hDlg);

				::GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
								 pThis->m_szName,MAX_NAME);
				::GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,
								 pThis->m_szCommand,MAX_COMMAND);
				if (pThis->m_hIcon!=NULL) {
					::DestroyIcon(pThis->m_hIcon);
					pThis->m_hIcon=NULL;
				}
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


bool CProgramGuideTool::GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName,int MaxFileName)
{
	LPCTSTR p;
	LPTSTR q;
	TCHAR cDelimiter;

	p=*ppszCommand;
	q=pszFileName;
	if (*p==_T('"')) {
		cDelimiter=_T('"');
		p++;
	} else
		cDelimiter=_T(' ');
	for (int i=0;*p!=_T('\0') && *p!=cDelimiter;i++) {
#ifndef UNICODE
		if (IsDBCSLeadByteEx(CP_ACP,*p) && *(p+1)!=_T('\0')) {
			*q++=*p++;
			i++;
		}
#endif
		if (i>=MaxFileName-1) {
			pszFileName[0]=_T('\0');
			return false;
		}
		*q++=*p++;
	}
	*q=_T('\0');
	if (*p==_T('"'))
		p++;
	*ppszCommand=p;
	return true;
}




CProgramGuideToolList::CProgramGuideToolList()
{
}


CProgramGuideToolList::CProgramGuideToolList(const CProgramGuideToolList &Src)
{
	*this=Src;
}


CProgramGuideToolList::~CProgramGuideToolList()
{
	Clear();
}


CProgramGuideToolList &CProgramGuideToolList::operator=(const CProgramGuideToolList &Src)
{
	if (&Src!=this) {
		Clear();
		if (Src.m_ToolList.size()>0) {
			m_ToolList.resize(Src.m_ToolList.size());
			for (size_t i=0;i<Src.m_ToolList.size();i++)
				m_ToolList[i]=new CProgramGuideTool(*Src.m_ToolList[i]);
		}
	}
	return *this;
}


void CProgramGuideToolList::Clear()
{
	for (size_t i=0;i<m_ToolList.size();i++)
		delete m_ToolList[i];
	m_ToolList.clear();
}


bool CProgramGuideToolList::Add(CProgramGuideTool *pTool)
{
	if (pTool==NULL)
		return false;
	m_ToolList.push_back(pTool);
	return true;
}


CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index)
{
	if (Index>=m_ToolList.size())
		return NULL;
	return m_ToolList[Index];
}


const CProgramGuideTool *CProgramGuideToolList::GetTool(size_t Index) const
{
	if (Index>=m_ToolList.size())
		return NULL;
	return m_ToolList[Index];
}
