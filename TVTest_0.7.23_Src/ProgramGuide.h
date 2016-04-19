#ifndef PROGRAM_GUIDE_H
#define PROGRAM_GUIDE_H


#include "View.h"
#include "EpgProgramList.h"
#include "EpgUtil.h"
#include "ChannelList.h"
#include "DriverManager.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "ProgramSearch.h"
#include "EventInfoPopup.h"
#include "Tooltip.h"
#include "StatusView.h"


namespace ProgramGuide
{

	class CEventItem;
	class CEventLayout;
	class CServiceInfo;

	class CEventLayoutList
	{
		std::vector<CEventLayout*> m_LayoutList;

	public:
		~CEventLayoutList();
		void Clear();
		size_t Length() const { return m_LayoutList.size(); }
		void Add(CEventLayout *pLayout);
		const CEventLayout *operator[](size_t Index) const;
	};

	class CServiceList
	{
		std::vector<CServiceInfo*> m_ServiceList;

	public:
		~CServiceList();
		size_t NumServices() const { return m_ServiceList.size(); }
		CServiceInfo *GetItem(size_t Index);
		const CServiceInfo *GetItem(size_t Index) const;
		CServiceInfo *FindItem(WORD TransportStreamID,WORD ServiceID);
		const CServiceInfo *FindItem(WORD TransportStreamID,WORD ServiceID) const;
		CEventInfoData *FindEvent(WORD TransportStreamID,WORD ServiceID,WORD EventID);
		const CEventInfoData *FindEvent(WORD TransportStreamID,WORD ServiceID,WORD EventID) const;
		void Add(CServiceInfo *pInfo);
		void Clear();
	};

}

class CProgramGuideTool
{
public:
	enum { MAX_NAME=64, MAX_COMMAND=MAX_PATH*2 };

private:
	TCHAR m_szName[MAX_NAME];
	TCHAR m_szCommand[MAX_COMMAND];
	HICON m_hIcon;

	static bool GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName,int MaxFileName);
	static CProgramGuideTool *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CProgramGuideTool();
	CProgramGuideTool(const CProgramGuideTool &Tool);
	CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand);
	~CProgramGuideTool();
	CProgramGuideTool &operator=(const CProgramGuideTool &Tool);
	LPCTSTR GetName() const { return m_szName; }
	LPCTSTR GetCommand() const { return m_szCommand; }
	bool GetPath(LPTSTR pszPath,int MaxLength) const;
	HICON GetIcon();
	bool Execute(const ProgramGuide::CServiceInfo *pServiceInfo,
				 const CEventInfoData *pEventInfo,HWND hwnd);
	bool ShowDialog(HWND hwndOwner);
};

class CProgramGuideToolList
{
	std::vector<CProgramGuideTool*> m_ToolList;

public:
	CProgramGuideToolList();
	CProgramGuideToolList(const CProgramGuideToolList &Src);
	~CProgramGuideToolList();
	CProgramGuideToolList &operator=(const CProgramGuideToolList &Src);
	void Clear();
	bool Add(CProgramGuideTool *pTool);
	CProgramGuideTool *GetTool(size_t Index);
	const CProgramGuideTool *GetTool(size_t Index) const;
	size_t NumTools() const { return m_ToolList.size(); }
};

class CProgramGuide : public CCustomWindow, protected CDoubleBufferingDraw
{
public:
	enum ListMode {
		LIST_SERVICES,
		LIST_WEEK
	};
	enum {
		DAY_TODAY,
		DAY_TOMORROW,
		DAY_DAYAFTERTOMORROW,
		DAY_2DAYSAFTERTOMORROW,
		DAY_3DAYSAFTERTOMORROW,
		DAY_4DAYSAFTERTOMORROW,
		DAY_5DAYSAFTERTOMORROW,
		DAY_LAST=DAY_5DAYSAFTERTOMORROW
	};
	enum {
		COLOR_BACK,
		COLOR_TEXT,
		COLOR_CHANNELNAMETEXT,
		COLOR_CURCHANNELNAMETEXT,
		COLOR_TIMETEXT,
		COLOR_TIMELINE,
		COLOR_CURTIMELINE,
		COLOR_CONTENT_NEWS,
		COLOR_CONTENT_SPORTS,
		COLOR_CONTENT_INFORMATION,
		COLOR_CONTENT_DRAMA,
		COLOR_CONTENT_MUSIC,
		COLOR_CONTENT_VARIETY,
		COLOR_CONTENT_MOVIE,
		COLOR_CONTENT_ANIME,
		COLOR_CONTENT_DOCUMENTARY,
		COLOR_CONTENT_THEATER,
		COLOR_CONTENT_EDUCATION,
		COLOR_CONTENT_WELFARE,
		COLOR_CONTENT_OTHER,
		COLOR_CONTENT_FIRST=COLOR_CONTENT_NEWS,
		COLOR_CONTENT_LAST=COLOR_CONTENT_OTHER,
		COLOR_LAST=COLOR_CONTENT_LAST,
		NUM_COLORS=COLOR_LAST+1
	};
	enum { MIN_LINES_PER_HOUR=8, MAX_LINES_PER_HOUR=60 };
	enum { MIN_ITEM_WIDTH=100, MAX_ITEM_WIDTH=500 };
	enum { TIME_BAR_BACK_COLORS=8 };

	enum {
		FILTER_FREE			=0x00000001U,
		FILTER_NEWPROGRAM	=0x00000002U,
		FILTER_ORIGINAL		=0x00000004U,
		FILTER_RERUN		=0x00000008U,
		FILTER_NOT_SHOPPING	=0x00000010U,
		FILTER_GENRE_FIRST	=0x00000100U,
		FILTER_GENRE_MASK	=0x000FFF00U,
		FILTER_NEWS			=0x00000100U,
		FILTER_SPORTS		=0x00000200U,
		FILTER_INFORMATION	=0x00000400U,
		FILTER_DRAMA		=0x00000800U,
		FILTER_MUSIC		=0x00001000U,
		FILTER_VARIETY		=0x00002000U,
		FILTER_MOVIE		=0x00004000U,
		FILTER_ANIME		=0x00008000U,
		FILTER_DOCUMENTARY	=0x00010000U,
		FILTER_THEATER		=0x00020000U,
		FILTER_EDUCATION	=0x00040000U,
		FILTER_WELFARE		=0x00080000U
	};

	struct ServiceInfo
	{
		WORD NetworkID;
		WORD TransportStreamID;
		WORD ServiceID;

		ServiceInfo() { Clear(); }
		ServiceInfo(WORD NID,WORD TSID,WORD SID)
			: NetworkID(NID), TransportStreamID(TSID), ServiceID(SID) {}
		void Clear() {
			NetworkID=0;
			TransportStreamID=0;
			ServiceID=0;
		}
	};

	typedef std::vector<ServiceInfo> ServiceInfoList;

	class ABSTRACT_CLASS(CFrame)
	{
	protected:
		CProgramGuide *m_pProgramGuide;
	public:
		CFrame();
		virtual ~CFrame() = 0;
		virtual void SetCaption(LPCTSTR pszCaption) {}
		virtual void OnDateChanged() {}
		virtual void OnSpaceChanged() {}
		virtual void OnListModeChanged() {}
		virtual void OnTimeRangeChanged() {}
		virtual bool SetAlwaysOnTop(bool fTop) { return false; }
		virtual bool GetAlwaysOnTop() const { return false; }
		friend class CProgramGuide;
	};

	class ABSTRACT_CLASS(CEventHandler)
	{
	protected:
		class CProgramGuide *m_pProgramGuide;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnClose() { return true; }
		virtual void OnDestroy() {}
		virtual void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo) {}
		virtual bool OnBeginUpdate() { return true; }
		virtual void OnEndUpdate() {}
		virtual bool OnRefresh() { return true; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual bool OnMenuInitialize(HMENU hmenu,UINT CommandBase) { return false; }
		virtual bool OnMenuSelected(UINT Command) { return false; }
		friend class CProgramGuide;
	};

	class ABSTRACT_CLASS(CProgramCustomizer)
	{
	protected:
		class CProgramGuide *m_pProgramGuide;
	public:
		CProgramCustomizer();
		virtual ~CProgramCustomizer();
		virtual bool Initialize() { return true; }
		virtual void Finalize() {}
		virtual bool DrawBackground(const CEventInfoData &Event,HDC hdc,
			const RECT &ItemRect,const RECT &TitleRect,const RECT &ContentRect,
			COLORREF BackgroundColor) { return false; }
		virtual bool InitializeMenu(const CEventInfoData &Event,HMENU hmenu,UINT CommandBase,
									const POINT &CursorPos,const RECT &ItemRect) { return false; }
		virtual bool ProcessMenu(const CEventInfoData &Event,UINT Command) { return false; }
		virtual bool OnLButtonDoubleClick(const CEventInfoData &Event,
										  const POINT &CursorPos,const RECT &ItemRect) { return false; }
		friend class CProgramGuide;
	};

	static bool Initialize(HINSTANCE hinst);

	CProgramGuide();
	~CProgramGuide();

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;

// CProgramGuide
	bool SetEpgProgramList(CEpgProgramList *pList);
	void Clear();
	bool UpdateProgramGuide(bool fUpdateList=false);
	bool SetTuningSpaceList(LPCTSTR pszDriverFileName,const CTuningSpaceList *pList,int Space);
	const CChannelList *GetChannelList() const { return &m_ChannelList; }
	const ProgramGuide::CServiceList &GetServiceList() const { return m_ServiceList; }
	bool UpdateChannelList();
	bool SetDriverList(const CDriverManager *pDriverManager);
	void SetCurrentService(WORD NetworkID,WORD TSID,WORD ServiceID);
	void ClearCurrentService() { SetCurrentService(0,0,0); }
	int GetCurrentTuningSpace() const { return m_CurrentTuningSpace; }
	bool GetTuningSpaceName(int Space,LPTSTR pszName,int MaxName) const;
	bool EnumDriver(int *pIndex,LPTSTR pszName,int MaxName) const;

	bool GetExcludeNoEventServices() const { return m_fExcludeNoEventServices; }
	bool SetExcludeNoEventServices(bool fExclude);
	bool SetExcludeServiceList(const ServiceInfoList &List);
	bool GetExcludeServiceList(ServiceInfoList *pList) const;
	bool IsExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const;
	bool SetExcludeService(WORD NetworkID,WORD TransportStreamID,WORD ServiceID,bool fExclude);

	ListMode GetListMode() const { return m_ListMode; }
	bool SetServiceListMode();
	bool SetWeekListMode(int Service);
	int GetWeekListService() const { return m_WeekListService; }
	bool SetBeginHour(int Hour);
	bool SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime);
	bool GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool GetCurrentTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool GetDayTimeRange(int Day,SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool SetViewDay(int Day);
	int GetViewDay() const { return m_Day; }
	int GetLinesPerHour() const { return m_LinesPerHour; }
	int GetItemWidth() const { return m_ItemWidth; }
	bool SetUIOptions(int LinesPerHour,int ItemWidth,int LineMargin);
	bool SetColor(int Type,COLORREF Color);
	void SetBackColors(const Theme::GradientInfo *pChannelBackGradient,
					   const Theme::GradientInfo *pCurChannelBackGradient,
					   const Theme::GradientInfo *pTimeBarMarginGradient,
					   const Theme::GradientInfo *pTimeBarBackGradient);
	bool SetFont(const LOGFONT *pFont);
	bool GetFont(LOGFONT *pFont) const;
	bool SetEventInfoFont(const LOGFONT *pFont);
	bool SetShowToolTip(bool fShow);
	bool GetShowToolTip() const { return m_fShowToolTip; }
	void SetEventHandler(CEventHandler *pEventHandler);
	void SetFrame(CFrame *pFrame);
	void SetProgramCustomizer(CProgramCustomizer *pProgramCustomizer);
	CProgramGuideToolList *GetToolList() { return &m_ToolList; }
	int GetWheelScrollLines() const { return m_WheelScrollLines; }
	void SetWheelScrollLines(int Lines) { m_WheelScrollLines=Lines; }
	bool GetDragScroll() const { return m_fDragScroll; }
	bool SetDragScroll(bool fDragScroll);
	bool SetFilter(unsigned int Filter);
	unsigned int GetFilter() const { return m_Filter; }
	void SetVisibleEventIcons(UINT VisibleIcons);
	UINT GetVisibleEventIcons() const { return m_VisibleEventIcons; }
	CProgramSearch *GetProgramSearch() { return &m_ProgramSearch; }
	void GetInfoPopupSize(int *pWidth,int *pHeight) { m_EventInfoPopup.GetSize(pWidth,pHeight); }
	bool SetInfoPopupSize(int Width,int Height) { return m_EventInfoPopup.SetSize(Width,Height); }
	bool OnClose();

private:
	CEpgProgramList *m_pProgramList;
	ProgramGuide::CServiceList m_ServiceList;
	ProgramGuide::CEventLayoutList m_EventLayoutList;
	ListMode m_ListMode;
	int m_WeekListService;
	int m_LinesPerHour;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	DrawUtil::CFont m_TimeFont;
	int m_FontHeight;
	int m_LineMargin;
	int m_ItemWidth;
	int m_ItemMargin;
	int m_TextLeftMargin;
	int m_HeaderHeight;
	int m_TimeBarWidth;
	POINT m_ScrollPos;
	POINT m_OldScrollPos;
	bool m_fDragScroll;
	HCURSOR m_hDragCursor1;
	HCURSOR m_hDragCursor2;
	struct {
		POINT StartCursorPos;
		POINT StartScrollPos;
	} m_DragInfo;
	DrawUtil::CMonoColorBitmap m_Chevron;
	CEpgIcons m_EpgIcons;
	UINT m_VisibleEventIcons;
	bool m_fBarShadow;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
								 , public CEventInfoPopup::CEventHandler
	{
		CProgramGuide *m_pProgramGuide;
	// CEventInfoPopupManager::CEventHandler
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
		bool OnShow(const CEventInfoData *pInfo);
	// CEventInfoPopup::CEventHandler
		bool OnMenuPopup(HMENU hmenu);
		void OnMenuSelected(int Command);
	public:
		CEventInfoPopupHandler(CProgramGuide *pProgramGuide);
	};
	friend CEventInfoPopupHandler;
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	bool m_fShowToolTip;
	CTooltip m_Tooltip;
	CChannelList m_ChannelList;
	CTuningSpaceList m_TuningSpaceList;
	int m_CurrentTuningSpace;
	ServiceInfo m_CurrentChannel;
	bool m_fExcludeNoEventServices;
	ServiceInfoList m_ExcludeServiceList;
	TCHAR m_szDriverFileName[MAX_PATH];
	const CDriverManager *m_pDriverManager;

	int m_BeginHour;
	SYSTEMTIME m_stFirstTime;
	SYSTEMTIME m_stLastTime;
	SYSTEMTIME m_stCurTime;
	int m_Day;
	int m_Hours;

	struct {
		bool fValid;
		int ListIndex;
		int EventIndex;
	} m_CurItem;
	bool m_fUpdating;
	CEventHandler *m_pEventHandler;
	CFrame *m_pFrame;
	CProgramCustomizer *m_pProgramCustomizer;
	COLORREF m_ColorList[NUM_COLORS];
	Theme::GradientInfo m_ChannelNameBackGradient;
	Theme::GradientInfo m_CurChannelNameBackGradient;
	Theme::GradientInfo m_TimeBarMarginGradient;
	Theme::GradientInfo m_TimeBarBackGradient[TIME_BAR_BACK_COLORS];
	CProgramGuideToolList m_ToolList;
	int m_WheelScrollLines;
	unsigned int m_Filter;

	CProgramSearch m_ProgramSearch;
	class CProgramSearchEventHandler : public CProgramSearch::CEventHandler {
		CProgramGuide *m_pProgramGuide;
	public:
		CProgramSearchEventHandler(CProgramGuide *pProgramGuide);
		bool Search(LPCTSTR pszKeyword);
	};
	friend CProgramSearchEventHandler;
	CProgramSearchEventHandler m_ProgramSearchEventHandler;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

	bool UpdateList(bool fUpdateList=false);
	bool UpdateService(ProgramGuide::CServiceInfo *pService,bool fUpdateEpg);
	bool SetTuningSpace(int Space);
	void CalcLayout();
	void DrawEventList(const ProgramGuide::CEventLayout *pLayout,
					   HDC hdc,const RECT &Rect,const RECT &PaintRect) const;
	void DrawHeaderBackground(HDC hdc,const RECT &Rect,bool fCur) const;
	void DrawServiceHeader(ProgramGuide::CServiceInfo *pServiceInfo,
						   HDC hdc,const RECT &Rect,int Chevron,
						   bool fLeftAlign=false);
	void DrawDayHeader(int Day,HDC hdc,const RECT &Rect) const;
	void DrawTimeBar(HDC hdc,const RECT &Rect,bool fRight);
	void Draw(HDC hdc,const RECT &PaintRect);
	int GetCurTimeLinePos() const;
	void GetProgramGuideRect(RECT *pRect) const;
	void Scroll(int XOffset,int YOffset);
	void SetScrollBar();
	void SetCaption();
	void SetTooltip();
	bool EventHitTest(int x,int y,int *pListIndex,int *pEventIndex,RECT *pItemRect=NULL) const;
	void OnCommand(int id);

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

class CCalendarBar : public CBasicWindow
{
	bool m_fUseBufferedPaint;
	CBufferedPaint m_BufferedPaint;

public:
	CCalendarBar();
	~CCalendarBar();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void EnableBufferedPaint(bool fEnable) { m_fUseBufferedPaint=fEnable; }
	bool SetButtons(const SYSTEMTIME *pDateList,int Days,int FirstCommand);
	void SelectButton(int Button);
	void UnselectButton();
	void GetBarSize(SIZE *pSize);
	bool OnNotify(LPARAM lParam,LRESULT *pResult);
};

class ABSTRACT_CLASS(CProgramGuideFrameBase) : public CProgramGuide::CFrame
{
public:
	CProgramGuideFrameBase(CProgramGuide *pProgramGuide);
	virtual ~CProgramGuideFrameBase() = 0;
	void SetStatusTheme(const CStatusView::ThemeInfo *pTheme);

protected:
	CProgramGuide *m_pProgramGuide;
	CStatusView m_StatusView[2];
	CCalendarBar m_CalendarBar;
	RECT m_StatusMargin;
	int m_StatusItemMargin;

// CProgramGuide::CFrame
	void OnDateChanged() override;
	void OnSpaceChanged() override;
	void OnListModeChanged() override;
	void OnTimeRangeChanged() override;

	LRESULT DefaultMessageHandler(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void OnWindowCreate(HWND hwnd,bool fBufferedPaint);
	void OnWindowDestroy();
	void OnSizeChanged(int Width,int Height);
};

class CProgramGuideFrame : public CProgramGuideFrameBase, public CCustomWindow
{
public:
	static bool Initialize(HINSTANCE hinst);

	CProgramGuideFrame(CProgramGuide *pProgramGuide);
	~CProgramGuideFrame();
	CProgramGuide *GetProgramGuide() { return m_pProgramGuide; }
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CProgramGuide::CFrame
	bool SetAlwaysOnTop(bool fTop) override;
	bool GetAlwaysOnTop() const override { return m_fAlwaysOnTop; }

private:
	CAeroGlass m_AeroGlass;
	bool m_fAlwaysOnTop;

// CProgramGuide::CFrame
	void SetCaption(LPCTSTR pszCaption) override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;
};

class CProgramGuideDisplay : public CProgramGuideFrameBase, public CDisplayView
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		class CProgramGuideDisplay *m_pProgramGuideDisplay;
	public:
		CEventHandler();
		virtual ~CEventHandler() = 0;
		virtual bool OnHide() { return true; }
		virtual bool SetAlwaysOnTop(bool fTop) = 0;
		virtual bool GetAlwaysOnTop() const = 0;
		virtual void OnRButtonDown(int x,int y) {}
		virtual void OnLButtonDoubleClick(int x,int y) {}
		friend class CProgramGuideDisplay;
	};

	static bool Initialize(HINSTANCE hinst);

	CProgramGuideDisplay(CProgramGuide *pProgramGuide);
	~CProgramGuideDisplay();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CProgramGuideDisplay
	void SetEventHandler(CEventHandler *pHandler);

private:
	CEventHandler *m_pEventHandler;

	static const LPCTSTR m_pszWindowClass;
	static HINSTANCE m_hinst;

// CDisplayView
	bool OnVisibleChange(bool fVisible) override;
// CProgramGuide::CFrame
	bool SetAlwaysOnTop(bool fTop) override;
	bool GetAlwaysOnTop() const override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
