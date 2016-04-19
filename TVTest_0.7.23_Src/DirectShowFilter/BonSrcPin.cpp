#include "StdAfx.h"
#include "BonSrcPin.h"
#include "BonSrcFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#ifndef TS_PACKETSIZE
#define TS_PACKETSIZE	188
#endif

#define SAMPLE_PACKETS 1024




CBonSrcPin::CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter)
	: CBaseOutputPin(TEXT("CBonSrcPin"), pFilter, pFilter->m_pLock, phr, L"TS")
	, m_pFilter(pFilter)
	, m_hThread(NULL)
	, m_SrcStream(0x1000)
	, m_bOutputWhenPaused(false)
{
	TRACE(TEXT("CBonSrcPin::CBonSrcPin() %p\n"),this);

	*phr = S_OK;
}

CBonSrcPin::~CBonSrcPin(void)
{
	//TRACE(TEXT("CBonSrcPin::~CBonSrcPin()\n"));

	EndStreamThread();
}

HRESULT CBonSrcPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pMediaType, E_POINTER);

	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;

	// メディアタイプ設定
	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Stream);
	pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_TRANSPORT);
	pMediaType->SetTemporalCompression(FALSE);
	pMediaType->SetSampleSize(TS_PACKETSIZE);

	return S_OK;
}

HRESULT CBonSrcPin::CheckMediaType(const CMediaType *pMediaType)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pMediaType, E_POINTER);

	// メディアタイプをチェックする
	CMediaType m_mt;
	GetMediaType(0, &m_mt);

	return (*pMediaType == m_mt) ? S_OK : E_FAIL;
}

HRESULT CBonSrcPin::Active(void)
{
	TRACE(TEXT("CBonSrcPin::Active()\n"));

	HRESULT hr=CBaseOutputPin::Active();
	if (FAILED(hr))
		return hr;

	if (m_hThread)
		return E_UNEXPECTED;

	m_bKillSignal=false;
	m_SrcStream.Reset();
	m_hThread=(HANDLE)::_beginthreadex(NULL,0,StreamThread,this,0,NULL);
	if (m_hThread==NULL) {
		return E_FAIL;
	}
	return S_OK;
}

void CBonSrcPin::EndStreamThread()
{
	if (m_hThread) {
		m_bKillSignal=true;
		if (::WaitForSingleObject(m_hThread, 1000) == WAIT_TIMEOUT) {
			::TerminateThread(m_hThread, -1);
		}
		::CloseHandle(m_hThread);
		m_hThread=NULL;
	}
}

HRESULT CBonSrcPin::Inactive(void)
{
	TRACE(TEXT("CBonSrcPin::Inactive()\n"));

	HRESULT hr=CBaseOutputPin::Inactive();

	EndStreamThread();

	return hr;
}

HRESULT CBonSrcPin::Run(REFERENCE_TIME tStart)
{
	TRACE(TEXT("CBonSrcPin::Run()\n"));

	return CBaseOutputPin::Run(tStart);
}

HRESULT CBonSrcPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	// バッファは1個あればよい
	if(!pRequest->cBuffers)pRequest->cBuffers = 1;

	// とりあえずTSパケット長確保
	if(pRequest->cbBuffer < TS_PACKETSIZE * SAMPLE_PACKETS)
		pRequest->cbBuffer = TS_PACKETSIZE * SAMPLE_PACKETS;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
	if(FAILED(hr))return hr;

	// 要求を受け入れられたか判定
	if(Actual.cbBuffer < pRequest->cbBuffer)return E_FAIL;

	return S_OK;
}


const bool CBonSrcPin::InputMedia(CMediaData *pMediaData)
{
	for (int i=0;m_SrcStream.IsBufferFull();i++) {
		if (i==100) {
			Flush();
			return false;
		}
		::Sleep(10);
	}
	m_SrcStream.InputMedia(pMediaData);

	return true;
}

void CBonSrcPin::Reset()
{
	m_SrcStream.Reset();
}

void CBonSrcPin::Flush()
{
	TRACE(TEXT("CBonSrcPin::Flush()\n"));
	Reset();
	DeliverBeginFlush();
	DeliverEndFlush();
}

bool CBonSrcPin::EnableSync(bool bEnable)
{
	return m_SrcStream.EnableSync(bEnable);
}

bool CBonSrcPin::IsSyncEnabled() const
{
	return m_SrcStream.IsSyncEnabled();
}

void CBonSrcPin::SetVideoPID(WORD PID)
{
	m_SrcStream.SetVideoPID(PID);
}

void CBonSrcPin::SetAudioPID(WORD PID)
{
	m_SrcStream.SetAudioPID(PID);
}

unsigned int __stdcall CBonSrcPin::StreamThread(LPVOID lpParameter)
{
	CBonSrcPin *pThis=static_cast<CBonSrcPin*>(lpParameter);

	TRACE(TEXT("CBonSrcPin::StreamThread() Start\n"));

	::CoInitialize(NULL);

	while (!pThis->m_bKillSignal) {
		if (pThis->m_SrcStream.IsDataAvailable()) {
			// 空のメディアサンプルを要求する
			IMediaSample *pSample = NULL;
			HRESULT hr = pThis->GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (SUCCEEDED(hr)) {
				// 書き込み先ポインタを取得する
				BYTE *pSampleData = NULL;
				pSample->GetPointer(&pSampleData);
				DWORD Size = SAMPLE_PACKETS;
				if (pThis->m_SrcStream.GetData(pSampleData, &Size)) {
					pSample->SetActualDataLength(Size * TS_PACKETSIZE);
					pThis->Deliver(pSample);
				}
				pSample->Release();
			}
		} else {
			::Sleep(5);
		}
	}

	::CoUninitialize();

	TRACE(TEXT("CBonSrcPin::StreamThread() End\n"));

	return 0;
}
