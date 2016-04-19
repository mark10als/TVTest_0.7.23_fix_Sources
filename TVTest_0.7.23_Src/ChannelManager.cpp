#include "stdafx.h"
#include "TVTest.h"
#include "ChannelManager.h"
#include "Settings.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CChannelManager::CChannelManager()
{
	Reset();
}


CChannelManager::~CChannelManager()
{
}


void CChannelManager::Reset()
{
	m_CurrentSpace=SPACE_INVALID;
	m_CurrentChannel=-1;
	m_CurrentServiceID=-1;
	m_ChangingChannel=-1;

	m_TuningSpaceList.Clear();
	m_DriverTuningSpaceList.Clear();
	m_fUseDriverChannelList=false;
	m_fChannelFileHasStreamIDs=false;
	m_ChannelFileName.clear();

#ifdef NETWORK_REMOCON_SUPPORT
	m_fNetworkRemocon=false;
	m_pNetworkRemoconChannelList=NULL;
	m_NetworkRemoconCurrentChannel=-1;
#endif
}


static void SkipSpaces(char **ppText)
{
	char *p=*ppText;

	while (*p==' ' || *p=='\t')
		p++;
	*ppText=p;
}


bool CChannelManager::LoadOldChannelFile(LPCTSTR pszFileName)
{
	// 古い形式のチャンネル設定ファイル
	HANDLE hFile;
	DWORD Size,Read;
	char *pszText,*p,*q;
	CChannelList ChannelList;
	int NumSpaces;

	hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
														OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	Size=::GetFileSize(hFile,NULL);
	if (Size==0 || Size==INVALID_FILE_SIZE) {
		::CloseHandle(hFile);
		return false;
	}
	pszText=new char[Size+1];
	if (!::ReadFile(hFile,pszText,Size,&Read,NULL) || Read!=Size) {
		delete [] pszText;
		::CloseHandle(hFile);
		return false;
	}
	::CloseHandle(hFile);
	NumSpaces=m_DriverTuningSpaceList.NumSpaces();
	pszText[Size]='\0';
	p=pszText;
	do {
		TCHAR szName[MAX_CHANNEL_NAME];
		int Space,ChannelNo,Channel,ChannelIndex;

		while (*p=='\r' || *p=='\n' || *p==' ' || *p=='\t')
			p++;
		if (*p==';') {	// コメント
			while (*p!='\r' && *p!='\n' && *p!='\0')
				p++;
			continue;
		}
		if (*p=='\0')
			break;
		// チャンネル名
		q=p;
		while (*p!=',' && *p!='\0') {
			if (::IsDBCSLeadByteEx(CP_ACP,*p) && *(p+1)!='\0')
				p++;
			p++;
		}
		if (*p!=',')
			goto Next;
		*p++='\0';
#ifdef UNICODE
		::MultiByteToWideChar(CP_ACP,0,q,-1,szName,MAX_CHANNEL_NAME);
#else
		::lstrcpyn(szName,q,MAX_CHANNEL_NAME);
#endif
		SkipSpaces(&p);
		// リモコン番号
		ChannelNo=0;
		while (*p>='0' && *p<='9') {
			ChannelNo=ChannelNo*10+(*p-'0');
			p++;
		}
		SkipSpaces(&p);
		if (*p!=',')
			goto Next;
		p++;
		SkipSpaces(&p);
		// チャンネル
		bool fCompatible=false;
		bool fOK=false;
		int Length;
		TCHAR szChannel[64];
		int i,j;
		if (*p=='C' || *p=='c' || (*p>='0' && *p<='9')) {
			// 物理チャンネル番号による指定か判定(cap_hdusとの互換用)
			Length=0;
			if (*p=='C' || *p=='c') {
				Space=1;
				Length++;
			} else
				Space=0;
			Channel=0;
			while (p[Length]>='0' && p[Length]<='9') {
				Channel=Channel*10+(p[Length]-'0');
				Length++;
			}
			if ((p[Length]=='\r' || p[Length]=='\n' || p[Length]=='\0' || p[Length]==',')
					&& Channel>=FIRST_UHF_CHANNEL) {
				::wsprintf(szChannel,TEXT("%s%dch"),
							(*p=='C' || *p=='c')?TEXT("C"):TEXT(""),Channel);
				p+=Length;
				Length=::lstrlen(szChannel);
				fCompatible=true;
			}
		}
		if (!fCompatible) {
			q=p;
			while (*p!='\0' && *p!='\r' && *p!='\n' && *p!=',')
				p++;
			Length=::MultiByteToWideChar(CP_ACP,0,q,(int)(p-q),szChannel,lengthof(szChannel));
			Channel=0;
		}
		if (NumSpaces>0) {
			for (i=0;i<NumSpaces;i++) {
				const CChannelList *pList=m_DriverTuningSpaceList.GetChannelList(i);

				for (j=0;j<pList->NumChannels();j++) {
					const CChannelInfo *pChInfo=pList->GetChannelInfo(j);
					LPCTSTR pszChannel=pChInfo->GetName();

					if (::StrCmpNI(pszChannel,szChannel,Length)==0
							&& (pszChannel[Length]==_T(' ') || pszChannel[Length]==_T('\0'))) {
						Space=i;
						ChannelIndex=pChInfo->GetChannelIndex();
						fOK=true;
						break;
					}
				}
			}
		}
		if (!fOK && fCompatible) {
			ChannelIndex=Channel-FIRST_UHF_CHANNEL;
			fOK=true;
		}
#if 0
		// サービス
		int Service=0;
		if (*p==',') {
			p++;
			SkipSpaces(&p);
			while (*p>='0' && *p<='9') {
				Service=Service*10+(*p-'0');
				p++;
			}
		}
#endif
		if (ChannelNo>0 && fOK) {
			CChannelInfo ChInfo(Space,ChannelIndex,ChannelNo,szName);
			ChInfo.SetPhysicalChannel(Channel);
			ChannelList.AddChannel(ChInfo);
			TRACE(TEXT("Channel loaded : %s sp %d ch %d id %d\n"),
								szName,Space,ChannelIndex,ChannelNo);
		}
	Next:
		while (*p!='\r' && *p!='\n' && *p!='\0')
			p++;
	} while (*p!='\0');
	delete [] pszText;
	return m_TuningSpaceList.Create(&ChannelList,NumSpaces);
}


bool CChannelManager::LoadChannelList(LPCTSTR pszFileName)
{
	if (IsStringEmpty(pszFileName))
		return false;

	if (::PathMatchSpec(pszFileName,TEXT("*.ch2"))
#ifdef TVH264
		|| ::PathMatchSpec(pszFileName,TEXT("*.ch1"))
#endif
		) {
		// 新しい形式のチャンネル設定ファイル
		if (!m_TuningSpaceList.LoadFromFile(pszFileName))
			return false;

		// 古いバージョンのチャンネルファイルはTSIDやNIDが無い場合がある
		m_fChannelFileHasStreamIDs=true;
		for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
			const CTuningSpaceInfo *pSpace=m_TuningSpaceList.GetTuningSpaceInfo(i);
			if (pSpace!=NULL) {
				const int NumChannels=pSpace->NumChannels();
				for (int j=0;j<NumChannels;j++) {
					const CChannelInfo *pChannel=pSpace->GetChannelInfo(j);
					if (pChannel!=NULL) {
						if (pChannel->GetServiceID()==0
								|| pChannel->GetNetworkID()==0
								|| pChannel->GetTransportStreamID()==0) {
							m_fChannelFileHasStreamIDs=false;
							goto Break;
						}
					}
				}
			}
		}
	Break:
		;
	} else if (::PathMatchSpec(pszFileName,TEXT("*.ch"))) {
		// 古い形式のチャンネル設定ファイル(cap_hdus等との互換用)
		if (!LoadOldChannelFile(pszFileName))
			return false;
		m_fChannelFileHasStreamIDs=false;
	} else {
		return false;
	}

	m_ChannelFileName=pszFileName;

	for (int i=0;i<m_DriverTuningSpaceList.NumSpaces();i++) {
		CTuningSpaceInfo *pTuningSpace=m_TuningSpaceList.GetTuningSpaceInfo(i);

		if (pTuningSpace!=NULL) {
			const CTuningSpaceInfo *pDriverTuningSpace=m_DriverTuningSpaceList.GetTuningSpaceInfo(i);
			const CChannelList *pDriverChannelList=pDriverTuningSpace->GetChannelList();

			pTuningSpace->SetName(pDriverTuningSpace->GetName());

			const int NumChannels=pTuningSpace->NumChannels();
			for (int j=0;j<NumChannels;j++) {
				CChannelInfo *pChInfo=pTuningSpace->GetChannelInfo(j);

				if (pChInfo->GetPhysicalChannel()==0) {
					int Channel=pDriverChannelList->GetPhysicalChannel(pChInfo->GetChannelIndex());

					if (Channel>0)
						pChInfo->SetPhysicalChannel(Channel);
				}
			}
		}
	}

	return true;
}


bool CChannelManager::SetTuningSpaceList(const CTuningSpaceList *pList)
{
	m_TuningSpaceList=*pList;
	return true;
}


bool CChannelManager::MakeDriverTuningSpaceList(const CBonSrcDecoder *pSrcDecoder)
{
	const int NumSpaces=pSrcDecoder->NumSpaces();

	m_DriverTuningSpaceList.Clear();
	if (!m_DriverTuningSpaceList.Reserve(NumSpaces))
		return false;

	for (int i=0;i<NumSpaces;i++) {
		CChannelList *pList=m_DriverTuningSpaceList.GetChannelList(i);
		LPCTSTR pszName;

		for (int j=0;(pszName=pSrcDecoder->GetChannelName(i,j))!=NULL;j++) {
			CChannelInfo ChInfo(i,j,j+1,pszName);

			LPCTSTR p=pszName;
			int Channel=0;
			while (*p!=_T('\0')) {
				if (*p>=_T('0') && *p<=_T('9')) {
					do {
						Channel=Channel*10+(*p-_T('0'));
						p++;
					} while (*p>=_T('0') && *p<=_T('9'));
					if (::StrCmpNI(p,TEXT("ch"),2)!=0)
						Channel=0;
				} else {
					p++;
				}
			}
			ChInfo.SetPhysicalChannel(Channel);

			pList->AddChannel(ChInfo);
		}
		m_DriverTuningSpaceList.GetTuningSpaceInfo(i)->SetName(pSrcDecoder->GetSpaceName(i));
	}

	m_DriverTuningSpaceList.MakeAllChannelList();

	return true;
}


bool CChannelManager::SetUseDriverChannelList(bool fUse)
{
	m_fUseDriverChannelList=fUse;
	return true;
}


bool CChannelManager::SetCurrentChannel(int Space,int Channel)
{
#ifdef NETWORK_REMOCON_SUPPORT
	if (!m_fNetworkRemocon)
#endif
	{
		if (Space!=SPACE_ALL && Space!=SPACE_INVALID) {
			if (Space<0 || Space>=NumSpaces())
				return false;
		}
	}
	if (Space!=SPACE_INVALID) {
		const CChannelList *pList=GetChannelList(Space);
		if (pList==NULL || Channel<-1 || Channel>=pList->NumChannels())
			return false;
	} else {
		Channel=-1;
	}
	m_CurrentSpace=Space;
	m_CurrentChannel=Channel;
	return true;
}


bool CChannelManager::SetCurrentServiceID(int ServiceID)
{
	m_CurrentServiceID=ServiceID;
	return true;
}


bool CChannelManager::SetChangingChannel(int Channel)
{
	m_ChangingChannel=Channel;
	return true;
}


const CChannelInfo *CChannelManager::GetCurrentChannelInfo() const
{
	const CChannelList *pList=GetCurrentChannelList();

	if (pList==NULL)
		return NULL;
	return pList->GetChannelInfo(
#ifdef NETWORK_REMOCON_SUPPORT
		m_fNetworkRemocon?m_NetworkRemoconCurrentChannel:
#endif
		m_CurrentChannel);
}


const CChannelInfo *CChannelManager::GetCurrentRealChannelInfo() const
{
	const CChannelList *pList=GetChannelList(m_CurrentSpace);

	if (pList==NULL)
		return NULL;
	return pList->GetChannelInfo(m_CurrentChannel);
}


const CChannelInfo *CChannelManager::GetChangingChannelInfo() const
{
	const CChannelList *pList=GetCurrentChannelList();

	if (pList!=NULL)
		return pList->GetChannelInfo(m_ChangingChannel);
	return NULL;
}


const CChannelInfo *CChannelManager::GetNextChannelInfo(bool fNext) const
{
	int Channel=-1,No;
	const CChannelInfo *pInfo;

	if (m_ChangingChannel>=0)
		Channel=m_ChangingChannel;
#ifdef NETWORK_REMOCON_SUPPORT
	if (m_fNetworkRemocon) {
		if (Channel<0) {
			if (m_NetworkRemoconCurrentChannel<0)
				return NULL;
			Channel=m_NetworkRemoconCurrentChannel;
		}
		No=m_pNetworkRemoconChannelList->GetChannelNo(Channel);
		if (fNext)
			No=m_pNetworkRemoconChannelList->GetNextChannelNo(No,true);
		else
			No=m_pNetworkRemoconChannelList->GetPrevChannelNo(No,true);
		pInfo=m_pNetworkRemoconChannelList->GetChannelInfo(
							m_pNetworkRemoconChannelList->FindChannelNo(No));
	} else
#endif
	{
		const CChannelList *pList=GetCurrentChannelList();
		int i;

		if (pList==NULL)
			return NULL;
		if (Channel<0) {
			if (m_CurrentChannel<0)
				return NULL;
			Channel=m_CurrentChannel;
		}
		//if (pList->HasRemoteControlKeyID()) {
		if ((No=pList->GetChannelNo(Channel))>0) {
			if (fNext)
				No=pList->GetNextChannelNo(No,true);
			else
				No=pList->GetPrevChannelNo(No,true);
			pInfo=pList->GetChannelInfo(pList->FindChannelNo(No));
		} else {
			No=Channel;
			for (i=pList->NumChannels();i>0;i--) {
				if (fNext) {
					No++;
					if (No>=pList->NumChannels())
						No=0;
				} else {
					No--;
					if (No<0)
						No=pList->NumChannels()-1;
				}
				pInfo=pList->GetChannelInfo(No);
				if (pInfo->IsEnabled())
					break;
			}
			if (i==0)
				return NULL;
		}
	}
	return pInfo;
}


const CChannelList *CChannelManager::GetCurrentChannelList() const
{
#ifdef NETWORK_REMOCON_SUPPORT
	if (m_fNetworkRemocon)
		return m_pNetworkRemoconChannelList;
#endif
	return GetChannelList(m_CurrentSpace);
}


const CChannelList *CChannelManager::GetCurrentRealChannelList() const
{
	return GetChannelList(m_CurrentSpace);
}


const CChannelList *CChannelManager::GetChannelList(int Space) const
{
	if (Space==SPACE_ALL)
		return GetAllChannelList();
	if (!m_fUseDriverChannelList) {
		const CChannelList *pList=m_TuningSpaceList.GetChannelList(Space);
		if (pList!=NULL && pList->NumChannels()>0)
			return pList;
	}
	return m_DriverTuningSpaceList.GetChannelList(Space);
}


const CChannelList *CChannelManager::GetFileChannelList(int Space) const
{
	if (Space==SPACE_ALL)
		return m_TuningSpaceList.GetAllChannelList();
	return m_TuningSpaceList.GetChannelList(Space);
}


const CChannelList *CChannelManager::GetDriverChannelList(int Space) const
{
	if (Space==SPACE_ALL)
		return m_DriverTuningSpaceList.GetAllChannelList();
	return m_DriverTuningSpaceList.GetChannelList(Space);
}


const CChannelList *CChannelManager::GetAllChannelList() const
{
	/*
	CChannelList *pList=m_TuningSpaceList.GetAllChannelList();

	if (pList->NumChannels()>0)
		return pList;
	return m_DriverTuningSpaceList.GetAllChannelList();
	*/
	if (m_fUseDriverChannelList)
		return m_DriverTuningSpaceList.GetAllChannelList();
	return m_TuningSpaceList.GetAllChannelList();
}


const CChannelList *CChannelManager::GetFileAllChannelList() const
{
	return m_TuningSpaceList.GetAllChannelList();
}


const CChannelList *CChannelManager::GetDriverAllChannelList() const
{
	return m_DriverTuningSpaceList.GetAllChannelList();
}


LPCTSTR CChannelManager::GetTuningSpaceName(int Space) const
{
	return m_DriverTuningSpaceList.GetTuningSpaceName(Space);
}


int CChannelManager::FindChannelInfo(const CChannelInfo *pInfo) const
{
	const CChannelList *pList=GetCurrentChannelList();
	int i;

	if (pList==NULL)
		return -1;
	for (i=0;i<pList->NumChannels();i++) {
		if (pList->GetChannelInfo(i)==pInfo)
			return i;
	}
	return -1;
}


int CChannelManager::FindChannelByIDs(int Space,WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const
{
	const CChannelList *pChannelList;

	if (Space==SPACE_ALL)
		pChannelList=m_TuningSpaceList.GetAllChannelList();
	else
		pChannelList=m_TuningSpaceList.GetChannelList(Space);
	if (pChannelList==nullptr)
		return -1;

	return pChannelList->FindByIDs(NetworkID,TransportStreamID,ServiceID);
}


int CChannelManager::NumSpaces() const
{
	if (m_fUseDriverChannelList)
		return m_DriverTuningSpaceList.NumSpaces();
	return max(m_TuningSpaceList.NumSpaces(),m_DriverTuningSpaceList.NumSpaces());
}


bool CChannelManager::GetChannelFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (pszFileName==NULL
			|| m_ChannelFileName.empty()
			|| MaxLength<=(int)m_ChannelFileName.length())
		return false;

	::lstrcpy(pszFileName,m_ChannelFileName.c_str());

	return true;
}


#ifdef NETWORK_REMOCON_SUPPORT

bool CChannelManager::SetNetworkRemoconMode(bool fNetworkRemocon,CChannelList *pList)
{
	if (fNetworkRemocon && pList==NULL)
		return false;
	m_fNetworkRemocon=fNetworkRemocon;
	m_pNetworkRemoconChannelList=pList;
	m_NetworkRemoconCurrentChannel=-1;
	return true;
}


bool CChannelManager::SetNetworkRemoconCurrentChannel(int Channel)
{
	if (m_pNetworkRemoconChannelList==NULL
			|| Channel<-1 || Channel>=m_pNetworkRemoconChannelList->NumChannels())
		return false;
	m_NetworkRemoconCurrentChannel=Channel;
	return true;
}

#endif	// NETWORK_REMOCON_SUPPORT


bool CChannelManager::UpdateStreamInfo(int Space,int ChannelIndex,
						WORD NetworkID,WORD TransportStreamID,WORD ServiceID)
{
#ifdef NETWORK_REMOCON_SUPPORT
	if (m_fNetworkRemocon) {
		m_pNetworkRemoconChannelList->UpdateStreamInfo(Space,ChannelIndex,
										NetworkID,TransportStreamID,ServiceID);
	} else
#endif
	{
		if (!m_fChannelFileHasStreamIDs)
			m_TuningSpaceList.UpdateStreamInfo(Space,ChannelIndex,
											   NetworkID,TransportStreamID,ServiceID);
	}
	return true;
}


bool CChannelManager::LoadChannelSettings(LPCTSTR pszFileName,LPCTSTR pszDriverName)
{
	if (m_TuningSpaceList.IsEmpty() || m_fChannelFileHasStreamIDs)
		return true;

	CSettings Settings;
	int SpaceCount;

	TRACE(TEXT("ストリーム情報の読み込み : \"%s\" [%s]\n"),pszFileName,pszDriverName);
	if (!Settings.Open(pszFileName,CSettings::OPEN_READ)
			|| !Settings.SetSection(::PathFindFileName(pszDriverName)))
		return false;
	if (Settings.Read(TEXT("SpaceCount"),&SpaceCount) && SpaceCount>0) {
		for (int i=0;i<SpaceCount;i++) {
			int NumChannels;
			TCHAR szName[64];

			::wsprintf(szName,TEXT("Space%d_Count"),i);
			if (Settings.Read(szName,&NumChannels) && NumChannels>0) {
				for (int j=0;j<NumChannels;j++) {
					int ChannelIndex;
					int NumServices;
					unsigned int NetworkID,TSID,ServiceID;

					::wsprintf(szName,TEXT("Space%d_ChannelMap%d"),i,j);
					if (Settings.Read(szName,&ChannelIndex)) {
						::wsprintf(szName,TEXT("Space%d_Channel%d_NID"),i,ChannelIndex);
						if (!Settings.Read(szName,&NetworkID))
							NetworkID=0;
						::wsprintf(szName,TEXT("Space%d_Channel%d_TSID"),i,ChannelIndex);
						if (!Settings.Read(szName,&TSID))
							TSID=0;
						if (NetworkID!=0 || TSID!=0) {
							::wsprintf(szName,TEXT("Space%d_Channel%d_Count"),i,ChannelIndex);
							if (Settings.Read(szName,&NumServices) && NumServices>0) {
								for (int k=0;k<NumServices;k++) {
									::wsprintf(szName,TEXT("Space%d_Channel%d_Service%d_SID"),i,ChannelIndex,k);
									if (Settings.Read(szName,&ServiceID) && ServiceID!=0)
										UpdateStreamInfo(i,ChannelIndex,NetworkID,TSID,ServiceID);
								}
							}
						}
					}
				}
			}
		}
	}
	return true;
}


bool CChannelManager::SaveChannelSettings(LPCTSTR pszFileName,LPCTSTR pszDriverName)
{
	if (m_TuningSpaceList.IsEmpty() || m_fChannelFileHasStreamIDs)
		return true;

	CSettings Settings;
	int SpaceCount;

	TRACE(TEXT("ストリーム情報の保存 : \"%s\" [%s]\n"),pszFileName,pszDriverName);
	if (!Settings.Open(pszFileName,CSettings::OPEN_WRITE)
			|| !Settings.SetSection(::PathFindFileName(pszDriverName)))
		return false;
	SpaceCount=m_TuningSpaceList.NumSpaces();
	Settings.Clear();
	Settings.Write(TEXT("SpaceCount"),SpaceCount);
	for (int i=0;i<SpaceCount;i++) {
		const CChannelList *pList=m_TuningSpaceList.GetChannelList(i);
		int NumChannels=pList->NumChannels();
		TCHAR szName[64];

		::wsprintf(szName,TEXT("Space%d_Count"),i);
		Settings.Write(szName,NumChannels);
		int LastIndex=0;
		for (int j=0;j<NumChannels;j++) {
			int Index=pList->GetChannelIndex(j);
			if (Index>LastIndex)
				LastIndex=Index;
		}
		int *pServiceCount=new int[LastIndex+1];
		int Map=0;
		for (int j=0;j<=LastIndex;j++) {
			int NumServices=0;
			for (int k=0;k<NumChannels;k++) {
				if (pList->GetChannelIndex(k)==j)
					NumServices++;
			}
			if (NumServices>0) {
				::wsprintf(szName,TEXT("Space%d_ChannelMap%d"),i,Map++);
				Settings.Write(szName,j);
				::wsprintf(szName,TEXT("Space%d_Channel%d_Count"),i,j);
				Settings.Write(szName,NumServices);
			}
			pServiceCount[j]=0;
		}
		for (int j=0;j<NumChannels;j++) {
			const CChannelInfo *pChInfo=pList->GetChannelInfo(j);

			if (pChInfo->GetNetworkID()!=0) {
				::wsprintf(szName,TEXT("Space%d_Channel%d_NID"),
					pChInfo->GetSpace(),pChInfo->GetChannelIndex());
				Settings.Write(szName,pChInfo->GetNetworkID());
			}
			if (pChInfo->GetTransportStreamID()!=0) {
				::wsprintf(szName,TEXT("Space%d_Channel%d_TSID"),
					pChInfo->GetSpace(),pChInfo->GetChannelIndex());
				Settings.Write(szName,pChInfo->GetTransportStreamID());
			}
			if (pChInfo->GetServiceID()!=0) {
				::wsprintf(szName,TEXT("Space%d_Channel%d_Service%d_SID"),
					pChInfo->GetSpace(),pChInfo->GetChannelIndex(),
					pServiceCount[pChInfo->GetChannelIndex()]);
				Settings.Write(szName,pChInfo->GetServiceID());
			}
			pServiceCount[pChInfo->GetChannelIndex()]++;
		}
		delete [] pServiceCount;
	}
	return true;
}




CChannelSpec::CChannelSpec()
{
	m_Space=-1;
	m_Channel=-1;
	m_ServiceID=-1;
}


CChannelSpec::~CChannelSpec()
{
}


bool CChannelSpec::Store(const CChannelManager *pChannelManager)
{
	m_Space=pChannelManager->GetCurrentSpace();
	m_Channel=pChannelManager->GetCurrentChannel();
	m_ServiceID=pChannelManager->GetCurrentServiceID();
	return true;
}


bool CChannelSpec::SetSpace(int Space)
{
	m_Space=Space;
	return true;
}


bool CChannelSpec::SetChannel(int Channel)
{
	m_Channel=Channel;
	return true;
}


bool CChannelSpec::SetServiceID(int ServiceID)
{
	m_ServiceID=ServiceID;
	return true;
}
