#include "StdAfx.h"
#include <initguid.h>
#include "H264ParserFilter.h"
#include "DirectShowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/*
	処理内容はワンセグ仕様前提なので、それ以外の場合は問題が出ると思います。
*/

// REFERENCE_TIMEの一秒
#define REFERENCE_TIME_SECOND 10000000LL

#ifdef BONTSENGINE_1SEG_SUPPORT

// フレームレート
#define FRAME_RATE (REFERENCE_TIME_SECOND * 15000 / 1001)
// フレームの表示時間を算出
#define FRAME_TIME(time) ((LONGLONG)(time) * REFERENCE_TIME_SECOND * 1001 / 15000)
// バッファサイズ
#define BUFFER_SIZE 0x100000L

// 初期値はほとんど何でもいいみたい
// (ワンセグ用の設定でも BD の m2ts を再生できる)
#define INITIAL_BITRATE	320000
#define INITIAL_WIDTH	320
#define INITIAL_HEIGHT	240

#else

#define FRAME_RATE (REFERENCE_TIME_SECOND * 30000 / 1001)
#define FRAME_TIME(time) ((LONGLONG)(time) * REFERENCE_TIME_SECOND * 1001 / 30000)
#define BUFFER_SIZE 0x800000L
#define INITIAL_BITRATE	32000000;
#define INITIAL_WIDTH	1920;
#define INITIAL_HEIGHT	1080;

#endif


CH264ParserFilter::CH264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(H264PARSERFILTER_NAME, pUnk, CLSID_H264ParserFilter)
	, m_pfnVideoInfoCallback(NULL)
	, m_pCallbackParam(NULL)
	, m_H264Parser(this)
	, m_bAdjustTime(
#ifdef BONTSENGINE_1SEG_SUPPORT
		true
#else
		false
#endif
	  )
	, m_bAdjustFrameRate(true)
	, m_bAttachMediaType(false)
{
	TRACE(TEXT("CH264ParserFilter::CH264ParserFilter() %p\n"),this);

	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Video);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_H264);
	m_MediaType.SetTemporalCompression(TRUE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_VideoInfo);
	VIDEOINFOHEADER *pvih = reinterpret_cast<VIDEOINFOHEADER*>(m_MediaType.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	if (!pvih) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	::ZeroMemory(pvih, sizeof(VIDEOINFOHEADER));
	pvih->dwBitRate = INITIAL_BITRATE;
	pvih->AvgTimePerFrame = FRAME_TIME(1);
	pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvih->bmiHeader.biWidth = INITIAL_WIDTH;
	pvih->bmiHeader.biHeight = INITIAL_HEIGHT;
	pvih->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');

	*phr = S_OK;
}

CH264ParserFilter::~CH264ParserFilter(void)
{
	TRACE(TEXT("CH264ParserFilter::~CH264ParserFilter()\n"));
}

IBaseFilter* WINAPI CH264ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CH264ParserFilter *pNewFilter = new CH264ParserFilter(pUnk, phr);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_IBaseFilter, (void**)&pFilter);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	return pFilter;
}

HRESULT CH264ParserFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video)
		return S_OK;

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CH264ParserFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Video
			&& (*mtOut->Subtype() == MEDIASUBTYPE_H264
				|| *mtOut->Subtype() == MEDIASUBTYPE_h264
				|| *mtOut->Subtype() == MEDIASUBTYPE_H264_bis
				|| *mtOut->Subtype() == MEDIASUBTYPE_AVC1
				|| *mtOut->Subtype() == MEDIASUBTYPE_avc1)) {
		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CH264ParserFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	// バッファは1個あればよい
	if(!pprop->cBuffers)pprop->cBuffers = 1L;

	if(pprop->cbBuffer < BUFFER_SIZE)pprop->cbBuffer = BUFFER_SIZE;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if(FAILED(hr))return hr;

	// 要求を受け入れられたか判定
	if(Actual.cbBuffer < pprop->cbBuffer)return E_FAIL;

	return S_OK;
}

HRESULT CH264ParserFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pMediaType, E_POINTER);

	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}

HRESULT CH264ParserFilter::StartStreaming(void)
{
	CAutoLock AutoLock(m_pLock);

	Reset();
	m_VideoInfo.Reset();

	m_BitRateCalculator.Initialize();

	return S_OK;
}

HRESULT CH264ParserFilter::StopStreaming(void)
{
	m_BitRateCalculator.Reset();

	return S_OK;
}

HRESULT CH264ParserFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock AutoLock(m_pLock);

	// 入力データポインタを取得する
	BYTE *pInData = NULL;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pIn->GetActualDataLength();

	m_pOutSample = pOut;

	// 出力メディアサンプル設定
	if (m_bAdjustTime) {
		pOut->SetActualDataLength(0UL);

		// タイムスタンプ取得
		REFERENCE_TIME StartTime, EndTime;
		hr = pIn->GetTime(&StartTime, &EndTime);
		if (hr == S_OK || hr == VFW_S_NO_STOP_TIME) {
			if (m_bAdjustFrameRate) {
				if (m_PrevTime >= 0
						&& (m_PrevTime >= StartTime
							|| m_PrevTime + REFERENCE_TIME_SECOND * 3 < StartTime)) {
					TRACE(TEXT("Reset H.264 media queue\n"));
					m_MediaQueue.clear();
				} else if (!m_MediaQueue.empty()) {
					const REFERENCE_TIME Duration = StartTime - m_PrevTime;
					const int Frames = (int)m_MediaQueue.size();
					REFERENCE_TIME Start, End;
					BYTE *pOutData = NULL;

					Start = m_PrevTime;
					pOut->GetPointer(&pOutData);
					for (int i = 1; i <= Frames; i++) {
						CMediaData *pMediaData = m_MediaQueue.front();
						::CopyMemory(pOutData, pMediaData->GetData(), pMediaData->GetSize());
						pOut->SetActualDataLength(pMediaData->GetSize());
						End = m_PrevTime + Duration * i / Frames - 1;
						pOut->SetTime(&Start, &End);
						m_DeliverResult = m_pOutput->Deliver(pOut);
						if (FAILED(m_DeliverResult)) {
							m_MediaQueue.clear();
							break;
						}
						m_MediaQueue.pop();
						delete pMediaData;
						Start = End + 1;
					}
					pOut->SetActualDataLength(0);
				}
				m_PrevTime = StartTime;
			} else {
				if (m_PrevTime < 0
						|| _abs64((m_PrevTime + FRAME_TIME(m_SampleCount)) - StartTime) > REFERENCE_TIME_SECOND / 5LL) {
#ifdef _DEBUG
					if (m_PrevTime >= 0)
						TRACE(TEXT("Reset H.264 sample time\n"));
#endif
					m_PrevTime = StartTime;
					m_SampleCount = 0;
				}
			}
		}
	} else {
		BYTE *pOutData = NULL;
		HRESULT hr = pOut->GetPointer(&pOutData);
		if (SUCCEEDED(hr)) {
			hr = pOut->SetActualDataLength(DataSize);
			if (SUCCEEDED(hr))
				::CopyMemory(pOutData, pInData, DataSize);
		}
	}
	m_DeliverResult = S_OK;

	m_H264Parser.StoreEs(pInData, DataSize);

	m_BitRateCalculator.Update(DataSize);

	if (pOut->GetActualDataLength() == 0)
		return S_FALSE;

	return m_DeliverResult;
}

HRESULT CH264ParserFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	IMediaSample *pOutSample;
	HRESULT hr;

	hr = InitializeOutputSample(pSample, &pOutSample);
	if (FAILED(hr))
		return hr;
	hr = Transform(pSample, pOutSample);
	if (SUCCEEDED(hr)) {
		if (hr == S_OK)
			hr = m_pOutput->Deliver(pOutSample);
		else if (hr == S_FALSE)
			hr = S_OK;
		m_bSampleSkipped = FALSE;
	}

	pOutSample->Release();

	return hr;
}

HRESULT CH264ParserFilter::BeginFlush(void)
{
	HRESULT hr = S_OK;

	Reset();
	m_VideoInfo.Reset();
	if (m_pOutput)
		hr = m_pOutput->DeliverBeginFlush();
	return hr;
}

void CH264ParserFilter::SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam)
{
	CAutoLock Lock(m_pLock);

	m_pfnVideoInfoCallback = pCallback;
	m_pCallbackParam = pParam;
}

bool CH264ParserFilter::GetVideoInfo(CMpeg2VideoInfo *pInfo) const
{
	if (!pInfo)
		return false;

	CAutoLock Lock(&m_VideoInfoLock);

	*pInfo = m_VideoInfo;
	return true;
}

void CH264ParserFilter::ResetVideoInfo()
{
	CAutoLock Lock(&m_VideoInfoLock);

	m_VideoInfo.Reset();
}

bool CH264ParserFilter::SetAdjustTime(bool bAdjust)
{
	CAutoLock Lock(m_pLock);

	if (m_bAdjustTime != bAdjust) {
		m_bAdjustTime = bAdjust;
		Reset();
	}
	return true;
}

bool CH264ParserFilter::SetAdjustFrameRate(bool bAdjust)
{
	CAutoLock Lock(m_pLock);

	if (m_bAdjustFrameRate != bAdjust) {
		m_bAdjustFrameRate = bAdjust;
		if (m_bAdjustTime)
			Reset();
	}
	return true;
}

void CH264ParserFilter::SetAttachMediaType(bool bAttach)
{
	m_bAttachMediaType = bAttach;
}

DWORD CH264ParserFilter::GetBitRate() const
{
	return m_BitRateCalculator.GetBitRate();
}

void CH264ParserFilter::Reset()
{
	m_H264Parser.Reset();
	m_PrevTime = -1;
	m_SampleCount = 0;
	m_MediaQueue.clear();
}

void CH264ParserFilter::OnAccessUnit(const CH264Parser *pParser, const CH264AccessUnit *pAccessUnit)
{
	WORD OrigWidth, OrigHeight;
	OrigWidth = pAccessUnit->GetHorizontalSize();
	OrigHeight = pAccessUnit->GetVerticalSize();

	if (m_bAttachMediaType && m_VideoInfo.m_OrigWidth != OrigWidth) {
		CMediaType MediaType(m_pOutput->CurrentMediaType());
		VIDEOINFOHEADER *pVideoInfo=(VIDEOINFOHEADER*)MediaType.Format();

		if (pVideoInfo
				&& (pVideoInfo->bmiHeader.biWidth != OrigWidth
					|| pVideoInfo->bmiHeader.biHeight != OrigHeight)) {
			pVideoInfo->bmiHeader.biWidth = OrigWidth;
			pVideoInfo->bmiHeader.biHeight = OrigHeight;
			m_pOutSample->SetMediaType(&MediaType);
		}
	}

	if (m_bAdjustTime) {
		/*
			1フレーム単位でタイムスタンプを設定しないとかくつく
		*/
		if (m_bAdjustFrameRate && m_PrevTime >= 0) {
			m_MediaQueue.push(new CMediaData(*pAccessUnit));
		} else {
			if (SUCCEEDED(m_DeliverResult)) {
				BYTE *pOutData = NULL;
				HRESULT hr = m_pOutSample->GetPointer(&pOutData);
				if (SUCCEEDED(hr)) {
					hr = m_pOutSample->SetActualDataLength(pAccessUnit->GetSize());
					if (SUCCEEDED(hr)) {
						::CopyMemory(pOutData, pAccessUnit->GetData(), pAccessUnit->GetSize());
						// タイムスタンプ設定
						if (m_PrevTime >= 0) {
							REFERENCE_TIME StartTime = m_PrevTime + FRAME_TIME(m_SampleCount);
							REFERENCE_TIME EndTime = m_PrevTime + FRAME_TIME(m_SampleCount + 1) - 1;
							m_pOutSample->SetTime(&StartTime, &EndTime);
						} else {
							m_pOutSample->SetTime(NULL, NULL);
						}
						m_DeliverResult = m_pOutput->Deliver(m_pOutSample);
						m_pOutSample->SetActualDataLength(0);
					}
				}
			}

			m_SampleCount++;
		}
	}

	WORD DisplayWidth, DisplayHeight;
	BYTE AspectX = 0, AspectY = 0;

	DisplayWidth = OrigWidth;
	DisplayHeight = OrigHeight;

	WORD SarX = 0, SarY = 0;
	if (pAccessUnit->GetSAR(&SarX, &SarY) && SarX != 0 && SarY != 0) {
		DWORD Width = OrigWidth * SarX, Height = OrigHeight * SarY;

		// とりあえず 16:9 と 4:3 だけ
		if (Width * 9 == Height * 16) {
			AspectX = 16;
			AspectY = 9;
		} else if (Width * 3 == Height * 4) {
			AspectX = 4;
			AspectY = 3;
		}
	} else {
		if (OrigWidth * 9 == OrigHeight * 16) {
			AspectX = 16;
			AspectY = 9;
		} else if (OrigWidth * 3 == OrigHeight * 4) {
			AspectX = 4;
			AspectY = 3;
		}
	}

	CMpeg2VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	CH264AccessUnit::TimingInfo TimingInfo;
	if (pAccessUnit->GetTimingInfo(&TimingInfo)) {
		// 実際のフレームレートではない
		Info.m_FrameRate.Num = TimingInfo.TimeScale;
		Info.m_FrameRate.Denom = TimingInfo.NumUnitsInTick;
	}

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		m_VideoInfoLock.Lock();
		m_VideoInfo = Info;
		m_VideoInfoLock.Unlock();

		TRACE(TEXT("H.264 access unit %d x %d [SAR %d:%d (DAR %d:%d)] %lu/%lu\n"),
			  OrigWidth, OrigHeight, SarX, SarY, AspectX, AspectY,
			  Info.m_FrameRate.Num, Info.m_FrameRate.Denom);

		// 通知
		if (m_pfnVideoInfoCallback)
			m_pfnVideoInfoCallback(&m_VideoInfo, m_pCallbackParam);
	}
}
