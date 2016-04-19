#ifndef TVTEST_VIEW_H
#define TVTEST_VIEW_H


#include "BasicWindow.h"
#include "DtvEngine.h"
#include "Theme.h"


class ABSTRACT_CLASS(CDisplayView) : public CCustomWindow
{
	friend class CDisplayBase;

	void SetDisplayVisible(bool fVisible);

protected:
	class CDisplayBase *m_pDisplayBase;

	virtual bool OnVisibleChange(bool fVisible);
	virtual bool GetCloseButtonRect(RECT *pRect) const;
	bool CloseButtonHitTest(int x,int y) const;
	void DrawCloseButton(HDC hdc) const;

public:
	CDisplayView();
	virtual ~CDisplayView()=0;
// CBasicWindow
	void SetVisible(bool fVisible) override;
};

class CDisplayBase
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	public:
		virtual ~CEventHandler() = 0;
		virtual bool OnVisibleChange(bool fVisible) { return true; }
	};

	CDisplayBase();
	~CDisplayBase();
	void SetEventHandler(CEventHandler *pHandler);
	void SetParent(CBasicWindow *pWindow);
	CBasicWindow *GetParent() const { return m_pParentWindow; }
	void SetDisplayView(CDisplayView *pView);
	CDisplayView *GetDisplayView() const { return m_pDisplayView; }
	bool SetVisible(bool fVisible);
	bool IsVisible() const;
	void AdjustPosition();
	void SetPosition(int Left,int Top,int Width,int Height);
	void SetPosition(const RECT *pRect);
	void SetFocus();

private:
	CBasicWindow *m_pParentWindow;
	CDisplayView *m_pDisplayView;
	CEventHandler *m_pEventHandler;
	bool m_fVisible;
};

class CVideoContainerWindow : public CCustomWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CVideoContainerWindow *m_pVideoContainer;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnSizeChanged(int Width,int Height) {}
		friend class CVideoContainerWindow;
	};

	CVideoContainerWindow();
	~CVideoContainerWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID,CDtvEngine *pDtvEngine);
	CDtvEngine *GetDtvEngine() { return m_pDtvEngine; }
	const CDtvEngine *GetDtvEngine() const { return m_pDtvEngine; }
	void SetDisplayBase(CDisplayBase *pDisplayBase);
	void SetEventHandler(CEventHandler *pEventHandler);

	static bool Initialize(HINSTANCE hinst);

private:
	static HINSTANCE m_hinst;

	CDtvEngine *m_pDtvEngine;
	CDisplayBase *m_pDisplayBase;
	CEventHandler *m_pEventHandler;
	SIZE m_ClientSize;

// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};

class CViewWindow : public CCustomWindow
{
public:
	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CViewWindow *m_pView;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnSizeChanged(int Width,int Height) {}
		friend class CViewWindow;
	};

	CViewWindow();
	~CViewWindow();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0) override;
// CViewWindow
	void SetVideoContainer(CVideoContainerWindow *pVideoContainer);
	void SetMessageWindow(HWND hwnd);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool SetLogo(HBITMAP hbm);
	void SetBorder(const Theme::BorderInfo *pInfo);
	void ShowCursor(bool fShow);
	bool CalcClientRect(RECT *pRect) const;
	bool CalcWindowRect(RECT *pRect) const;

	static bool Initialize(HINSTANCE hinst);

private:
	static HINSTANCE m_hinst;

	CVideoContainerWindow *m_pVideoContainer;
	HWND m_hwndMessage;
	CEventHandler *m_pEventHandler;
	HBITMAP m_hbmLogo;
	Theme::BorderInfo m_BorderInfo;
	bool m_fShowCursor;

// CCustomWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
