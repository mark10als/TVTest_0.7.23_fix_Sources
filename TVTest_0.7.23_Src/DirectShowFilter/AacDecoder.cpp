// AacDecoder.cpp: CAacDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AacDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// "Code from FAAD2 is copyright (c) Nero AG, www.nero.com"
#pragma comment(lib, "LibFaad.lib")


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CAacDecoder::CAacDecoder(IPcmHandler *pPcmHandler)
	: m_pPcmHandler(pPcmHandler)
	, m_hDecoder(NULL)
	, m_bInitRequest(false)
	, m_byLastChannelConfig(0xFF)
{
}


CAacDecoder::~CAacDecoder()
{
	CloseDecoder();
}


const bool CAacDecoder::OpenDecoder(void)
{
	CloseDecoder();

	// FAAD2オープン
	m_hDecoder = ::NeAACDecOpen();
	if (m_hDecoder==NULL)
		return false;

	// デフォルト設定取得
	NeAACDecConfigurationPtr pDecodeConfig = ::NeAACDecGetCurrentConfiguration(m_hDecoder);

	/*
	// Openが成功すればNULLにはならない
	if (pDecodeConfig==NULL) {
		CloseDecoder();
		return false;
	}
	*/

	// デコーダ設定
	pDecodeConfig->defObjectType = LC;
	pDecodeConfig->defSampleRate = 48000UL;
	pDecodeConfig->outputFormat = FAAD_FMT_16BIT;
	pDecodeConfig->downMatrix = 0;
	pDecodeConfig->useOldADTSFormat = 0;

	if (!::NeAACDecSetConfiguration(m_hDecoder, pDecodeConfig)) {
		CloseDecoder();
		return false;
	}

	m_bInitRequest = true;
	m_byLastChannelConfig = 0xFFU;

	return true;
}


void CAacDecoder::CloseDecoder()
{
	// FAAD2クローズ
	if (m_hDecoder) {
		::NeAACDecClose(m_hDecoder);
		m_hDecoder = NULL;
	}
}


const bool CAacDecoder::ResetDecoder(void)
{
	if (m_hDecoder == NULL)
		return false;

	return OpenDecoder();
}


const bool CAacDecoder::Decode(const CAdtsFrame *pFrame)
{
	if (m_hDecoder == NULL) {
		/*
		if (!OpenDecoder())
			return false;
		*/
		return false;
	}

	// デコード

	// 初回フレーム解析
	if (m_bInitRequest || pFrame->GetChannelConfig() != m_byLastChannelConfig) {
		if (!m_bInitRequest) {
			// チャンネル設定が変化した、デコーダリセット
			if (!ResetDecoder())
				return false;
		}

		unsigned long SampleRate;
		unsigned char Channels;
		if (::NeAACDecInit(m_hDecoder, const_cast<BYTE*>(pFrame->GetData()), pFrame->GetSize(), &SampleRate, &Channels) < 0) {
			return false;
		}

		m_bInitRequest = false;
		m_byLastChannelConfig = pFrame->GetChannelConfig();
	}

	// デコード
	NeAACDecFrameInfo FrameInfo;
	//::ZeroMemory(&FrameInfo, sizeof(FrameInfo));

	BYTE *pPcmBuffer = (BYTE *)::NeAACDecDecode(m_hDecoder, &FrameInfo, const_cast<BYTE*>(pFrame->GetData()), pFrame->GetSize());

	if (FrameInfo.error==0) {
		if (FrameInfo.samples>0) {
			// L-PCMハンドラに通知
			if (m_pPcmHandler)
				m_pPcmHandler->OnPcmFrame(this, pPcmBuffer, FrameInfo.samples / FrameInfo.channels, FrameInfo.channels);
		}
	} else {
		// エラー発生
#ifdef _DEBUG
		::OutputDebugString(TEXT("CAacDecoder::Decode error - "));
		::OutputDebugStringA(NeAACDecGetErrorMessage(FrameInfo.error));
		::OutputDebugString(TEXT("\n"));
#endif
#if 0
		// 何回も初期化するとメモリリークする
		m_bInitRequest = true;
#else
		// リセットしてみる
		// 本来フレームのデコードでエラーが起きる度に一々リセットする必要はないはずだが…
		ResetDecoder();
#endif
	}

	return true;
}


const BYTE CAacDecoder::GetChannelConfig() const
{
	return m_byLastChannelConfig;
}
