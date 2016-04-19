/*
	TVTest �v���O�C���T���v��

	�R�}���h��o�^����
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"


// �v���O�C���N���X
class CCommandSample : public TVTest::CTVTestPlugin
{
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
public:
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


bool CCommandSample::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Command Sample";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�R�}���h��o�^����T���v��";
	return true;
}


bool CCommandSample::Initialize()
{
	// ����������

	// �R�}���h��o�^

	// ����o�^�����
	m_pApp->RegisterCommand(1, L"Command1", L"�R�}���h1");
	m_pApp->RegisterCommand(2, L"Command2", L"�R�}���h2");

	// �܂Ƃ߂ēo�^�����
	static const TVTest::CommandInfo CommandList[] = {
		{3,	L"Command3",	L"�R�}���h3"},
		{4,	L"Command4",	L"�R�}���h4"},
	};
	m_pApp->RegisterCommand(CommandList, sizeof(CommandList) / sizeof(TVTest::CommandInfo));

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback, this);

	return true;
}


bool CCommandSample::Finalize()
{
	// �I������

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CCommandSample::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CCommandSample *pThis = static_cast<CCommandSample*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_COMMAND:
		{
			TCHAR szText[256];

			::wsprintf(szText, TEXT("�R�}���h%d���I������܂����B"),(int)lParam1);
			::MessageBox(pThis->m_pApp->GetAppWindow(), szText, TEXT("�R�}���h"), MB_OK);
		}
		return TRUE;
	}
	return 0;
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CCommandSample;
}
