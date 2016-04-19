#include "stdafx.h"
#include "Common.h"
#include "TsAnalyzer.h"
#include "TsTable.h"
#ifdef TS_ANALYZER_EIT_SUPPORT
#include "TsEncode.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#define TABLE_DEBUG	true
#else
#define TABLE_DEBUG
#endif




CTsAnalyzer::CTsAnalyzer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
{
	Reset();
}


CTsAnalyzer::~CTsAnalyzer()
{
}


const bool CTsAnalyzer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	{
		CBlockLock Lock(&m_DecoderLock);

		/*
		if (dwInputIndex >= GetInputNum())
			return false;

		CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

		// ���̓��f�B�A�f�[�^�͌݊������Ȃ�
		if (!pTsPacket)
			return false;
		*/

		CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

		m_DecoderEvent = EVENT_INVALID;

		// PID���[�e�B���O
		m_PidMapManager.StorePacket(pTsPacket);

		// ���̃t�B���^�Ƀf�[�^��n��
		OutputMedia(pMediaData);
	}

	// �C�x���g���������Ă�����ʒm����
	if (m_DecoderEvent != EVENT_INVALID) {
		SendDecoderEvent((DWORD)m_DecoderEvent);
	}

	return true;
}


void CTsAnalyzer::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	// �C�x���g�n���h���Ƀ��Z�b�g��ʒm����
	NotifyResetEvent();

	// �S�e�[�u���A���}�b�v
	m_PidMapManager.UnmapAllTarget();

	// �T�[�r�X���X�g�N���A
	m_ServiceList.clear();

	// �g�����X�|�[�g�X�g���[��ID������
	m_TransportStreamID = 0x0000;

	// �l�b�g���[�NID������
	m_NetworkID = 0x0000;

	// PAT�e�[�u��PID�}�b�v�ǉ�
	m_PidMapManager.MapTarget(PID_PAT, new CPatTable(TABLE_DEBUG), OnPatUpdated, this);

	// NIT�e�[�u��PID�}�b�v�ǉ�
	m_PidMapManager.MapTarget(PID_NIT, new CNitTable, OnNitUpdated, this);
	::ZeroMemory(&m_NitInfo, sizeof(m_NitInfo));

#ifdef TS_ANALYZER_EIT_SUPPORT
	// H-EIT�e�[�u��PID�}�b�v�ǉ�
	m_PidMapManager.MapTarget(PID_HEIT, new CEitPfTable);

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	// L-EIT�e�[�u��PID�}�b�v�ǉ�
	m_PidMapManager.MapTarget(PID_LEIT, new CEitPfTable);
#endif
#endif

	// TOT�e�[�u��PID�}�b�v�ǉ�
	m_PidMapManager.MapTarget(PID_TOT, new CTotTable);
}


WORD CTsAnalyzer::GetServiceNum()
{
	CBlockLock Lock(&m_DecoderLock);

	// �T�[�r�X����Ԃ�
	return (WORD)m_ServiceList.size();
}


bool CTsAnalyzer::GetServiceID(const int Index, WORD *pServiceID)
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	// �T�[�r�XID���擾����
	if (Index < 0) {
#ifndef BONTSENGINE_1SEG_SUPPORT
		if (m_ServiceList.size() == 0 || !m_ServiceList[0].bIsUpdated)
			return false;
		*pServiceID = m_ServiceList[0].ServiceID;
#else
		WORD MinPID = 0xFFFF;
		size_t MinIndex;
		for (size_t i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].PmtPID >= 0x1FC8 && m_ServiceList[i].PmtPID <= 0x1FCF
					&& m_ServiceList[i].PmtPID < MinPID) {
				MinPID = m_ServiceList[i].PmtPID;
				MinIndex = i;
			}
		}
		if (MinPID == 0xFFFF || !m_ServiceList[MinIndex].bIsUpdated)
			return false;
		*pServiceID = m_ServiceList[MinIndex].ServiceID;
#endif
	} else if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pServiceID = m_ServiceList[Index].ServiceID;
	} else {
		return false;
	}
	return true;
}


int CTsAnalyzer::GetServiceIndexByID(const WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	// �v���O����ID����T�[�r�X�C���f�b�N�X����������
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			return (int)Index;
	}

	// �v���O����ID��������Ȃ�
	return -1;
}


bool CTsAnalyzer::IsViewableService(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	return
#if defined(BONTSENGINE_MPEG2_SUPPORT) && defined(BONTSENGINE_H264_SUPPORT)
		   m_ServiceList[Index].VideoStreamType == STREAM_TYPE_MPEG2
		|| m_ServiceList[Index].VideoStreamType == STREAM_TYPE_H264
#elif defined(BONTSENGINE_MPEG2_SUPPORT)
		   m_ServiceList[Index].VideoStreamType == STREAM_TYPE_MPEG2
#elif defined(BONTSENGINE_H264_SUPPORT)
		   m_ServiceList[Index].VideoStreamType == STREAM_TYPE_H264
#endif
#ifdef BONTSENGINE_RADIO_SUPPORT
		|| (m_ServiceList[Index].VideoStreamType == 0xFF
			&& m_ServiceList[Index].AudioEsList.size() > 0)
#endif
		;
}


WORD CTsAnalyzer::GetViewableServiceNum()
{
	CBlockLock Lock(&m_DecoderLock);
	WORD Count = 0;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (IsViewableService((int)i))
			Count++;
	}
	return Count;
}


bool CTsAnalyzer::GetViewableServiceID(const int Index, WORD *pServiceID)
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	int j = 0;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (IsViewableService((int)i)) {
			if (j == Index) {
				*pServiceID = m_ServiceList[i].ServiceID;
				return true;
			}
			j++;
		}
	}
	return false;
}


bool CTsAnalyzer::GetFirstViewableServiceID(WORD *pServiceID)
{
	if (pServiceID == NULL)
		return false;

#ifndef BONTSENGINE_1SEG_SUPPORT
	CBlockLock Lock(&m_DecoderLock);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (!m_ServiceList[i].bIsUpdated)
			return false;
		if (IsViewableService((int)i)) {
			*pServiceID = m_ServiceList[i].ServiceID;
			return true;
		}
	}
	return false;
#else
	return GetServiceID(-1, pServiceID);
#endif
}


int CTsAnalyzer::GetViewableServiceIndexByID(const WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	int j = 0;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (IsViewableService((int)i)) {
			if (m_ServiceList[i].ServiceID == ServiceID)
				return j;
			j++;
		}
	}
	return -1;
}


bool CTsAnalyzer::GetServiceInfo(const int Index, ServiceInfo *pInfo)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pInfo != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pInfo = m_ServiceList[Index];
		return true;
	}
	return false;
}


bool CTsAnalyzer::IsServiceUpdated(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		return m_ServiceList[Index].bIsUpdated;
	}
	return false;
}


bool CTsAnalyzer::GetPmtPID(const int Index, WORD *pPmtPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pPmtPID != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pPmtPID = m_ServiceList[Index].PmtPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoEsPID(const int Index, WORD *pVideoPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pVideoPID != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].VideoEs.PID != PID_INVALID) {
		*pVideoPID = m_ServiceList[Index].VideoEs.PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoStreamType(const int Index, BYTE *pStreamType)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pStreamType != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pStreamType = m_ServiceList[Index].VideoStreamType;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetVideoComponentTag(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return m_ServiceList[Index].VideoEs.ComponentTag;
	return COMPONENTTAG_INVALID;
}


WORD CTsAnalyzer::GetAudioEsNum(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].AudioEsList.size();
	return 0;
}


bool CTsAnalyzer::GetAudioEsPID(const int Index, const int AudioIndex, WORD *pAudioPID)
{
	if (pAudioPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		*pAudioPID = m_ServiceList[Index].AudioEsList[AudioIndex].PID;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetAudioComponentTag(const int Index, const int AudioIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		return m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag;
	}
	return COMPONENTTAG_INVALID;
}


#ifdef TS_ANALYZER_EIT_SUPPORT

BYTE CTsAnalyzer::GetVideoComponentType(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CComponentDesc *pComponentDesc = dynamic_cast<const CComponentDesc*>(pDescBlock->GetDescByTag(CComponentDesc::DESC_TAG));

			if (pComponentDesc != NULL)
				return pComponentDesc->GetComponentType();
		}
	}
	return 0;
}


int CTsAnalyzer::GetAudioIndexByComponentTag(const int Index, const BYTE ComponentTag)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].AudioEsList.size() > 0) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			for (WORD i = 0; i < pDescBlock->GetDescNum(); i++) {
				const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

				if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
					const CAudioComponentDesc *pAudioDesc = dynamic_cast<const CAudioComponentDesc*>(pDesc);

					if (pAudioDesc) {
						for (size_t j = 0; j < m_ServiceList[Index].AudioEsList.size(); j++) {
							if (pAudioDesc->GetComponentTag() == m_ServiceList[Index].AudioEsList[j].ComponentTag)
								return (int)j;
						}
					}
				}
			}
		}
	}
	return -1;
}


BYTE CTsAnalyzer::GetAudioComponentType(const int Index, const int AudioIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag);
			if (pAudioDesc != NULL)
				return pAudioDesc->GetComponentType();
		}
	}
	return 0;
}


int CTsAnalyzer::GetAudioComponentText(const int Index, const int AudioIndex, LPTSTR pszText, int MaxLength)
{
	if (pszText == NULL || MaxLength < 1)
		return 0;

	pszText[0] = '\0';

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag);
			if (pAudioDesc != NULL)
				return pAudioDesc->GetText(pszText, MaxLength);
		}
	}
	return 0;
}

#endif	// TS_ANALYZER_EIT_SUPPORT


WORD CTsAnalyzer::GetCaptionEsNum(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].CaptionEsList.size();
	return 0;
}


bool CTsAnalyzer::GetCaptionEsPID(const int Index, const WORD CaptionIndex, WORD *pCaptionPID)
{
	if (pCaptionPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& (size_t)CaptionIndex < m_ServiceList[Index].CaptionEsList.size()) {
		*pCaptionPID = m_ServiceList[Index].CaptionEsList[CaptionIndex].PID;
		return true;
	}
	return false;
}


WORD CTsAnalyzer::GetDataCarrouselEsNum(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].DataCarrouselEsList.size();
	return 0;
}


bool CTsAnalyzer::GetDataCarrouselEsPID(const int Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID)
{
	if (pDataCarrouselPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& (size_t)DataCarrouselIndex < m_ServiceList[Index].DataCarrouselEsList.size()) {
		*pDataCarrouselPID = m_ServiceList[Index].DataCarrouselEsList[DataCarrouselIndex].PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrPID(const int Index, WORD *pPcrPID)
{
	if (pPcrPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].PcrPID != PID_INVALID) {
		*pPcrPID = m_ServiceList[Index].PcrPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrTimeStamp(const int Index, ULONGLONG *pTimeStamp)
{
	if (pTimeStamp == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	// PCR���擾����
	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pTimeStamp = m_ServiceList[Index].PcrTimeStamp;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetEcmPID(const int Index, WORD *pEcmPID)
{
	if (pEcmPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].EcmPID != PID_INVALID) {
		*pEcmPID = m_ServiceList[Index].EcmPID;
		return true;
	}
	return false;
}


int CTsAnalyzer::GetServiceName(const int Index, LPTSTR pszName, const int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	// �T�[�r�X�����擾����
	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		if (pszName != NULL && MaxLength > 0)
			::lstrcpyn(pszName, m_ServiceList[Index].szServiceName, MaxLength);
		return ::lstrlen(m_ServiceList[Index].szServiceName);
	}
	return 0;
}


BYTE CTsAnalyzer::GetServiceType(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return m_ServiceList[Index].ServiceType;
	return SERVICE_TYPE_INVALID;
}


WORD CTsAnalyzer::GetLogoID(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return m_ServiceList[Index].LogoID;
	return 0xFFFF;
}


bool CTsAnalyzer::GetServiceList(ServiceList *pList)
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	*pList = m_ServiceList;
	return true;
}


bool CTsAnalyzer::GetViewableServiceList(ServiceList *pList)
{
	if (!pList)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->clear();
	for (size_t i = 0 ; i < m_ServiceList.size() ; i++) {
		if (IsViewableService((int)i))
			pList->push_back(m_ServiceList[i]);
	}
	return true;
}


WORD CTsAnalyzer::GetTransportStreamID() const
{
	return m_TransportStreamID;
}


WORD CTsAnalyzer::GetNetworkID() const
{
	return m_NetworkID;
}


BYTE CTsAnalyzer::GetBroadcastingID() const
{
	return m_NitInfo.BroadcastingID;
}


int CTsAnalyzer::GetNetworkName(LPTSTR pszName, int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName != NULL && MaxLength > 0)
		::lstrcpyn(pszName, m_NitInfo.szNetworkName, MaxLength);
	return ::lstrlen(m_NitInfo.szNetworkName);
}


BYTE CTsAnalyzer::GetRemoteControlKeyID() const
{
	return m_NitInfo.RemoteControlKeyID;
}


int CTsAnalyzer::GetTsName(LPTSTR pszName,int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName != NULL && MaxLength > 0)
		::lstrcpyn(pszName, m_NitInfo.szTSName, MaxLength);
	return ::lstrlen(m_NitInfo.szTSName);
}


#ifdef TS_ANALYZER_EIT_SUPPORT


WORD CTsAnalyzer::GetEventID(const int ServiceIndex, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable=dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));

		if (pEitTable) {
			int Index=pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index>=0)
				return pEitTable->GetEventID(Index,fNext?1:0);
		}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
		pEitTable=dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));

		if (pEitTable) {
			int Index=pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index>=0)
				return pEitTable->GetEventID(Index,fNext?1:0);
		}
#endif
	}
	return 0;
}


bool CTsAnalyzer::GetEventStartTime(const int ServiceIndex, SYSTEMTIME *pSystemTime, const bool bNext)
{
	if (pSystemTime == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0) {
				const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Index, bNext ? 1 : 0);
				if (pStartTime) {
					*pSystemTime = *pStartTime;
					return true;
				}
			}
		}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
		pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0) {
				const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Index, bNext ? 1 : 0);
				if (pStartTime) {
					*pSystemTime = *pStartTime;
					return true;
				}
			}
		}
#endif
	}
	return false;
}


DWORD CTsAnalyzer::GetEventDuration(const int ServiceIndex, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetDuration(Index, bNext ? 1 : 0);
		}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
		pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetDuration(Index, bNext ? 1 : 0);
		}
#endif
	}
	return 0;
}


int CTsAnalyzer::GetEventName(const int ServiceIndex, LPTSTR pszName, int MaxLength, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	pDescBlock = GetLEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}
#endif
	return 0;
}


int CTsAnalyzer::GetEventText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}

#ifdef TS_ANALYZER_L_EIT_SUPPORT
	pDescBlock = GetLEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}
#endif
	return 0;
}


bool CTsAnalyzer::GetEventVideoInfo(const int ServiceIndex, EventVideoInfo *pInfo, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CComponentDesc *pComponentDesc = dynamic_cast<const CComponentDesc *>(pDescBlock->GetDescByTag(CComponentDesc::DESC_TAG));

		if (pComponentDesc) {
			pInfo->StreamContent = pComponentDesc->GetStreamContent();
			pInfo->ComponentType = pComponentDesc->GetComponentType();
			pInfo->ComponentTag = pComponentDesc->GetComponentTag();
			pInfo->LanguageCode = pComponentDesc->GetLanguageCode();
			pComponentDesc->GetText(pInfo->szText, EventVideoInfo::MAX_TEXT);
			return true;
		}
	}
	return false;
}


static void AudioComponentDescToAudioInfo(const CAudioComponentDesc *pAudioDesc, CTsAnalyzer::EventAudioInfo *pInfo)
{
	pInfo->StreamContent = pAudioDesc->GetStreamContent();
	pInfo->ComponentType = pAudioDesc->GetComponentType();
	pInfo->ComponentTag = pAudioDesc->GetComponentTag();
	pInfo->SimulcastGroupTag = pAudioDesc->GetSimulcastGroupTag();
	pInfo->bESMultiLingualFlag = pAudioDesc->GetESMultiLingualFlag();
	pInfo->bMainComponentFlag = pAudioDesc->GetMainComponentFlag();
	pInfo->QualityIndicator = pAudioDesc->GetQualityIndicator();
	pInfo->SamplingRate = pAudioDesc->GetSamplingRate();
	pInfo->LanguageCode = pAudioDesc->GetLanguageCode();
	pInfo->LanguageCode2 = pAudioDesc->GetLanguageCode2();
	pAudioDesc->GetText(pInfo->szText, CTsAnalyzer::EventAudioInfo::MAX_TEXT);
}

bool CTsAnalyzer::GetEventAudioInfo(const int ServiceIndex, const int AudioIndex, EventAudioInfo *pInfo, bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[ServiceIndex].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);

		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc =
				GetAudioComponentDescByComponentTag(pDescBlock, m_ServiceList[ServiceIndex].AudioEsList[AudioIndex].ComponentTag);
			if (pAudioDesc != NULL) {
				AudioComponentDescToAudioInfo(pAudioDesc, pInfo);
				return true;
			}
		}
	}
	return false;
}


bool CTsAnalyzer::GetEventAudioList(const int ServiceIndex, EventAudioList *pList, const bool bNext)
{
	if (pList == NULL)
		return false;

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		pList->clear();
		for (WORD i = 0; i < pDescBlock->GetDescNum(); i++) {
			const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

			if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
				const CAudioComponentDesc *pAudioComponent = dynamic_cast<const CAudioComponentDesc*>(pDesc);

				if (pAudioComponent) {
					EventAudioInfo AudioInfo;

					AudioComponentDescToAudioInfo(pAudioComponent, &AudioInfo);
					pList->push_back(AudioInfo);
				}
			}
		}
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetEventContentNibble(const int ServiceIndex, EventContentNibble *pInfo, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock != NULL) {
		const CContentDesc *pContentDesc = dynamic_cast<const CContentDesc *>(pDescBlock->GetDescByTag(CContentDesc::DESC_TAG));

		if (pContentDesc) {
			pInfo->NibbleCount = pContentDesc->GetNibbleCount();
			for (int i = 0; i < pInfo->NibbleCount; i++)
				pContentDesc->GetNibble(i, &pInfo->NibbleList[i]);
			return true;
		}
	}
	return false;
}


int CTsAnalyzer::GetEventExtendedText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bUseEventGroup, const bool bNext)
{
	if (pszText == NULL || MaxLength < 1)
		return 0;

	pszText[0] = '\0';

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock == NULL)
		return 0;
	if (pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL) {
		if (!bUseEventGroup)
			return 0;

		// �C�x���g���L�̎Q�Ɛ悩������擾����
#if 1
		const CEventGroupDesc *pEventGroup = dynamic_cast<const CEventGroupDesc*>(pDescBlock->GetDescByTag(CEventGroupDesc::DESC_TAG));
		if (pEventGroup == NULL
				|| pEventGroup->GetGroupType() != CEventGroupDesc::GROUPTYPE_COMMON
				|| pEventGroup->GetEventNum() < 1)
			return 0;
		const WORD EventID = GetEventID(ServiceIndex, bNext);
		int i;
		// ���Ȃ̋L�q���Ȃ��ꍇ�͎Q�ƌ�
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)
					&& EventInfo.ServiceID == m_ServiceList[ServiceIndex].ServiceID
					&& EventInfo.EventID == EventID)
				return 0;
		}
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)) {
				int Index = GetServiceIndexByID(EventInfo.ServiceID);
				if (Index >= 0) {
					if (pEitTable->GetEventID(pEitTable->GetServiceIndexByID(EventInfo.ServiceID), bNext ? 1 : 0) != EventInfo.EventID
							|| (pDescBlock = GetHEitItemDesc(Index, bNext)) == NULL
							|| pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL)
						return 0;
					break;
				}
			}
		}
		if (i == pEventGroup->GetEventNum())
			return 0;
#else
		/*
			�Q�Ɛ�ɂ����C�x���g�O���[�v�L�q�q������������A
			�Q�ƌ��Ȃ̂Ɏ��Ȃ�Service_id��event_id���L�q���Ă������肷��̂�(�Ȃ�?)
			��肠�����擪�T�[�r�X���Q�Ɛ�ɂȂ��Ă���ꍇ�̂ݑz�肷��
		*/
		// �ǋL: �P�ɃC�x���g�L�q�q�̉�͂��ς������݂���
		WORD ServiceID;
		if (!GetFirstViewableServiceID(&ServiceID)
				|| ServiceID == m_ServiceList[ServiceIndex].ServiceID)
			return 0;
		pDescBlock = GetHEitItemDesc(GetServiceIndexByID(ServiceID), bNext);
		if (pDescBlock == NULL)
			return false;
		const CEventGroupDesc *pEventGroup = dynamic_cast<const CEventGroupDesc*>(pDescBlock->GetDescByTag(CEventGroupDesc::DESC_TAG));
		if (pEventGroup == NULL
				|| pEventGroup->GetGroupType() != CEventGroupDesc::GROUPTYPE_COMMON
				|| pEventGroup->GetEventNum() <= 1)
			return 0;
		const WORD EventID = GetEventID(ServiceIndex, bNext);
		int i;
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)
					&& EventInfo.ServiceID == m_ServiceList[ServiceIndex].ServiceID
					&& EventInfo.EventID == EventID) {
				break;
			}
		}
		if (i == pEventGroup->GetEventNum())
			return 0;
		int Index = GetServiceIndexByID(ServiceID);
		if (Index < 0
				|| (pDescBlock = GetHEitItemDesc(Index, bNext)) == NULL
				|| pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL)
			return 0;
#endif
	}

	std::vector<const CExtendedEventDesc *> DescList;
	for (int i = 0; i < pDescBlock->GetDescNum(); i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);
		if (pDesc != NULL && pDesc->GetTag() == CExtendedEventDesc::DESC_TAG) {
			const CExtendedEventDesc *pExtendedEvent = dynamic_cast<const CExtendedEventDesc *>(pDesc);
			if (pExtendedEvent != NULL) {
				DescList.push_back(pExtendedEvent);
			}
		}
	}
	if (DescList.size() == 0)
		return 0;

	// descriptor_number ���Ƀ\�[�g����
	for (int i = (int)DescList.size() - 2; i >= 0; i--) {
		const CExtendedEventDesc *pKey = DescList[i];
		int j;
		for (j = i + 1; j < (int)DescList.size() && DescList[j]->GetDescriptorNumber() < pKey->GetDescriptorNumber(); j++)
			DescList[j - 1] = DescList[j];
		DescList[j - 1] = pKey;
	}

	struct ItemInfo {
		BYTE DescriptorNumber;
		LPCTSTR pszDescription;
		int Data1Length;
		const BYTE *pData1;
		int Data2Length;
		const BYTE *pData2;
	};
	std::vector<ItemInfo> ItemList;
	for (int i = 0; i < (int)DescList.size(); i++) {
		const CExtendedEventDesc *pExtendedEvent = DescList[i];
		for (int j = 0; j < pExtendedEvent->GetItemCount(); j++) {
			const CExtendedEventDesc::ItemInfo *pItem = pExtendedEvent->GetItem(j);
			if (pItem == NULL)
				continue;
			if (pItem->szDescription[0] != '\0') {
				// �V�K����
				ItemInfo Item;
				Item.DescriptorNumber = pExtendedEvent->GetDescriptorNumber();
				Item.pszDescription = pItem->szDescription;
				Item.Data1Length = pItem->ItemLength;
				Item.pData1 = pItem->ItemChar;
				Item.Data2Length = 0;
				Item.pData2 = NULL;
				ItemList.push_back(Item);
			} else if (ItemList.size() > 0) {
				// �O�̍��ڂ̑���
				ItemInfo &Item = ItemList[ItemList.size() - 1];
				if (Item.DescriptorNumber == pExtendedEvent->GetDescriptorNumber() - 1
						&& Item.pData2 == NULL) {
					Item.Data2Length = pItem->ItemLength;
					Item.pData2 = pItem->ItemChar;
				}
			}
		}
	}

	TCHAR szText[1024];
	int Length;
	int Pos = 0;
	for (int i = 0; i < (int)ItemList.size(); i++) {
		ItemInfo &Item = ItemList[i];
		Length = ::lstrlen(Item.pszDescription);
		if (Length + 2 >= MaxLength - Pos)
			break;
		::lstrcpy(&pszText[Pos], Item.pszDescription);
		Pos += Length;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
		if (Item.pData2 == NULL) {
			CAribString::AribToString(szText, 1024, Item.pData1, Item.Data1Length);
		} else {
			BYTE Buffer[220 * 2];
			::CopyMemory(Buffer, Item.pData1, Item.Data1Length);
			::CopyMemory(Buffer + Item.Data1Length, Item.pData2, Item.Data2Length);
			CAribString::AribToString(szText, 1024, Buffer, Item.Data1Length + Item.Data2Length);
		}
		LPTSTR p = szText;
		while (*p != '\0') {
			if (Pos >= MaxLength - 1)
				break;
			pszText[Pos++] = *p;
			if (*p == '\r') {
				if (*(p + 1) != '\n') {
					if (Pos == MaxLength - 1)
						break;
					pszText[Pos++] = '\n';
				}
			}
			p++;
		}
		if (Pos + 2 >= MaxLength)
			break;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
	}
	pszText[Pos] = '\0';

	return Pos;
}


bool CTsAnalyzer::GetEventSeriesInfo(const int ServiceIndex, EventSeriesInfo *pInfo, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock != NULL) {
		const CSeriesDesc *pSeriesDesc = dynamic_cast<const CSeriesDesc *>(pDescBlock->GetDescByTag(CSeriesDesc::DESC_TAG));

		if (pSeriesDesc) {
			pInfo->SeriesID = pSeriesDesc->GetSeriesID();
			pInfo->RepeatLabel = pSeriesDesc->GetRepeatLabel();
			pInfo->ProgramPattern = pSeriesDesc->GetProgramPattern();
			pInfo->bIsExpireDateValid = pSeriesDesc->IsExpireDateValid()
				&& pSeriesDesc->GetExpireDate(&pInfo->ExpireDate);
			pInfo->EpisodeNumber = pSeriesDesc->GetEpisodeNumber();
			pInfo->LastEpisodeNumber = pSeriesDesc->GetLastEpisodeNumber();
			pSeriesDesc->GetSeriesName(pInfo->szSeriesName, CSeriesDesc::MAX_SERIES_NAME);
			return true;
		}
	}
	return false;
}


bool CTsAnalyzer::GetEventInfo(const int ServiceIndex, EventInfo *pInfo, const bool bUseEventGroup, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex < 0 || (size_t)ServiceIndex >= m_ServiceList.size())
		return false;

	const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));
#ifdef TS_ANALYZER_L_EIT_SUPPORT
	if (pEitTable == NULL)
		pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));
#endif
	if (pEitTable == NULL)
		return false;

	const int Service = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);
	if (Service < 0)
		return false;

	const DWORD EventIndex = bNext ? 1 : 0;

	pInfo->EventID = pEitTable->GetEventID(Service, EventIndex);
	const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Service, EventIndex);
	if (pStartTime != NULL) {
		pInfo->bValidStartTime = true;
		pInfo->StartTime = *pStartTime;
	} else {
		pInfo->bValidStartTime = false;
		::ZeroMemory(&pInfo->StartTime, sizeof(SYSTEMTIME));
	}
	pInfo->Duration = pEitTable->GetDuration(Service, EventIndex);
	pInfo->RunningStatus = pEitTable->GetRunningStatus(Service, EventIndex);
	pInfo->bFreeCaMode = pEitTable->GetFreeCaMode(Service, EventIndex);
	if (pInfo->pszEventName != NULL && pInfo->MaxEventName > 0) {
		pInfo->pszEventName[0] = '\0';
		GetEventName(ServiceIndex, pInfo->pszEventName, pInfo->MaxEventName, bNext);
	}
	if (pInfo->pszEventText != NULL && pInfo->MaxEventText > 0) {
		pInfo->pszEventText[0] = '\0';
		GetEventText(ServiceIndex, pInfo->pszEventText, pInfo->MaxEventText, bNext);
	}
	if (pInfo->pszEventExtendedText != NULL && pInfo->MaxEventExtendedText > 0) {
		pInfo->pszEventExtendedText[0] = '\0';
		GetEventExtendedText(ServiceIndex, pInfo->pszEventExtendedText, pInfo->MaxEventExtendedText, bUseEventGroup, bNext);
	}

	::ZeroMemory(&pInfo->Video, sizeof(EventVideoInfo));
	GetEventVideoInfo(ServiceIndex, &pInfo->Video, bNext);

	pInfo->Audio.clear();
	GetEventAudioList(ServiceIndex, &pInfo->Audio, bNext);

	if (!GetEventContentNibble(ServiceIndex, &pInfo->ContentNibble, bNext))
		pInfo->ContentNibble.NibbleCount = 0;

	return true;
}


const CDescBlock *CTsAnalyzer::GetHEitItemDesc(const int ServiceIndex, const bool bNext) const
{
	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_HEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}


#ifdef TS_ANALYZER_L_EIT_SUPPORT
const CDescBlock *CTsAnalyzer::GetLEitItemDesc(const int ServiceIndex, const bool bNext) const
{
	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CEitPfTable *pEitTable = dynamic_cast<const CEitPfTable*>(m_PidMapManager.GetMapTarget(PID_LEIT));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}
#endif


const CAudioComponentDesc *CTsAnalyzer::GetAudioComponentDescByComponentTag(const CDescBlock *pDescBlock, const BYTE ComponentTag)
{
	if (pDescBlock != NULL) {
		for (WORD i = 0; i < pDescBlock->GetDescNum(); i++) {
			const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

			if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
				const CAudioComponentDesc *pAudioDesc = dynamic_cast<const CAudioComponentDesc*>(pDesc);

				if (pAudioDesc != NULL
						&& pAudioDesc->GetComponentTag() == ComponentTag)
					return pAudioDesc;
			}
		}
	}
	return NULL;
}


#endif	// TS_ANALYZER_EIT_SUPPORT


bool CTsAnalyzer::GetTotTime(SYSTEMTIME *pTime)
{
	if (pTime == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CTotTable *pTotTable = dynamic_cast<const CTotTable*>(m_PidMapManager.GetMapTarget(0x0014));
	if (pTotTable)
		return pTotTable->GetDateTime(pTime);
	return false;
}


bool CTsAnalyzer::GetSatelliteDeliverySystemList(SatelliteDeliverySystemList *pList)
{
	if (pList == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->clear();

	const CNitTable *pNitTable = dynamic_cast<const CNitTable*>(m_PidMapManager.GetMapTarget(PID_NIT));
	if (pNitTable == NULL)
		return false;

	for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
		const CDescBlock *pDescBlock = pNitTable->GetItemDesc(i);
		if (pDescBlock) {
			const CSatelliteDeliverySystemDesc *pSatelliteDesc =
				dynamic_cast<const CSatelliteDeliverySystemDesc *>(pDescBlock->GetDescByTag(CSatelliteDeliverySystemDesc::DESC_TAG));
			if (pSatelliteDesc) {
				SatelliteDeliverySystemInfo Info;

				Info.TransportStreamID = pNitTable->GetTransportStreamID(i);
				Info.Frequency = pSatelliteDesc->GetFrequency();
				Info.OrbitalPosition = pSatelliteDesc->GetOrbitalPosition();
				Info.bWestEastFlag = pSatelliteDesc->GetWestEastFlag();
				Info.Polarization = pSatelliteDesc->GetPolarization();
				Info.Modulation = pSatelliteDesc->GetModulation();
				Info.SymbolRate = pSatelliteDesc->GetSymbolRate();
				Info.FECInner = pSatelliteDesc->GetFECInner();
				pList->push_back(Info);
			}
		}
	}

	return pList->size() > 0;
}


bool CTsAnalyzer::GetTerrestrialDeliverySystemList(TerrestrialDeliverySystemList *pList)
{
	if (pList == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pList->clear();

	const CNitTable *pNitTable = dynamic_cast<const CNitTable*>(m_PidMapManager.GetMapTarget(PID_NIT));
	if (pNitTable == NULL)
		return false;

	for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
		const CDescBlock *pDescBlock = pNitTable->GetItemDesc(i);
		if (pDescBlock) {
			const CTerrestrialDeliverySystemDesc *pTerrestrialDesc =
				dynamic_cast<const CTerrestrialDeliverySystemDesc *>(pDescBlock->GetDescByTag(CTerrestrialDeliverySystemDesc::DESC_TAG));
			if (pTerrestrialDesc) {
				TerrestrialDeliverySystemInfo Info;

				Info.TransportStreamID = pNitTable->GetTransportStreamID(i);
				Info.AreaCode = pTerrestrialDesc->GetAreaCode();
				Info.GuardInterval = pTerrestrialDesc->GetGuardInterval();
				Info.TransmissionMode = pTerrestrialDesc->GetTransmissionMode();
				Info.Frequency.resize(pTerrestrialDesc->GetFrequencyNum());
				for (int j = 0; j < (int)Info.Frequency.size(); j++)
					Info.Frequency[j] = pTerrestrialDesc->GetFrequency(j);
				pList->push_back(Info);
			}
		}
	}

	return pList->size() > 0;
}


bool CTsAnalyzer::AddEventHandler(IAnalyzerEventHandler *pHandler)
{
	if (pHandler == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	m_EventHandlerList.push_back(pHandler);

	return true;
}


bool CTsAnalyzer::RemoveEventHandler(IAnalyzerEventHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	for (std::vector<IAnalyzerEventHandler*>::iterator itr = m_EventHandlerList.begin(); itr != m_EventHandlerList.end(); itr++) {
		if (*itr == pHandler) {
			m_EventHandlerList.erase(itr);
			return true;
		}
	}
	return false;
}


void CTsAnalyzer::CallEventHandler(EventType Type)
{
	for (size_t i = 0; i < m_EventHandlerList.size(); i++) {
		m_EventHandlerList[i]->OnEvent(this, Type);
	}

#if 0
	//SendDecoderEvent((DWORD)Type);
#else
#ifdef _DEBUG
	if (m_DecoderEvent != EVENT_INVALID) ::DebugBreak();
#endif
	m_DecoderEvent = Type;
#endif
}


void CTsAnalyzer::NotifyResetEvent()
{
	for (size_t i = 0; i < m_EventHandlerList.size(); i++) {
		m_EventHandlerList[i]->OnReset(this);
	}
}


void CALLBACK CTsAnalyzer::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CTsAnalyzer::OnPatUpdated()\n"));

	// PAT���X�V���ꂽ
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == NULL)
		return;

	// �g�����X�|�[�g�X�g���[��ID�X�V
	pThis->m_TransportStreamID = pPatTable->m_CurSection.GetTableIdExtension();

	// ��PMT/PCR��PID���A���}�b�v����
	for (size_t Index = 0; Index < pThis->m_ServiceList.size(); Index++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PmtPID);
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PcrPID);
	}

	// �VPMT���X�g�A����
	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t Index = 0; Index < pThis->m_ServiceList.size(); Index++) {
		// �T�[�r�X���X�g�X�V
		pThis->m_ServiceList[Index].bIsUpdated = false;
		pThis->m_ServiceList[Index].ServiceID = pPatTable->GetProgramID((WORD)Index);
		pThis->m_ServiceList[Index].PmtPID = pPatTable->GetPmtPID((WORD)Index);
		pThis->m_ServiceList[Index].VideoStreamType = STREAM_TYPE_INVALID;
		pThis->m_ServiceList[Index].VideoEs.PID = PID_INVALID;
		pThis->m_ServiceList[Index].VideoEs.ComponentTag = COMPONENTTAG_INVALID;
		pThis->m_ServiceList[Index].AudioEsList.clear();
		pThis->m_ServiceList[Index].CaptionEsList.clear();
		pThis->m_ServiceList[Index].DataCarrouselEsList.clear();
		pThis->m_ServiceList[Index].PcrPID = PID_INVALID;
		pThis->m_ServiceList[Index].EcmPID = PID_INVALID;
		pThis->m_ServiceList[Index].RunningStatus = 0xFF;
		pThis->m_ServiceList[Index].bIsCaService = false;
		pThis->m_ServiceList[Index].szServiceName[0] = '\0';
		pThis->m_ServiceList[Index].ServiceType = SERVICE_TYPE_INVALID;
		pThis->m_ServiceList[Index].LogoID = LOGOID_INVALID;

		// PMT��PID���}�b�v
		pMapManager->MapTarget(pPatTable->GetPmtPID((WORD)Index), new CPmtTable(TABLE_DEBUG), OnPmtUpdated, pParam);
	}

	// �C�x���g�n���h���Ăяo��
	pThis->CallEventHandler(EVENT_PAT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CTsAnalyzer::OnPmtUpdated()\n"));

	// PMT���X�V���ꂽ
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == NULL)
		return;

	// �T�[�r�X�C���f�b�N�X������
	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	// ES��PID���X�g�A
	Info.VideoStreamType = 0xFF;
	Info.VideoEs.PID = PID_INVALID;
	Info.VideoEs.ComponentTag = COMPONENTTAG_INVALID;
	Info.AudioEsList.clear();
	Info.CaptionEsList.clear();
	Info.DataCarrouselEsList.clear();
	for (WORD EsIndex = 0; EsIndex < pPmtTable->GetEsInfoNum(); EsIndex++) {
		const BYTE StreamType = pPmtTable->GetStreamTypeID(EsIndex);
		const WORD EsPID = pPmtTable->GetEsPID(EsIndex);

		BYTE ComponentTag = COMPONENTTAG_INVALID;
		const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);
		if (pDescBlock) {
			const CStreamIdDesc *pStreamIdDesc = dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

			if (pStreamIdDesc)
				ComponentTag = pStreamIdDesc->GetComponentTag();
		}

		switch (StreamType) {
		case STREAM_TYPE_MPEG2:
			// ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2
			if (Info.VideoEs.PID == PID_INVALID
					|| Info.VideoStreamType != 0x02) {
				Info.VideoStreamType = StreamType;
				Info.VideoEs.PID = EsPID;
				Info.VideoEs.ComponentTag = ComponentTag;
			}
			break;

		case STREAM_TYPE_CAPTION:
			// ITU-T Rec.H.222 | ISO/IEC 13818-1
			Info.CaptionEsList.push_back(EsInfo(EsPID, ComponentTag));
			break;

		case STREAM_TYPE_DATACARROUSEL:
			Info.DataCarrouselEsList.push_back(EsInfo(EsPID, ComponentTag));
			break;

		case STREAM_TYPE_AAC:
			// ISO/IEC 13818-7 Audio (ADTS Transport Syntax)
			Info.AudioEsList.push_back(EsInfo(EsPID, ComponentTag));
			break;

		case STREAM_TYPE_H264:
			// ITU-T Rec.H.264 | ISO/IEC 14496-10Video
			if (Info.VideoEs.PID == PID_INVALID) {
				Info.VideoStreamType = StreamType;
				Info.VideoEs.PID = EsPID;
				Info.VideoEs.ComponentTag = ComponentTag;
			}
			break;
		}
	}

	WORD PcrPID = pPmtTable->GetPcrPID();
	if (PcrPID < 0x1FFFU) {
		Info.PcrPID = PcrPID;
		CTsPidMapTarget *pMap = pMapManager->GetMapTarget(PcrPID);
		if (!pMap) {
			// �V�KMap
			pMapManager->MapTarget(PcrPID, new CPcrTable(ServiceIndex), OnPcrUpdated, pParam);
		} else {
			// ����Map
			CPcrTable *pPcrTable = dynamic_cast<CPcrTable*>(pMap);
			if(pPcrTable) {
				// �T�[�r�X�ǉ�
				pPcrTable->AddServiceIndex(ServiceIndex);
			}
		}
	}

	Info.EcmPID = pPmtTable->GetEcmPID();

	// �X�V�ς݃}�[�N
	Info.bIsUpdated = true;

	// SDT�e�[�u�����ă}�b�v����
	pMapManager->MapTarget(PID_SDT, new CSdtTable, OnSdtUpdated, pParam);

	// �C�x���g�n���h���Ăяo��
	pThis->CallEventHandler(EVENT_PMT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CTsAnalyzer::OnSdtUpdated()\n"));

	// SDT���X�V���ꂽ
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CSdtTable *pSdtTable = dynamic_cast<CSdtTable *>(pMapTarget);
	if (pSdtTable == NULL)
		return;

	pThis->m_NetworkID = pSdtTable->GetNetworkID();

	for (WORD SdtIndex = 0 ; SdtIndex < pSdtTable->GetServiceNum() ; SdtIndex++) {
		// �T�[�r�XID������
		const int ServiceIndex = pThis->GetServiceIndexByID(pSdtTable->GetServiceID(SdtIndex));
		if (ServiceIndex < 0)
			continue;

		// �T�[�r�X���X�V
		pThis->m_ServiceList[ServiceIndex].RunningStatus = pSdtTable->GetRunningStatus(SdtIndex);
		pThis->m_ServiceList[ServiceIndex].bIsCaService = pSdtTable->GetFreeCaMode(SdtIndex);

		// �T�[�r�X���X�V
		pThis->m_ServiceList[ServiceIndex].szServiceName[0] = '\0';

		const CDescBlock *pDescBlock = pSdtTable->GetItemDesc(SdtIndex);

		const CServiceDesc *pServiceDesc = dynamic_cast<const CServiceDesc *>(pDescBlock->GetDescByTag(CServiceDesc::DESC_TAG));
		if (pServiceDesc) {
			pServiceDesc->GetServiceName(pThis->m_ServiceList[ServiceIndex].szServiceName, 256);
			pThis->m_ServiceList[ServiceIndex].ServiceType = pServiceDesc->GetServiceType();
		}

		const CLogoTransmissionDesc *pLogoDesc = dynamic_cast<const CLogoTransmissionDesc*>(pDescBlock->GetDescByTag(CLogoTransmissionDesc::DESC_TAG));
		if (pLogoDesc) {
			pThis->m_ServiceList[ServiceIndex].LogoID = pLogoDesc->GetLogoID();
		}
	}

	// �C�x���g�n���h���Ăяo��
	pThis->CallEventHandler(EVENT_SDT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam){

	TRACE(TEXT("CTsAnalyzer::OnNitUpdated()\n"));

	CTsAnalyzer *pThis = static_cast<CTsAnalyzer*>(pParam);
	CNitTable *pNitTable = dynamic_cast<CNitTable*>(pMapTarget);
	if (pNitTable == NULL)
		return;

	pThis->m_NetworkID = pNitTable->GetNetworkID();

	const CDescBlock *pDescBlock;
	pDescBlock = pNitTable->GetNetworkDesc();
	if (pDescBlock) {
		const CNetworkNameDesc *pNetworkDesc =
			dynamic_cast<const CNetworkNameDesc *>(pDescBlock->GetDescByTag(CNetworkNameDesc::DESC_TAG));
		if(pNetworkDesc) {
			pNetworkDesc->GetNetworkName(pThis->m_NitInfo.szNetworkName,
										 sizeof(pThis->m_NitInfo.szNetworkName) / sizeof(TCHAR));
		}
		const CSystemManageDesc *pSysManageDesc =
			dynamic_cast<const CSystemManageDesc *>(pDescBlock->GetDescByTag(CSystemManageDesc::DESC_TAG));
		if (pSysManageDesc) {
			pThis->m_NitInfo.BroadcastingID = pSysManageDesc->GetBroadcastingID();
		}
	}

	pDescBlock = pNitTable->GetItemDesc(0);
	if (pDescBlock) {
		const CTSInfoDesc *pTsInfoDesc =
			dynamic_cast<const CTSInfoDesc *>(pDescBlock->GetDescByTag(CTSInfoDesc::DESC_TAG));
		if (pTsInfoDesc) {
			pTsInfoDesc->GetTSName(pThis->m_NitInfo.szTSName,
								   sizeof(pThis->m_NitInfo.szTSName) / sizeof(TCHAR));
			pThis->m_NitInfo.RemoteControlKeyID = pTsInfoDesc->GetRemoteControlKeyID();
		}
	}

	for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
		pDescBlock = pNitTable->GetItemDesc(i);
		if (pDescBlock) {
			const CServiceListDesc *pServiceListDesc =
				dynamic_cast<const CServiceListDesc*>(pDescBlock->GetDescByTag(CServiceListDesc::DESC_TAG));
			if (pServiceListDesc) {
				for (int j = 0; j < pServiceListDesc->GetServiceNum(); j++) {
					CServiceListDesc::ServiceInfo Info;
					if (pServiceListDesc->GetServiceInfo(j, &Info)) {
						int Index = pThis->GetServiceIndexByID(Info.ServiceID);
						if (Index >= 0) {
							ServiceInfo &Service = pThis->m_ServiceList[Index];
							if (Service.ServiceType == SERVICE_TYPE_INVALID)
								Service.ServiceType = Info.ServiceType;
						}
					}
				}
			}
		}
	}

	// �C�x���g�n���h���Ăяo��
	pThis->CallEventHandler(EVENT_NIT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PCR���X�V���ꂽ
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPcrTable *pPcrTable = dynamic_cast<CPcrTable *>(pMapTarget);
	if (pPcrTable == NULL)
		return;

	const ULONGLONG TimeStamp = pPcrTable->GetPcrTimeStamp();

	WORD ServiceIndex;
	for (WORD Index = 0; pPcrTable->GetServiceIndex(&ServiceIndex, Index); Index++) {
		if (ServiceIndex < pThis->m_ServiceList.size()) {
			pThis->m_ServiceList[ServiceIndex].PcrTimeStamp = TimeStamp;
		}
	}

	// �C�x���g�n���h���Ăяo��
	pThis->CallEventHandler(EVENT_PCR_UPDATED);
}




CTsAnalyzer::IAnalyzerEventHandler::IAnalyzerEventHandler()
{
}


CTsAnalyzer::IAnalyzerEventHandler::~IAnalyzerEventHandler()
{
}


void CTsAnalyzer::IAnalyzerEventHandler::OnReset(CTsAnalyzer *pAnalyzer)
{
}
