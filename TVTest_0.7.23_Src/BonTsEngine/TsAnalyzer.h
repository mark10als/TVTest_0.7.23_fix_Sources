#pragma once

#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsDescriptor.h"

// EIT �̉�͂��s��
#define TS_ANALYZER_EIT_SUPPORT
#ifdef BONTSENGINE_1SEG_SUPPORT
// L-EIT[p/f] �̉�͂��s��
#define TS_ANALYZER_L_EIT_SUPPORT
#endif


// TS ��̓N���X
class CTsAnalyzer : public CMediaDecoder
{
public:
	enum {
		PID_INVALID = 0xFFFF,
		COMPONENTTAG_INVALID = 0xFF,
		LOGOID_INVALID = 0xFFFF
	};

	struct EsInfo {
		WORD PID;
		BYTE ComponentTag;
		EsInfo() : PID(PID_INVALID), ComponentTag(COMPONENTTAG_INVALID) {}
		EsInfo(WORD pid, BYTE Tag) : PID(pid), ComponentTag(Tag) {}
	};

	struct ServiceInfo {
		bool bIsUpdated;
		WORD ServiceID;
		WORD PmtPID;
		BYTE VideoStreamType;
		EsInfo VideoEs;
		std::vector<EsInfo> AudioEsList;
		std::vector<EsInfo> CaptionEsList;
		std::vector<EsInfo> DataCarrouselEsList;
		WORD PcrPID;
		ULONGLONG PcrTimeStamp;
		WORD EcmPID;
		BYTE RunningStatus;
		bool bIsCaService;
		TCHAR szServiceName[256];
		BYTE ServiceType;
		WORD LogoID;
	};

	typedef std::vector<ServiceInfo> ServiceList;

	CTsAnalyzer(IEventHandler *pEventHandler = NULL);
	virtual ~CTsAnalyzer();

// CMediaDecoder
	virtual void Reset();
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsAnalyzer
	WORD GetServiceNum();
	bool GetServiceID(const int Index, WORD *pServiceID);
	int GetServiceIndexByID(const WORD ServiceID);
	bool IsViewableService(const int Index);
	WORD GetViewableServiceNum();
	bool GetViewableServiceID(const int Index, WORD *pServiceID);
	bool GetFirstViewableServiceID(WORD *pServiceID);
	int GetViewableServiceIndexByID(const WORD ServiceID);
	bool GetServiceInfo(const int Index, ServiceInfo *pInfo);
	bool IsServiceUpdated(const int Index);
	bool GetPmtPID(const int Index, WORD *pPmtPID);
	bool GetVideoEsPID(const int Index, WORD *pVideoPID);
	bool GetVideoStreamType(const int Index, BYTE *pStreamType);
	BYTE GetVideoComponentTag(const int Index);
	WORD GetAudioEsNum(const int Index);
	bool GetAudioEsPID(const int Index, const int AudioIndex, WORD *pAudioPID);
	BYTE GetAudioComponentTag(const int Index, const int AudioIndex);
#ifdef TS_ANALYZER_EIT_SUPPORT
	BYTE GetVideoComponentType(const int Index);
	int GetAudioIndexByComponentTag(const int Index, const BYTE ComponentTag);
	BYTE GetAudioComponentType(const int Index, const int AudioIndex);
	int GetAudioComponentText(const int Index, const int AudioIndex, LPTSTR pszText, int MaxLength);
#endif
	WORD GetCaptionEsNum(const int Index);
	bool GetCaptionEsPID(const int Index, const WORD CaptionIndex, WORD *pCaptionPID);
	WORD GetDataCarrouselEsNum(const int Index);
	bool GetDataCarrouselEsPID(const int Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID);
	bool GetPcrPID(const int Index, WORD *pPcrPID);
	bool GetPcrTimeStamp(const int Index, ULONGLONG *pTimeStamp);
	bool GetEcmPID(const int Index, WORD *pEcmPID);
	int GetServiceName(const int Index, LPTSTR pszName, const int MaxLength);
	BYTE GetServiceType(const int Index);
	WORD GetLogoID(const int Index);

	bool GetServiceList(ServiceList *pList);
	bool GetViewableServiceList(ServiceList *pList);

	WORD GetTransportStreamID() const;
	WORD GetNetworkID() const;
	BYTE GetBroadcastingID() const;
	int GetNetworkName(LPTSTR pszName, int MaxLength);
	BYTE GetRemoteControlKeyID() const;
	int GetTsName(LPTSTR pszName, int MaxLength);

#ifdef TS_ANALYZER_EIT_SUPPORT
	WORD GetEventID(const int ServiceIndex, const bool bNext = false);
	bool GetEventStartTime(const int ServiceIndex, SYSTEMTIME *pSystemTime, const bool bNext = false);
	DWORD GetEventDuration(const int ServiceIndex, const bool bNext = false);
	int GetEventName(const int ServiceIndex, LPTSTR pszName, int MaxLength, const bool bNext = false);
	int GetEventText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bNext = false);
	int GetEventExtendedText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bUseEventGroup = true, const bool bNext = false);
	struct EventSeriesInfo {
		WORD SeriesID;
		BYTE RepeatLabel;
		BYTE ProgramPattern;
		bool bIsExpireDateValid;
		SYSTEMTIME ExpireDate;
		WORD EpisodeNumber;
		WORD LastEpisodeNumber;
		TCHAR szSeriesName[CSeriesDesc::MAX_SERIES_NAME];
	};
	bool GetEventSeriesInfo(const int ServiceIndex, EventSeriesInfo *pInfo, const bool bNext = false);
	struct EventVideoInfo {
		enum { MAX_TEXT = 64 };
		BYTE StreamContent;
		BYTE ComponentType;
		BYTE ComponentTag;
		DWORD LanguageCode;
		TCHAR szText[MAX_TEXT];
	};
	struct EventAudioInfo {
		enum { MAX_TEXT = 64 };
		BYTE StreamContent;
		BYTE ComponentType;
		BYTE ComponentTag;
		BYTE SimulcastGroupTag;
		bool bESMultiLingualFlag;
		bool bMainComponentFlag;
		BYTE QualityIndicator;
		BYTE SamplingRate;
		DWORD LanguageCode;
		DWORD LanguageCode2;
		TCHAR szText[MAX_TEXT];
	};
	typedef std::vector<EventAudioInfo> EventAudioList;
	struct EventContentNibble {
		int NibbleCount;
		CContentDesc::Nibble NibbleList[7];
	};
	struct EventInfo {
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		BYTE RunningStatus;
		bool bFreeCaMode;
		LPTSTR pszEventName;
		int MaxEventName;
		LPTSTR pszEventText;
		int MaxEventText;
		LPTSTR pszEventExtendedText;
		int MaxEventExtendedText;
		EventVideoInfo Video;
		EventAudioList Audio;
		EventContentNibble ContentNibble;
	};
	bool GetEventVideoInfo(const int ServiceIndex, EventVideoInfo *pInfo, const bool bNext = false);
	bool GetEventAudioInfo(const int ServiceIndex, const int AudioIndex, EventAudioInfo *pInfo, bool bNext = false);
	bool GetEventAudioList(const int ServiceIndex, EventAudioList *pList, const bool bNext = false);
	bool GetEventContentNibble(const int ServiceIndex, EventContentNibble *pInfo, const bool bNext = false);
	bool GetEventInfo(const int ServiceIndex, EventInfo *pInfo, const bool bUseEventGroup = true, const bool bNext = false);
#endif	// TS_ANALYZER_EIT_SUPPORT

	bool GetTotTime(SYSTEMTIME *pTime);

	struct SatelliteDeliverySystemInfo {
		WORD TransportStreamID;
		DWORD Frequency;
		WORD OrbitalPosition;
		bool bWestEastFlag;
		BYTE Polarization;
		BYTE Modulation;
		DWORD SymbolRate;
		BYTE FECInner;
	};
	struct TerrestrialDeliverySystemInfo {
		WORD TransportStreamID;
		WORD AreaCode;
		BYTE GuardInterval;
		BYTE TransmissionMode;
		std::vector<WORD> Frequency;
	};
	typedef std::vector<SatelliteDeliverySystemInfo> SatelliteDeliverySystemList;
	typedef std::vector<TerrestrialDeliverySystemInfo> TerrestrialDeliverySystemList;
	bool GetSatelliteDeliverySystemList(SatelliteDeliverySystemList *pList);
	bool GetTerrestrialDeliverySystemList(TerrestrialDeliverySystemList *pList);

	enum EventType {
		EVENT_PAT_UPDATED,
		EVENT_PMT_UPDATED,
		EVENT_SDT_UPDATED,
		EVENT_NIT_UPDATED,
		EVENT_PCR_UPDATED,
		EVENT_INVALID,
		NUM_EVENTS = EVENT_INVALID,
		EVENT_LAST = NUM_EVENTS - 1
	};
	class IAnalyzerEventHandler {
	public:
		IAnalyzerEventHandler();
		virtual ~IAnalyzerEventHandler();
		virtual void OnEvent(CTsAnalyzer *pAnalyzer, EventType Type) = 0;
		virtual void OnReset(CTsAnalyzer *pAnalyzer);
	};
	bool AddEventHandler(IAnalyzerEventHandler *pHandler);
	bool RemoveEventHandler(IAnalyzerEventHandler *pHandler);

protected:
	void CallEventHandler(EventType Type);
	void NotifyResetEvent();

#ifdef TS_ANALYZER_EIT_SUPPORT
	const CDescBlock *GetHEitItemDesc(const int ServiceIndex, const bool bNext = false) const;
#ifdef TS_ANALYZER_L_EIT_SUPPORT
	const CDescBlock *GetLEitItemDesc(const int ServiceIndex, const bool bNext = false) const;
#endif
	const CAudioComponentDesc *GetAudioComponentDescByComponentTag(const CDescBlock *pDescBlock, const BYTE ComponentTag);
#endif

	CTsPidMapManager m_PidMapManager;

	ServiceList m_ServiceList;
	WORD m_TransportStreamID;
	WORD m_NetworkID;

	struct NitInfo {
		BYTE BroadcastingID;
		BYTE RemoteControlKeyID;
		TCHAR szNetworkName[32];
		TCHAR szTSName[32];
	};
	NitInfo m_NitInfo;

	std::vector<IAnalyzerEventHandler*> m_EventHandlerList;
	EventType m_DecoderEvent;

private:
	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};
