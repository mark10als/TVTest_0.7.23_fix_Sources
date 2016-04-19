// TsPacketParser.cpp: CTsPacketParser �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsPacketParser.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TS_HEADSYNCBYTE		(0x47U)


//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////


CTsPacketParser::CTsPacketParser(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_bOutputNullPacket(false)
	, m_InputPacketCount(0)
	, m_OutputPacketCount(0)
	, m_ErrorPacketCount(0)
	, m_ContinuityErrorPacketCount(0)
#ifdef BONTSENGINE_1SEG_SUPPORT
	, m_bGeneratePAT(true)
#endif
{
	// �p�P�b�g�A�����J�E���^������������
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);
}

CTsPacketParser::~CTsPacketParser()
{
}

void CTsPacketParser::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// �p�P�b�g�J�E���^���N���A����
	m_InputPacketCount = 0;
	m_OutputPacketCount = 0;
	m_ErrorPacketCount = 0;
	m_ContinuityErrorPacketCount = 0;

	// �p�P�b�g�A�����J�E���^������������
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);

	// ��Ԃ����Z�b�g����
	m_TsPacket.ClearSize();

#ifdef BONTSENGINE_1SEG_SUPPORT
	m_PATGenerator.Reset();
#endif
}

const bool CTsPacketParser::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;
	*/

	// TS�p�P�b�g����������
	SyncPacket(pMediaData->GetData(), pMediaData->GetSize());

	return true;
}

void CTsPacketParser::SetOutputNullPacket(const bool bEnable)
{
	// NULL�p�P�b�g�̏o�͗L����ݒ肷��
	m_bOutputNullPacket = bEnable;
}

const DWORD CTsPacketParser::GetInputPacketCount(void) const
{
	// ���̓p�P�b�g����Ԃ�
	return (DWORD)m_InputPacketCount;
}

const DWORD CTsPacketParser::GetOutputPacketCount(void) const
{
	// �o�̓p�P�b�g����Ԃ�
	return (DWORD)m_OutputPacketCount;
}

const DWORD CTsPacketParser::GetErrorPacketCount(void) const
{
	// �G���[�p�P�b�g����Ԃ�
	return (DWORD)m_ErrorPacketCount;
}

const DWORD CTsPacketParser::GetContinuityErrorPacketCount(void) const
{
	// �A�����G���[�p�P�b�g����Ԃ�
	return (DWORD)m_ContinuityErrorPacketCount;
}

void CTsPacketParser::ResetErrorPacketCount(void)
{
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
}

void inline CTsPacketParser::SyncPacket(const BYTE *pData, const DWORD dwSize)
{
	// �����̕��@�͊��S�ł͂Ȃ��A���������ꂽ�ꍇ�ɑO��Ăяo�����̃f�[�^�܂ł����̂ڂ��Ă͍ē����͂ł��Ȃ�
	DWORD dwCurSize;
	DWORD dwCurPos = 0UL;

	while (dwCurPos < dwSize) {
		dwCurSize = m_TsPacket.GetSize();

		if (dwCurSize==0) {
			// �����o�C�g�҂���
			do {
				if (pData[dwCurPos++] == TS_HEADSYNCBYTE) {
					// �����o�C�g����
					m_TsPacket.AddByte(TS_HEADSYNCBYTE);
					break;
				}
			} while (dwCurPos < dwSize);
		} else if (dwCurSize == TS_PACKETSIZE) {
			// �p�P�b�g�T�C�Y���f�[�^���������

			if (pData[dwCurPos] == TS_HEADSYNCBYTE) {
				// ���̃f�[�^�͓����o�C�g
				ParsePacket();
			} else {
				// �����G���[
				m_TsPacket.ClearSize();

				// �ʒu�����ɖ߂�
				if (dwCurPos >= (TS_PACKETSIZE - 1UL))
					dwCurPos -= (TS_PACKETSIZE - 1UL);
				else
					dwCurPos = 0UL;
			}
		} else {
			// �f�[�^�҂�
			DWORD dwRemain = (TS_PACKETSIZE - dwCurSize);
			if ((dwSize - dwCurPos) >= dwRemain) {
				m_TsPacket.AddData(&pData[dwCurPos], dwRemain);
				dwCurPos += dwRemain;
			} else {
				m_TsPacket.AddData(&pData[dwCurPos], dwSize - dwCurPos);
				break;
			}
		}
	}
}

bool inline CTsPacketParser::ParsePacket(void)
{
	bool bOK;

	// ���̓J�E���g�C���N�������g
	m_InputPacketCount++;

	// �p�P�b�g�����/�`�F�b�N����
	switch (m_TsPacket.ParsePacket(m_abyContCounter)) {
	case CTsPacket::EC_CONTINUITY:
		m_ContinuityErrorPacketCount++;
	case CTsPacket::EC_VALID:
		{
#ifdef BONTSENGINE_1SEG_SUPPORT
			/*
			// PAT �̖�����Ԃ��V�~�����[�g
			if (m_TsPacket.GetPID() == 0) {
				bOK = true;
				break;
			}
			*/
			if (m_PATGenerator.StorePacket(&m_TsPacket) && m_bGeneratePAT) {
				if (m_PATGenerator.GetPAT(&m_PATPacket)) {
					OutputMedia(&m_PATPacket);
				}
			}
#endif

			// ���̃f�R�[�_�Ƀf�[�^��n��
			if (m_bOutputNullPacket || m_TsPacket.GetPID() != 0x1FFFU) {
				// �o�̓J�E���g�C���N�������g
				OutputMedia(&m_TsPacket);

				m_OutputPacketCount++;
			}
		}
		bOK=true;
		break;
	case CTsPacket::EC_FORMAT:
	case CTsPacket::EC_TRANSPORT:
		// �G���[�J�E���g�C���N�������g
		m_ErrorPacketCount++;
		bOK=false;
		break;
	}

	// �T�C�Y���N���A�����̃X�g�A�ɔ�����
	m_TsPacket.ClearSize();

	return bOK;
}


#ifdef BONTSENGINE_1SEG_SUPPORT

bool CTsPacketParser::EnablePATGeneration(bool bEnable)
{
	m_bGeneratePAT = bEnable;
	return true;
}


bool CTsPacketParser::SetTransportStreamID(WORD TransportStreamID)
{
	return m_PATGenerator.SetTransportStreamID(TransportStreamID);
}

#endif
