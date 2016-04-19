// DtvEngine.cpp: CDtvEngine クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DtvEngine.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pEventHandler(NULL)

	, m_wCurTransportStream(0U)
	, m_CurServiceIndex(SERVICE_INVALID)
	, m_CurServiceID(SID_INVALID)
	, m_SpecServiceID(SID_INVALID)
	, m_CurAudioStream(0)

	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsAnalyzer(this)
	, m_TsDescrambler(this)
	, m_MediaViewer(this)
	, m_MediaTee(this)
	, m_FileWriter(this)
	//, m_FileReader(this)
	, m_MediaBuffer(this)
	, m_MediaGrabber(this)
	, m_TsSelector(this)
	, m_EventManager(this)
	, m_CaptionDecoder(this)
	, m_LogoDownloader(this)

	, m_bBuiled(false)
	, m_bIsFileMode(false)
	, m_bDescramble(true)
	, m_bBuffering(false)
	, m_bStartStreamingOnDriverOpen(false)

	, m_bDescrambleCurServiceOnly(false)
	, m_bWriteCurServiceOnly(false)
	, m_WriteStream(CTsSelector::STREAM_ALL)
{
}


CDtvEngine::~CDtvEngine(void)
{
	CloseEngine();
}


const bool CDtvEngine::BuildEngine(CEventHandler *pEventHandler,
								   bool bDescramble, bool bBuffering, bool bEventManager)
{
	// 完全に暫定
	if (m_bBuiled)
		return true;

	/*
	グラフ構成図

	CBonSrcDecoder
	    ↓
	CTsPacketParser
	    ↓
	CTsAnalyzer
	    ↓
	CTsDescrambler
	    ↓
	CMediaTee─────┐
	    ↓             ↓
	CMediaBuffer  CEventManager
	    ↓             ↓
	CMediaViewer  CCaptionDecoder
	                   ↓
	              CLogoDownloader
	                   ↓
	              CMediaGrabber → プラグイン
	                   ↓
	              CTsSelector
	                   ↓
	              CFileWriter
	*/

	Trace(TEXT("デコーダグラフを構築しています..."));

	// デコーダグラフ構築
	m_TsPacketParser.SetOutputDecoder(&m_TsAnalyzer);
	m_TsAnalyzer.SetOutputDecoder(&m_TsDescrambler);
	m_TsDescrambler.SetOutputDecoder(&m_MediaTee);
	m_TsDescrambler.EnableDescramble(bDescramble);
	m_bDescramble = bDescramble;
	if (bBuffering) {
		m_MediaTee.SetOutputDecoder(&m_MediaBuffer, 0);
		m_MediaBuffer.SetOutputDecoder(&m_MediaViewer);
		m_MediaBuffer.Play();
	} else {
		m_MediaTee.SetOutputDecoder(&m_MediaViewer, 0);
	}
	m_bBuffering=bBuffering;
	if (bEventManager) {
		m_MediaTee.SetOutputDecoder(&m_EventManager, 1UL);
		m_EventManager.SetOutputDecoder(&m_CaptionDecoder);
	} else {
		m_MediaTee.SetOutputDecoder(&m_CaptionDecoder, 1UL);
	}
	m_CaptionDecoder.SetOutputDecoder(&m_LogoDownloader);
	m_LogoDownloader.SetOutputDecoder(&m_MediaGrabber);
	m_MediaGrabber.SetOutputDecoder(&m_TsSelector);
	m_TsSelector.SetOutputDecoder(&m_FileWriter);

	// イベントハンドラ設定
	m_pEventHandler = pEventHandler;
	m_pEventHandler->m_pDtvEngine = this;

	m_bBuiled = true;

	return true;
}


const bool CDtvEngine::IsBuildComplete() const
{
	return m_bBuiled && IsSrcFilterOpen() && m_MediaViewer.IsOpen()
		&& (!m_bDescramble || m_TsDescrambler.IsBcasCardOpen());
}


const bool CDtvEngine::CloseEngine(void)
{
	//if (!m_bBuiled)
	//	return true;

	Trace(TEXT("DtvEngineを閉じています..."));

	//m_MediaViewer.Stop();

	ReleaseSrcFilter();

	Trace(TEXT("バッファのストリーミングを停止しています..."));
	m_MediaBuffer.Stop();

	Trace(TEXT("カードリーダを閉じています..."));
	m_TsDescrambler.CloseBcasCard();

	Trace(TEXT("メディアビューアを閉じています..."));
	m_MediaViewer.CloseViewer();

	// イベントハンドラ解除
	m_pEventHandler = NULL;

	m_bBuiled = false;

	Trace(TEXT("DtvEngineを閉じました。"));

	return true;
}


const bool CDtvEngine::ResetEngine(void)
{
	if (!m_bBuiled)
		return false;

	// デコーダグラフリセット
	ResetStatus();
	/*if (m_bIsFileMode)
		m_FileReader.ResetGraph();
	else*/
		m_BonSrcDecoder.ResetGraph();

	return true;
}


const bool CDtvEngine::OpenSrcFilter_BonDriver(HMODULE hBonDriverDll)
{
	ReleaseSrcFilter();
	// ソースフィルタを開く
	Trace(TEXT("チューナを開いています..."));
	if (!m_BonSrcDecoder.OpenTuner(hBonDriverDll)) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}
	m_MediaBuffer.SetFileMode(false);
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
	if (m_bStartStreamingOnDriverOpen) {
		Trace(TEXT("ストリームの再生を開始しています..."));
		if (!m_BonSrcDecoder.Play()) {
			SetError(m_BonSrcDecoder.GetLastErrorException());
			return false;
		}
	}
	//ResetEngine();
	ResetStatus();

	m_bIsFileMode = false;
	return true;
}


#if 0
const bool CDtvEngine::OpenSrcFilter_File(LPCTSTR lpszFileName)
{
	ReleaseSrcFilter();
	// ファイルを開く
	if (!m_FileReader.OpenFile(lpszFileName)) {
		return false;
	}
	m_MediaBuffer.SetFileMode(true);
	m_FileReader.SetOutputDecoder(&m_TsPacketParser);
	if (!m_FileReader.StartReadAnsync()) {
		return false;
	}
	ResetEngine();

	m_bIsFileMode = true;
	return true;
}


const bool CDtvEngine::PlayFile(LPCTSTR lpszFileName)
{
	// ※グラフ全体の排他制御を実装しないとまともに動かない！！

	// 再生中の場合は閉じる
	m_FileReader.StopReadAnsync();

	// スレッド終了を待つ
	while(m_FileReader.IsAnsyncReadBusy()){
		::Sleep(1UL);
		}

	m_FileReader.CloseFile();

	// デバイスから再生中の場合はグラフを再構築する
	if(!m_bIsFileMode){
		m_BonSrcDecoder.Stop();
		m_BonSrcDecoder.SetOutputDecoder(NULL);
		m_FileReader.SetOutputDecoder(&m_TsPacketParser);
		}

	try{
		// グラフリセット
		m_FileReader.Reset();

		// ファイルオープン
		if(!m_FileReader.OpenFile(lpszFileName))throw 0UL;

		// 非同期再生開始
		if(!m_FileReader.StartReadAnsync())throw 1UL;
		}
	catch(const DWORD dwErrorStep){
		// エラー発生
		StopFile();
		return false;
		}

	m_bIsFileMode = true;

	return true;
}

void CDtvEngine::StopFile(void)
{
	// ※グラフ全体の排他制御を実装しないとまともに動かない！！

	m_bIsFileMode = false;

	// 再生中の場合は閉じる
	m_FileReader.StopReadAnsync();

	// スレッド終了を待つ
	while(m_FileReader.IsAnsyncReadBusy()){
		::Sleep(1UL);
		}

	m_FileReader.CloseFile();

	// グラフを再構築する
	m_FileReader.SetOutputDecoder(NULL);
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
	m_BonSrcDecoder.Play();
}
#endif


const bool CDtvEngine::ReleaseSrcFilter()
{
	// ソースフィルタを開放する
	/*if (m_bIsFileMode) {
		m_FileReader.StopReadAnsync();
		m_FileReader.CloseFile();
		m_FileReader.SetOutputDecoder(NULL);
	} else */{
		if (m_BonSrcDecoder.IsOpen()) {
			m_BonSrcDecoder.CloseTuner();
			m_BonSrcDecoder.SetOutputDecoder(NULL);
		}
	}
	return true;
}


const bool CDtvEngine::IsSrcFilterOpen() const
{
	/*
	if (m_bIsFileMode)
		return m_FileReader.IsOpen();
	*/
	return m_BonSrcDecoder.IsOpen();
}


const bool CDtvEngine::EnablePreview(const bool bEnable)
{
	if (!m_MediaViewer.IsOpen())
		return false;

	bool bOK;

	if (bEnable) {
		// プレビュー有効
		bOK = m_MediaViewer.Play();
	} else {
		// プレビュー無効
		bOK = m_MediaViewer.Stop();
	}

	return bOK;
}


const bool CDtvEngine::SetViewSize(const int x,const int y)
{
	// ウィンドウサイズを設定する
	return m_MediaViewer.SetViewSize(x,y);
}


const bool CDtvEngine::SetVolume(const float fVolume)
{
	// オーディオボリュームを設定する( -100.0(無音) < fVolume < 0(最大) )
	return m_MediaViewer.SetVolume(fVolume);
}


const bool CDtvEngine::GetVideoSize(WORD *pwWidth,WORD *pwHeight)
{
	return m_MediaViewer.GetVideoSize(pwWidth,pwHeight);
}


const bool CDtvEngine::GetVideoAspectRatio(BYTE *pbyAspectRateX,BYTE *pbyAspectRateY)
{
	return m_MediaViewer.GetVideoAspectRatio(pbyAspectRateX,pbyAspectRateY);
}


const BYTE CDtvEngine::GetAudioChannelNum()
{
	return m_MediaViewer.GetAudioChannelNum();
}


const int CDtvEngine::GetAudioStreamNum(const int Service)
{
	WORD ServiceID;
	if (Service<0) {
		if (!GetServiceID(&ServiceID))
			return 0;
	} else {
		if (!m_TsAnalyzer.GetViewableServiceID(Service, &ServiceID))
			return 0;
	}
	return m_TsAnalyzer.GetAudioEsNum(m_TsAnalyzer.GetServiceIndexByID(ServiceID));
}


const bool CDtvEngine::SetAudioStream(int StreamIndex)
{
	WORD AudioPID;

	if (!m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, StreamIndex, &AudioPID))
		return false;

	if (!m_MediaViewer.SetAudioPID(AudioPID))
		return false;

	m_CurAudioStream = StreamIndex;

	return true;
}


const int CDtvEngine::GetAudioStream() const
{
	return m_CurAudioStream;
}


const BYTE CDtvEngine::GetAudioComponentType(const int AudioIndex)
{
	return m_TsAnalyzer.GetAudioComponentType(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex);
}


const int CDtvEngine::GetAudioComponentText(LPTSTR pszText, int MaxLength, const int AudioIndex)
{
	return m_TsAnalyzer.GetAudioComponentText(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex, pszText, MaxLength);
}


const bool CDtvEngine::SetStereoMode(int iMode)
{
	return m_MediaViewer.SetStereoMode(iMode);
}


const WORD CDtvEngine::GetEventID(bool bNext)
{
	return m_TsAnalyzer.GetEventID(m_CurServiceIndex, bNext);
}


const int CDtvEngine::GetEventName(LPTSTR pszName, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventName(m_CurServiceIndex, pszName, MaxLength, bNext);
}


const int CDtvEngine::GetEventText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventText(m_CurServiceIndex, pszText, MaxLength, bNext);
}


const int CDtvEngine::GetEventExtendedText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_TsAnalyzer.GetEventExtendedText(m_CurServiceIndex, pszText, MaxLength, true, bNext);
}


const bool CDtvEngine::GetEventTime(SYSTEMTIME *pStartTime, SYSTEMTIME *pEndTime, bool bNext)
{
	SYSTEMTIME stStart;

	if (!m_TsAnalyzer.GetEventStartTime(m_CurServiceIndex, &stStart, bNext))
		return false;
	if (pStartTime)
		*pStartTime = stStart;
	if (pEndTime) {
		DWORD Duration = m_TsAnalyzer.GetEventDuration(m_CurServiceIndex, bNext);
		if (Duration == 0)
			return false;

		CDateTime Time(stStart);
		Time.Offset(CDateTime::SECONDS(Duration));
		Time.Get(pEndTime);
	}
	return true;
}


const DWORD CDtvEngine::GetEventDuration(bool bNext)
{
	return m_TsAnalyzer.GetEventDuration(m_CurServiceIndex, bNext);
}


const bool CDtvEngine::GetEventSeriesInfo(CTsAnalyzer::EventSeriesInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventSeriesInfo(m_CurServiceIndex, pInfo, bNext);
}


const bool CDtvEngine::GetEventInfo(CTsAnalyzer::EventInfo *pInfo, bool bNext)
{
	return m_TsAnalyzer.GetEventInfo(m_CurServiceIndex, pInfo, true, bNext);
}


const bool CDtvEngine::GetEventAudioInfo(CTsAnalyzer::EventAudioInfo *pInfo, const int AudioIndex, bool bNext)
{
	return m_TsAnalyzer.GetEventAudioInfo(m_CurServiceIndex, AudioIndex < 0 ? m_CurAudioStream : AudioIndex, pInfo, bNext);
}

const bool CDtvEngine::GetVideoDecoderName(LPWSTR lpName,int iBufLen)
{
	return m_MediaViewer.GetVideoDecoderName(lpName, iBufLen);
}


const bool CDtvEngine::SetChannel(const BYTE byTuningSpace, const WORD wChannel, const WORD ServiceID)
{
	TRACE(TEXT("CDtvEngine::SetChannel(%d, %d, %04x)\n"),
		  byTuningSpace, wChannel, ServiceID);

	CBlockLock Lock(&m_EngineLock);

	// サービスはPATが来るまで保留
	m_SpecServiceID = ServiceID;

	// チャンネル変更
	if (!m_BonSrcDecoder.SetChannelAndPlay((DWORD)byTuningSpace, (DWORD)wChannel)) {
		m_SpecServiceID = SID_INVALID;
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	ResetStatus();

	return true;
}


const bool CDtvEngine::SetService(const WORD wService)
{
	CBlockLock Lock(&m_EngineLock);

	// サービス変更(wService==0xFFFFならPAT先頭サービス)

	if (wService == 0xFFFF || wService < m_TsAnalyzer.GetViewableServiceNum()) {
		WORD wServiceID;

		if (wService == 0xFFFF) {
			// 先頭PMTが到着するまで失敗にする
			if (!m_TsAnalyzer.GetFirstViewableServiceID(&wServiceID))
				return false;
		} else {
			if (!m_TsAnalyzer.GetViewableServiceID(wService, &wServiceID))
				return false;
		}
		m_CurServiceIndex = m_TsAnalyzer.GetServiceIndexByID(wServiceID);

		m_CurServiceID = wServiceID;
		m_SpecServiceID = wServiceID;

		WORD wVideoPID = CMediaViewer::PID_INVALID;
		WORD wAudioPID = CMediaViewer::PID_INVALID;

		m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, &wVideoPID);
		if (!m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, m_CurAudioStream, &wAudioPID)
				&& m_CurAudioStream != 0) {
			m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, 0, &wAudioPID);
			m_CurAudioStream = 0;
		}

		TRACE(TEXT("------- Service Select -------\n"));
		TRACE(TEXT("%d (ServiceID = %04X)\n"), m_CurServiceIndex, wServiceID);

		m_MediaViewer.SetVideoPID(wVideoPID);
		m_MediaViewer.SetAudioPID(wAudioPID);

		if (m_bDescrambleCurServiceOnly)
			SetDescrambleService(wServiceID);

		if (m_bWriteCurServiceOnly)
			SetWriteStream(wServiceID, m_WriteStream);

		m_CaptionDecoder.SetTargetStream(wServiceID);

		return true;
	}

	return false;
}


/*
const WORD CDtvEngine::GetService(void)
{
	// サービス取得
	WORD ServiceID;
	if (!m_TsAnalyzer.GetServiceID(m_CurServiceIndex, &ServiceID))
		return SERVICE_INVALID;
	return m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
}
*/


const bool CDtvEngine::GetServiceID(WORD *pServiceID)
{
	// サービスID取得
	return m_TsAnalyzer.GetServiceID(m_CurServiceIndex, pServiceID);
}


const bool CDtvEngine::SetServiceByID(const WORD ServiceID, const bool bReserve)
{
	CBlockLock Lock(&m_EngineLock);

	// bReserve == true の場合、まだPATが来ていなくてもエラーにしない

	int Index = m_TsAnalyzer.GetViewableServiceIndexByID(ServiceID);
	if (Index < 0) {
		if (!bReserve || m_wCurTransportStream != 0)
			return false;
	} else {
		SetService(Index);
	}
	m_SpecServiceID = ServiceID;

	return true;
}


const unsigned __int64 CDtvEngine::GetPcrTimeStamp()
{
	// PCRタイムスタンプ取得
	unsigned __int64 TimeStamp;
	if (m_TsAnalyzer.GetPcrTimeStamp(m_CurServiceIndex, &TimeStamp))
		return TimeStamp;
	return 0ULL;
}


const DWORD CDtvEngine::OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	// デコーダからのイベントを受け取る(暫定)
	if (pDecoder == &m_TsAnalyzer) {
		switch (dwEventID) {
		case CTsAnalyzer::EVENT_PAT_UPDATED:
		case CTsAnalyzer::EVENT_PMT_UPDATED:
			// サービスの構成が変化した
			{
				CBlockLock Lock(&m_EngineLock);
				WORD wTransportStream = m_TsAnalyzer.GetTransportStreamID();
				bool bStreamChanged = m_wCurTransportStream != wTransportStream;

				if (bStreamChanged) {
					// ストリームIDが変わっているなら初期化
					TRACE(TEXT("CDtvEngine ■Stream Change!! %04X\n"), wTransportStream);

					m_CurServiceIndex = SERVICE_INVALID;
					m_CurServiceID = SID_INVALID;
					m_CurAudioStream = 0;
					m_wCurTransportStream = wTransportStream;

					bool bSetService = true;
					WORD Service = 0xFFFF;
					if (m_SpecServiceID != SID_INVALID) {
						// サービスが指定されている
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID);
						if (ServiceIndex < 0) {
							// サービスがPATにない
							if (m_TsAnalyzer.GetViewableServiceNum()>0) {
								TRACE(TEXT("Service not found %d\n"), m_SpecServiceID);
								m_SpecServiceID = SID_INVALID;
							} else {
								// まだ視聴可能なサービスのPMTが一つも来ていない場合は保留
								bSetService = false;
							}
						} else {
							const int ViewServiceIndex = m_TsAnalyzer.GetViewableServiceIndexByID(m_SpecServiceID);
							if (ViewServiceIndex >= 0) {
								Service = (WORD)ViewServiceIndex;
							} else if (!m_TsAnalyzer.IsServiceUpdated(ServiceIndex)) {
								// サービスはPATにあるが、まだPMTが来ていない
								bSetService = false;
							}
						}
					}
					if (!bSetService || !SetService(Service)) {
						m_MediaViewer.SetVideoPID(CMediaViewer::PID_INVALID);
						m_MediaViewer.SetAudioPID(CMediaViewer::PID_INVALID);
					}
				} else {
					// ストリームIDは同じだが、構成ESのPIDが変わった可能性がある
					TRACE(TEXT("CDtvEngine ■Stream Updated!! %04X\n"), wTransportStream);

					bool bSetService = true;
					WORD Service = 0xFFFF;
					if (m_SpecServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID);
						if (ServiceIndex < 0) {
							if (m_TsAnalyzer.GetViewableServiceNum()>0)
								m_SpecServiceID = SID_INVALID;
							else
								bSetService = false;
						} else {
							const int ViewServiceIndex = m_TsAnalyzer.GetViewableServiceIndexByID(m_SpecServiceID);
							if (ViewServiceIndex >= 0)
								Service = (WORD)ViewServiceIndex;
							else if (!m_TsAnalyzer.IsServiceUpdated(ServiceIndex))
								bSetService = false;
						}
					}
					if (bSetService && Service == 0xFFFF && m_CurServiceID != SID_INVALID) {
						const int ServiceIndex = m_TsAnalyzer.GetServiceIndexByID(m_CurServiceID);
						if (ServiceIndex < 0) {
							if (m_TsAnalyzer.GetViewableServiceNum()>0)
								m_CurServiceID = SID_INVALID;
							else
								bSetService = false;
						} else {
							const int ViewServiceIndex = m_TsAnalyzer.GetViewableServiceIndexByID(m_CurServiceID);
							if (ViewServiceIndex >= 0)
								Service = (WORD)ViewServiceIndex;
							else if (!m_TsAnalyzer.IsServiceUpdated(ServiceIndex))
								bSetService = false;
						}
					}
					if (bSetService)
						SetService(Service);
				}
				if (m_pEventHandler)
					m_pEventHandler->OnServiceListUpdated(&m_TsAnalyzer, bStreamChanged);
			}
			return 0UL;

		case CTsAnalyzer::EVENT_SDT_UPDATED:
			// サービスの情報が更新された
			if (m_pEventHandler)
				m_pEventHandler->OnServiceInfoUpdated(&m_TsAnalyzer);
			return 0UL;
		}
	}
	/* else if(pDecoder == &m_FileReader) {
		CFileReader *pFileReader = dynamic_cast<CFileReader *>(pDecoder);

		// ファイルリーダからのイベント
		switch(dwEventID){
		case CFileReader::EID_READ_ASYNC_START:
			// 非同期リード開始
			return 0UL;

		case CFileReader::EID_READ_ASYNC_END:
			// 非同期リード終了
			return 0UL;

		case CFileReader::EID_READ_ASYNC_POSTREAD:
			// 非同期リード後
			if (pFileReader->GetReadPos() >= pFileReader->GetFileSize()) {
				// 最初に巻き戻す(ループ再生)
				pFileReader->SetReadPos(0ULL);
				//pFileReader->Reset();
				ResetEngine();
			}
			return 0UL;
		}
	}*/ else if (pDecoder == &m_FileWriter) {
		switch (dwEventID) {
		case CBufferedFileWriter::EID_WRITE_ERROR:
			// 書き込みエラーが発生した
			if (m_pEventHandler)
				m_pEventHandler->OnFileWriteError(&m_FileWriter);
			return 0UL;
		}
	} else if (pDecoder == &m_MediaViewer) {
		switch (dwEventID) {
		case CMediaViewer::EID_VIDEO_SIZE_CHANGED:
			if (m_pEventHandler)
				m_pEventHandler->OnVideoSizeChanged(&m_MediaViewer);
			return 0UL;
		}
	} else if (pDecoder == &m_TsDescrambler) {
		switch (dwEventID) {
		case CTsDescrambler::EVENT_EMM_PROCESSED:
			// EMM処理が行われた
			if (m_pEventHandler)
				m_pEventHandler->OnEmmProcessed(static_cast<const BYTE*>(pParam));
			return 0UL;

		case CTsDescrambler::EVENT_ECM_ERROR:
			// ECM処理でエラーが発生した
			if (m_pEventHandler) {
				CTsDescrambler::EcmErrorInfo *pInfo = static_cast<CTsDescrambler::EcmErrorInfo*>(pParam);

				if (m_TsDescrambler.GetEcmPIDByServiceID(m_CurServiceID) == pInfo->EcmPID)
					m_pEventHandler->OnEcmError(pInfo->pszText);
			}
			return 0UL;

		case CTsDescrambler::EVENT_ECM_REFUSED:
			// ECMが受け付けられない
			if (m_pEventHandler) {
				CTsDescrambler::EcmErrorInfo *pInfo = static_cast<CTsDescrambler::EcmErrorInfo*>(pParam);

				if (m_TsDescrambler.GetEcmPIDByServiceID(m_CurServiceID) == pInfo->EcmPID)
					m_pEventHandler->OnEcmRefused();
			}
			return 0UL;

		case CTsDescrambler::EVENT_CARD_READER_HUNG:
			// カードリーダーから応答が無い
			if (m_pEventHandler)
				m_pEventHandler->OnCardReaderHung();
			return 0UL;
		}
	}

	return 0UL;
}


bool CDtvEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	if (!m_MediaViewer.OpenViewer(hwndHost,hwndMessage,VideoRenderer,
								  pszMpeg2Decoder,pszAudioDevice)) {
		SetError(m_MediaViewer.GetLastErrorException());
		return false;
	}
	return true;
}


bool CDtvEngine::RebuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	bool bOK;

	EnablePreview(false);
	m_MediaBuffer.SetOutputDecoder(NULL);
	m_MediaViewer.CloseViewer();
	bOK=m_MediaViewer.OpenViewer(hwndHost,hwndMessage,VideoRenderer,
								 pszMpeg2Decoder,pszAudioDevice);
	if (!bOK) {
		SetError(m_MediaViewer.GetLastErrorException());
	}
	if (m_bBuffering)
		m_MediaBuffer.SetOutputDecoder(&m_MediaViewer);

	return bOK;
}


bool CDtvEngine::CloseMediaViewer()
{
	m_MediaViewer.CloseViewer();
	return true;
}


bool CDtvEngine::ResetMediaViewer()
{
	if (!m_MediaViewer.IsOpen())
		return false;

	m_MediaViewer.Reset();

	if (m_bBuffering)
		m_MediaBuffer.Reset();

	CBlockLock Lock(&m_EngineLock);

	WORD VideoPID = CMediaViewer::PID_INVALID;
	WORD AudioPID = CMediaViewer::PID_INVALID;
	if (m_TsAnalyzer.GetVideoEsPID(m_CurServiceIndex, &VideoPID))
		m_MediaViewer.SetVideoPID(VideoPID);
	if (m_TsAnalyzer.GetAudioEsPID(m_CurServiceIndex, m_CurAudioStream, &AudioPID))
		m_MediaViewer.SetAudioPID(AudioPID);

	return true;
}


bool CDtvEngine::OpenBcasCard(CCardReader::ReaderType CardReaderType, LPCTSTR pszReaderName)
{
	// B-CASカードを開く
	if (CardReaderType!=CCardReader::READER_NONE) {
		Trace(TEXT("B-CASカードを開いています..."));
		if (!m_TsDescrambler.OpenBcasCard(CardReaderType,pszReaderName)) {
			TCHAR szText[256];

			if (m_TsDescrambler.GetLastErrorText()!=NULL)
				::wsprintf(szText,TEXT("B-CASカードの初期化に失敗しました。%s"),m_TsDescrambler.GetLastErrorText());
			else
				::lstrcpy(szText,TEXT("B-CASカードの初期化に失敗しました。"));
			SetError(0,szText,
					 TEXT("カードリーダが接続されているか、設定で有効なカードリーダが選択されているか確認してください。"),
					 m_TsDescrambler.GetLastErrorSystemMessage());
			return false;
		}
	} else if (m_TsDescrambler.IsBcasCardOpen()) {
		m_TsDescrambler.CloseBcasCard();
	}
	return true;
}


bool CDtvEngine::CloseBcasCard()
{
	if (m_TsDescrambler.IsBcasCardOpen())
		m_TsDescrambler.CloseBcasCard();
	return true;
}


bool CDtvEngine::SetDescramble(bool bDescramble)
{
	if (!m_bBuiled) {
		SetError(0,TEXT("内部エラー : DtvEngineが構築されていません。"));
		return false;
	}

	if (m_bDescramble != bDescramble) {
		m_TsDescrambler.EnableDescramble(bDescramble);
		m_bDescramble = bDescramble;
	}
	return true;
}


bool CDtvEngine::ResetBuffer()
{
	m_MediaBuffer.ResetBuffer();
	return true;
}


bool CDtvEngine::GetOriginalVideoSize(WORD *pWidth,WORD *pHeight)
{
	return m_MediaViewer.GetOriginalVideoSize(pWidth,pHeight);
}


bool CDtvEngine::SetDescrambleService(WORD ServiceID)
{
	return m_TsDescrambler.SetTargetServiceID(ServiceID);
}


bool CDtvEngine::SetDescrambleCurServiceOnly(bool bOnly)
{
	if (m_bDescrambleCurServiceOnly != bOnly) {
		WORD ServiceID = 0;

		m_bDescrambleCurServiceOnly = bOnly;
		if (bOnly)
			GetServiceID(&ServiceID);
		SetDescrambleService(ServiceID);
	}
	return true;
}


bool CDtvEngine::SetWriteStream(WORD ServiceID, DWORD Stream)
{
	return m_TsSelector.SetTargetServiceID(ServiceID, Stream);
}


bool CDtvEngine::GetWriteStream(WORD *pServiceID, DWORD *pStream)
{
	if (pServiceID)
		*pServiceID = m_TsSelector.GetTargetServiceID();
	if (pStream)
		*pStream = m_TsSelector.GetTargetStream();
	return true;
}


bool CDtvEngine::SetWriteCurServiceOnly(bool bOnly, DWORD Stream)
{
	if (m_bWriteCurServiceOnly != bOnly || m_WriteStream != Stream) {
		m_bWriteCurServiceOnly = bOnly;
		m_WriteStream = Stream;
		if (bOnly) {
			WORD ServiceID = 0;

			GetServiceID(&ServiceID);
			SetWriteStream(ServiceID, Stream);
		} else {
			SetWriteStream(0, Stream);
		}
	}
	return true;
}


void CDtvEngine::SetStartStreamingOnDriverOpen(bool bStart)
{
	m_bStartStreamingOnDriverOpen = bStart;
}


void CDtvEngine::SetTracer(CTracer *pTracer)
{
	CBonBaseClass::SetTracer(pTracer);
	m_MediaViewer.SetTracer(pTracer);
}


void CDtvEngine::ResetStatus()
{
	m_wCurTransportStream = 0;
	m_CurServiceIndex = SERVICE_INVALID;
	m_CurServiceID = SID_INVALID;
}
