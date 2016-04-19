#include "stdafx.h"
#include "TVTest.h"
#include "Taskbar.h"
#include "AppMain.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#ifndef __ITaskbarList3_INTERFACE_DEFINED__
#define __ITaskbarList3_INTERFACE_DEFINED__

typedef enum THUMBBUTTONFLAGS {
	THBF_ENABLED		= 0,
	THBF_DISABLED		= 0x1,
	THBF_DISMISSONCLICK	= 0x2,
	THBF_NOBACKGROUND	= 0x4,
	THBF_HIDDEN			= 0x8,
	THBF_NONINTERACTIVE	= 0x10
} THUMBBUTTONFLAGS;
//DEFINE_ENUM_FLAG_OPERATORS(THUMBBUTTONFLAGS)
typedef enum THUMBBUTTONMASK {
	THB_BITMAP	= 0x1,
	THB_ICON	= 0x2,
	THB_TOOLTIP	= 0x4,
	THB_FLAGS	= 0x8
} THUMBBUTTONMASK;
//DEFINE_ENUM_FLAG_OPERATORS(THUMBBUTTONMASK)
#include <pshpack8.h>
typedef struct THUMBBUTTON {
	THUMBBUTTONMASK dwMask;
	UINT iId;
	UINT iBitmap;
	HICON hIcon;
	WCHAR szTip[260];
	THUMBBUTTONFLAGS dwFlags;
} THUMBBUTTON;
typedef struct THUMBBUTTON *LPTHUMBBUTTON;
#include <poppack.h>
#define THBN_CLICKED 0x1800
typedef enum TBPFLAG {
	TBPF_NOPROGRESS		= 0,
	TBPF_INDETERMINATE	= 0x1,
	TBPF_NORMAL			= 0x2,
	TBPF_ERROR			= 0x4,
	TBPF_PAUSED			= 0x8
} TBPFLAG;
//DEFINE_ENUM_FLAG_OPERATORS(TBPFLAG)

static const IID IID_ITaskbarList3 = {
	0xEA1AFB91, 0x9E28, 0x4B86, {0x90, 0xE9, 0x9E, 0x9F, 0x8A, 0x5E, 0xEF, 0xAF}
};

MIDL_INTERFACE("ea1afb91-9e28-4b86-90e9-9e9f8a5eefaf")
ITaskbarList3 : public ITaskbarList2
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetProgressValue(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ ULONGLONG ullCompleted,
		/* [in] */ ULONGLONG ullTotal) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetProgressState(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ TBPFLAG tbpFlags) = 0;
	virtual HRESULT STDMETHODCALLTYPE RegisterTab(
		/* [in] */ __RPC__in HWND hwndTab,
		/* [in] */ __RPC__in HWND hwndMDI) = 0;
	virtual HRESULT STDMETHODCALLTYPE UnregisterTab(
		/* [in] */ __RPC__in HWND hwndTab) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTabOrder(
		/* [in] */ __RPC__in HWND hwndTab,
		/* [in] */ __RPC__in HWND hwndInsertBefore) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetTabActive(
		/* [in] */ __RPC__in HWND hwndTab,
		/* [in] */ __RPC__in HWND hwndMDI,
		/* [in] */ DWORD dwReserved) = 0;
	virtual HRESULT STDMETHODCALLTYPE ThumbBarAddButtons(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ UINT cButtons,
		/* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton) = 0;
	virtual HRESULT STDMETHODCALLTYPE ThumbBarUpdateButtons(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ UINT cButtons,
		/* [size_is][in] */ __RPC__in_ecount_full(cButtons) LPTHUMBBUTTON pButton) = 0;
	virtual HRESULT STDMETHODCALLTYPE ThumbBarSetImageList(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ __RPC__in_opt HIMAGELIST himl) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetOverlayIcon(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ __RPC__in HICON hIcon,
		/* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszDescription) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetThumbnailTooltip(
		/* [in] */ __RPC__in HWND hwnd,
		/* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszTip) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetThumbnailClip(
		/* [in] */ __RPC__in HWND hwnd,
		/* [in] */ __RPC__in RECT *prcClip) = 0;
};

#endif	// ndef __ITaskbarList3_INTERFACE_DEFINED__




CTaskbarManager::CTaskbarManager()
	: m_hwnd(NULL)
	, m_TaskbarButtonCreatedMessage(0)
	, m_pTaskbarList(NULL)
{
}


CTaskbarManager::~CTaskbarManager()
{
	Finalize();
}


bool CTaskbarManager::Initialize(HWND hwnd)
{
	if (m_TaskbarButtonCreatedMessage==0) {
		OSVERSIONINFO osvi;
		osvi.dwOSVersionInfoSize=sizeof(osvi);
		::GetVersionEx(&osvi);
		if (osvi.dwMajorVersion<6
				|| (osvi.dwMajorVersion==6 && osvi.dwMinorVersion<1))
			return true;

		m_TaskbarButtonCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarButtonCreated"));

#ifndef MSGFLT_ADD
#define MSGFLT_ADD 1
#endif
		typedef BOOL (WINAPI *ChangeWindowMessageFilterFunc)(UINT,DWORD);
		HMODULE hLib=::GetModuleHandle(TEXT("user32.dll"));
		if (hLib!=NULL) {
			ChangeWindowMessageFilterFunc pChangeFilter=
				reinterpret_cast<ChangeWindowMessageFilterFunc>(::GetProcAddress(hLib,"ChangeWindowMessageFilter"));
			if (pChangeFilter!=NULL)
				pChangeFilter(m_TaskbarButtonCreatedMessage,MSGFLT_ADD);
		}

		m_hwnd=hwnd;
	}
	return true;
}


void CTaskbarManager::Finalize()
{
	if (m_pTaskbarList!=NULL) {
		m_pTaskbarList->Release();
		m_pTaskbarList=NULL;
	}
	m_hwnd=NULL;
}


bool CTaskbarManager::HandleMessage(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	if (uMsg!=0 && uMsg==m_TaskbarButtonCreatedMessage) {
		if (m_pTaskbarList==NULL) {
			if (FAILED(::CoCreateInstance(CLSID_TaskbarList,NULL,
										  CLSCTX_INPROC_SERVER,
										  IID_ITaskbarList3,
										  reinterpret_cast<void**>(&m_pTaskbarList))))
				return true;
		}
		HINSTANCE hinst=GetAppClass().GetResourceInstance();
		HIMAGELIST himl=::ImageList_LoadBitmap(hinst,
											   MAKEINTRESOURCE(IDB_THUMBBAR),
											   16,1,RGB(192,192,192));
		if (himl!=NULL) {
			m_pTaskbarList->ThumbBarSetImageList(m_hwnd,himl);
			THUMBBUTTON tb[3];
			tb[0].iId=CM_FULLSCREEN;
			tb[1].iId=CM_DISABLEVIEWER;
			tb[2].iId=CM_PROGRAMGUIDE;
			for (int i=0;i<lengthof(tb);i++) {
				tb[i].dwMask=(THUMBBUTTONMASK)(THB_BITMAP | THB_TOOLTIP | THB_FLAGS);
				tb[i].iBitmap=i;
				::LoadStringW(hinst,tb[i].iId,tb[i].szTip,lengthof(tb[0].szTip));
				tb[i].dwFlags=(THUMBBUTTONFLAGS)(THBF_ENABLED | THBF_DISMISSONCLICK);
			}
			m_pTaskbarList->ThumbBarAddButtons(m_hwnd,lengthof(tb),tb);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::SetRecordingStatus(bool fRecording)
{
	if (m_pTaskbarList!=NULL) {
		if (fRecording) {
			HICON hico=static_cast<HICON>(::LoadImage(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDI_TASKBAR_RECORDING),IMAGE_ICON,16,16,LR_DEFAULTCOLOR));

			if (hico==NULL)
				return false;
			m_pTaskbarList->SetOverlayIcon(m_hwnd,hico,TEXT("˜^‰æ’†"));
			::DestroyIcon(hico);
		} else {
			m_pTaskbarList->SetOverlayIcon(m_hwnd,NULL,NULL);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::SetProgress(int Pos,int Max)
{
	if (m_pTaskbarList!=NULL) {
		if (Pos>=Max) {
			m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NOPROGRESS);
		} else {
			if (Pos==0)
				m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hwnd,Pos,Max);
		}
		return true;
	}
	return false;
}


bool CTaskbarManager::EndProgress()
{
	if (m_pTaskbarList!=NULL)
		m_pTaskbarList->SetProgressState(m_hwnd,TBPF_NOPROGRESS);
	return true;
}
