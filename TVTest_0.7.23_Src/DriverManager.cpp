#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "DriverManager.h"
#include "IBonDriver2.h"
#include <algorithm>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


typedef IBonDriver *(*CreateBonDriverFunc)();




CDriverInfo::CDriverInfo(LPCTSTR pszFileName)
	: m_FileName(pszFileName)
	, m_fChannelFileLoaded(false)
	, m_fDriverSpaceLoaded(false)
{
}


CDriverInfo::~CDriverInfo()
{
}


bool CDriverInfo::LoadTuningSpaceList(LoadTuningSpaceListMode Mode)
{
	CAppMain &App=GetAppClass();
	LPCTSTR pszFileName=m_FileName.Get();

	bool fUseDriver;
	if (Mode==LOADTUNINGSPACE_NOLOADDRIVER) {
		fUseDriver=false;
	} else if (Mode==LOADTUNINGSPACE_USEDRIVER) {
		fUseDriver=true;
	} else {
		// チューナを開かずにチューニング空間とチャンネルを取得できない
		// ドライバはロードしないようにする
		fUseDriver=!::PathMatchSpec(pszFileName,TEXT("BonDriver_Spinel*.dll"))
				&& !::PathMatchSpec(pszFileName,TEXT("BonDriver_Friio*.dll"));
	}

	if (!m_fChannelFileLoaded) {
		TCHAR szChannelFileName[MAX_PATH];

		App.GetChannelFileName(pszFileName,szChannelFileName,lengthof(szChannelFileName));
		if (m_TuningSpaceList.LoadFromFile(szChannelFileName)) {
#if 0
			if (fUseDriver && Mode==LOADTUNINGSPACE_DEFAULT) {
				const int NumSpaces=m_TuningSpaceList.NumSpaces();
				int i;
				for (i=0;i<NumSpaces;i++) {
					if (m_TuningSpaceList.GetTuningSpaceName(i)==NULL
							|| m_TuningSpaceList.GetChannelList(i)->NumChannels()==0)
						break;
				}
				if (i==NumSpaces)
					fUseDriver=false;
			}
#else
			if (Mode==LOADTUNINGSPACE_DEFAULT)
				fUseDriver=false;
#endif
			m_fChannelFileLoaded=true;
		} else {
			if (!fUseDriver && !m_fDriverSpaceLoaded)
				return false;
		}
	}

	if (fUseDriver && !m_fDriverSpaceLoaded) {
		TCHAR szFilePath[MAX_PATH];

		if (::PathIsRelative(pszFileName)) {
			TCHAR szTemp[MAX_PATH];
			GetAppClass().GetDriverDirectory(szTemp);
			::PathAppend(szTemp,pszFileName);
			::PathCanonicalize(szFilePath,szTemp);
		} else {
			::lstrcpy(szFilePath,pszFileName);
		}

		HMODULE hLib=::GetModuleHandle(szFilePath);
		if (hLib!=NULL) {
			TCHAR szCurDriverPath[MAX_PATH];

			if (App.GetCoreEngine()->GetDriverPath(szCurDriverPath)
					&& ::lstrcmpi(szFilePath,szCurDriverPath)==0) {
				m_DriverSpaceList=*App.GetChannelManager()->GetDriverTuningSpaceList();
				m_fDriverSpaceLoaded=true;
			}
		} else if ((hLib=::LoadLibrary(szFilePath))!=NULL) {
			CreateBonDriverFunc pCreate=
				reinterpret_cast<CreateBonDriverFunc>(::GetProcAddress(hLib,"CreateBonDriver"));
			IBonDriver *pBonDriver;

			if (pCreate!=NULL && (pBonDriver=pCreate())!=NULL) {
				IBonDriver2 *pBonDriver2=dynamic_cast<IBonDriver2*>(pBonDriver);

				if (pBonDriver2!=NULL) {
					int NumSpaces;

					for (NumSpaces=0;pBonDriver2->EnumTuningSpace(NumSpaces)!=NULL;NumSpaces++);
					m_DriverSpaceList.Reserve(NumSpaces);
					m_TunerName.Set(pBonDriver2->GetTunerName());
					for (int i=0;i<NumSpaces;i++) {
						CTuningSpaceInfo *pTuningSpaceInfo=m_DriverSpaceList.GetTuningSpaceInfo(i);
						LPCTSTR pszName=pBonDriver2->EnumTuningSpace(i);

						pTuningSpaceInfo->SetName(pszName);
						CChannelList *pChannelList=pTuningSpaceInfo->GetChannelList();
						for (int j=0;(pszName=pBonDriver2->EnumChannelName(i,j))!=NULL;j++) {
							pChannelList->AddChannel(CChannelInfo(i,j,j+1,pszName));
						}
					}
					m_fDriverSpaceLoaded=true;
				}
				pBonDriver->Release();
			}
			::FreeLibrary(hLib);
		}
		for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
			if (m_TuningSpaceList.GetTuningSpaceName(i)==NULL)
				m_TuningSpaceList.GetTuningSpaceInfo(i)->SetName(m_DriverSpaceList.GetTuningSpaceName(i));
		}
	}
	if (!m_fChannelFileLoaded && !m_fDriverSpaceLoaded)
		return false;
	return true;
}


const CTuningSpaceList *CDriverInfo::GetAvailableTuningSpaceList() const
{
	if (m_fChannelFileLoaded)
		return &m_TuningSpaceList;
	if (m_fDriverSpaceLoaded)
		return &m_DriverSpaceList;
	return NULL;
}


const CChannelList *CDriverInfo::GetChannelList(int Space) const
{
	const CChannelList *pChannelList;

	pChannelList=m_TuningSpaceList.GetChannelList(Space);
	if (pChannelList==NULL) {
		pChannelList=m_DriverSpaceList.GetChannelList(Space);
	} else if (pChannelList->NumChannels()==0) {
		const CChannelList *pDriverChannelList=m_DriverSpaceList.GetChannelList(Space);

		if (pDriverChannelList!=NULL && pDriverChannelList->NumChannels()>0)
			pChannelList=pDriverChannelList;
	}
	return pChannelList;
}




CDriverManager::CDriverManager()
{
}


CDriverManager::~CDriverManager()
{
	Clear();
}


void CDriverManager::Clear()
{
	for (size_t i=0;i<m_DriverList.size();i++)
		delete m_DriverList[i];
	m_DriverList.clear();
}


bool CDriverManager::CompareDriverFileName(const CDriverInfo *pDriver1,const CDriverInfo *pDriver2)
{
	return ::lstrcmpi(pDriver1->GetFileName(),pDriver2->GetFileName())<0;
}


bool CDriverManager::Find(LPCTSTR pszDirectory)
{
	if (pszDirectory==NULL)
		return false;

	TCHAR szMask[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	Clear();
	::PathCombine(szMask,pszDirectory,TEXT("BonDriver*.dll"));
	hFind=::FindFirstFile(szMask,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			if ((wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
				m_DriverList.push_back(new CDriverInfo(wfd.cFileName));
			}
		} while (::FindNextFile(hFind,&wfd));
		::FindClose(hFind);
	}
	if (m_DriverList.size()>1)
		std::sort(m_DriverList.begin(),m_DriverList.end(),CompareDriverFileName);
	m_BaseDirectory.Set(pszDirectory);
	return true;
}


CDriverInfo *CDriverManager::GetDriverInfo(int Index)
{
	if (Index<0 || (size_t)Index>=m_DriverList.size())
		return NULL;
	return m_DriverList[Index];
}


const CDriverInfo *CDriverManager::GetDriverInfo(int Index) const
{
	if (Index<0 || (size_t)Index>=m_DriverList.size())
		return NULL;
	return m_DriverList[Index];
}
