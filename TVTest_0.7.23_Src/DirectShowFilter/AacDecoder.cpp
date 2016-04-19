// AacDecoder.cpp: CAacDecoder �N���X�̃C���v�������e�[�V����
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
// �\�z/����
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

	// FAAD2�I�[�v��
	m_hDecoder = ::NeAACDecOpen();
	if (m_hDecoder==NULL)
		return false;

	// �f�t�H���g�ݒ�擾
	NeAACDecConfigurationPtr pDecodeConfig = ::NeAACDecGetCurrentConfiguration(m_hDecoder);

	/*
	// Open�����������NULL�ɂ͂Ȃ�Ȃ�
	if (pDecodeConfig==NULL) {
		CloseDecoder();
		return false;
	}
	*/

	// �f�R�[�_�ݒ�
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
	// FAAD2�N���[�Y
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

	// �f�R�[�h

	// ����t���[�����
	if (m_bInitRequest || pFrame->GetChannelConfig() != m_byLastChannelConfig) {
		if (!m_bInitRequest) {
			// �`�����l���ݒ肪�ω������A�f�R�[�_���Z�b�g
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

	// �f�R�[�h
	NeAACDecFrameInfo FrameInfo;
	//::ZeroMemory(&FrameInfo, sizeof(FrameInfo));

	BYTE *pPcmBuffer = (BYTE *)::NeAACDecDecode(m_hDecoder, &FrameInfo, const_cast<BYTE*>(pFrame->GetData()), pFrame->GetSize());

	if (FrameInfo.error==0) {
		if (FrameInfo.samples>0) {
			// L-PCM�n���h���ɒʒm
			if (m_pPcmHandler)
				m_pPcmHandler->OnPcmFrame(this, pPcmBuffer, FrameInfo.samples / FrameInfo.channels, FrameInfo.channels);
		}
	} else {
		// �G���[����
#ifdef _DEBUG
		::OutputDebugString(TEXT("CAacDecoder::Decode error - "));
		::OutputDebugStringA(NeAACDecGetErrorMessage(FrameInfo.error));
		::OutputDebugString(TEXT("\n"));
#endif
#if 0
		// ���������������ƃ��������[�N����
		m_bInitRequest = true;
#else
		// ���Z�b�g���Ă݂�
		// �{���t���[���̃f�R�[�h�ŃG���[���N����x�Ɉ�X���Z�b�g����K�v�͂Ȃ��͂������c
		ResetDecoder();
#endif
	}

	return true;
}


const BYTE CAacDecoder::GetChannelConfig() const
{
	return m_byLastChannelConfig;
}
