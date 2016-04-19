#include "StdAfx.h"
#include <initguid.h>
#include <mmreg.h>
#include "AacDecFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// 周波数(48kHz)
static const int FREQUENCY = 48000;

// AACのフレーム当たりのサンプル数
static const int SAMPLES_PER_FRAME = 1024;

// REFERENCE_TIMEの一秒
static const REFERENCE_TIME REFERENCE_TIME_SECOND = 10000000LL;

// MediaSample用に確保するバッファ数
static const int NUM_SAMPLE_BUFFERS = 4;

// 5.1chダウンミックス設定
/*
本来の計算式 (STD-B21 6.2)
┌─┬──┬─┬───┬────────────────┐
│*1│*2  │*3│kの値 │計算式(*4)                      │
├─┼──┼─┼───┼────────────────┤
│ 1│ 0/1│ 0│1/√2 │Set1                            │
│  │    │ 1│1/2   │Lt=a*(L+1/√2*C＋k*Sl)          │
│  │    │ 2│1/2√2│Rt=a*(R+1/√2*C＋k*Sr)          │
│  │    │ 3│0     │a=1/√2                         │
├─┼──┼─┼───┼────────────────┤
│ 0│    │  │      │Set3                            │
│  │    │  │      │Lt=(1/√2)*(L+1/√2*C＋1/√2*Sl)│
│  │    │  │      │Rt=(1/√2)*(R+1/√2*C＋1/√2*Sr)│
└─┴──┴─┴───┴────────────────┘
*1 matrix_mixdown_idx_present
*2 pseudo_surround_enable
*3 matrix_mixdown_idx
*4 L=Left, R=Right, C=Center, Sl=Rear left, Sr=Rear right

必要な値は NeAACDecStruct.pce (NeAACDecStruct* = NeAACDecHandle) で参照できるが、
今のところそこまでしていない。
*/
#define RSQR	(1.0 / 1.4142135623730950488016887242097)
#define DMR_CENTER		RSQR
#define DMR_FRONT		1.0
#define DMR_REAR		RSQR
#define DMR_LFE			RSQR

// 整数演算によるダウンミックス
#define DOWNMIX_INT

#ifdef DOWNMIX_INT
#define IDOWNMIX_DENOM	4096
#define IDOWNMIX_CENTER	((int)(DMR_CENTER * IDOWNMIX_DENOM + 0.5))
#define IDOWNMIX_FRONT	((int)(DMR_FRONT * IDOWNMIX_DENOM + 0.5))
#define IDOWNMIX_REAR	((int)(DMR_REAR * IDOWNMIX_DENOM + 0.5))
#define IDOWNMIX_LFE	((int)(DMR_LFE * IDOWNMIX_DENOM + 0.5))
#endif


#if 0
static void OutputLog(LPCTSTR pszFormat, ...)
{
	va_list Args;
	TCHAR szText[1024];

	va_start(Args,pszFormat);
	int Length=::wvsprintf(szText,pszFormat,Args);
	va_end(Args);

	TRACE(szText);

	static bool fStopLog=false;
	static SIZE_T OutputSize=0;

	if (!fStopLog) {
		TCHAR szFileName[MAX_PATH];
		::GetModuleFileName(NULL,szFileName,MAX_PATH);
		::PathRenameExtension(szFileName,TEXT(".PassthroughTest.log"));
		HANDLE hFile=::CreateFile(szFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
								  OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			DWORD FileSize=::GetFileSize(hFile,NULL);
			DWORD Wrote;

#ifdef UNICODE
			if (FileSize==0) {
				static const WORD BOM=0xFEFF;
				::WriteFile(hFile,&BOM,2,&Wrote,NULL);
			}
#endif
			::SetFilePointer(hFile,0,NULL,FILE_END);
			SYSTEMTIME st;
			TCHAR szTime[32];
			::GetLocalTime(&st);
			int TimeLength=::wsprintf(szTime,TEXT("%02d:%02d:%02d.%03d "),st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
			::WriteFile(hFile,szTime,TimeLength*sizeof(TCHAR),&Wrote,NULL);
			::WriteFile(hFile,szText,Length*sizeof(TCHAR),&Wrote,NULL);
			OutputSize+=Length;
			if (OutputSize>=0x8000) {
				Length=::wsprintf(szText,TEXT("多くなりすぎたのでログの記録を停止します。\r\n\r\n"));
				::WriteFile(hFile,szText,Length*sizeof(TCHAR),&Wrote,NULL);
				fStopLog=true;
			}
			::FlushFileBuffers(hFile);
			::CloseHandle(hFile);
		}
	}
}
#else
#define OutputLog TRACE
#endif


CAacDecFilter::CAacDecFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(AACDECFILTER_NAME, pUnk, CLSID_AACDECFILTER)
	, m_AdtsParser(this)
	, m_AacDecoder(this)
	, m_byCurChannelNum(0)
	, m_bDualMono(false)

	, m_StereoMode(STEREOMODE_STEREO)
	, m_AutoStereoMode(STEREOMODE_STEREO)
	, m_bDownMixSurround(true)

	, m_SpdifOptions(SPDIF_MODE_DISABLED, 0)
	, m_bPassthrough(false)

	, m_pStreamCallback(NULL)

	, m_bGainControl(false)
	, m_Gain(1.0f)
	, m_SurroundGain(1.0f)

	, m_bAdjustStreamTime(false)
	, m_StartTime(-1)
	, m_SampleCount(0)
	, m_bDiscontinuity(true)
	, m_pAdtsFrame(NULL)
{
	TRACE(TEXT("CAacDecFilter::CAacDecFilter %p\n"), this);

	// メディアタイプ設定
	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Audio);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_PCM);
	m_MediaType.SetTemporalCompression(FALSE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_WaveFormatEx);

	 // フォーマット構造体確保
#if 1
	// 2ch
	WAVEFORMATEX *pWaveInfo = reinterpret_cast<WAVEFORMATEX *>(m_MediaType.AllocFormatBuffer(sizeof(WAVEFORMATEX)));
	if (pWaveInfo == NULL) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// WAVEFORMATEX構造体設定(48kHz 16bit ステレオ固定)
	pWaveInfo->wFormatTag = WAVE_FORMAT_PCM;
	pWaveInfo->nChannels = 2;
	pWaveInfo->nSamplesPerSec = FREQUENCY;
	pWaveInfo->wBitsPerSample = 16;
	pWaveInfo->nBlockAlign = pWaveInfo->wBitsPerSample * pWaveInfo->nChannels / 8;
	pWaveInfo->nAvgBytesPerSec = pWaveInfo->nSamplesPerSec * pWaveInfo->nBlockAlign;
	pWaveInfo->cbSize = 0;
#else
	// 5.1ch
	WAVEFORMATEXTENSIBLE *pWaveInfo = reinterpret_cast<WAVEFORMATEXTENSIBLE *>(m_MediaType.AllocFormatBuffer(sizeof(WAVEFORMATEXTENSIBLE)));
	if (pWaveInfo == NULL) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	// WAVEFORMATEXTENSIBLE構造体設定(48KHz 16bit 5.1ch固定)
	pWaveInfo->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
	pWaveInfo->Format.nChannels = 6;
	pWaveInfo->Format.nSamplesPerSec = FREQUENCY;
	pWaveInfo->Format.wBitsPerSample = 16;
	pWaveInfo->Format.nBlockAlign = pWaveInfo->Format.wBitsPerSample * pWaveInfo->Format.nChannels / 8;
	pWaveInfo->Format.nAvgBytesPerSec = pWaveInfo->Format.nSamplesPerSec * pWaveInfo->Format.nBlockAlign;
	pWaveInfo->Format.cbSize  = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
	pWaveInfo->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
							   SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
							   SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
	pWaveInfo->Samples.wValidBitsPerSample = 16;
	pWaveInfo->SubFormat = MEDIASUBTYPE_PCM;
#endif

	*phr = S_OK;
}

CAacDecFilter::~CAacDecFilter(void)
{
	TRACE(TEXT("CAacDecFilter::~CAacDecFilter\n"));
}

IBaseFilter* WINAPI CAacDecFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CAacDecFilter *pNewFilter = new CAacDecFilter(pUnk, phr);
	if (FAILED(*phr))
		goto OnError;

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_IBaseFilter, (void**)&pFilter);
	if (FAILED(*phr))
		goto OnError;

	return pFilter;

OnError:
	delete pNewFilter;
	return NULL;
}

HRESULT CAacDecFilter::CheckInputType(const CMediaType* mtIn)
{
    CheckPointer(mtIn, E_POINTER);

	// 何でもOK

	return S_OK;
}

HRESULT CAacDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Audio) {
		if (*mtOut->Subtype() == MEDIASUBTYPE_PCM) {

			// GUID_NULLではデバッグアサートが発生するのでダミーを設定して回避
			CMediaType MediaType;
			MediaType.InitMediaType();
			MediaType.SetType(&MEDIATYPE_Stream);
			MediaType.SetSubtype(&MEDIASUBTYPE_None);

			m_pInput->SetMediaType(&MediaType);

			return S_OK;
		}
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CAacDecFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	if (pprop->cBuffers < NUM_SAMPLE_BUFFERS)
		pprop->cBuffers = NUM_SAMPLE_BUFFERS;

	// フレーム当たりのサンプル数 * 16ビット * 5.1ch 分のバッファを確保する
	static const long BufferSize = SAMPLES_PER_FRAME * 2 * 6;
	if (pprop->cbBuffer < BufferSize)
		pprop->cbBuffer = BufferSize;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if (FAILED(hr))
		return hr;

	// 要求を受け入れられたか判定
	if (Actual.cBuffers < pprop->cBuffers || Actual.cbBuffer < pprop->cbBuffer)
		return E_FAIL;

	return S_OK;
}

HRESULT CAacDecFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pMediaType, E_POINTER);

	if (iPosition < 0)
		return E_INVALIDARG;
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}

HRESULT CAacDecFilter::StartStreaming(void)
{
	CAutoLock AutoLock(m_pLock);

	// ADTSパーサ初期化
	m_AdtsParser.Reset();

	// AACデコーダオープン
	if(!m_AacDecoder.OpenDecoder())
		return E_FAIL;

	m_StartTime = -1;
	m_SampleCount = 0;
	m_bDiscontinuity = true;

	m_BitRateCalculator.Initialize();

	return S_OK;
}

HRESULT CAacDecFilter::StopStreaming(void)
{
	CAutoLock AutoLock(m_pLock);

	// AACデコーダクローズ
	m_AacDecoder.CloseDecoder();

	m_BitRateCalculator.Reset();

	return S_OK;
}

HRESULT CAacDecFilter::BeginFlush()
{
	HRESULT hr = S_OK;

	m_AdtsParser.Reset();
	m_AacDecoder.ResetDecoder();
	m_StartTime = -1;
	m_SampleCount = 0;
	m_bDiscontinuity = true;
	if (m_pOutput) {
		hr = m_pOutput->DeliverBeginFlush();
	}
	return hr;
}

const BYTE CAacDecFilter::GetCurrentChannelNum()
{
	if (m_byCurChannelNum == 0)
		return CHANNEL_INVALID;
	if (m_bDualMono)
		return CHANNEL_DUALMONO;
	return m_byCurChannelNum;
}

HRESULT CAacDecFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock AutoLock(m_pLock);

	// 入力データポインタを取得する
	BYTE *pInData;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;

	// ADTSパーサに入力
	LONG InSize = pIn->GetActualDataLength();
	m_AdtsParser.StoreEs(pInData, InSize);

	m_BitRateCalculator.Update(InSize);

#if 0
	if (!m_bPassthrough) {
		if (pOut->GetActualDataLength() == 0)
			return S_FALSE;

		if (m_bAdjustStreamTime) {
			// ストリーム時間を実際のサンプルの長さを元に設定する
			REFERENCE_TIME StartTime, EndTime;

			hr = pIn->GetTime(&StartTime, &EndTime);
			if (hr == S_OK || hr == VFW_S_NO_STOP_TIME) {
				if (m_StartTime >= 0) {
					REFERENCE_TIME CurTime = m_StartTime + (m_SampleCount * REFERENCE_TIME_SECOND / FREQUENCY);
					if (_abs64(StartTime - CurTime) > REFERENCE_TIME_SECOND / 5LL) {
						// TODO: いきなりリセットしないで徐々に合わせる
						TRACE(TEXT("Reset audio time\n"));
						m_StartTime = StartTime;
						m_SampleCount = 0;
					}
				} else {
					m_StartTime = StartTime;
					m_SampleCount = 0;
				}
			}
			if (m_StartTime >= 0) {
				DWORD Samples = pOut->GetActualDataLength() / sizeof(short);
				if (m_bDownMixSurround || m_byCurChannelNum == 2)
					Samples /= 2;
				else
					Samples /= 6;
				StartTime = m_StartTime + (m_SampleCount * REFERENCE_TIME_SECOND / FREQUENCY);
				m_SampleCount += Samples;
				EndTime = m_StartTime + (m_SampleCount * REFERENCE_TIME_SECOND / FREQUENCY) - 1;
				pOut->SetTime(&StartTime, &EndTime);
			}
		}
	}
#endif

	return S_OK;
}

HRESULT CAacDecFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	HRESULT hr;

	/*if (!m_bPassthrough) {
		IMediaSample *pOutSample;

		hr = InitializeOutputSample(pSample, &pOutSample);
		if (FAILED(hr))
			return hr;
		hr = Transform(pSample, pOutSample);
		if (SUCCEEDED(hr)) {
			if (hr == S_OK) {
				hr = m_pOutput->Deliver(pOutSample);
			} else {
				hr = S_OK;
			}
			m_bSampleSkipped = FALSE;
		}
		pOutSample->Release();
	} else */{
		REFERENCE_TIME rtStart = -1, rtEnd;
		hr = pSample->GetTime(&rtStart, &rtEnd);
		if (pSample->IsDiscontinuity() == S_OK) {
			m_bDiscontinuity = true;
		} else if (hr == S_OK) {
			if (!m_bAdjustStreamTime) {
				m_StartTime = rtStart;
			} else if (m_StartTime >= 0 && _abs64(rtStart - m_StartTime) >= REFERENCE_TIME_SECOND / 5LL) {
				TRACE(TEXT("Resync audio stream time\n"));
				m_StartTime = rtStart;
			}
		}
		if (m_StartTime < 0 || m_bDiscontinuity) {
			TRACE(TEXT("Initialize audio stream time\n"));
			m_StartTime = rtStart;
		}

		hr = Transform(pSample, NULL);
		if (SUCCEEDED(hr)) {
			hr = S_OK;
			m_bSampleSkipped = FALSE;
		}
	}

	return hr;
}

void CAacDecFilter::OnPcmFrame(const CAacDecoder *pAacDecoder, const BYTE *pData, const DWORD dwSamples, const BYTE byChannels)
{
	if (byChannels != 1 && byChannels != 2 && byChannels != 6)
		return;

	const bool bDualMono = byChannels == 2 && pAacDecoder->GetChannelConfig() == 0;
	const bool bSurround = byChannels == 6 && !m_bDownMixSurround;

	bool bPassthrough = false;
	if (m_SpdifOptions.Mode == SPDIF_MODE_PASSTHROUGH) {
		bPassthrough = true;
	} else if (m_SpdifOptions.Mode == SPDIF_MODE_AUTO) {
		UINT ChannelFlag;
		if (bDualMono) {
			ChannelFlag = SPDIF_CHANNELS_DUALMONO;
		} else {
			switch (byChannels) {
			case 1: ChannelFlag = SPDIF_CHANNELS_MONO;		break;
			case 2: ChannelFlag = SPDIF_CHANNELS_STEREO;	break;
			case 6: ChannelFlag = SPDIF_CHANNELS_SURROUND;	break;
			}
		}
		if ((m_SpdifOptions.PassthroughChannels & ChannelFlag) != 0)
			bPassthrough = true;
	}

	m_bPassthrough = bPassthrough;

	if (bDualMono != m_bDualMono) {
		m_bDualMono = bDualMono;
		if (bDualMono) {
			m_StereoMode = m_AutoStereoMode;
			TRACE(TEXT("CAacDecFilter : Change stereo mode %d\n"), m_StereoMode);
		} else if (m_AutoStereoMode != STEREOMODE_STEREO) {
			m_StereoMode = STEREOMODE_STEREO;
			TRACE(TEXT("CAacDecFilter : Reset stereo mode\n"));
		}
	}

	m_byCurChannelNum = byChannels;

	if (m_bPassthrough) {
		OutputSpdif();
		return;
	}

	HRESULT hr;

	// メディアタイプの更新
	bool bMediaTypeChanged = false;
	WAVEFORMATEX *pwfx = reinterpret_cast<WAVEFORMATEX*>(m_MediaType.Format());
	if (*m_MediaType.FormatType() != FORMAT_WaveFormatEx
			|| (!bSurround && pwfx->wFormatTag != WAVE_FORMAT_PCM)
			|| (bSurround && pwfx->wFormatTag != WAVE_FORMAT_EXTENSIBLE)) {
		CMediaType mt;
		mt.SetType(&MEDIATYPE_Audio);
		mt.SetSubtype(&MEDIASUBTYPE_PCM);
		mt.SetFormatType(&FORMAT_WaveFormatEx);

		pwfx = reinterpret_cast<WAVEFORMATEX*>(
			mt.AllocFormatBuffer(bSurround ? sizeof (WAVEFORMATEXTENSIBLE) : sizeof(WAVEFORMATEX)));
		if (pwfx == NULL)
			return;
		if (!bSurround) {
			pwfx->wFormatTag = WAVE_FORMAT_PCM;
			pwfx->nChannels = 2;
			pwfx->cbSize = 0;
		} else {
			WAVEFORMATEXTENSIBLE *pExtensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
			pExtensible->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
			pExtensible->Format.nChannels = 6;
			pExtensible->Format.cbSize  = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
			pExtensible->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
										 SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
										 SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
			pExtensible->Samples.wValidBitsPerSample = 16;
			pExtensible->SubFormat = MEDIASUBTYPE_PCM;
		}
		pwfx->nSamplesPerSec = FREQUENCY;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		mt.SetSampleSize(pwfx->nBlockAlign);

		hr = ReconnectOutput(/*dwSamples*/SAMPLES_PER_FRAME * pwfx->nBlockAlign, mt);
		if (FAILED(hr))
			return;

		if (hr == S_OK) {
			OutputLog(TEXT("出力メディアタイプを更新します。\r\n"));
			hr = m_pOutput->SetMediaType(&mt);
			if (FAILED(hr)) {
				OutputLog(TEXT("出力メディアタイプを設定できません。(%08x)\r\n"), hr);
				return;
			}
			m_MediaType = mt;
			m_bDiscontinuity = true;
			bMediaTypeChanged = true;
		}
	}

	IMediaSample *pOutSample = NULL;
	hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
	if (FAILED(hr) || !pOutSample) {
		OutputLog(TEXT("出力メディアサンプルを取得できません。(%08x)\r\n"), hr);
		return;
	}
	if (bMediaTypeChanged)
		pOutSample->SetMediaType(&m_MediaType);

	// 出力ポインタ取得
	BYTE *pOutBuff = NULL;
	if (FAILED(pOutSample->GetPointer(&pOutBuff)) || !pOutBuff) {
		pOutSample->Release();
		OutputLog(TEXT("サンプルのバッファを取得できません。(%08x)\r\n"), hr);
		return;
	}

	DWORD dwOutSize;

	// ダウンミックス
	switch (byChannels) {
	case 1U:
		dwOutSize = MonoToStereo((short *)pOutBuff, (const short *)pData, dwSamples);
		break;
	case 2U:
		dwOutSize = DownMixStereo((short *)pOutBuff, (const short *)pData, dwSamples);
		break;
	case 6U:
		if (bSurround) {
			dwOutSize = MapSurroundChannels((short *)pOutBuff, (const short *)pData, dwSamples);
		} else {
			dwOutSize = DownMixSurround((short *)pOutBuff, (const short *)pData, dwSamples);
		}
		break;
	}

	if (m_bGainControl && (byChannels < 6 || bSurround))
		GainControl((short*)pOutBuff, dwOutSize / sizeof(short),
					bSurround ? m_SurroundGain : m_Gain);

	if (m_pStreamCallback)
		m_pStreamCallback((short*)pOutBuff, dwSamples, bSurround ? 6 : 2, m_pStreamCallbackParam);

	// メディアサンプル有効サイズ設定
	pOutSample->SetActualDataLength(dwOutSize);

	if (m_StartTime >= 0) {
		REFERENCE_TIME rtDuration, rtEnd;
		rtDuration = REFERENCE_TIME_SECOND * (LONGLONG)dwSamples / FREQUENCY;
		rtEnd = m_StartTime + rtDuration;
		pOutSample->SetTime(&m_StartTime, &rtEnd);
		m_StartTime = rtEnd;
	}
	pOutSample->SetMediaTime(NULL, NULL);
	pOutSample->SetPreroll(FALSE);
	pOutSample->SetDiscontinuity(m_bDiscontinuity);
	m_bDiscontinuity = false;
	pOutSample->SetSyncPoint(TRUE);

	m_pOutput->Deliver(pOutSample);
	pOutSample->Release();
}

const DWORD CAacDecFilter::MonoToStereo(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 1ch → 2ch 二重化
	const short *p = pSrc, *pEnd = pSrc + dwSamples;
	short *q = pDst;

	while (p < pEnd) {
		short Value = *p++;
		*q++ = Value;	// L
		*q++ = Value;	// R
	}

	// バッファサイズを返す
	return dwSamples * (sizeof(short) * 2);
}

const DWORD CAacDecFilter::DownMixStereo(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	if (m_StereoMode == STEREOMODE_STEREO) {
		// 2ch → 2ch スルー
		::CopyMemory(pDst, pSrc, dwSamples * (sizeof(short) * 2));
	} else {
		const short *p = pSrc, *pEnd = pSrc + dwSamples * 2;
		short *q = pDst;

		if (m_StereoMode == STEREOMODE_LEFT) {
			// 左のみ
			while (p < pEnd) {
				short Value = *p;
				*q++ = Value;	// L
				*q++ = Value;	// R
				p += 2;
			}
		} else {
			// 右のみ
			while (p < pEnd) {
				short Value = p[1];
				*q++ = Value;	// L
				*q++ = Value;	// R
				p += 2;
			}
		}
	}

	// バッファサイズを返す
	return dwSamples * (sizeof(short) * 2);
}

#ifndef DOWNMIX_INT

/*
	浮動小数点数演算版(オリジナル)
*/
const DWORD CAacDecFilter::DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 5.1ch → 2ch ダウンミックス

	const double Level = m_bGainControl ? m_SurroundGain : 1.0;
	int iOutLch, iOutRch;

	for(DWORD dwPos = 0UL ; dwPos < dwSamples ; dwPos++){
		// ダウンミックス
		iOutLch = (int)((
					(double)pSrc[dwPos * 6UL + 1UL]	* DMR_FRONT		+
					(double)pSrc[dwPos * 6UL + 3UL]	* DMR_REAR		+
					(double)pSrc[dwPos * 6UL + 0UL]	* DMR_CENTER	+
					(double)pSrc[dwPos * 6UL + 5UL]	* DMR_LFE
					) * Level);

		iOutRch = (int)((
					(double)pSrc[dwPos * 6UL + 2UL]	* DMR_FRONT		+
					(double)pSrc[dwPos * 6UL + 4UL]	* DMR_REAR		+
					(double)pSrc[dwPos * 6UL + 0UL]	* DMR_CENTER	+
					(double)pSrc[dwPos * 6UL + 5UL]	* DMR_LFE
					) * Level);

		// クリップ
		if(iOutLch > 32767L)iOutLch = 32767L;
		else if(iOutLch < -32768L)iOutLch = -32768L;

		if(iOutRch > 32767L)iOutRch = 32767L;
		else if(iOutRch < -32768L)iOutRch = -32768L;

		pDst[dwPos * 2UL + 0UL] = (short)iOutLch;	// L
		pDst[dwPos * 2UL + 1UL] = (short)iOutRch;	// R
		}

	// バッファサイズを返す
	return dwSamples * (sizeof(short) * 2);
}

#else	// DOWNMIX_INT

/*
	整数演算版
*/
const DWORD CAacDecFilter::DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 5.1ch → 2ch ダウンミックス

	int iOutLch, iOutRch;

	if (!m_bGainControl) {
		for (DWORD dwPos = 0UL ; dwPos < dwSamples ; dwPos++) {
			// ダウンミックス
			iOutLch =	((int)pSrc[dwPos * 6UL + 1UL] * IDOWNMIX_FRONT	+
						 (int)pSrc[dwPos * 6UL + 3UL] * IDOWNMIX_REAR	+
						 (int)pSrc[dwPos * 6UL + 0UL] * IDOWNMIX_CENTER	+
						 (int)pSrc[dwPos * 6UL + 5UL] * IDOWNMIX_LFE) /
						IDOWNMIX_DENOM;

			iOutRch =	((int)pSrc[dwPos * 6UL + 2UL] * IDOWNMIX_FRONT	+
						 (int)pSrc[dwPos * 6UL + 4UL] * IDOWNMIX_REAR	+
						 (int)pSrc[dwPos * 6UL + 0UL] * IDOWNMIX_CENTER	+
						 (int)pSrc[dwPos * 6UL + 5UL] * IDOWNMIX_LFE) /
						IDOWNMIX_DENOM;

			// クリップ
			if(iOutLch > 32767)iOutLch = 32767;
			else if(iOutLch < -32768L)iOutLch = -32768;

			if(iOutRch > 32767)iOutRch = 32767;
			else if(iOutRch < -32768)iOutRch = -32768;

			pDst[dwPos * 2UL + 0UL] = (short)iOutLch;	// L
			pDst[dwPos * 2UL + 1UL] = (short)iOutRch;	// R
		}
	} else {
		static const int Factor = 4096;
		static const LONGLONG Divisor = Factor * IDOWNMIX_DENOM;
		const LONGLONG Level = (LONGLONG)(m_SurroundGain * (float)Factor);

		for (DWORD dwPos = 0UL ; dwPos < dwSamples ; dwPos++) {
			// ダウンミックス
			iOutLch =	(int)pSrc[dwPos * 6UL + 1UL] * IDOWNMIX_FRONT	+
						(int)pSrc[dwPos * 6UL + 3UL] * IDOWNMIX_REAR	+
						(int)pSrc[dwPos * 6UL + 0UL] * IDOWNMIX_CENTER	+
						(int)pSrc[dwPos * 6UL + 5UL] * IDOWNMIX_LFE;

			iOutRch =	(int)pSrc[dwPos * 6UL + 2UL] * IDOWNMIX_FRONT	+
						(int)pSrc[dwPos * 6UL + 4UL] * IDOWNMIX_REAR	+
						(int)pSrc[dwPos * 6UL + 0UL] * IDOWNMIX_CENTER	+
						(int)pSrc[dwPos * 6UL + 5UL] * IDOWNMIX_LFE;

			iOutLch = (int)((LONGLONG)iOutLch * Level / Divisor);
			iOutRch = (int)((LONGLONG)iOutRch * Level / Divisor);

			// クリップ
			if(iOutLch > 32767)iOutLch = 32767;
			else if(iOutLch < -32768L)iOutLch = -32768;

			if(iOutRch > 32767)iOutRch = 32767;
			else if(iOutRch < -32768)iOutRch = -32768;

			pDst[dwPos * 2UL + 0UL] = (short)iOutLch;	// L
			pDst[dwPos * 2UL + 1UL] = (short)iOutRch;	// R
		}
	}

	// バッファサイズを返す
	return dwSamples * (sizeof(short) * 2);
}

#endif	// DOWNMIX_INT

const DWORD CAacDecFilter::MapSurroundChannels(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	for (DWORD i = 0 ; i < dwSamples ; i++) {
		pDst[i * 6 + 0] = pSrc[i * 6 + 1];	// Front left
		pDst[i * 6 + 1] = pSrc[i * 6 + 2];	// Font right
		pDst[i * 6 + 2] = pSrc[i * 6 + 0];	// Front center
		pDst[i * 6 + 3] = pSrc[i * 6 + 5];	// Low frequency
		pDst[i * 6 + 4] = pSrc[i * 6 + 3];	// Back left
		pDst[i * 6 + 5] = pSrc[i * 6 + 4];	// Back right
	}

	// バッファサイズを返す
	return dwSamples * (sizeof(short) * 6);
}

bool CAacDecFilter::ResetDecoder()
{
	CAutoLock AutoLock(m_pLock);

	return m_AacDecoder.ResetDecoder();
}

bool CAacDecFilter::SetStereoMode(int StereoMode)
{
	CAutoLock AutoLock(m_pLock);

	switch (StereoMode) {
	case STEREOMODE_STEREO:
	case STEREOMODE_LEFT:
	case STEREOMODE_RIGHT:
		m_StereoMode = StereoMode;
		TRACE(TEXT("CAacDecFilter : Stereo mode %d\n"), StereoMode);
		return true;
	}
	return false;
}

bool CAacDecFilter::SetAutoStereoMode(int StereoMode)
{
	CAutoLock AutoLock(m_pLock);

	switch (StereoMode) {
	case STEREOMODE_STEREO:
	case STEREOMODE_LEFT:
	case STEREOMODE_RIGHT:
		m_AutoStereoMode = StereoMode;
		TRACE(TEXT("CAacDecFilter : Auto stereo mode %d\n"), StereoMode);
		return true;
	}
	return false;
}

bool CAacDecFilter::SetDownMixSurround(bool bDownMix)
{
	CAutoLock AutoLock(m_pLock);

	m_bDownMixSurround = bDownMix;
	return true;
}

bool CAacDecFilter::SetGainControl(bool bGainControl, float Gain, float SurroundGain)
{
	CAutoLock AutoLock(m_pLock);

	m_bGainControl = bGainControl;
	m_Gain = Gain;
	m_SurroundGain = SurroundGain;
	return true;
}

bool CAacDecFilter::GetGainControl(float *pGain, float *pSurroundGain) const
{
	if (pGain)
		*pGain = m_Gain;
	if (pSurroundGain)
		*pSurroundGain = m_SurroundGain;
	return m_bGainControl;
}

void CAacDecFilter::GainControl(short *pBuffer, const DWORD Samples, const float Gain)
{
	static const int Factor = 0x1000;
	const int Level = (int)(Gain * (float)Factor);
	short *p, *pEnd;
	int Value;

	if (Level == Factor)
		return;
	p= pBuffer;
	pEnd= p + Samples;
	while (p < pEnd) {
		Value = ((int)*p * Level) / Factor;
		*p++ = Value > 32767 ? 32767 : Value < -32768 ? -32768 : Value;
	}
}

bool CAacDecFilter::SetSpdifOptions(const SpdifOptions *pOptions)
{
	if (!pOptions)
		return false;

	CAutoLock AutoLock(m_pLock);

	m_SpdifOptions = *pOptions;

	return true;
}

bool CAacDecFilter::GetSpdifOptions(SpdifOptions *pOptions) const
{
	if (!pOptions)
		return false;

	CAutoLock AutoLock(m_pLock);

	*pOptions = m_SpdifOptions;

	return true;
}

bool CAacDecFilter::SetAdjustStreamTime(bool bAdjust)
{
	CAutoLock AutoLock(m_pLock);

	m_bAdjustStreamTime = bAdjust;

	m_StartTime = -1;
	m_SampleCount = 0;

	return true;
}

bool CAacDecFilter::SetStreamCallback(StreamCallback pCallback, void *pParam)
{
	CAutoLock AutoLock(m_pLock);

	m_pStreamCallback = pCallback;
	m_pStreamCallbackParam = pParam;

	return true;
}

DWORD CAacDecFilter::GetBitRate() const
{
	return m_BitRateCalculator.GetBitRate();
}

void CAacDecFilter::OnAdtsFrame(const CAdtsParser *pAdtsParser, const CAdtsFrame *pFrame)
{
	m_pAdtsFrame = pFrame;

	/*
		S/PDIFパススルー時にもデコードするのは無駄に見えるが、
		ステレオなのにchannel_configurationが1になっていたりして
		ADTSヘッダの情報が信用できないので実際にデコードしてみる必要がある
	*/
	m_AacDecoder.Decode(pFrame);

	m_pAdtsFrame = NULL;
}

void CAacDecFilter::OutputSpdif()
{
	static const int PREAMBLE_SIZE = sizeof(WORD) * 4;

	const CAdtsFrame *pFrame = m_pAdtsFrame;

	if (pFrame->GetRawDataBlockNum() != 0) {
		OutputLog(TEXT("no_raw_data_blocks_in_frameが不正です。(%d)\r\n"), pFrame->GetRawDataBlockNum());
		return;
	}

	HRESULT hr;

	const int FrameSize = pFrame->GetFrameLength();
	int DataBurstSize = PREAMBLE_SIZE + FrameSize;
	static const int PacketSize = SAMPLES_PER_FRAME * 4;
	if (DataBurstSize > PacketSize) {
		OutputLog(TEXT("ビットレートが不正です。(Frame size %d / Data-burst size %d / Packet size %d)\r\n"),
				  FrameSize, DataBurstSize, PacketSize);
		return;
	}

#ifdef _DEBUG
	static bool bFirst=true;
	if (bFirst) {
		OutputLog(TEXT("出力開始(Frame size %d / Data-burst size %d / Packet size %d)\r\n"),
				  FrameSize, DataBurstSize, PacketSize);
		bFirst=false;
	}
#endif

	bool bMediaTypeChanged = false;
	WAVEFORMATEX *pwfx = reinterpret_cast<WAVEFORMATEX*>(m_MediaType.Format());
	if (*m_MediaType.FormatType() != FORMAT_WaveFormatEx
			|| pwfx->wFormatTag != WAVE_FORMAT_DOLBY_AC3_SPDIF) {
		CMediaType mt;
		mt.SetType(&MEDIATYPE_Audio);
		mt.SetSubtype(&MEDIASUBTYPE_PCM);
		mt.SetFormatType(&FORMAT_WaveFormatEx);

		pwfx = reinterpret_cast<WAVEFORMATEX*>(mt.AllocFormatBuffer(sizeof(WAVEFORMATEX)));
		if (pwfx == NULL)
			return;
		pwfx->wFormatTag = WAVE_FORMAT_DOLBY_AC3_SPDIF;
		pwfx->nChannels = 2;
		pwfx->nSamplesPerSec = FREQUENCY;
		pwfx->wBitsPerSample = 16;
		pwfx->nBlockAlign = pwfx->nChannels * pwfx->wBitsPerSample / 8;
		pwfx->nAvgBytesPerSec = pwfx->nSamplesPerSec * pwfx->nBlockAlign;
		pwfx->cbSize = 0;
		/*
		// Windows 7 では WAVEFORMATEXTENSIBLE_IEC61937 を使った方がいい?
		WAVEFORMATEXTENSIBLE_IEC61937 *pwfx;
		...
		pwfx->FormatExt.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		...
		pwfx->FormatExt.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE_IEC61937) - sizeof(WAVEFORMATEX);
		pwfx->FormatExt.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT |
										SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY |
										SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
		pwfx->FormatExt.Samples.wValidBitsPerSample = 16;
		pwfx->FormatExt.SubFormat = KSDATAFORMAT_SUBTYPE_IEC61937_AAC;
		pwfx->dwEncodedSamplesPerSec = FREQUENCY;
		pwfx->dwEncodedChannelCount = 6;
		pwfx->dwAverageBytesPerSec = 0;
		*/
		mt.SetSampleSize(pwfx->nBlockAlign);

		hr = ReconnectOutput(PacketSize, mt);
		if (FAILED(hr))
			return;

		if (hr == S_OK) {
			OutputLog(TEXT("出力メディアタイプを更新します。\r\n"));
			hr = m_pOutput->SetMediaType(&mt);
			if (FAILED(hr)) {
				OutputLog(TEXT("出力メディアタイプを設定できません。(%08x)\r\n"), hr);
				return;
			}
			m_MediaType = mt;
			m_bDiscontinuity = true;
			bMediaTypeChanged = true;
		}
	}

	IMediaSample *pOutSample = NULL;
	hr = m_pOutput->GetDeliveryBuffer(&pOutSample, NULL, NULL, 0);
	if (FAILED(hr) || !pOutSample) {
		OutputLog(TEXT("出力メディアサンプルを取得できません。(%08x)\r\n"), hr);
		return;
	}
	if (bMediaTypeChanged)
		pOutSample->SetMediaType(&m_MediaType);

	BYTE *pOutBuff = NULL;
	hr = pOutSample->GetPointer(&pOutBuff);
	if (FAILED(hr) || !pOutBuff) {
		pOutSample->Release();
		OutputLog(TEXT("サンプルのバッファを取得できません。(%08x)\r\n"), hr);
		return;
	}

	WORD *pWordData = reinterpret_cast<WORD*>(pOutBuff);
	// Burst-preamble
	pWordData[0] = 0xF872;						// Pa(Sync word 1)
	pWordData[1] = 0x4E1F;						// Pb(Sync word 2)
	pWordData[2] = 0x0007;						// Pc(Burst-info = MPEG2 AAC ADTS)
	//pWordData[3] = ((FrameSize + 1) & ~1) * 8;	// Pd(Length-code)
	pWordData[3] = FrameSize * 8;				// Pd(Length-code)
	// Burst-payload
	_swab((char*)pFrame->GetData(), (char*)&pWordData[4], FrameSize & ~1);
	if (FrameSize & 1) {
		pOutBuff[PREAMBLE_SIZE + FrameSize - 1] = 0;
		pOutBuff[PREAMBLE_SIZE + FrameSize] = pFrame->GetAt(FrameSize - 1);
		DataBurstSize++;
	}
	// Stuffing
	if (DataBurstSize < PacketSize)
		::ZeroMemory(&pOutBuff[DataBurstSize], PacketSize - DataBurstSize);

	pOutSample->SetActualDataLength(PacketSize);

	if (m_StartTime >= 0) {
		REFERENCE_TIME rtDuration, rtEnd;
		rtDuration = REFERENCE_TIME_SECOND * (LONGLONG)SAMPLES_PER_FRAME / FREQUENCY;
		rtEnd = m_StartTime + rtDuration;
		pOutSample->SetTime(&m_StartTime, &rtEnd);
		m_StartTime = rtEnd;
	}
	pOutSample->SetMediaTime(NULL, NULL);
	pOutSample->SetPreroll(FALSE);
	pOutSample->SetDiscontinuity(m_bDiscontinuity);
	m_bDiscontinuity = false;
	pOutSample->SetSyncPoint(TRUE);

	hr = m_pOutput->Deliver(pOutSample);
	pOutSample->Release();
	if (FAILED(hr)) {
		OutputLog(TEXT("サンプルの送信エラー(%08x)\r\n"), hr);
		return;
	}
}

HRESULT CAacDecFilter::ReconnectOutput(long BufferSize, const CMediaType &mt)
{
	HRESULT hr;

	IPin *pPin = m_pOutput->GetConnected();
	if (pPin == NULL)
		return E_POINTER;

	IMemInputPin *pMemInputPin = NULL;
	hr = pPin->QueryInterface(IID_IMemInputPin, reinterpret_cast<void**>(&pMemInputPin));
	if (FAILED(hr)) {
		OutputLog(TEXT("IMemInputPinインターフェースが取得できません。(%08x)\r\n"), hr);
	} else {
		IMemAllocator *pAllocator = NULL;
		hr = pMemInputPin->GetAllocator(&pAllocator);
		if (FAILED(hr)) {
			OutputLog(TEXT("IMemAllocatorインターフェースが取得できません。(%08x)\r\n"), hr);
		} else {
			ALLOCATOR_PROPERTIES Props;
			hr = pAllocator->GetProperties(&Props);
			if (FAILED(hr)) {
				OutputLog(TEXT("IMemAllocatorのプロパティが取得できません。(%08x)\r\n"), hr);
			} else {
				if (mt != m_pOutput->CurrentMediaType()
						|| Props.cBuffers < NUM_SAMPLE_BUFFERS
						|| Props.cbBuffer < BufferSize) {
					hr = S_OK;
					if (Props.cBuffers < NUM_SAMPLE_BUFFERS
							|| Props.cbBuffer < BufferSize) {
						ALLOCATOR_PROPERTIES ActualProps;

						Props.cBuffers = NUM_SAMPLE_BUFFERS;
						Props.cbBuffer = BufferSize * 3 / 2;
						OutputLog(TEXT("バッファサイズを設定します。(%ld bytes)\r\n"), Props.cbBuffer);
						if (SUCCEEDED(hr = m_pOutput->DeliverBeginFlush())
								&& SUCCEEDED(hr = m_pOutput->DeliverEndFlush())
								&& SUCCEEDED(hr = pAllocator->Decommit())
								&& SUCCEEDED(hr = pAllocator->SetProperties(&Props, &ActualProps))
								&& SUCCEEDED(hr = pAllocator->Commit())) {
							if (ActualProps.cBuffers < Props.cBuffers
									|| ActualProps.cbBuffer < BufferSize) {
								OutputLog(TEXT("バッファサイズの要求が受け付けられません。(%ld / %ld)\r\n"),
										  ActualProps.cbBuffer, Props.cbBuffer);
								hr = E_FAIL;
								NotifyEvent(EC_ERRORABORT, hr, 0);
							} else {
								OutputLog(TEXT("ピンの再接続成功\r\n"));
								hr = S_OK;
							}
						} else {
							OutputLog(TEXT("ピンの再接続ができません。(%08x)\r\n"), hr);
						}
					}
				} else {
					hr = S_FALSE;
				}
			}

			pAllocator->Release();
		}

		pMemInputPin->Release();
	}

	return hr;
}
