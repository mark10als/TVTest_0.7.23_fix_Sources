#ifndef EPG_OPTIONS_H
#define EPG_OPTIONS_H


#include "CoreEngine.h"
#include "EpgProgramList.h"
#include "Options.h"
#include "LogoManager.h"
#include "EpgDataLoader.h"


class CEpgOptions : public COptions
{
public:
	class CEpgFileLoadEventHandler {
	public:
		virtual ~CEpgFileLoadEventHandler() {}
		virtual void OnBeginLoad() {}
		virtual void OnEndLoad(bool fSuccess) {}
	};
	class CEDCBDataLoadEventHandler {
	public:
		virtual ~CEDCBDataLoadEventHandler() {}
		virtual void OnStart() {}
		virtual void OnEnd(bool fSuccess,CEventManager *pEventManager) {}
	};

private:
	bool m_fSaveEpgFile;
	TCHAR m_szEpgFileName[MAX_PATH];
	bool m_fUpdateWhenStandby;
	bool m_fUseEDCBData;
	TCHAR m_szEDCBDataFolder[MAX_PATH];
	bool m_fSaveLogoFile;
	TCHAR m_szLogoFileName[MAX_PATH];
	CCoreEngine *m_pCoreEngine;
	CLogoManager *m_pLogoManager;
	HANDLE m_hLoadThread;
	CEpgDataLoader *m_pEpgDataLoader;

	class CEpgDataLoaderEventHandler : public CEpgDataLoader::CEventHandler {
		CEDCBDataLoadEventHandler *m_pEventHandler;
		bool m_fLoading;
	public:
		CEpgDataLoaderEventHandler()
			: m_pEventHandler(NULL)
			, m_fLoading(false) {
		}
		void SetEventHandler(CEDCBDataLoadEventHandler *pHandler) {
			m_pEventHandler=pHandler;
		}
		void OnStart() {
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnStart();
			m_fLoading=true;
		}
		void OnEnd(bool fSuccess,CEventManager *pEventManager) {
			m_fLoading=false;
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnEnd(fSuccess,pEventManager);
		}
		bool IsLoading() const { return m_fLoading; }
	};
	CEpgDataLoaderEventHandler m_EpgDataLoaderEventHandler;

	LOGFONT m_EventInfoFont;
	LOGFONT m_CurEventInfoFont;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	bool GetEpgFileFullPath(LPTSTR pszFileName);
	static DWORD WINAPI LoadThread(LPVOID lpParameter);

public:
	CEpgOptions(CCoreEngine *pCoreEngine,CLogoManager *pLogoManager);
	~CEpgOptions();
// CSettingsBase
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CEpgOptions
	void Finalize();
	bool LoadEpgFile(CEpgProgramList *pEpgList);
	bool AsyncLoadEpgFile(CEpgProgramList *pEpgList,CEpgFileLoadEventHandler *pEventHandler=NULL);
	bool SaveEpgFile(CEpgProgramList *pEpgList);
	LPCTSTR GetEpgFileName() const { return m_szEpgFileName; }
	bool GetUpdateWhenStandby() const { return m_fUpdateWhenStandby; }
	bool LoadEDCBData();
	void SetEDCBDataLoadEventHandler(CEDCBDataLoadEventHandler *pEventHandler);
	bool AsyncLoadEDCBData();
	bool IsEDCBDataLoading() const;
	bool LoadLogoFile();
	bool SaveLogoFile();
	const LOGFONT *GetEventInfoFont() const { return &m_EventInfoFont; }
};


#endif
