// TsTable.h: TS�e�[�u�����b�p�[�N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include "MediaData.h"
#include "TsStream.h"
#include "TsDescriptor.h"


using std::vector;


/////////////////////////////////////////////////////////////////////////////
// PSI�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////
/*
class CPsiTable		// �ۗ��F ���ۂ̃��[�X�P�[�X�𔻒f������Ŏd�l�����߂�K�v����
{
public:
	CPsiTable();
	CPsiTable(const CPsiTable &Operand);

	CPsiTable & operator = (const CPsiTable &Operand);

	const bool StoreSection(const CPsiSection *pSection, bool *pbUpdate = NULL);

	const WORD GetExtensionNum(void) const;
	const bool GetExtension(const WORD wIndex, WORD *pwExtension) const;
	const bool GetSectionNum(const WORD wIndex, WORD *pwSectionNum) const;
	const CMediaData * GetSectionData(const WORD wIndex = 0U, const BYTE bySectionNo = 0U) const;

	void Reset(void);

protected:
	struct TAG_TABLEITEM
	{
		WORD wTableIdExtension;				// �e�[�u��ID�g��
		WORD wSectionNum;					// �Z�N�V������
		BYTE byVersionNo;					// �o�[�W�����ԍ�
		vector<CMediaData> SectionArray;	// �Z�N�V�����f�[�^
	};

	vector<TAG_TABLEITEM> m_TableArray;		// �e�[�u��
};
*/

/////////////////////////////////////////////////////////////////////////////
// PSI�V���O���e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiSingleTable
	: public CTsPidMapTarget
	, public CPsiSectionParser::IPsiSectionHandler
{
public:
	CPsiSingleTable(const bool bTargetSectionExt = true);
	CPsiSingleTable(const CPsiSingleTable &Operand);
	virtual ~CPsiSingleTable() = 0;

	CPsiSingleTable & operator = (const CPsiSingleTable &Operand);

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket);
	virtual void OnPidUnmapped(const WORD wPID);

// CPsiSingleTable
	virtual void Reset(void);

	const DWORD GetCrcErrorCount(void) const;

	CPsiSection m_CurSection;

protected:
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection);
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	bool m_bTableUpdated;

private:
	CPsiSectionParser m_PsiSectionParser;
};

/////////////////////////////////////////////////////////////////////////////
// �X�g���[���^�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class ABSTRACT_CLASS_DECL CPsiStreamTable
	: public CTsPidMapTarget
	, public CPsiSectionParser::IPsiSectionHandler
{
public:
	class ABSTRACT_CLASS_DECL ISectionHandler {
	public:
		virtual ~ISectionHandler() = 0;
		virtual void OnReset(CPsiStreamTable *pTable) {}
		virtual void OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection) {}
	};

	CPsiStreamTable(ISectionHandler *pHandler = NULL, const bool bTargetSectionExt = true, const bool bIgnoreSectionNumber = false);
	CPsiStreamTable(const CPsiStreamTable &Operand);
	virtual ~CPsiStreamTable() = 0;

	CPsiStreamTable & operator = (const CPsiStreamTable &Operand);

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket);
	virtual void OnPidUnmapped(const WORD wPID);

// CPsiStreamTable
	virtual void Reset(void);

	const DWORD GetCrcErrorCount(void) const;

protected:
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection);
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	bool m_bTableUpdated;

private:
	CPsiSectionParser m_PsiSectionParser;
	ISectionHandler *m_pSectionHandler;
};

/////////////////////////////////////////////////////////////////////////////
// �����������Ȃ��e�[�u�����ۉ��N���X(���AdaptationField�����p)
// PSI�e�[�u���Ƃ��ď�������ׂ��ł͂Ȃ���������Ȃ����A����ケ���ɋL�q
/////////////////////////////////////////////////////////////////////////////
class CPsiNullTable :	public CTsPidMapTarget
{
public:
	CPsiNullTable();
	CPsiNullTable(const CPsiNullTable &Operand);
	virtual ~CPsiNullTable();

	CPsiNullTable & operator = (const CPsiNullTable &Operand);

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket) = 0;
	virtual void OnPidUnmapped(const WORD wPID);

// CPsiNullTable

};

/////////////////////////////////////////////////////////////////////////////
// PSI�e�[�u���Z�b�g���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////
/*
class CPsiTableSuite	// �ۗ��F ���ۂ̃��[�X�P�[�X�𔻒f������Ŏd�l�����߂�K�v����
{
public:
	CPsiTableSuite();
	CPsiTableSuite(const CPsiTableSuite &Operand);

	CPsiTableSuite & operator = (const CPsiTableSuite &Operand);

	const bool StorePacket(const CTsPacket *pPacket);

	void SetTargetSectionExt(const bool bTargetExt);
	const bool AddTable(const BYTE byTableID);

	const WORD GetIndexByID(const BYTE byTableID);
	const CPsiTable * GetTable(const WORD wIndex = 0U) const;
	const CMediaData * GetSectionData(const WORD wIndex = 0U, const WORD wSubIndex = 0U, const BYTE bySectionNo = 0U) const;

	const DWORD GetCrcErrorCount(void) const;
	void Reset(void);

protected:
	static void CALLBACK StoreSection(const CPsiSection *pSection, const PVOID pParam);

	struct TAG_TABLESET
	{
		BYTE byTableID;						// �e�[�u��ID
		CPsiTable PsiTable;					// �e�[�u��
	};

	vector<TAG_TABLESET> m_TableSet;		// �e�[�u���Z�b�g

	bool m_bTargetSectionExt;
	bool m_bTableUpdated;

private:
	CPsiSectionParser m_PsiSectionParser;
};
*/


/////////////////////////////////////////////////////////////////////////////
// PAT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CPatTable : public CPsiSingleTable
{
public:
	CPatTable(
#ifdef _DEBUG
		bool bTrace = false
#endif
	);
	CPatTable(const CPatTable &Operand);

	CPatTable & operator = (const CPatTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CPatTable
	const WORD GetTransportStreamID(void) const;

	const WORD GetNitPID(const WORD wIndex = 0U) const;
	const WORD GetNitNum(void) const;

	const WORD GetPmtPID(const WORD wIndex = 0U) const;
	const WORD GetProgramID(const WORD wIndex = 0U) const;
	const WORD GetProgramNum(void) const;

	const bool IsPmtTablePID(const WORD wPID) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_PATITEM
	{
		WORD wProgramID;	// �����ԑg�ԍ�ID
		WORD wPID;			// PMT��PID
	};

	vector<WORD> m_NitPIDArray;
	vector<TAG_PATITEM> m_PmtPIDArray;

#ifdef _DEBUG
	bool m_bDebugTrace;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// CAT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCatTable : public CPsiSingleTable
{
public:
	CCatTable();
	virtual ~CCatTable(void);
	CCatTable(const CCatTable &Operand);

	CCatTable  & operator = (const CCatTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CCatTable
	const CCaMethodDesc * GetCaDescBySystemID(const WORD SystemID) const;
	WORD GetEmmPID() const;
	WORD GetEmmPID(const WORD CASystemID) const;
	const CDescBlock * GetCatDesc() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	CDescBlock m_DescBlock;
};


/////////////////////////////////////////////////////////////////////////////
// PMT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CPmtTable : public CPsiSingleTable
{
public:
	CPmtTable(
#ifdef _DEBUG
		bool bTrace = false
#endif
	);
	CPmtTable(const CPmtTable &Operand);

	CPmtTable & operator = (const CPmtTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CPmtTable
	const WORD GetProgramNumberID(void) const;

	const WORD GetPcrPID(void) const;
	const CDescBlock * GetTableDesc(void) const;
	const WORD GetEcmPID(void) const;
	const WORD GetEcmPID(const WORD CASystemID) const;

	const WORD GetEsInfoNum(void) const;
	const BYTE GetStreamTypeID(const WORD wIndex) const;
	const WORD GetEsPID(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_PMTITEM
	{
		BYTE byStreamTypeID;			// Stream Type ID
		WORD wEsPID;					// Elementary Stream PID
		CDescBlock DescBlock;			// Stream ID Descriptor ��
	};

	vector<TAG_PMTITEM> m_EsInfoArray;

	WORD m_wPcrPID;						// PCR_PID
	CDescBlock m_TableDescBlock;		// Conditional Access Method Descriptor ��

#ifdef _DEBUG
	bool m_bDebugTrace;
#endif
};


/////////////////////////////////////////////////////////////////////////////
// SDT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSdtTable : public CPsiSingleTable
{
public:
	CSdtTable();
	CSdtTable(const CSdtTable &Operand);

	CSdtTable & operator = (const CSdtTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CSdtTable
	const WORD GetNetworkID(void) const;
	const WORD GetServiceNum(void) const;
	const WORD GetServiceIndexByID(const WORD wServiceID);
	const WORD GetServiceID(const WORD wIndex) const;
	const bool GetEITScheduleFlag(const WORD wIndex) const;
	const bool GetEITPresentFollowingFlag(const WORD wIndex) const;
	const BYTE GetRunningStatus(const WORD wIndex) const;
	const bool GetFreeCaMode(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual void OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection);
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_SDTITEM
	{
		WORD wServiceID;				// Service ID
		bool bEITScheduleFlag;			// EIT Schedule Flag
		bool bEITPresentFollowingFlag;	// EIT Present Following Flag
		BYTE byRunningStatus;			// Running Status
		bool bFreeCaMode;				// Free CA Mode(true: CA / false: Free)
		CDescBlock DescBlock;			// Service Descriptor ��
	};

	WORD m_wNetworkID;
	vector<TAG_SDTITEM> m_ServiceInfoArray;
};


/////////////////////////////////////////////////////////////////////////////
// NIT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CNitTable : public CPsiSingleTable
{
public:
	CNitTable();
	CNitTable(const CNitTable &Operand);

	CNitTable & operator = (const CNitTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CNitTable
	const WORD GetNetworkID(void) const;
	const CDescBlock * GetNetworkDesc(void) const;
	const WORD GetTransportStreamNum(void) const;
	const WORD GetTransportStreamID(const WORD wIndex) const;
	const WORD GetOriginalNetworkID(const WORD wIndex) const;
	const CDescBlock * GetItemDesc(const WORD wIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	struct TAG_NITITEM {
		WORD wTransportStreamID;
		WORD wOriginalNetworkID;
		CDescBlock DescBlock;
	};

	WORD m_wNetworkID;				// Network ID
	CDescBlock m_NetworkDescBlock;	// Network descriptor
	vector<TAG_NITITEM> m_TransportStreamArray;
};


/////////////////////////////////////////////////////////////////////////////
// EIT[p/f]�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CEitPfTable : public CPsiStreamTable
{
public:
	CEitPfTable();
	CEitPfTable(const CEitPfTable &Operand);

	CEitPfTable & operator = (const CEitPfTable &Operand);

// CPsiSingleTable
	virtual void Reset(void);

// CEitPfTable
	const DWORD GetServiceNum(void) const;
	const int GetServiceIndexByID(WORD ServiceID) const;
	const WORD GetServiceID(DWORD Index) const;
	const WORD GetTransportStreamID(DWORD Index) const;
	const WORD GetOriginalNetworkID(DWORD Index) const;
	const WORD GetEventID(DWORD Index, DWORD EventIndex) const;
	const SYSTEMTIME *GetStartTime(DWORD Index, DWORD EventIndex) const;
	const DWORD GetDuration(DWORD Index, DWORD EventIndex) const;
	const BYTE GetRunningStatus(DWORD Index, DWORD EventIndex) const;
	const bool GetFreeCaMode(DWORD Index, DWORD EventIndex) const;
	const CDescBlock * GetItemDesc(DWORD Index, DWORD EventIndex) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	struct EventInfo {
		bool bEnable;
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		BYTE RunningStatus;
		bool FreeCaMode;
		CDescBlock DescBlock;
		EventInfo() : bEnable(false) {}
	};

	struct ServiceInfo {
		WORD ServiceID;
		WORD TransportStreamID;
		WORD OriginalNetworkID;
		EventInfo EventList[2];
	};

	vector<ServiceInfo> m_ServiceList;
};


/////////////////////////////////////////////////////////////////////////////
// TOT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CTotTable : public CPsiSingleTable
{
public:
	enum { TABLE_ID = 0x73 };

	CTotTable();
	virtual ~CTotTable();

// CPsiSingleTable
	virtual void Reset(void);

// CTotTable
	const bool GetDateTime(SYSTEMTIME *pTime) const;
	const int GetLocalTimeOffset() const;
	const CDescBlock * GetTotDesc(void) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

	bool m_bValidDateTime;
	SYSTEMTIME m_DateTime;	// ���ݓ��t/����
	CDescBlock m_DescBlock;	// �L�q�q�̈�
};


/////////////////////////////////////////////////////////////////////////////
// CDT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCdtTable : public CPsiStreamTable
{
public:
	enum { TABLE_ID = 0xC8 };

	CCdtTable(ISectionHandler *pHandler = NULL);
	virtual ~CCdtTable();

// CPsiStreamTable
	virtual void Reset(void);

// CCdtTable
	// �f�[�^�̎��
	enum {
		DATATYPE_LOGO		= 0x01,	// ���S
		DATATYPE_INVALID	= 0xFF	// ����
	};

	const WORD GetOriginalNetworkId() const;
	const BYTE GetDataType() const;
	const CDescBlock * GetDesc() const;
	const WORD GetDataModuleSize() const;
	const BYTE * GetDataModuleByte() const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	WORD m_OriginalNetworkId;	// original_network_id
	BYTE m_DataType;			// data_type
	CDescBlock m_DescBlock;		// �L�q�q�̈�
	CMediaData m_ModuleData;
};


/////////////////////////////////////////////////////////////////////////////
// SDTT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSdttTable : public CPsiStreamTable
{
public:
	enum { TABLE_ID = 0xC3 };

	struct ScheduleDescription
	{
		SYSTEMTIME StartTime;				// start_time
		DWORD Duration;						// duration
	};

	struct ContentInfo
	{
		BYTE GroupID;						// group_id
		WORD TargetVersion;					// target_version
		WORD NewVersion;					// new_version
		BYTE DownloadLevel;					// donwload_level
		BYTE VersionIndicator;				// version_indicator
		BYTE ScheduleTimeShiftInformation;	// schedule_time-shift_information
		std::vector<ScheduleDescription> ScheduleList;
		CDescBlock DescBlock;				// �L�q�q�̈�
	};

	CSdttTable(ISectionHandler *pHandler = NULL);
	virtual ~CSdttTable();

// CPsiStreamTable
	virtual void Reset(void);

// CSdttTable
	const BYTE GetMakerID() const;
	const BYTE GetModelID() const;
	const bool IsCommon() const;
	const WORD GetTransportStreamID() const;
	const WORD GetOriginalNetworkID() const;
	const WORD GetServiceID() const;
	const BYTE GetNumOfContents() const;
	const ContentInfo * GetContentInfo(const BYTE Index) const;
	const bool IsSchedule(DWORD Index) const;
	const CDescBlock * GetContentDesc(DWORD Index) const;

protected:
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

	BYTE m_MakerID;				// maker_id
	BYTE m_ModelID;				// model_id
	WORD m_TransportStreamID;	// transport_stream_id
	WORD m_OriginalNetworkID;	// original_network_id
	WORD m_ServiceID;			// service_id
	std::vector<ContentInfo> m_ContentList;
};


/////////////////////////////////////////////////////////////////////////////
// PCR���ۉ��N���X
// ���XDemux�̉ӏ��ɂ��������̂����g���ĂȂ��悤�������̂ŁATable���Ɉړ�
// �����_�Ŏg������̂Ƃ͌����
/////////////////////////////////////////////////////////////////////////////

class CPcrTable : public CPsiNullTable
{
public:
	CPcrTable(WORD wServiceIndex);
	CPcrTable(const CPcrTable &Operand);

	CPcrTable & operator = (const CPcrTable &Operand);

// CPsiNullTable
	virtual const bool StorePacket(const CTsPacket *pPacket);

// CPcrTable
	void AddServiceIndex(WORD wServiceIndex);
	const WORD GetServiceIndex(WORD *pwServiceIndex, WORD wIndex=0);
	const unsigned __int64 GetPcrTimeStamp();

protected:
	vector<WORD> m_ServiceIndex;
	unsigned __int64 m_ui64_Pcr;
};
