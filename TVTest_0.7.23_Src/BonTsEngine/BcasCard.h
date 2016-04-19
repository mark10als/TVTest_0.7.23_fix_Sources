// BcasCard.h: CBcasCard クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "BonBaseClass.h"
#include "CardReader.h"

// ECMデータの最小/最大サイズ
#define MIN_ECM_DATA_SIZE 30
#define MAX_ECM_DATA_SIZE 256
// EMMデータの最大サイズ
#define MAX_EMM_DATA_SIZE 263

class CBcasCard : public CBonBaseClass
{
public:
	// エラーコード
	enum {
		ERR_NOERROR			= 0x00000000UL,	// エラーなし
		ERR_INTERNALERROR	= 0x00000001UL,	// 内部エラー
		ERR_NOTESTABLISHED	= 0x00000002UL,	// コンテキスト確立失敗
		ERR_NOCARDREADERS	= 0x00000003UL,	// カードリーダがない
		ERR_ALREADYOPEN		= 0x00000004UL,	// 既にオープン済み
		ERR_CARDOPENERROR	= 0x00000005UL,	// カードオープン失敗
		ERR_CARDNOTOPEN		= 0x00000006UL,	// カード未オープン
		ERR_TRANSMITERROR	= 0x00000007UL,	// 通信エラー
		ERR_BADARGUMENT		= 0x00000008UL,	// 引数が不正
		ERR_ECMREFUSED		= 0x00000009UL,	// ECM受付拒否
		ERR_EMMERROR		= 0x0000000AUL	// EMM処理エラー
	};

	enum {
		CARDTYPE_PREPAID	= 0x00,
		CARDTYPE_STANDARD	= 0x01,
		CARDTYPE_INVALID	= 0xFF
	};

	struct BcasCardInfo
	{
		WORD CASystemID;				// CA_system_id
		BYTE BcasCardID[6];				// Card ID
		BYTE CardType;					// Card type
		BYTE MessagePartitionLength;	// Message partition length
		BYTE SystemKey[32];				// Descrambling system key
		BYTE InitialCbc[8];				// Descrambler CBC initial value
		BYTE CardManufacturerID;		// Manufacturer identifier
		BYTE CardVersion;				// Version
		WORD CheckCode;					// Check code
	};

	CBcasCard();
	~CBcasCard();

	const DWORD GetCardReaderNum(void) const;
	LPCTSTR EnumCardReader(const DWORD dwIndex) const;

	const bool OpenCard(CCardReader::ReaderType ReaderType = CCardReader::READER_SCARD, LPCTSTR lpszReader = NULL);
	void CloseCard(void);
	const bool ReOpenCard();
	const bool IsCardOpen() const;
	CCardReader::ReaderType GetCardReaderType() const;
	LPCTSTR GetCardReaderName() const;

	const bool GetBcasCardInfo(BcasCardInfo *pInfo);
	const bool GetCASystemID(WORD *pID);
	const BYTE * GetBcasCardID(void);
	const BYTE GetCardType(void);
	const BYTE GetMessagePartitionLength(void);
	const BYTE * GetInitialCbc(void);
	const BYTE * GetSystemKey(void);
	const BYTE * GetKsFromEcm(const BYTE *pEcmData, const DWORD dwEcmSize);
	const bool SendEmmSection(const BYTE *pEmmData, const DWORD dwEmmSize);
	const bool SendCommand(const BYTE *pSendData, const DWORD SendSize, BYTE *pReceiveData, DWORD *pReceiveSize);
	const int FormatCardID(LPTSTR pszText, int MaxLength) const;
	const char GetCardManufacturerID() const;
	const BYTE GetCardVersion() const;

protected:
	const bool OpenAndInitialize(LPCTSTR pszReader);
	const bool InitialSetting(void);

	CCardReader *m_pCardReader;

	BcasCardInfo m_BcasCardInfo;

	struct TAG_ECMSTATUS
	{
		DWORD dwLastEcmSize;					// 最後に問い合わせのあったECMサイズ
		BYTE LastEcmData[MAX_ECM_DATA_SIZE];	// 最後に問い合わせのあったECMデータ
		BYTE KsData[16];						// Ks Odd + Even
		bool bSucceeded;						// ECMが受け付けられたか
	} m_EcmStatus;
};
