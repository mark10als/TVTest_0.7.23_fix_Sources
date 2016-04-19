#ifndef CARD_READER_H
#define CARD_READER_H


#include <winscard.h>
#include <dshow.h>
#include "BonBaseClass.h"


// カードリーダー基底クラス
class __declspec(novtable) CCardReader : public CBonBaseClass
{
public:
	enum ReaderType {
		READER_NONE,
		READER_SCARD,
		READER_SCARD_DYNAMIC,
		READER_BONCASCLIENT,
		READER_HDUS,
		READER_LAST=READER_HDUS
	};

	CCardReader();
	virtual ~CCardReader();
	virtual bool Open(LPCTSTR pszReader=NULL)=0;
	virtual void Close()=0;
	virtual LPCTSTR GetReaderName() const=0;
	virtual int NumReaders() const { return 1; }
	virtual LPCTSTR EnumReader(int Index) const;
	ReaderType GetReaderType() const { return m_ReaderType; }
	virtual bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize)=0;
	static CCardReader *CreateCardReader(ReaderType Type);

private:
	ReaderType m_ReaderType;
};

// スマートカードリーダー(スタティックリンク)
class CSCardReader : public CCardReader
{
	SCARDCONTEXT m_ScardContext;
	SCARDHANDLE m_hSCard;
	bool m_bIsEstablish;
	LPTSTR m_pReaderList;
	int m_NumReaders;
	LPTSTR m_pszReaderName;

public:
	CSCardReader();
	~CSCardReader();
	bool Open(LPCTSTR pszReader);
	void Close();
	LPCTSTR GetReaderName() const;
	int NumReaders() const;
	LPCTSTR EnumReader(int Index) const;
	bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize);
};

// スマートカードリーダー(ダイナミックリンク)
// わざわざスタティックリンクと分けているのは、BonCasLink等がバージョンによって
// スタティックリンクでなければ利用できないため
class CDynamicSCardReader : public CCardReader
{
	HMODULE m_hLib;
	SCARDCONTEXT m_ScardContext;
	SCARDHANDLE m_hSCard;
	LPTSTR m_pReaderList;
	LPTSTR m_pszReaderName;
	typedef LONG (WINAPI *SCardTransmitFunc)(SCARDHANDLE,LPCSCARD_IO_REQUEST,LPCBYTE,
											 DWORD,LPSCARD_IO_REQUEST,LPBYTE,LPDWORD);
	SCardTransmitFunc m_pSCardTransmit;

	bool Load(LPCTSTR pszFileName);

public:
	CDynamicSCardReader();
	~CDynamicSCardReader();
	bool Open(LPCTSTR pszReader);
	void Close();
	LPCTSTR GetReaderName() const;
	int NumReaders() const;
	LPCTSTR EnumReader(int Index) const;
	bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize);
};

// BonCasClient
class CBonCasClientCardReader : public CCardReader
{
	HMODULE m_hLib;
	SCARDCONTEXT m_ScardContext;
	SCARDHANDLE m_hSCard;
	LPTSTR m_pReaderList;
	LPTSTR m_pszReaderName;
	typedef LONG (WINAPI *CasLinkTransmitFunc)(SCARDHANDLE,LPCSCARD_IO_REQUEST,LPCBYTE,
											   DWORD,LPSCARD_IO_REQUEST,LPBYTE,LPDWORD);
	CasLinkTransmitFunc m_pCasLinkTransmit;

	HMODULE Load(LPCTSTR pszFileName);
	bool Connect(LPCTSTR pszReader);

public:
	CBonCasClientCardReader();
	~CBonCasClientCardReader();
	bool Open(LPCTSTR pszReader);
	void Close();
	LPCTSTR GetReaderName() const;
	int NumReaders() const;
	LPCTSTR EnumReader(int Index) const;
	bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize);
};

// HDUS内蔵カードリーダー
class CHdusCardReader : public CCardReader
{
	IBaseFilter *m_pTuner;
	bool m_bSent;

	IBaseFilter *FindDevice(REFCLSID category,BSTR varFriendlyName);
	HRESULT Send(const void *pSendData,DWORD SendSize);
	HRESULT Receive(void *pRecvData,DWORD *pRecvSize);

public:
	CHdusCardReader();
	~CHdusCardReader();
	bool Open(LPCTSTR pszReader);
	void Close();
	LPCTSTR GetReaderName() const;
	bool Transmit(const void *pSendData,DWORD SendSize,void *pRecvData,DWORD *pRecvSize);
};


#endif
