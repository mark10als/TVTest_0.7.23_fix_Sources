// BcasCard.cpp: CBcasCard �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BcasCard.h"
#include "StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CARD_NOT_OPEN_ERROR_TEXT	TEXT("�J�[�h���[�_���J����Ă��܂���B")
#define BAD_ARGUMENT_ERROR_TEXT		TEXT("�������s���ł��B")
#define ECM_REFUSED_ERROR_TEXT		TEXT("ECM���󂯕t�����܂���B")




CBcasCard::CBcasCard()
	: m_pCardReader(NULL)
{
	// ������ԏ�����
	::ZeroMemory(&m_BcasCardInfo, sizeof(m_BcasCardInfo));
	::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));
}


CBcasCard::~CBcasCard()
{
	CloseCard();
}


const DWORD CBcasCard::GetCardReaderNum(void) const
{
	// �J�[�h���[�_�[����Ԃ�
	if (m_pCardReader)
		return m_pCardReader->NumReaders();
	return 0;
}


LPCTSTR CBcasCard::EnumCardReader(const DWORD dwIndex) const
{
	if (m_pCardReader)
		return m_pCardReader->EnumReader(dwIndex);
	return NULL;
}


const bool CBcasCard::OpenCard(CCardReader::ReaderType ReaderType, LPCTSTR lpszReader)
{
	// ��U�N���[�Y����
	CloseCard();

	m_pCardReader = CCardReader::CreateCardReader(ReaderType);
	if (m_pCardReader == NULL) {
		SetError(ERR_CARDOPENERROR, TEXT("�J�[�h���[�_�̃^�C�v�������ł��B"));
		return false;
	}

	bool bSuccess = false;

	if (lpszReader || m_pCardReader->NumReaders() <= 1) {
		// �w�肳�ꂽ���[�_�[���J��
		if (OpenAndInitialize(lpszReader))
			bSuccess = true;
	} else {
		// ���p�\�ȃ��[�_�[��T���ĊJ��
		LPCTSTR pszReaderName;

		for (int i = 0; (pszReaderName = m_pCardReader->EnumReader(i)) != NULL; i++) {
			if (OpenAndInitialize(pszReaderName)) {
				bSuccess = true;
				break;
			}
		}
	}

	if (!bSuccess) {
		delete m_pCardReader;
		m_pCardReader = NULL;
		return false;
	}

	ClearError();

	return true;
}


void CBcasCard::CloseCard(void)
{
	// �J�[�h���N���[�Y����
	if (m_pCardReader) {
		m_pCardReader->Close();
		delete m_pCardReader;
		m_pCardReader = NULL;
	}
}


const bool CBcasCard::ReOpenCard()
{
	if (m_pCardReader == NULL) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return false;
	}

	CCardReader::ReaderType Type = m_pCardReader->GetReaderType();
	LPTSTR pszReaderName = StdUtil::strdup(m_pCardReader->GetReaderName());
	bool bResult=OpenCard(Type, pszReaderName);
	delete [] pszReaderName;
	return bResult;
}


const bool CBcasCard::IsCardOpen() const
{
	return m_pCardReader != NULL;
}


CCardReader::ReaderType CBcasCard::GetCardReaderType() const
{
	if (m_pCardReader)
		return m_pCardReader->GetReaderType();
	return CCardReader::READER_NONE;
}


LPCTSTR CBcasCard::GetCardReaderName() const
{
	if (m_pCardReader)
		return m_pCardReader->GetReaderName();
	return NULL;
}


const bool CBcasCard::OpenAndInitialize(LPCTSTR pszReader)
{
	if (!m_pCardReader->Open(pszReader)) {
		SetError(m_pCardReader->GetLastErrorException());
		SetErrorCode(ERR_CARDOPENERROR);
		return false;
	}

	// �J�[�h������(���s�����烊�g���C���Ă݂�)
	if (!InitialSetting() && !InitialSetting()) {
		m_pCardReader->Close();
		return false;
	}

	return true;
}


const bool CBcasCard::InitialSetting(void)
{
	// �uInitial Setting Conditions Command�v����������
	/*
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return false;
	}
	*/

	// �o�b�t�@����
	DWORD dwRecvSize;
	BYTE RecvData[1024];

	// �R�}���h���M
	static const BYTE InitSettingCmd[] = {0x90U, 0x30U, 0x00U, 0x00U, 0x00U};
	::ZeroMemory(RecvData, sizeof(RecvData));
	dwRecvSize = sizeof(RecvData);
	if (!m_pCardReader->Transmit(InitSettingCmd, sizeof(InitSettingCmd), RecvData, &dwRecvSize)) {
		SetError(ERR_TRANSMITERROR, m_pCardReader->GetLastErrorText());
		return false;
	}

	if (dwRecvSize < 57UL) {
		SetError(ERR_TRANSMITERROR, TEXT("��M�f�[�^�̃T�C�Y���s���ł��B"));
		return false;
	}

	// ���X�|���X���
	m_BcasCardInfo.CASystemID = ((WORD)RecvData[6] << 8) | (WORD)RecvData[7];
	::CopyMemory(m_BcasCardInfo.BcasCardID, &RecvData[8], 6UL);		// +8	Card ID
	m_BcasCardInfo.CardType = RecvData[14];
	m_BcasCardInfo.MessagePartitionLength = RecvData[15];
	::CopyMemory(m_BcasCardInfo.SystemKey, &RecvData[16], 32UL);	// +16	Descrambling system key
	::CopyMemory(m_BcasCardInfo.InitialCbc, &RecvData[48], 8UL);	// +48	Descrambler CBC initial value

	const static BYTE CardIDInfoCmd[] = {0x90, 0x32, 0x00, 0x00, 0x00};
	::ZeroMemory(RecvData, sizeof(RecvData));
	dwRecvSize = sizeof(RecvData);
	if (m_pCardReader->Transmit(CardIDInfoCmd, sizeof(CardIDInfoCmd), RecvData, &dwRecvSize)
			&& dwRecvSize >= 17) {
		m_BcasCardInfo.CardManufacturerID = RecvData[7];
		m_BcasCardInfo.CardVersion = RecvData[8];
		m_BcasCardInfo.CheckCode = ((WORD)RecvData[15] << 8) | (WORD)RecvData[16];
	}

	// ECM�X�e�[�^�X������
	::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));

	return true;
}


const bool CBcasCard::GetBcasCardInfo(BcasCardInfo *pInfo)
{
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return false;
	}

	if (pInfo == NULL) {
		SetError(ERR_BADARGUMENT, BAD_ARGUMENT_ERROR_TEXT);
		return false;
	}

	*pInfo = m_BcasCardInfo;

	ClearError();

	return true;
}


const bool CBcasCard::GetCASystemID(WORD *pID)
{
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return false;
	}

	if (pID == NULL) {
		SetError(ERR_BADARGUMENT, BAD_ARGUMENT_ERROR_TEXT);
		return false;
	}

	*pID = m_BcasCardInfo.CASystemID;

	ClearError();

	return true;
}


const BYTE * CBcasCard::GetBcasCardID(void)
{
	// Card ID ��Ԃ�
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return NULL;
	}

	ClearError();

	return m_BcasCardInfo.BcasCardID;
}


const BYTE CBcasCard::GetCardType(void)
{
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return CARDTYPE_INVALID;
	}

	return m_BcasCardInfo.CardType;
}


const BYTE CBcasCard::GetMessagePartitionLength(void)
{
	return m_BcasCardInfo.MessagePartitionLength;
}


const BYTE * CBcasCard::GetInitialCbc(void)
{
	// Descrambler CBC Initial Value ��Ԃ�
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return NULL;
	}

	ClearError();

	return m_BcasCardInfo.InitialCbc;
}


const BYTE * CBcasCard::GetSystemKey(void)
{
	// Descrambling System Key ��Ԃ�
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return NULL;
	}

	ClearError();

	return m_BcasCardInfo.SystemKey;
}


const BYTE * CBcasCard::GetKsFromEcm(const BYTE *pEcmData, const DWORD dwEcmSize)
{
	// �uECM Receive Command�v����������
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return NULL;
	}

	// ECM�T�C�Y���`�F�b�N
	if (!pEcmData || (dwEcmSize < MIN_ECM_DATA_SIZE) || (dwEcmSize > MAX_ECM_DATA_SIZE)) {
		SetError(ERR_BADARGUMENT, BAD_ARGUMENT_ERROR_TEXT);
		return NULL;
	}

	// �L���b�V�����`�F�b�N����
	if (m_EcmStatus.dwLastEcmSize == dwEcmSize
			&& ::memcmp(m_EcmStatus.LastEcmData, pEcmData, dwEcmSize) == 0) {
		// ECM������̏ꍇ�̓L���b�V���ς�Ks��Ԃ�
		if (m_EcmStatus.bSucceeded) {
			ClearError();
			return m_EcmStatus.KsData;
		} else {
			SetError(ERR_ECMREFUSED, ECM_REFUSED_ERROR_TEXT);
			return NULL;
		}
	}

	// �o�b�t�@����
	static const BYTE EcmReceiveCmd[] = {0x90, 0x34, 0x00, 0x00};
	BYTE SendData[MAX_ECM_DATA_SIZE + 6];
	BYTE RecvData[1024];
	::ZeroMemory(RecvData, sizeof(RecvData));

	// �R�}���h�\�z
	::CopyMemory(SendData, EcmReceiveCmd, sizeof(EcmReceiveCmd));				// CLA, INS, P1, P2
	SendData[sizeof(EcmReceiveCmd)] = (BYTE)dwEcmSize;							// COMMAND DATA LENGTH
	::CopyMemory(&SendData[sizeof(EcmReceiveCmd) + 1], pEcmData, dwEcmSize);	// ECM
	SendData[sizeof(EcmReceiveCmd) + dwEcmSize + 1] = 0x00U;					// RESPONSE DATA LENGTH

	// �R�}���h���M
	DWORD dwRecvSize = sizeof(RecvData);
	if (!m_pCardReader->Transmit(SendData, sizeof(EcmReceiveCmd) + dwEcmSize + 2UL, RecvData, &dwRecvSize)){
		::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));
		SetError(ERR_TRANSMITERROR, m_pCardReader->GetLastErrorText());
		return NULL;
	}

	// �T�C�Y�`�F�b�N
	if (dwRecvSize != 25UL) {
		::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));
		SetError(ERR_TRANSMITERROR, TEXT("ECM�̃��X�|���X�T�C�Y���s���ł��B"));
		return NULL;
	}

	// ECM�f�[�^��ۑ�����
	m_EcmStatus.dwLastEcmSize = dwEcmSize;
	::CopyMemory(m_EcmStatus.LastEcmData, pEcmData, dwEcmSize);

	// ���X�|���X���
	::CopyMemory(m_EcmStatus.KsData, &RecvData[6], sizeof(m_EcmStatus.KsData));

	// ���^�[���R�[�h���
	switch (((WORD)RecvData[4] << 8) | (WORD)RecvData[5]) {
	// Purchased: Viewing
	case 0x0200U :	// Payment-deferred PPV
	case 0x0400U :	// Prepaid PPV
	case 0x0800U :	// Tier

	case 0x4480U :	// Payment-deferred PPV
	case 0x4280U :	// Prepaid PPV
		ClearError();
		m_EcmStatus.bSucceeded = true;
		return m_EcmStatus.KsData;
	}
	// ��L�ȊO(�����s��)

	m_EcmStatus.bSucceeded = false;
	SetError(ERR_ECMREFUSED, ECM_REFUSED_ERROR_TEXT);

	return NULL;
}


const bool CBcasCard::SendEmmSection(const BYTE *pEmmData, const DWORD dwEmmSize)
{
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return false;
	}

	if (pEmmData == NULL || dwEmmSize < 17UL || dwEmmSize > MAX_EMM_DATA_SIZE) {
		SetError(ERR_BADARGUMENT, BAD_ARGUMENT_ERROR_TEXT);
		return false;
	}

	static const BYTE EmmReceiveCmd[] = {0x90, 0x36, 0x00, 0x00};
	BYTE SendData[MAX_EMM_DATA_SIZE + 6], RecvData[1024];

	::CopyMemory(SendData, EmmReceiveCmd, sizeof(EmmReceiveCmd));
	SendData[sizeof(EmmReceiveCmd)] = (BYTE)dwEmmSize;
	::CopyMemory(&SendData[sizeof(EmmReceiveCmd) + 1], pEmmData, dwEmmSize);
	SendData[sizeof(EmmReceiveCmd) + 1 + dwEmmSize] = 0x00;

	::ZeroMemory(RecvData, sizeof(RecvData));
	DWORD RecvSize = sizeof(RecvData);
	if (!m_pCardReader->Transmit(SendData, sizeof(EmmReceiveCmd) + dwEmmSize + 2UL, RecvData, &RecvSize)) {
		SetError(ERR_TRANSMITERROR, m_pCardReader->GetLastErrorText());
		return false;
	}

	if (RecvSize != 8UL) {
		SetError(ERR_TRANSMITERROR, TEXT("EMM�̃��X�|���X�T�C�Y���s���ł��B"));
		return false;
	}

	switch (((WORD)RecvData[4] << 8) | (WORD)RecvData[5]) {
	case 0x2100U :	// ����I��
		ClearError();
		return true;

	//case 0xA102U :	// ��^�p(�^�p�O�v���g�R���ԍ�)
	//case 0xA107U :	// �Z�L�����e�B�G���[(EMM������G���[)
	}

	SetError(ERR_EMMERROR, TEXT("EMM���󂯕t�����܂���B"));

	return false;
}


const bool CBcasCard::SendCommand(const BYTE *pSendData, const DWORD SendSize, BYTE *pReceiveData, DWORD *pReceiveSize)
{
	if (!m_pCardReader) {
		SetError(ERR_CARDNOTOPEN, CARD_NOT_OPEN_ERROR_TEXT);
		return false;
	}

	if (pSendData == NULL || SendSize == 0
			|| pReceiveData == NULL || pReceiveSize == NULL) {
		SetError(ERR_BADARGUMENT, BAD_ARGUMENT_ERROR_TEXT);
		return false;
	}

	if (!m_pCardReader->Transmit(pSendData, SendSize, pReceiveData, pReceiveSize)) {
		SetError(ERR_TRANSMITERROR, m_pCardReader->GetLastErrorText());
		return false;
	}

	ClearError();
	return true;
}


const int CBcasCard::FormatCardID(LPTSTR pszText, int MaxLength) const
{
	if (pszText == NULL || MaxLength <= 0)
		return 0;

	ULONGLONG ID;

	ID = ((((ULONGLONG)(m_BcasCardInfo.BcasCardID[0] & 0x1F) << 40) |
		 ((ULONGLONG)m_BcasCardInfo.BcasCardID[1] << 32) |
		 ((ULONGLONG)m_BcasCardInfo.BcasCardID[2] << 24) |
		 ((ULONGLONG)m_BcasCardInfo.BcasCardID[3] << 16) |
		 ((ULONGLONG)m_BcasCardInfo.BcasCardID[4] << 8) |
		 (ULONGLONG)m_BcasCardInfo.BcasCardID[5]) * 100000ULL) +
		 m_BcasCardInfo.CheckCode;
	return StdUtil::snprintf(pszText, MaxLength,
			TEXT("%d%03lu %04lu %04lu %04lu %04lu"),
			m_BcasCardInfo.BcasCardID[0] >> 5,
			(DWORD)(ID / (10000ULL * 10000ULL * 10000ULL * 10000ULL)) % 10000,
			(DWORD)(ID / (10000ULL * 10000ULL * 10000ULL)) % 10000,
			(DWORD)(ID / (10000ULL * 10000ULL) % 10000ULL),
			(DWORD)(ID / 10000ULL % 10000ULL),
			(DWORD)(ID % 10000ULL));
}


const char CBcasCard::GetCardManufacturerID() const
{
	return m_BcasCardInfo.CardManufacturerID;
}


const BYTE CBcasCard::GetCardVersion() const
{
	return m_BcasCardInfo.CardVersion;
}
