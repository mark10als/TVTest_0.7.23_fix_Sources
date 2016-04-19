// BonSrcDecoder.h: CBonSrcDecoder クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <deque>
#include "MediaDecoder.h"
#include "IBonDriver.h"
#include "IBonDriver2.h"


/////////////////////////////////////////////////////////////////////////////
// Bonソースデコーダ(チューナからTS平分ストリームを受信する)
/////////////////////////////////////////////////////////////////////////////
// Output	#0	: CMediaData		平分TSストリーム
/////////////////////////////////////////////////////////////////////////////

class CBonSrcDecoder : public CMediaDecoder
{
public:
	// エラーコード
	enum {
		ERR_NOERROR,		// エラーなし
		ERR_DRIVER,			// ドライバエラー
		ERR_TUNEROPEN,		// チューナオープンエラー
		ERR_TUNER,			// チューナエラー
		ERR_NOTOPEN,		// チューナが開かれていない
		ERR_ALREADYOPEN,	// チューナが既に開かれている
		ERR_NOTPLAYING,		// 再生されていない
		ERR_ALREADYPLAYING,	// 既に再生されている
		ERR_TIMEOUT,		// タイムアウト
		ERR_PENDING,		// 
		ERR_INTERNAL		// 内部エラー
	};

	CBonSrcDecoder(IEventHandler *pEventHandler = NULL);
	virtual ~CBonSrcDecoder();

// IMediaDecoder
	virtual void Reset(void) override;
	virtual void ResetGraph(void) override;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL) override;

// CBonSrcDecoder
	const bool OpenTuner(HMODULE hBonDrvDll);
	const bool CloseTuner(void);
	const bool IsOpen() const;

	const bool Play(void);
	const bool Stop(void);

	const bool SetChannel(const BYTE byChannel);
	const bool SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	const bool SetChannelAndPlay(const DWORD dwSpace, const DWORD dwChannel);
	const float GetSignalLevel(void);

	const bool IsBonDriver2(void) const;
	LPCTSTR GetSpaceName(const DWORD dwSpace) const;
	LPCTSTR GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const;

	const bool PurgeStream(void);

	// Append by HDUSTestの中の人
	int NumSpaces() const;
	LPCTSTR GetTunerName() const;
	int GetCurSpace() const;
	int GetCurChannel() const;
	DWORD GetBitRate() const;
	DWORD GetStreamRemain() const;
	bool SetStreamThreadPriority(int Priority);
	int GetStreamThreadPriority() const { return m_StreamThreadPriority; }
	void SetPurgeStreamOnChannelChange(bool bPurge);
	bool IsPurgeStreamOnChannelChange() const { return m_bPurgeStreamOnChannelChange; }

private:
	struct StreamingRequest
	{
		enum RequestType
		{
			TYPE_SETCHANNEL,
			TYPE_SETCHANNEL2,
			TYPE_PURGESTREAM,
			TYPE_RESET
		};

		RequestType Type;
		bool bProcessing;

		union
		{
			struct {
				BYTE Channel;
			} SetChannel;
			struct {
				DWORD Space;
				DWORD Channel;
			} SetChannel2;
		};
	};

	static unsigned int __stdcall StreamingThread(LPVOID pParam);
	void StreamingMain();

	void ResetStatus();
	void AddRequest(const StreamingRequest &Request);
	void AddRequests(const StreamingRequest *pRequestList, int RequestCount);
	bool WaitAllRequests(DWORD Timeout);
	bool HasPendingRequest();
	void SetRequestTimeoutError();

	IBonDriver *m_pBonDriver;
	IBonDriver2 *m_pBonDriver2;	

	HANDLE m_hStreamRecvThread;
	CLocalEvent m_EndEvent;
	std::deque<StreamingRequest> m_RequestQueue;
	CCriticalLock m_RequestLock;
	CLocalEvent m_RequestEvent;
	DWORD m_RequestTimeout;
	BOOL m_bRequestResult;

	volatile bool m_bIsPlaying;

	float m_SignalLevel;
	CBitRateCalculator m_BitRateCalculator;
	DWORD m_StreamRemain;

	int m_StreamThreadPriority;
	bool m_bPurgeStreamOnChannelChange;
};
