#include "StdAfx.h"
#include <dvdmedia.h>
#include <initguid.h>
#include "Mpeg2SequenceFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMpeg2SequenceFilter::CMpeg2SequenceFilter(LPUNKNOWN pUnk, HRESULT *phr)
#ifndef MPEG2SEQUENCEFILTER_INPLACE
	: CTransformFilter(MPEG2SEQUENCEFILTER_NAME, pUnk, CLSID_MPEG2SEQFILTER)
#else
	: CTransInPlaceFilter(MPEG2SEQUENCEFILTER_NAME, pUnk, CLSID_MPEG2SEQFILTER, phr, FALSE)
#endif
	, m_pfnVideoInfoRecvFunc(NULL)
	, m_pCallbackParam(NULL)
	, m_Mpeg2Parser(this)
	, m_bAttachMediaType(false)
	, m_VideoInfo()
{
	TRACE(TEXT("CMpeg2SequenceFilter::CMpeg2SequenceFilter() %p\n"), this);

	*phr = S_OK;
}


CMpeg2SequenceFilter::~CMpeg2SequenceFilter(void)
{
	TRACE(TEXT("CMpeg2SequenceFilter::~CMpeg2SequenceFilter()\n"));
}


IBaseFilter* WINAPI CMpeg2SequenceFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CMpeg2SequenceFilter *pNewFilter = new CMpeg2SequenceFilter(pUnk, phr);
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


HRESULT CMpeg2SequenceFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video) {
		if (*mtIn->Subtype() == MEDIASUBTYPE_MPEG2_VIDEO) {
			return S_OK;
		}
	}
	return VFW_E_TYPE_NOT_ACCEPTED;
}


#ifndef MPEG2SEQUENCEFILTER_INPLACE


HRESULT CMpeg2SequenceFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if(*mtOut->Type() == MEDIATYPE_Video){
		if(*mtOut->Subtype() == MEDIASUBTYPE_MPEG2_VIDEO){
			return S_OK;
			}
		}

	return VFW_E_TYPE_NOT_ACCEPTED;
}


HRESULT CMpeg2SequenceFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	// バッファは1個あればよい
	if(!pprop->cBuffers)pprop->cBuffers = 1L;

	// とりあえず16MB確保
	if(pprop->cbBuffer < 0x1000000L)pprop->cbBuffer = 0x1000000L;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if(FAILED(hr))return hr;

	// 要求を受け入れられたか判定
	if(Actual.cbBuffer < pprop->cbBuffer)return E_FAIL;

	return S_OK;
}


HRESULT CMpeg2SequenceFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pMediaType, E_POINTER);

	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;
	*pMediaType=m_pInput->CurrentMediaType();
	return S_OK;
}


HRESULT CMpeg2SequenceFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	BYTE *pData;
	HRESULT hr = pIn->GetPointer(&pData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pIn->GetActualDataLength();

	pOut->SetActualDataLength(0);
	m_pOutSample = pOut;

	// シーケンスを取得
	m_Mpeg2Parser.StoreEs(pData, DataSize);

	m_BitRateCalculator.Update(DataSize);

	return pOut->GetActualDataLength() > 0 ? S_OK : S_FALSE;
}


void CMpeg2SequenceFilter::SetFixSquareDisplay(bool bFix)
{
	m_Mpeg2Parser.SetFixSquareDisplay(bFix);
}


#else	// ndef MPEG2SEQUENCEFILTER_INPLACE


HRESULT CMpeg2SequenceFilter::Transform(IMediaSample *pSample)
{
	BYTE *pData;
	HRESULT hr = pSample->GetPointer(&pData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pSample->GetActualDataLength();
	m_pOutSample = pSample;

	// シーケンスを取得
	m_Mpeg2Parser.StoreEs(pData, DataSize);

	m_BitRateCalculator.Update(DataSize);

	return S_OK;
}


HRESULT CMpeg2SequenceFilter::Receive(IMediaSample *pSample)
{
	const AM_SAMPLE2_PROPERTIES *pProps = m_pInput->SampleProps();
	if (pProps->dwStreamId != AM_STREAM_MEDIA)
		return m_pOutput->Deliver(pSample);

	if (UsingDifferentAllocators()) {
		pSample = Copy(pSample);
		if (pSample == NULL)
			return E_UNEXPECTED;
	}

	HRESULT hr = Transform(pSample);
	if (SUCCEEDED(hr)) {
		if (hr == S_OK)
			hr = m_pOutput->Deliver(pSample);
		else if (hr == S_FALSE)
			hr = S_OK;
	}

	if (UsingDifferentAllocators())
		pSample->Release();

	return hr;
}


#endif	// MPEG2SEQUENCEFILTER_INPLACE


HRESULT CMpeg2SequenceFilter::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
	return S_OK;
}


HRESULT CMpeg2SequenceFilter::StartStreaming(void)
{
	CAutoLock Lock(m_pLock);

	m_Mpeg2Parser.Reset();
	m_VideoInfo.Reset();

	m_BitRateCalculator.Initialize();

	return S_OK;
}


HRESULT CMpeg2SequenceFilter::StopStreaming(void)
{
	m_BitRateCalculator.Reset();

	return S_OK;
}


HRESULT CMpeg2SequenceFilter::BeginFlush()
{
	HRESULT hr = S_OK;

	m_Mpeg2Parser.Reset();
	m_VideoInfo.Reset();
	if (m_pOutput)
		hr = m_pOutput->DeliverBeginFlush();
	return hr;
}


void CMpeg2SequenceFilter::SetRecvCallback(MPEG2SEQUENCE_VIDEOINFO_FUNC pCallback, const PVOID pParam)
{
	m_pfnVideoInfoRecvFunc = pCallback;
	m_pCallbackParam = pParam;
}


void CMpeg2SequenceFilter::OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence)
{
#ifndef MPEG2SEQUENCEFILTER_INPLACE
	LONG Offset = m_pOutSample->GetActualDataLength();
	BYTE *pData;
	if (SUCCEEDED(m_pOutSample->GetPointer(&pData))
			&& SUCCEEDED(m_pOutSample->SetActualDataLength(Offset + pSequence->GetSize()))) {
		::CopyMemory(&pData[Offset], pSequence->GetData(), pSequence->GetSize());
	}
#endif

	BYTE AspectX, AspectY;
	WORD OrigWidth, OrigHeight;
	WORD DisplayWidth, DisplayHeight;

	if (!pSequence->GetAspectRatio(&AspectX, &AspectY))
		AspectX = AspectY = 0;

	OrigWidth = pSequence->GetHorizontalSize();
	OrigHeight = pSequence->GetVerticalSize();

	if (pSequence->GetExtendDisplayInfo()) {
		DisplayWidth = pSequence->GetExtendDisplayHorizontalSize();
		DisplayHeight = pSequence->GetExtendDisplayVerticalSize();
	} else {
		DisplayWidth = OrigWidth;
		DisplayHeight = OrigHeight;
	}

	CMpeg2VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	pSequence->GetFrameRate(&Info.m_FrameRate.Num, &Info.m_FrameRate.Denom);

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		if (m_bAttachMediaType && m_VideoInfo.m_OrigWidth != OrigWidth) {
			CMediaType MediaType(m_pOutput->CurrentMediaType());
			MPEG2VIDEOINFO *pVideoInfo=(MPEG2VIDEOINFO*)MediaType.Format();

			if (pVideoInfo
					&& (pVideoInfo->hdr.bmiHeader.biWidth != OrigWidth
						|| pVideoInfo->hdr.bmiHeader.biHeight != OrigHeight)) {
				pVideoInfo->hdr.bmiHeader.biWidth = OrigWidth;
				pVideoInfo->hdr.bmiHeader.biHeight = OrigHeight;
				m_pOutSample->SetMediaType(&MediaType);
			}
		}

		m_VideoInfoLock.Lock();
		m_VideoInfo = Info;
		m_VideoInfoLock.Unlock();

		TRACE(TEXT("Mpeg2 sequence %d x %d [%d x %d (%d=%d:%d)]\n"),
			  OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, pSequence->GetAspectRatioInfo(), AspectX, AspectY);

		// 通知
		if (m_pfnVideoInfoRecvFunc)
			m_pfnVideoInfoRecvFunc(&m_VideoInfo, m_pCallbackParam);
	}
}


const bool CMpeg2SequenceFilter::GetVideoSize(WORD *pX, WORD *pY) const
{
	CAutoLock Lock(&m_VideoInfoLock);

	if (m_VideoInfo.m_DisplayWidth == 0 || m_VideoInfo.m_DisplayHeight == 0)
		return false;
	if (pX) *pX = m_VideoInfo.m_DisplayWidth;
	if (pY) *pY = m_VideoInfo.m_DisplayHeight;
	return true;
}


const bool CMpeg2SequenceFilter::GetAspectRatio(BYTE *pX, BYTE *pY) const
{
	CAutoLock Lock(&m_VideoInfoLock);

	if (m_VideoInfo.m_AspectRatioX == 0 || m_VideoInfo.m_AspectRatioY == 0)
		return false;
	if (pX) *pX = m_VideoInfo.m_AspectRatioX;
	if (pY) *pY = m_VideoInfo.m_AspectRatioY;
	return true;
}


const bool CMpeg2SequenceFilter::GetOriginalVideoSize(WORD *pWidth, WORD *pHeight) const
{
	CAutoLock Lock(&m_VideoInfoLock);

	if (m_VideoInfo.m_OrigWidth == 0 || m_VideoInfo.m_OrigHeight == 0)
		return false;
	if (pWidth)
		*pWidth = m_VideoInfo.m_OrigWidth;
	if (pHeight)
		*pHeight = m_VideoInfo.m_OrigHeight;
	return true;
}


const bool CMpeg2SequenceFilter::GetVideoInfo(CMpeg2VideoInfo *pInfo) const
{
	if (!pInfo)
		return false;

	CAutoLock Lock(&m_VideoInfoLock);

	*pInfo = m_VideoInfo;
	return true;
}


void CMpeg2SequenceFilter::ResetVideoInfo()
{
	CAutoLock Lock(&m_VideoInfoLock);

	m_VideoInfo.Reset();
}


void CMpeg2SequenceFilter::SetAttachMediaType(bool bAttach)
{
	m_bAttachMediaType = bAttach;
}


DWORD CMpeg2SequenceFilter::GetBitRate() const
{
	return m_BitRateCalculator.GetBitRate();
}
