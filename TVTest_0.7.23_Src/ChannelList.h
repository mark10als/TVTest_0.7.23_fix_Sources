#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H


#include <vector>


inline bool IsBSNetworkID(WORD NetworkID) { return NetworkID==4; }
inline bool IsCSNetworkID(WORD NetworkID) { return NetworkID>=6 && NetworkID<=10; }

enum NetworkType
{
	NETWORK_TERRESTRIAL,
	NETWORK_BS,
	NETWORK_CS
};

inline NetworkType GetNetworkType(WORD NetworkID)
{
	if (IsBSNetworkID(NetworkID))
		return NETWORK_BS;
	if (IsCSNetworkID(NetworkID))
		return NETWORK_CS;
	return NETWORK_TERRESTRIAL;
}


#define FIRST_UHF_CHANNEL 13
#define MAX_CHANNEL_NAME 64


class CChannelInfo
{
	int m_Space;						// チューニング空間
	int m_ChannelIndex;					// チャンネルインデックス(BonDriverでの番号)
	int m_ChannelNo;					// リモコンチャンネル番号
	int m_PhysicalChannel;				// 物理チャンネル番号
	TCHAR m_szName[MAX_CHANNEL_NAME];	// チャンネル名
	WORD m_NetworkID;					// ネットワークID
	WORD m_TransportStreamID;			// トランスポートストリームID
	WORD m_ServiceID;					// サービスID
	bool m_fEnabled;					// 有効

public:
	CChannelInfo(int Space,int ChannelIndex,int No,LPCTSTR pszName);
	CChannelInfo(const CChannelInfo &Info);
	virtual ~CChannelInfo() {}
	CChannelInfo &operator=(const CChannelInfo &Info);
	int GetSpace() const { return m_Space; }
	bool SetSpace(int Space);
	int GetChannelIndex() const { return m_ChannelIndex; }
	bool SetChannelIndex(int Channel);
	bool SetChannelNo(int ChannelNo);
	int GetChannelNo() const { return m_ChannelNo; }
	int GetPhysicalChannel() const { return m_PhysicalChannel; }
	bool SetPhysicalChannel(int Channel);
	LPCTSTR GetName() const { return m_szName; }
	bool SetName(LPCTSTR pszName);
	bool SetNetworkID(WORD NetworkID);
	WORD GetNetworkID() const { return m_NetworkID; }
	bool SetTransportStreamID(WORD TransportStreamID);
	WORD GetTransportStreamID() const { return m_TransportStreamID; }
	bool SetServiceID(WORD ServiceID);
	WORD GetServiceID() const { return m_ServiceID; }
	void Enable(bool fEnable) { m_fEnabled=fEnable; }
	bool IsEnabled() const { return m_fEnabled; }
};

class CChannelList
{
	std::vector<CChannelInfo*> m_ChannelList;

public:
	CChannelList();
	CChannelList(const CChannelList &Src);
	~CChannelList();
	CChannelList &operator=(const CChannelList &Src);
	int NumChannels() const { return (int)m_ChannelList.size(); }
	int NumEnableChannels() const;
	bool AddChannel(const CChannelInfo &Info);
	bool AddChannel(CChannelInfo *pInfo);
	CChannelInfo *GetChannelInfo(int Index);
	const CChannelInfo *GetChannelInfo(int Index) const;
	int GetSpace(int Index) const;
	int GetChannelIndex(int Index) const;
	int GetChannelNo(int Index) const;
	int GetPhysicalChannel(int Index) const;
	LPCTSTR GetName(int Index) const;
	bool IsEnabled(int Index) const;
	bool DeleteChannel(int Index);
	void Clear();
	int Find(const CChannelInfo *pInfo) const;
	int Find(int Space,int ChannelIndex,int ServiceID=-1) const;
	int FindPhysicalChannel(int Channel) const;
	int FindChannelNo(int No) const;
	int FindServiceID(WORD ServiceID) const;
	int FindByIDs(WORD NetworkID,WORD TransportStreamID,WORD ServiceID) const;
	int GetNextChannelNo(int ChannelNo,bool fWrap=false) const;
	int GetPrevChannelNo(int ChannelNo,bool fWrap=false) const;
	int GetMaxChannelNo() const;
	enum SortType {
		SORT_SPACE,
		SORT_CHANNELINDEX,
		SORT_CHANNELNO,
		SORT_PHYSICALCHANNEL,
		SORT_NAME,
		SORT_NETWORKID,
		SORT_SERVICEID,
		SORT_TRAILER
	};
	bool Sort(SortType Type,bool fDescending=false);
	bool UpdateStreamInfo(int Space,int ChannelIndex,
		WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
	bool HasRemoteControlKeyID() const;
	bool HasMultiService() const;
};

class CTuningSpaceInfo
{
public:
	enum TuningSpaceType {
		SPACE_ERROR=-1,
		SPACE_UNKNOWN=0,
		SPACE_TERRESTRIAL,
		SPACE_BS,
		SPACE_110CS
	};

private:
	CChannelList *m_pChannelList;
	CDynamicString m_Name;
	TuningSpaceType m_Space;

public:
	CTuningSpaceInfo();
	CTuningSpaceInfo(const CTuningSpaceInfo &Info);
	~CTuningSpaceInfo();
	CTuningSpaceInfo &operator=(const CTuningSpaceInfo &Info);
	bool Create(const CChannelList *pList=NULL,LPCTSTR pszName=NULL);
	CChannelList *GetChannelList() { return m_pChannelList; }
	const CChannelList *GetChannelList() const { return m_pChannelList; }
	CChannelInfo *GetChannelInfo(int Index);
	const CChannelInfo *GetChannelInfo(int Index) const;
	LPCTSTR GetName() const { return m_Name.Get(); }
	bool SetName(LPCTSTR pszName);
	TuningSpaceType GetType() const { return m_Space; }
	int NumChannels() const;
};

class CTuningSpaceList
{
	std::vector<CTuningSpaceInfo*> m_TuningSpaceList;
	CChannelList m_AllChannelList;
	bool MakeTuningSpaceList(const CChannelList *pList,int Spaces=0);

public:
	CTuningSpaceList();
	CTuningSpaceList(const CTuningSpaceList &List);
	~CTuningSpaceList();
	CTuningSpaceList &operator=(const CTuningSpaceList &List);
	int NumSpaces() const { return (int)m_TuningSpaceList.size(); }
	bool IsEmpty() const { return m_TuningSpaceList.empty(); }
	CTuningSpaceInfo *GetTuningSpaceInfo(int Space);
	const CTuningSpaceInfo *GetTuningSpaceInfo(int Space) const;
	CChannelList *GetChannelList(int Space);
	const CChannelList *GetChannelList(int Space) const;
	CChannelList *GetAllChannelList() { return &m_AllChannelList; }
	const CChannelList *GetAllChannelList() const { return &m_AllChannelList; }
	LPCTSTR GetTuningSpaceName(int Space) const;
	CTuningSpaceInfo::TuningSpaceType GetTuningSpaceType(int Space) const;
	bool Create(const CChannelList *pList,int Spaces=0);
	bool Reserve(int Spaces);
	bool MakeAllChannelList();
	void Clear();
	bool SaveToFile(LPCTSTR pszFileName) const;
	bool LoadFromFile(LPCTSTR pszFileName);
	bool UpdateStreamInfo(int Space,int ChannelIndex,
		WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
};


#endif
