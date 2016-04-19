/*
	TVTest �v���O�C���T���v��

	�p�P�b�g�𐔂��邾��
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"


#define PACKET_COUNTER_WINDOW_CLASS TEXT("Packet Counter Window")




// �v���O�C���N���X
class CPacketCounter : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	HFONT m_hfont;
	ULONGLONG m_PacketCount;
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static BOOL CALLBACK StreamCallback(BYTE *pData,void *pClientData);
	static CPacketCounter *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CPacketCounter()
	{
		m_hwnd=NULL;
		m_hfont=NULL;
		m_PacketCount=0;
	}
	~CPacketCounter()
	{
		if (m_hfont)
			::DeleteObject(m_hfont);
	}
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


bool CPacketCounter::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Packet Counter";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�p�P�b�g�𐔂���";
	return true;
}


bool CPacketCounter::Initialize()
{
	// ����������

	WNDCLASS wc;
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=g_hinstDLL;
	wc.hIcon=NULL;
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=PACKET_COUNTER_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	LOGFONT lf;
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hfont=::CreateFontIndirect(&lf);

	RECT rc;
	::SetRect(&rc,0,0,160,abs(lf.lfHeight)+4);
	::AdjustWindowRectEx(&rc,WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
						 FALSE,WS_EX_TOOLWINDOW);

	if (::CreateWindowEx(WS_EX_TOOLWINDOW,PACKET_COUNTER_WINDOW_CLASS,
							TEXT("Packet Counter"),
							WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
							0,0,rc.right-rc.left,rc.bottom-rc.top,
							m_pApp->GetAppWindow(),NULL,g_hinstDLL,this)==NULL)
		return false;

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	// �X�g���[���R�[���o�b�N�֐���o�^
	m_pApp->SetStreamCallback(0,StreamCallback,this);

	return true;
}


bool CPacketCounter::Finalize()
{
	// �I������

	::DestroyWindow(m_hwnd);

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CPacketCounter::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CPacketCounter *pThis=static_cast<CPacketCounter*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		::ShowWindow(pThis->m_hwnd,lParam1!=0?SW_SHOW:SW_HIDE);
		return TRUE;
	}
	return 0;
}


// �X�g���[���R�[���o�b�N�֐�
// 188�o�C�g�̃p�P�b�g�f�[�^���n�����
BOOL CALLBACK CPacketCounter::StreamCallback(BYTE *pData,void *pClientData)
{
	CPacketCounter *pThis=static_cast<CPacketCounter*>(pClientData);

	pThis->m_PacketCount++;
	return TRUE;	// FALSE��Ԃ��ƃp�P�b�g���j�������
}


CPacketCounter *CPacketCounter::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPacketCounter*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPacketCounter::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CPacketCounter *pThis=static_cast<CPacketCounter*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;

			// �\���X�V�p�^�C�}�̐ݒ�
			::SetTimer(hwnd,1,500,NULL);
		}
		return TRUE;

	case WM_PAINT:
		{
			CPacketCounter *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HFONT hfontOld;
			int OldBkMode;
			COLORREF crOldTextColor;
			TCHAR szText[32];
			RECT rc;

			::BeginPaint(hwnd,&ps);
			hfontOld=static_cast<HFONT>(::SelectObject(ps.hdc,pThis->m_hfont));
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			crOldTextColor=::SetTextColor(ps.hdc,::GetSysColor(COLOR_WINDOWTEXT));
			_ui64tot_s(pThis->m_PacketCount,szText,32,10);
			::GetClientRect(hwnd,&rc);
			rc.right-=2;
			::DrawText(ps.hdc,szText,-1,&rc,DT_RIGHT | DT_SINGLELINE | DT_VCENTER);
			::SelectObject(ps.hdc,hfontOld);
			::SetBkMode(ps.hdc,OldBkMode);
			::SetTextColor(ps.hdc,crOldTextColor);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		// �\���X�V
		::InvalidateRect(hwnd,NULL,TRUE);
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam&0xFFF0)==SC_CLOSE) {
			// ���鎞�̓v���O�C���𖳌��ɂ���
			CPacketCounter *pThis=GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			CPacketCounter *pThis=GetThis(hwnd);

			::KillTimer(hwnd,1);	// �ʂɂ��Ȃ��Ă���������...
			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CPacketCounter;
}
