#pragma once

#include "../BonTsEngine/TsMedia.h"
#include "../BonTsEngine/TsUtilClass.h"
#include "VideoInfo.h"


// InPlaceフィルタにする
#define MPEG2SEQUENCEFILTER_INPLACE

// テンプレート名
#define MPEG2SEQUENCEFILTER_NAME	L"Mpeg2 Sequence Filter"

// このフィルタのGUID {3F8400DA-65F1-4694-BB05-303CDE739680}
DEFINE_GUID(CLSID_MPEG2SEQFILTER, 0x3f8400da, 0x65f1, 0x4694, 0xbb, 0x5, 0x30, 0x3c, 0xde, 0x73, 0x96, 0x80);

class CMpeg2SequenceFilter
#ifndef MPEG2SEQUENCEFILTER_INPLACE
	: public CTransformFilter
#else
	: public CTransInPlaceFilter
#endif
	, protected CMpeg2Parser::ISequenceHandler
{
public:
	DECLARE_IUNKNOWN

	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransformFilter
#ifndef MPEG2SEQUENCEFILTER_INPLACE
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
#else
// CTransInPlaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
#endif
	HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);
	HRESULT BeginFlush();

// CMpeg2SequenceFilter
	typedef void (CALLBACK MPEG2SEQUENCE_VIDEOINFO_FUNC)(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam);
	void SetRecvCallback(MPEG2SEQUENCE_VIDEOINFO_FUNC pCallback, const PVOID pParam = NULL);

	const bool GetVideoSize(WORD *pX, WORD *pY) const;
	const bool GetAspectRatio(BYTE *pX,BYTE *pY) const;

	// Append by HDUSTestの中の人
	const bool GetOriginalVideoSize(WORD *pWidth,WORD *pHeight) const;
	const bool GetVideoInfo(CMpeg2VideoInfo *pInfo) const;
	void ResetVideoInfo();
#ifndef MPEG2SEQUENCEFILTER_INPLACE
	void SetFixSquareDisplay(bool bFix);
#endif
	void SetAttachMediaType(bool bAttach);
	DWORD GetBitRate() const;

protected:
	CMpeg2SequenceFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CMpeg2SequenceFilter(void);

#ifndef MPEG2SEQUENCEFILTER_INPLACE
	HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);
#else
	HRESULT Transform(IMediaSample *pSample);
	HRESULT Receive(IMediaSample *pSample);
#endif
	IMediaSample *m_pOutSample;

// CMpeg2Parser::ISequenceHandler
	virtual void OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence);

// CMpeg2SequenceFilter
	MPEG2SEQUENCE_VIDEOINFO_FUNC *m_pfnVideoInfoRecvFunc;
	LPVOID m_pCallbackParam;

	CMpeg2Parser m_Mpeg2Parser;
	CMpeg2VideoInfo m_VideoInfo;
	mutable CCritSec m_VideoInfoLock;

	bool m_bAttachMediaType;

	CBitRateCalculator m_BitRateCalculator;
};
