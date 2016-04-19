#pragma once


#include <map>
#include <set>
#include <vector>
#include "MediaDecoder.h"
#include "TsTable.h"


/*
	î‘ëgèÓïÒä«óùÉNÉâÉX
*/
class CEventManager : public CMediaDecoder
					, public CPsiStreamTable::ISectionHandler
{
public:
	class CEventInfo {
	public:
		struct VideoInfo {
			enum { MAX_TEXT = 64 };
			BYTE StreamContent;
			BYTE ComponentType;
			BYTE ComponentTag;
			DWORD LanguageCode;
			TCHAR szText[MAX_TEXT];

			VideoInfo()
				: StreamContent(0)
				, ComponentType(0)
				, ComponentTag(0)
				, LanguageCode(0)
			{
				szText[0]='\0';
			}

			bool operator==(const VideoInfo &Op) const
			{
				return StreamContent == Op.StreamContent
					&& ComponentType == Op.ComponentType
					&& ComponentTag == Op.ComponentTag
					&& LanguageCode == Op.LanguageCode
					&& ::lstrcmp(szText, Op.szText) == 0;
			}

			bool operator!=(const VideoInfo &Op) const
			{
				return !(*this==Op);
			}
		};

		struct AudioInfo {
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

			AudioInfo() { szText[0]='\0'; }

			bool operator==(const AudioInfo &Op) const
			{
				return StreamContent == Op.StreamContent
					&& ComponentType == Op.ComponentType
					&& ComponentTag == Op.ComponentTag
					&& SimulcastGroupTag == Op.SimulcastGroupTag
					&& bESMultiLingualFlag == Op.bESMultiLingualFlag
					&& bMainComponentFlag == Op.bMainComponentFlag
					&& QualityIndicator == Op.QualityIndicator
					&& SamplingRate == Op.SamplingRate
					&& LanguageCode == Op.LanguageCode
					&& LanguageCode2 == Op.LanguageCode2
					&& ::lstrcmp(szText, Op.szText) == 0;
			}

			bool operator!=(const AudioInfo &Op) const
			{
				return !(*this == Op);
			}
		};
		typedef std::vector<AudioInfo> AudioList;

		struct ContentNibble {
			int NibbleCount;
			CContentDesc::Nibble NibbleList[7];

			ContentNibble() : NibbleCount(0) {}

			bool operator==(const ContentNibble &Operand) const
			{
				if (NibbleCount == Operand.NibbleCount) {
					for (int i = 0; i < NibbleCount; i++) {
						if (NibbleList[i] != Operand.NibbleList[i])
							return false;
					}
					return true;
				}
				return false;
			}

			bool operator!=(const ContentNibble &Operand) const
			{
				return !(*this == Operand);
			}
		};

		struct EventGroupInfo {
			BYTE GroupType;
			std::vector<CEventGroupDesc::EventInfo> EventList;

			bool operator==(const EventGroupInfo &Operand) const
			{
				return GroupType == Operand.GroupType
					&& EventList == Operand.EventList;
			}

			bool operator!=(const EventGroupInfo &Operand) const
			{
				return !(*this == Operand);
			}
		};
		typedef std::vector<EventGroupInfo> EventGroupList;

		struct CommonEventInfo {
			WORD ServiceID;
			WORD EventID;

			bool operator==(const CommonEventInfo &Op) const
			{
				return ServiceID==Op.ServiceID
					&& EventID==Op.EventID;
			}

			bool operator!=(const CommonEventInfo &Op) const
			{
				return !(*this==Op);
			}
		};

		CEventInfo();
		CEventInfo(const CEventInfo &Src);
		~CEventInfo();
		CEventInfo &operator=(const CEventInfo &Src);
		ULONGLONG GetUpdateTime() const { return m_UpdateTime; }
		WORD GetEventID() const { return m_EventID; }
		const SYSTEMTIME &GetStartTime() const { return m_StartTime; }
		DWORD GetDuration() const { return m_Duration; }
		bool GetEndTime(SYSTEMTIME *pTime) const;
		BYTE GetRunningStatus() const { return m_RunningStatus; }
		bool GetFreeCaMode() const { return m_bFreeCaMode; }
		LPCTSTR GetEventName() const { return m_pszEventName; }
		void SetEventName(LPCTSTR pszEventName);
		LPCTSTR GetEventText() const { return m_pszEventText; }
		void SetEventText(LPCTSTR pszEventText);
		LPCTSTR GetEventExtendedText() const { return m_pszEventExtendedText; }
		void SetEventExtendedText(LPCTSTR pszEventExtendedText);
		const VideoInfo &GetVideoInfo() const { return m_VideoInfo; }
		const AudioList &GetAudioList() const { return m_AudioList; }
		const ContentNibble &GetContentNibble() const { return m_ContentNibble; }
		const EventGroupList &GetEventGroupList() const { return m_EventGroupList; }
		bool IsCommonEvent() const { return m_bCommonEvent; }
		const CommonEventInfo &GetCommonEvent() const { return m_CommonEventInfo; }

	private:
		ULONGLONG m_UpdateTime;
		WORD m_EventID;
		SYSTEMTIME m_StartTime;
		DWORD m_Duration;
		BYTE m_RunningStatus;
		bool m_bFreeCaMode;
		LPTSTR m_pszEventName;
		LPTSTR m_pszEventText;
		LPTSTR m_pszEventExtendedText;
		VideoInfo m_VideoInfo;
		AudioList m_AudioList;
		ContentNibble m_ContentNibble;
		EventGroupList m_EventGroupList;
		bool m_bCommonEvent;
		CommonEventInfo m_CommonEventInfo;
		void SetText(LPTSTR *ppszText, LPCTSTR pszNewText);
		friend class CEventManager;
	};
	typedef std::vector<CEventInfo> EventList;

	struct ServiceInfo {
		WORD OriginalNetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
	};
	typedef std::vector<ServiceInfo> ServiceList;

	CEventManager(IEventHandler *pEventHandler = NULL);
	virtual ~CEventManager();

// CMediaDecoder
	virtual void Reset();
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CEventManager
	void Clear();
	int GetServiceNum();
	bool GetServiceList(ServiceList *pList);
	bool IsServiceUpdated(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID);
	bool GetEventList(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID, EventList *pList);
	bool GetEventInfo(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
					  const WORD EventID, CEventInfo *pInfo);
	bool GetEventInfo(const WORD NetworkID, const WORD TransportStreamID, const WORD ServiceID,
					  const SYSTEMTIME *pTime, CEventInfo *pInfo);

protected:
	struct TimeEventInfo {
		ULONGLONG StartTime;
		DWORD Duration;
		WORD EventID;
		ULONGLONG UpdateTime;
		TimeEventInfo(const SYSTEMTIME *pStartTime);
		bool operator<(const TimeEventInfo &Obj) const {
			return StartTime < Obj.StartTime;
		}
	};

	typedef std::map<WORD, CEventInfo> EventMap;
	typedef std::set<TimeEventInfo> TimeEventMap;

	struct ServiceEventMap {
		ServiceInfo Info;
		EventMap EventMap;
		TimeEventMap TimeMap;
		bool bUpdated;
	};

	typedef ULONGLONG ServiceMapKey;
	typedef std::map<ServiceMapKey, ServiceEventMap> ServiceMap;
	static ServiceMapKey GetServiceMapKey(WORD OriginalNetworkID, WORD TransportStreamID, WORD ServiceID) {
		return ((ServiceMapKey)OriginalNetworkID << 32)
			| ((ServiceMapKey)TransportStreamID << 16)
			| (ServiceMapKey)ServiceID;
	}

	CTsPidMapManager m_PidMapManager;
	ServiceMap m_ServiceMap;
	ULONGLONG m_CurTotTime;

	static bool RemoveEvent(EventMap *pMap, const WORD EventID);

private:
// CPsiStreamTable::ISectionHandler
	void OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection);

	static void CALLBACK OnTotUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};
