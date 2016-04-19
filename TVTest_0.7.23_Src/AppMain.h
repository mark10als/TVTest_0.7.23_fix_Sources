#ifndef APP_MAIN_H
#define APP_MAIN_H


#include "CoreEngine.h"
#include "UICore.h"
#include "ChannelManager.h"
#include "Record.h"
#include "Settings.h"


class CCommandList;
class CDriverManager;
class CLogoManager;
class CControllerManager;
class CEpgProgramList;

namespace TVTest
{
	class CFavoritesManager;
}

class CAppMain
{
	CUICore m_UICore;
	bool m_fSilent;
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szDefaultChannelFileName[MAX_PATH];
	TCHAR m_szChannelSettingFileName[MAX_PATH];
	TCHAR m_szFavoritesFileName[MAX_PATH];
	CSettings m_Settings;
	bool m_fFirstExecute;

	bool SetService(int Service);
	bool GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName) const;

	// ÉRÉsÅ[ã÷é~
	CAppMain(const CAppMain &);
	CAppMain &operator=(const CAppMain &);

public:
	CAppMain();
	bool Initialize();
	bool Finalize();
	HINSTANCE GetInstance() const;
	HINSTANCE GetResourceInstance() const;
	bool GetAppDirectory(LPTSTR pszDirectory) const;
	bool GetDriverDirectory(LPTSTR pszDirectory) const;
	LPCTSTR GetIniFileName() const { return m_szIniFileName; }
	LPCTSTR GetFavoritesFileName() const { return m_szFavoritesFileName; }
	void AddLog(LPCTSTR pszText, ...);
	void OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle=NULL);
	void SetSilent(bool fSilent);
	bool IsSilent() const { return m_fSilent; }
	bool LoadSettings();
	enum {
		SETTINGS_SAVE_STATUS	=0x0001,
		SETTINGS_SAVE_OPTIONS	=0x0002,
		SETTINGS_SAVE_ALL		=SETTINGS_SAVE_STATUS | SETTINGS_SAVE_OPTIONS
	};
	bool SaveSettings(unsigned int Flags);
	void InitializeCommandSettings();
	bool SaveCurrentChannel();
	bool IsFirstExecute() const;

	bool SaveChannelSettings();
	bool InitializeChannel();
	bool GetChannelFileName(LPCTSTR pszDriverFileName,
							LPTSTR pszChannelFileName,int MaxChannelFileName);
	bool RestoreChannel();
	bool UpdateChannelList(const CTuningSpaceList *pList);
	const CChannelInfo *GetCurrentChannelInfo() const;
	bool SetChannel(int Space,int Channel,int ServiceID=-1);
	bool FollowChannelChange(WORD TransportStreamID,WORD ServiceID);
	bool SetServiceByIndex(int Service);
	bool SetServiceByID(WORD ServiceID,int *pServiceIndex=NULL);
	bool SetDriver(LPCTSTR pszFileName);
	bool SetDriverAndChannel(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo);
	bool OpenTuner();
	bool CloseTuner();
	bool OpenBcasCard(bool fRetry=false);

	bool StartRecord(LPCTSTR pszFileName=NULL,
					 const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					 const CRecordManager::TimeSpecInfo *pStopTime=NULL,
					 CRecordManager::RecordClient Client=CRecordManager::CLIENT_USER,
					 bool fTimeShift=false);
	bool ModifyRecord(LPCTSTR pszFileName=NULL,
					  const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					  const CRecordManager::TimeSpecInfo *pStopTime=NULL,
					  CRecordManager::RecordClient Client=CRecordManager::CLIENT_USER);
	bool StartReservedRecord();
	bool CancelReservedRecord();
	bool StopRecord();
	bool RelayRecord(LPCTSTR pszFileName);
	LPCTSTR GetDefaultRecordFolder() const;

	bool ShowHelpContent(int ID);
	void BeginChannelScan(int Space);
	bool IsChannelScanning() const;
	bool IsDriverNoSignalLevel(LPCTSTR pszFileName) const;
	void SetProgress(int Pos,int Max);
	void EndProgress();
	COLORREF GetColor(LPCTSTR pszText) const;

	CCoreEngine *GetCoreEngine();
	const CCoreEngine *GetCoreEngine() const;
	CUICore *GetUICore();
	const CCommandList *GetCommandList() const;
	const CChannelManager *GetChannelManager() const;
	const CRecordManager *GetRecordManager() const;
	const CDriverManager *GetDriverManager() const;
	CEpgProgramList *GetEpgProgramList() const;
	CLogoManager *GetLogoManager() const;
	CControllerManager *GetControllerManager() const;
	TVTest::CFavoritesManager *GetFavoritesManager() const;
};


CAppMain &GetAppClass();


#endif
