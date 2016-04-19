#pragma once


#include "TsSrcStream.h"
#include "MediaData.h"


class CBonSrcFilter;

class CBonSrcPin : public CBaseOutputPin
{
public:
	CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter);
	virtual ~CBonSrcPin();

// CBasePin
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CheckMediaType(const CMediaType *pMediaType);

	HRESULT Active(void);
	HRESULT Inactive(void);
	HRESULT Run(REFERENCE_TIME tStart);

// CBaseOutputPin
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);

// CBonSrcPin
	const bool InputMedia(CMediaData *pMediaData);

	void Reset();
	void Flush();
	bool EnableSync(bool bEnable);
	bool IsSyncEnabled() const;
	void SetVideoPID(WORD PID);
	void SetAudioPID(WORD PID);
	void SetOutputWhenPaused(bool bOutput) { m_bOutputWhenPaused=bOutput; }

protected:
	void EndStreamThread();
	static unsigned int __stdcall StreamThread(LPVOID lpParameter);

	CBonSrcFilter* m_pFilter;

	HANDLE m_hThread;
	volatile bool m_bKillSignal;
	CTsSrcStream m_SrcStream;

	bool m_bOutputWhenPaused;
};
