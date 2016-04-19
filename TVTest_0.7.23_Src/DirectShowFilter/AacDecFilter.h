#pragma once

#include "MediaData.h"
#include "TsMedia.h"
#include "TsUtilClass.h"
#include "AacDecoder.h"

// テンプレート名
#define AACDECFILTER_NAME	(L"AAC Decoder Filter")


// このフィルタのGUID {8D1E3E25-D92B-4849-8D38-C787DA78352C}
DEFINE_GUID(CLSID_AACDECFILTER, 0x8d1e3e25, 0xd92b, 0x4849, 0x8d, 0x38, 0xc7, 0x87, 0xda, 0x78, 0x35, 0x2c);

// フィルタ情報の外部参照宣言
//extern const AMOVIESETUP_FILTER g_AacDecFilterInfo;

class CAacDecFilter :	public CTransformFilter
					,	protected CAacDecoder::IPcmHandler
					,	protected CAdtsParser::IFrameHandler
{
public:
	DECLARE_IUNKNOWN

	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransformFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);
	HRESULT BeginFlush(void);
	HRESULT Receive(IMediaSample *pSample);

// CAacDecFilter
	enum {
		CHANNEL_DUALMONO	= 0x00,
		CHANNEL_INVALID		= 0xFF
	};

	enum {
		STEREOMODE_STEREO,
		STEREOMODE_LEFT,
		STEREOMODE_RIGHT
	};

	enum SpdifMode {
		SPDIF_MODE_DISABLED,
		SPDIF_MODE_PASSTHROUGH,
		SPDIF_MODE_AUTO
	};

	enum {
		SPDIF_CHANNELS_MONO		= 0x01U,
		SPDIF_CHANNELS_STEREO	= 0x02U,
		SPDIF_CHANNELS_DUALMONO	= 0x04U,
		SPDIF_CHANNELS_SURROUND	= 0x08U
	};

	struct SpdifOptions {
		SpdifMode Mode;
		UINT PassthroughChannels;

		SpdifOptions() : Mode(SPDIF_MODE_DISABLED), PassthroughChannels(0) {}
		SpdifOptions(SpdifMode mode,UINT channels = 0) : Mode(mode), PassthroughChannels(channels) {}
		bool operator==(const SpdifOptions &Op) const {
			return Mode == Op.Mode && PassthroughChannels == Op.PassthroughChannels;
		}
		bool operator!=(const SpdifOptions &Op) const { return !(*this==Op); }
	};

	const BYTE GetCurrentChannelNum();
	bool ResetDecoder();
	bool SetStereoMode(int StereoMode);
	int GetStereoMode() const { return m_StereoMode; }
	bool SetAutoStereoMode(int StereoMode);
	int GetAutoStereoMode() const { return m_AutoStereoMode; }
	bool SetDownMixSurround(bool bDownMix);
	bool GetDownMixSurround() const { return m_bDownMixSurround; }
	bool SetGainControl(bool bGainControl, float Gain = 1.0f, float SurroundGain = 1.0f);
	bool GetGainControl(float *pGain = NULL, float *pSurroundGain = NULL) const;
	bool SetSpdifOptions(const SpdifOptions *pOptions);
	bool GetSpdifOptions(SpdifOptions *pOptions) const;
	bool IsSpdifPassthrough() const { return m_bPassthrough; }

	bool SetAdjustStreamTime(bool bAdjust);

	typedef void (CALLBACK *StreamCallback)(short *pData, DWORD Samples, int Channels, void *pParam);
	bool SetStreamCallback(StreamCallback pCallback, void *pParam = NULL);

	DWORD GetBitRate() const;

private:
	CAacDecFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CAacDecFilter(void);

	CCritSec m_cStateLock;
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);

// CAacDecoder::IPcmHandler
	void OnPcmFrame(const CAacDecoder *pAacDecoder, const BYTE *pData, const DWORD dwSamples, const BYTE byChannels) override;

// CAdtsParser::IFrameHandler
	void OnAdtsFrame(const CAdtsParser *pAdtsParser, const CAdtsFrame *pFrame) override;

// CAacDecFilter
	CAdtsParser m_AdtsParser;
	CAacDecoder m_AacDecoder;
	CMediaType m_MediaType;
	BYTE m_byCurChannelNum;
	bool m_bDualMono;

	const DWORD MonoToStereo(short *pDst, const short *pSrc, const DWORD dwSamples);
	const DWORD DownMixStereo(short *pDst, const short *pSrc, const DWORD dwSamples);
	const DWORD DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples);
	const DWORD MapSurroundChannels(short *pDst, const short *pSrc, const DWORD dwSamples);
	void GainControl(short *pBuffer, const DWORD Samples, const float Gain);

	void OutputSpdif();
	HRESULT ReconnectOutput(long BufferSize, const CMediaType &mt);

	int m_StereoMode;
	int m_AutoStereoMode;
	bool m_bDownMixSurround;

	SpdifOptions m_SpdifOptions;
	bool m_bPassthrough;

	bool m_bGainControl;
	float m_Gain;
	float m_SurroundGain;

	StreamCallback m_pStreamCallback;
	void *m_pStreamCallbackParam;

	bool m_bAdjustStreamTime;
	REFERENCE_TIME m_StartTime;
	LONGLONG m_SampleCount;
	bool m_bDiscontinuity;
	const CAdtsFrame *m_pAdtsFrame;

	CBitRateCalculator m_BitRateCalculator;
};
