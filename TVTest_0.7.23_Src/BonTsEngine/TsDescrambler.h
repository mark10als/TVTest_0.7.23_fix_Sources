// TsDescrambler.h: CTsDescrambler クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <deque>
#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsTable.h"
#include "TsUtilClass.h"
#include "BcasCard.h"


class CEcmProcessor;
class CEmmProcessor;

class CBcasAccess
{
public:
	virtual ~CBcasAccess() {}
	virtual bool Process(CBcasCard *pBcasCard) = 0;
};

class CBcasAccessQueue : public CBonBaseClass
{
	std::deque<CBcasAccess*> m_Queue;
	CBcasCard *m_pBcasCard;
	CCardReader::ReaderType m_ReaderType;
	LPCTSTR m_pszReaderName;
	HANDLE m_hThread;
	CLocalEvent m_Event;
	volatile bool m_bKillEvent;
	volatile bool m_bStartEvent;
	CCriticalLock m_Lock;

	static unsigned int __stdcall BcasAccessThread(LPVOID lpParameter);

public:
	CBcasAccessQueue(CBcasCard *pBcasCard);
	~CBcasAccessQueue();
	void Clear();
	bool Enqueue(CBcasAccess *pAccess);
	bool BeginBcasThread(CCardReader::ReaderType ReaderType, LPCTSTR pszReaderName);
	bool EndBcasThread();
};


/////////////////////////////////////////////////////////////////////////////
// MULTI2スクランブル解除(ECMによりペイロードのスクランブルを解除する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket		暗号TSパケット
// Output	#0	: CTsPacket		平分TSパケット
/////////////////////////////////////////////////////////////////////////////

class CTsDescrambler : public CMediaDecoder
{
public:
	enum {
		EVENT_EMM_PROCESSED		= 0x00000001UL,
		EVENT_ECM_ERROR			= 0x00000002UL,
		EVENT_ECM_REFUSED		= 0x00000003UL,
		EVENT_CARD_READER_HUNG	= 0x00000004UL
	};

	struct EcmErrorInfo {
		WORD EcmPID;
		LPCTSTR pszText;
	};

	enum InstructionType {
		INSTRUCTION_NORMAL,
		INSTRUCTION_SSE2,
		INSTRUCTION_SSSE3
	};

	CTsDescrambler(IEventHandler *pEventHandler = NULL);
	virtual ~CTsDescrambler();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsDescrambler
	const bool EnableDescramble(bool bDescramble);
	const bool EnableEmmProcess(bool bEnable);
	const bool OpenBcasCard(CCardReader::ReaderType ReaderType = CCardReader::READER_SCARD, LPCTSTR pszReaderName = NULL);
	void CloseBcasCard(void);
	const bool IsBcasCardOpen() const;
	CCardReader::ReaderType GetCardReaderType() const;
	LPCTSTR GetCardReaderName() const;
	const bool GetBcasCardInfo(CBcasCard::BcasCardInfo *pInfo);
	const bool GetBcasCardID(BYTE *pCardID);
	int FormatBcasCardID(LPTSTR pszText,int MaxLength) const;
	char GetBcasCardManufacturerID() const;
	BYTE GetBcasCardVersion() const;
	const DWORD GetInputPacketCount(void) const;
	const DWORD GetScramblePacketCount(void) const;
	void ResetScramblePacketCount(void);
	bool SetTargetServiceID(WORD ServiceID = 0);
	WORD GetEcmPIDByServiceID(const WORD ServiceID) const;
	static bool IsSSE2Available();
	static bool IsSSSE3Available();
	bool SetInstruction(InstructionType Type);
	bool SendBcasCommand(const BYTE *pSendData, DWORD SendSize, BYTE *pRecvData, DWORD *pRecvSize);

protected:
	class CEsProcessor;

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnCatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	int GetServiceIndexByID(WORD ServiceID) const;

#ifdef _DEBUG
	void PrintStatus(void) const;
#endif

	bool m_bDescramble;
	bool m_bProcessEmm;
	CTsPidMapManager m_PidMapManager;
	CBcasCard m_BcasCard;
	CBcasAccessQueue m_Queue;

	WORD m_CurTransportStreamID;
	WORD m_DescrambleServiceID;

	struct TAG_SERVICEINFO {
		bool bTarget;
		WORD ServiceID;
		WORD PmtPID;
		WORD EcmPID;
		std::vector<WORD> EsPIDList;
	};
	std::vector<TAG_SERVICEINFO> m_ServiceList;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_ScramblePacketCount;

	InstructionType m_Instruction;

	WORD m_EmmPID;

	friend class CEcmProcessor;
	friend class CEmmProcessor;
	friend class CDescramblePmtTable;
};
