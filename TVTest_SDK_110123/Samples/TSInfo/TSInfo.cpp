/*
	TVTest �v���O�C���T���v��

	�X�g���[���̊e�����\������
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"


// �v���O�C���N���X
class CTSInfo : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	HBRUSH m_hbrBack;
	COLORREF m_crTextColor;

	void SetItemText(int ID,LPCTSTR pszText);
	void UpdateItems();

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CTSInfo *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CTSInfo()
		: m_hwnd(NULL)
		, m_hbrBack(NULL)
	{
	}
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


// �v���O�C���̏���Ԃ�
bool CTSInfo::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"�`�����l���̏��";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�`�����l���̏���\�����܂��B";
	return true;
}


// ����������
bool CTSInfo::Initialize()
{
	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


// �I������
bool CTSInfo::Finalize()
{
	// �E�B���h�E�̔j��
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CTSInfo::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CTSInfo *pThis=static_cast<CTSInfo*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		{
			bool fEnable=lParam1!=0;

			if (fEnable) {
				if (pThis->m_hwnd==NULL) {
					if (::CreateDialogParam(g_hinstDLL,MAKEINTRESOURCE(IDD_MAIN),
											pThis->m_pApp->GetAppWindow(),DlgProc,
											reinterpret_cast<LPARAM>(pThis))==NULL)
						return FALSE;
				}
				pThis->UpdateItems();
			}
			::ShowWindow(pThis->m_hwnd,fEnable?SW_SHOW:SW_HIDE);
			if (fEnable)
				::SetTimer(pThis->m_hwnd,1,1000,NULL);
			else
				::KillTimer(pThis->m_hwnd,1);
		}
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// �F�̐ݒ肪�ω�����
		if (pThis->m_hwnd!=NULL) {
			HBRUSH hbrBack=::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));

			if (hbrBack!=NULL) {
				if (pThis->m_hbrBack!=NULL)
					::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack=hbrBack;
			}
			pThis->m_crTextColor=pThis->m_pApp->GetColor(L"PanelText");
			::RedrawWindow(pThis->m_hwnd,NULL,NULL,RDW_INVALIDATE | RDW_UPDATENOW);
		}
		return TRUE;
	}
	return 0;
}


// ���ڂ̕������ݒ肷��
void CTSInfo::SetItemText(int ID,LPCTSTR pszText)
{
	TCHAR szCurText[256];

	// �I�������������̂Ƃ�����h�~�̂��߂ɁA�ω������������̂ݐݒ肷��
	::GetDlgItemText(m_hwnd,ID,szCurText,256);
	if (::lstrcmp(szCurText,pszText)!=0)
		::SetDlgItemText(m_hwnd,ID,pszText);
}


// ���ڂ��X�V����
void CTSInfo::UpdateItems()
{
	TVTest::ChannelInfo ChannelInfo;
	int CurService,NumServices;
	TVTest::ServiceInfo ServiceInfo;
	TCHAR szText[256];

	if (m_pApp->GetCurrentChannelInfo(&ChannelInfo)) {
		TCHAR szSpaceName[32];

		if (m_pApp->GetTuningSpaceName(ChannelInfo.Space,szSpaceName,32)==0)
			::lstrcpy(szSpaceName,TEXT("???"));
		::wsprintf(szText,TEXT("%d (%s)"),ChannelInfo.Space,szSpaceName);
		SetItemText(IDC_SPACE,szText);
		::wsprintf(szText,TEXT("%d (%s)"),ChannelInfo.Channel,ChannelInfo.szChannelName);
		SetItemText(IDC_CHANNEL,szText);
		::wsprintf(szText,TEXT("%#x"),ChannelInfo.NetworkID);
		SetItemText(IDC_NETWORKID,szText);
		SetItemText(IDC_NETWORKNAME,ChannelInfo.szNetworkName);
		::wsprintf(szText,TEXT("%#x"),ChannelInfo.TransportStreamID);
		SetItemText(IDC_TRANSPORTSTREAMID,szText);
		SetItemText(IDC_TRANSPORTSTREAMNAME,ChannelInfo.szTransportStreamName);
		if (ChannelInfo.RemoteControlKeyID!=0)
			::wsprintf(szText,TEXT("%d"),ChannelInfo.RemoteControlKeyID);
		else
			szText[0]='\0';
		SetItemText(IDC_REMOTECONTROLKEYID,szText);
	}
	CurService=m_pApp->GetService(&NumServices);
	if (CurService>=0
			&& m_pApp->GetServiceInfo(CurService,&ServiceInfo)) {
		::wsprintf(szText,TEXT("%d / %d"),CurService+1,NumServices);
		SetItemText(IDC_SERVICE,szText);
		::wsprintf(szText,TEXT("%#x"),ServiceInfo.ServiceID);
		SetItemText(IDC_SERVICEID,szText);
		SetItemText(IDC_SERVICENAME,ServiceInfo.szServiceName);
		::wsprintf(szText,TEXT("%#x"),ServiceInfo.VideoPID);
		SetItemText(IDC_VIDEOPID,szText);
		::wsprintf(szText,TEXT("%#x"),ServiceInfo.AudioPID[0]);
		if (ServiceInfo.NumAudioPIDs>1) {
			::wsprintf(szText+::lstrlen(szText),TEXT(" / %#x"),ServiceInfo.AudioPID[1]);
		}
		SetItemText(IDC_AUDIOPID,szText);
		if (ServiceInfo.SubtitlePID!=0)
			::wsprintf(szText,TEXT("%#x"),ServiceInfo.SubtitlePID);
		else
			::lstrcpy(szText,TEXT("<none>"));
		SetItemText(IDC_SUBTITLEPID,szText);
	}
}


// �_�C�A���O�̃n���h������this���擾����
CTSInfo *CTSInfo::GetThis(HWND hDlg)
{
	return static_cast<CTSInfo*>(::GetProp(hDlg,TEXT("This")));
}


// �_�C�A���O�v���V�[�W��
INT_PTR CALLBACK CTSInfo::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CTSInfo *pThis=reinterpret_cast<CTSInfo*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hwnd=hDlg;
			pThis->m_hbrBack=::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));
			pThis->m_crTextColor=pThis->m_pApp->GetColor(L"PanelText");
		}
		return TRUE;

	case WM_TIMER:
		{
			// ���X�V
			CTSInfo *pThis=GetThis(hDlg);

			pThis->UpdateItems();
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		// ���ڂ̔w�i�F��ݒ�
		{
			CTSInfo *pThis=GetThis(hDlg);
			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetBkMode(hdc,TRANSPARENT);
			::SetTextColor(hdc,pThis->m_crTextColor);
			return reinterpret_cast<INT_PTR>(pThis->m_hbrBack);
		}

	case WM_CTLCOLORDLG:
		// �_�C�A���O�̔w�i�F��ݒ�
		{
			CTSInfo *pThis=GetThis(hDlg);

			return reinterpret_cast<INT_PTR>(pThis->m_hbrBack);
		}

	case WM_COMMAND:
		if (LOWORD(wParam)==IDCANCEL) {
			// ���鎞�̓v���O�C���𖳌��ɂ���
			CTSInfo *pThis=GetThis(hDlg);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CTSInfo *pThis=GetThis(hDlg);

			::KillTimer(hDlg,1);
			if (pThis->m_hbrBack!=NULL) {
				::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack=NULL;
			}
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}




// �v���O�C���N���X�̃C���X�^���X�𐶐�����
TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CTSInfo;
}
