#include "StdAfx.h"
#include <initguid.h>
#include "BonSrcFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CBonSrcFilter::CBonSrcFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(TEXT("Bon Source Filter"), pUnk, &m_cStateLock, CLSID_BONSOURCE)
	, m_pSrcPin(NULL)
	, m_bOutputWhenPaused(false)
{
	TRACE(TEXT("CBonSrcFilter::CBonSrcFilter %p\n"),this);

	// ピンのインスタンス生成
	m_pSrcPin = new CBonSrcPin(phr, this);

	//*phr = (m_pSrcPin)? S_OK : E_OUTOFMEMORY;
	*phr=S_OK;
}

CBonSrcFilter::~CBonSrcFilter()
{
	// ピンのインスタンスを削除する
	if(m_pSrcPin)delete m_pSrcPin;
}

IBaseFilter* WINAPI CBonSrcFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CBonSrcFilter *pNewFilter = new CBonSrcFilter(pUnk, phr);
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


int CBonSrcFilter::GetPinCount(void)
{
	// ピン数を返す
	return 1;
}


CBasePin * CBonSrcFilter::GetPin(int n)
{
	// ピンのインスタンスを返す
	return (n == 0)? m_pSrcPin : NULL;
}


#ifdef _DEBUG

STDMETHODIMP CBonSrcFilter::Run(REFERENCE_TIME tStart)
{
	TRACE(L"■CBonSrcFilter::Run()\n");

	return CBaseFilter::Run(tStart);
}


STDMETHODIMP CBonSrcFilter::Pause(void)
{
	TRACE(L"■CBonSrcFilter::Pause()\n");

	return CBaseFilter::Pause();
}


STDMETHODIMP CBonSrcFilter::Stop(void)
{
	TRACE(L"■CBonSrcFilter::Stop()\n");

	return CBaseFilter::Stop();
}

#endif


STDMETHODIMP CBonSrcFilter::GetState(DWORD dw, FILTER_STATE *pState)
{
	*pState = m_State;
	if (m_State==State_Paused && !m_bOutputWhenPaused)
		return VFW_S_CANT_CUE;
	return S_OK;
}


const bool CBonSrcFilter::InputMedia(CMediaData *pMediaData)
{
	m_cStateLock.Lock();
	if (!m_pSrcPin
			|| m_State==State_Stopped
			|| (m_State==State_Paused && !m_bOutputWhenPaused)) {
		m_cStateLock.Unlock();
		return false;
	}
	m_cStateLock.Unlock();
	return m_pSrcPin->InputMedia(pMediaData);
}


void CBonSrcFilter::Reset()
{
	if (m_pSrcPin)
		m_pSrcPin->Reset();
}


void CBonSrcFilter::Flush()
{
	CAutoLock Lock(m_pLock);

	if (m_pSrcPin)
		m_pSrcPin->Flush();
}


bool CBonSrcFilter::EnableSync(bool bEnable)
{
	if (m_pSrcPin)
		return m_pSrcPin->EnableSync(bEnable);
	return false;
}


bool CBonSrcFilter::IsSyncEnabled() const
{
	if (m_pSrcPin)
		return m_pSrcPin->IsSyncEnabled();
	return false;
}


void CBonSrcFilter::SetVideoPID(WORD PID)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetVideoPID(PID);
}


void CBonSrcFilter::SetAudioPID(WORD PID)
{
	if (m_pSrcPin)
		return m_pSrcPin->SetAudioPID(PID);
}


void CBonSrcFilter::SetOutputWhenPaused(bool bOutput)
{
	m_bOutputWhenPaused=bOutput;
	if (m_pSrcPin)
		m_pSrcPin->SetOutputWhenPaused(bOutput);
}
