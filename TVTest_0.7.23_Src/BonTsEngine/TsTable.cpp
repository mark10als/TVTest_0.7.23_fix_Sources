// TsTable.cpp: TS�e�[�u�����b�p�[�N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsTable.h"
#include "TsEncode.h"
#include "TsUtilClass.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CPsiTable�N���X�̍\�z/����
//////////////////////////////////////////////////////////////////////
/*
CPsiTable::CPsiTable()
{
	Reset();
}

CPsiTable::CPsiTable(const CPsiTable &Operand)
{
	*this = Operand;
}

CPsiTable & CPsiTable::operator = (const CPsiTable &Operand)
{
	m_TableArray = Operand.m_TableArray;

	return *this;
}

const bool CPsiTable::StoreSection(const CPsiSection *pSection, bool *pbUpdate)
{
	bool bUpdate = false;

	// �Z�N�V�����̃T�C�Y���`�F�b�N
	if(!pSection->GetPayloadSize()){
		if(pbUpdate)*pbUpdate = bUpdate;
		return false;
		}

	// �J�����g�l�N�X�g�C���W�P�[�^�`�F�b�N(�{���͕ʃo���N�ɒ��߂�ׂ��H)
	if(!pSection->IsCurrentNext()){
		if(pbUpdate)*pbUpdate = bUpdate;
		return true;
		}

	// �e�[�u��ID�g������������
	DWORD dwIndex;

	for(dwIndex = 0UL ; dwIndex < m_TableArray.size() ; dwIndex++){
		if(m_TableArray[dwIndex].wTableIdExtension == pSection->GetTableIdExtension())break;
		}

	if(dwIndex >= m_TableArray.size()){
		// �e�[�u��ID�g����������Ȃ��A�e�[�u���ǉ�
		m_TableArray.resize(dwIndex + 1);
		m_TableArray[dwIndex].wTableIdExtension = pSection->GetTableIdExtension();
		m_TableArray[dwIndex].wSectionNum = (WORD)pSection->GetLastSectionNumber() + 1U;
		m_TableArray[dwIndex].byVersionNo = pSection->GetVersionNo();
		m_TableArray[dwIndex].SectionArray.resize(m_TableArray[dwIndex].wSectionNum);
		bUpdate = true;
		}
	else if(m_TableArray[dwIndex].byVersionNo != pSection->GetVersionNo()){
		// �o�[�W�������s��v�A�e�[�u�����X�V���ꂽ
		m_TableArray[dwIndex].wSectionNum = (WORD)pSection->GetLastSectionNumber() + 1U;
		m_TableArray[dwIndex].byVersionNo = pSection->GetVersionNo();
		m_TableArray[dwIndex].SectionArray.clear();
		m_TableArray[dwIndex].SectionArray.resize(m_TableArray[dwIndex].wSectionNum);
		bUpdate = true;
		}
	else if(m_TableArray[dwIndex].wSectionNum != ((WORD)pSection->GetLastSectionNumber() + 1U)){
		// �Z�N�V���������ω�����(��O�H)
		m_TableArray[dwIndex].wSectionNum = (WORD)pSection->GetLastSectionNumber() + 1U;
		m_TableArray[dwIndex].SectionArray.resize(m_TableArray[dwIndex].wSectionNum);
		}

	// �Z�N�V�����f�[�^���X�V����
	m_TableArray[dwIndex].SectionArray[pSection->GetSectionNumber()].SetData(pSection->GetPayloadData(), pSection->GetPayloadSize());

	// �X�V���ݒ�
	if(pbUpdate)*pbUpdate = bUpdate;

	return true;
}

const WORD CPsiTable::GetExtensionNum(void) const
{
	// �e�[�u���̐���Ԃ�
	return m_TableArray.size();
}

const bool CPsiTable::GetExtension(const WORD wIndex, WORD *pwExtension) const
{
	if(wIndex >= GetExtensionNum())return false;

	// �e�[�u��ID�g����Ԃ�
	*pwExtension = m_TableArray[wIndex].wTableIdExtension;

	return true;
}

const bool CPsiTable::GetSectionNum(const WORD wIndex, WORD *pwSectionNum) const
{
	if(wIndex >= GetExtensionNum())return false;

	// �e�[�u��ID�g����Ԃ�
	*pwSectionNum = m_TableArray[wIndex].wSectionNum;

	return true;
}

const CMediaData * CPsiTable::GetSectionData(const WORD wIndex, const BYTE bySectionNo) const
{
	if(wIndex >= GetExtensionNum())return NULL;
	if(bySectionNo > m_TableArray[wIndex].wSectionNum)return NULL;

	// �Z�N�V�����f�[�^��Ԃ�
	return &m_TableArray[wIndex].SectionArray[bySectionNo];
}

void CPsiTable::Reset(void)
{
	// �S�Ẵe�[�u�����폜����
	m_TableArray.clear();
}
*/


/////////////////////////////////////////////////////////////////////////////
// PSI�V���O���e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPsiSingleTable::CPsiSingleTable(const bool bTargetSectionExt)
	: CTsPidMapTarget()
	, m_PsiSectionParser(this, bTargetSectionExt)
	, m_bTableUpdated(false)
{

}

CPsiSingleTable::CPsiSingleTable(const CPsiSingleTable &Operand)
	: CTsPidMapTarget()
	, m_PsiSectionParser(this)
{
	// �R�s�[�R���X�g���N�^
	*this = Operand;
}

CPsiSingleTable::~CPsiSingleTable()
{

}

CPsiSingleTable & CPsiSingleTable::operator = (const CPsiSingleTable &Operand)
{
	// �C���X�^���X�̃R�s�[
	if (&Operand != this) {
		m_CurSection = Operand.m_CurSection;
		m_PsiSectionParser = Operand.m_PsiSectionParser;
		m_bTableUpdated = Operand.m_bTableUpdated;
	}

	return *this;
}

const bool CPsiSingleTable::StorePacket(const CTsPacket *pPacket)
{
	if(!pPacket)return false;

	m_bTableUpdated = false;

	// �p�P�b�g�X�g�A
	m_PsiSectionParser.StorePacket(pPacket);

	return m_bTableUpdated;
}

void CPsiSingleTable::OnPidUnmapped(const WORD wPID)
{
	// �C���X�^���X���J������
	delete this;
}

void CPsiSingleTable::Reset(void)
{
	// ��ԏ�����
	m_PsiSectionParser.Reset();
	m_CurSection.Reset();

	m_bTableUpdated = false;
}

const DWORD CPsiSingleTable::GetCrcErrorCount(void) const
{
	// CRC�G���[�J�E���g���擾����
	return m_PsiSectionParser.GetCrcErrorCount();
}

const bool CPsiSingleTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	// �f�t�H���g�̎����ł͉������Ȃ�
	return true;
}

void CPsiSingleTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	// �Z�N�V�����̃t�B���^�����O���s���ꍇ�͔h���N���X�ŃI�[�o�[���C�h����
	// �f�t�H���g�̎����ł̓Z�N�V�����y�C���[�h�X�V���ɉ��z�֐��ɒʒm����
	if(!(*pSection == m_CurSection)){
		// �Z�N�V�������X�V���ꂽ
		if(OnTableUpdate(pSection, &m_CurSection)){
			// �Z�N�V�����X�g�A
			m_CurSection = *pSection;
			m_bTableUpdated = true;
			}
		}
}


/////////////////////////////////////////////////////////////////////////////
// �X�g���[���^�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPsiStreamTable::CPsiStreamTable(ISectionHandler *pHandler, const bool bTargetSectionExt, const bool bIgnoreSectionNumber)
	: CTsPidMapTarget()
	, m_PsiSectionParser(this, bTargetSectionExt, bIgnoreSectionNumber)
	, m_pSectionHandler(pHandler)
	, m_bTableUpdated(false)
{

}

CPsiStreamTable::CPsiStreamTable(const CPsiStreamTable &Operand)
	: CTsPidMapTarget()
	, m_PsiSectionParser(this)
{
	// �R�s�[�R���X�g���N�^
	*this = Operand;
}

CPsiStreamTable::~CPsiStreamTable()
{

}

CPsiStreamTable & CPsiStreamTable::operator = (const CPsiStreamTable &Operand)
{
	// �C���X�^���X�̃R�s�[
	if (&Operand != this) {
		m_pSectionHandler = Operand.m_pSectionHandler;
		m_PsiSectionParser = Operand.m_PsiSectionParser;
		m_bTableUpdated = Operand.m_bTableUpdated;
	}

	return *this;
}

const bool CPsiStreamTable::StorePacket(const CTsPacket *pPacket)
{
	if(!pPacket)return false;

	m_bTableUpdated = false;

	// �p�P�b�g�X�g�A
	m_PsiSectionParser.StorePacket(pPacket);

	return m_bTableUpdated;
}

void CPsiStreamTable::OnPidUnmapped(const WORD wPID)
{
	// �C���X�^���X���J������
	delete this;
}

void CPsiStreamTable::Reset(void)
{
	// ��ԏ�����
	m_PsiSectionParser.Reset();

	m_bTableUpdated = false;

	if (m_pSectionHandler)
		m_pSectionHandler->OnReset(this);
}

const DWORD CPsiStreamTable::GetCrcErrorCount(void) const
{
	// CRC�G���[�J�E���g���擾����
	return m_PsiSectionParser.GetCrcErrorCount();
}

const bool CPsiStreamTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	// �f�t�H���g�̎����ł͉������Ȃ�
	return true;
}

void CPsiStreamTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	if (OnTableUpdate(pSection)) {
		if (m_pSectionHandler)
			m_pSectionHandler->OnSection(this, pSection);
		m_bTableUpdated = true;
	}
}

CPsiStreamTable::ISectionHandler::~ISectionHandler()
{
}


/////////////////////////////////////////////////////////////////////////////
// PSI NULL�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPsiNullTable::CPsiNullTable()
	: CTsPidMapTarget()
{

}

CPsiNullTable::CPsiNullTable(const CPsiNullTable &Operand)
	: CTsPidMapTarget()
{
	// �R�s�[�R���X�g���N�^
	*this = Operand;
}

CPsiNullTable::~CPsiNullTable()
{

}

CPsiNullTable & CPsiNullTable::operator = (const CPsiNullTable &Operand)
{
	// �C���X�^���X�̃R�s�[

	return *this;
}

/* �������z�֐��Ƃ��Ď���
const bool CPsiNullTable::StorePacket(const CTsPacket *pPacket)
{
	if(!pPacket)return false;
	return true;
}
*/

void CPsiNullTable::OnPidUnmapped(const WORD wPID)
{
	// �C���X�^���X���J������
	delete this;
}


/////////////////////////////////////////////////////////////////////////////
// PSI�e�[�u���Z�b�g���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////
/*
CPsiTableSuite::CPsiTableSuite()
	: m_bTargetSectionExt(true)
{
	m_PsiSectionParser.SetRecvCallback(m_bTargetSectionExt, CPsiTableSuite::StoreSection, this);

	Reset();
}

CPsiTableSuite::CPsiTableSuite(const CPsiTableSuite &Operand)
{
	*this = Operand;
}

CPsiTableSuite & CPsiTableSuite::operator = (const CPsiTableSuite &Operand)
{
	m_TableSet = Operand.m_TableSet;
	m_bTargetSectionExt = Operand.m_bTargetSectionExt;
	m_bTableUpdated = Operand.m_bTableUpdated;
	m_PsiSectionParser = Operand.m_PsiSectionParser;

	m_PsiSectionParser.SetRecvCallback(m_bTargetSectionExt, CPsiTableSuite::StoreSection, this);

	return *this;
}

const bool CPsiTableSuite::StorePacket(const CTsPacket *pPacket)
{
	m_bTableUpdated = false;

	// PSI�Z�N�V�������e�[�u���ɒǉ�����
	m_PsiSectionParser.StorePacket(pPacket);

	return m_bTableUpdated;
}

void CPsiTableSuite::SetTargetSectionExt(const bool bTargetExt)
{
	// �����ΏۂƂ���Z�N�V�����̎�ނ�ݒ肷��(�g�� or �W��)
	m_bTargetSectionExt = bTargetExt;
	m_PsiSectionParser.SetRecvCallback(bTargetExt, CPsiTableSuite::StoreSection, this);
}

const bool CPsiTableSuite::AddTable(const BYTE byTableID)
{
	const WORD wNum = m_TableSet.size();

	// ������ID���`�F�b�N
	if(GetIndexByID(byTableID) < wNum)return false;

	// �Ō���ɗv�f��ǉ�
	m_TableSet.resize(wNum + 1U);

	// ID���Z�b�g
	m_TableSet[wNum].byTableID = byTableID;

	return true;
}

const WORD CPsiTableSuite::GetIndexByID(const BYTE byTableID)
{
	// �����̃e�[�u��ID����������
	for(WORD wIndex = 0U ; wIndex < m_TableSet.size() ; wIndex++){
		if(m_TableSet[wIndex].byTableID == byTableID)return wIndex;
		}

	// �G���[���͏�Ƀe�[�u���̍ő吔���傫���C���f�b�N�X��Ԃ�
	return 0x0100U;
}

const CPsiTable * CPsiTableSuite::GetTable(const WORD wIndex) const
{
	// �e�[�u����Ԃ�
	return (wIndex < m_TableSet.size())? &m_TableSet[wIndex].PsiTable : NULL;
}

const CMediaData * CPsiTableSuite::GetSectionData(const WORD wIndex, const WORD wSubIndex, const BYTE bySectionNo) const
{
	// �Z�N�V�����f�[�^��Ԃ�
	const CPsiTable *pPsiTable = GetTable(wIndex);

	// �e�[�u����������Ȃ�
	if(!pPsiTable)return NULL;

	// �f�[�^��Ԃ�
	return pPsiTable->GetSectionData(wSubIndex, bySectionNo);
}

void CPsiTableSuite::Reset(void)
{
	m_bTableUpdated = false;

	m_PsiSectionParser.Reset();
	m_TableSet.clear();
}

const DWORD CPsiTableSuite::GetCrcErrorCount(void) const
{
	return m_PsiSectionParser.GetCrcErrorCount();
}

void CALLBACK CPsiTableSuite::StoreSection(const CPsiSection *pSection, const PVOID pParam)
{
	CPsiTableSuite *pThis = static_cast<CPsiTableSuite *>(pParam);

	// �ΏۊO�̃Z�N�V�����͏������Ȃ�
	if(pSection->IsExtendedSection() != pThis->m_bTargetSectionExt)return;

	// �e�[�u��ID������
	const WORD wIndex = pThis->GetIndexByID(pSection->GetTableID());

	// �e�[�u��ID��������Ȃ�
	if(wIndex >= pThis->m_TableSet.size())return;

	// �e�[�u���ɃX�g�A����
	bool bUpdate = false;
	pThis->m_TableSet[wIndex].PsiTable.StoreSection(pSection, &bUpdate);

	if(bUpdate)pThis->m_bTableUpdated = true;
}
*/


/////////////////////////////////////////////////////////////////////////////
// PAT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPatTable::CPatTable(
#ifdef _DEBUG
	bool bTrace
#endif
)
	: CPsiSingleTable()
#ifdef _DEBUG
	, m_bDebugTrace(bTrace)
#endif
{
	Reset();
}

CPatTable::CPatTable(const CPatTable &Operand)
{
	*this = Operand;
}

CPatTable & CPatTable::operator = (const CPatTable &Operand)
{
	if (&Operand != this) {
		CPsiSingleTable::operator = (Operand);

		m_NitPIDArray = Operand.m_NitPIDArray;
		m_PmtPIDArray = Operand.m_PmtPIDArray;
	}

	return *this;
}

void CPatTable::Reset(void)
{
	// ��Ԃ��N���A����
	CPsiSingleTable::Reset();

	m_NitPIDArray.clear();
	m_PmtPIDArray.clear();
}

const WORD CPatTable::GetTransportStreamID(void) const
{
	return m_CurSection.GetTableIdExtension();
}

const WORD CPatTable::GetNitPID(const WORD wIndex) const
{
	// NIT��PID��Ԃ�
	return (wIndex < m_NitPIDArray.size())? m_NitPIDArray[wIndex] : 0xFFFFU;	// 0xFFFF�͖���`��PID
}

const WORD CPatTable::GetNitNum(void) const
{
	// NIT�̐���Ԃ�
	return (WORD)m_NitPIDArray.size();
}

const WORD CPatTable::GetPmtPID(const WORD wIndex) const
{
	// PMT��PID��Ԃ�
	return (wIndex < m_PmtPIDArray.size())? m_PmtPIDArray[wIndex].wPID : 0xFFFFU;	// 0xFFFF�͖���`��PID
}

const WORD CPatTable::GetProgramID(const WORD wIndex) const
{
	// Program Number ID��Ԃ�
	return (wIndex < m_PmtPIDArray.size())? m_PmtPIDArray[wIndex].wProgramID : 0x0000U;	// 0xFFFF�͖���`��PID
}

const WORD CPatTable::GetProgramNum(void) const
{
	// PMT�̐���Ԃ�
	return (WORD)m_PmtPIDArray.size();
}

const bool CPatTable::IsPmtTablePID(const WORD wPID) const
{
	// PMT��PID���ǂ�����Ԃ�
	for (WORD wIndex = 0U ; wIndex < m_PmtPIDArray.size() ; wIndex++) {
		if (wPID == m_PmtPIDArray[wIndex].wPID)
			return true;
	}

	return false;
}

const bool CPatTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if(wDataSize % 4U)return false;						// �e�[�u���̃T�C�Y���s��
	if(pCurSection->GetTableID() != 0x00U)return false;	// �e�[�u��ID���s��

	// PID���N���A����
	m_NitPIDArray.clear();
	m_PmtPIDArray.clear();

	// �e�[�u������͂���

#ifdef _DEBUG
	if (m_bDebugTrace)
		TRACE(TEXT("\n------- PAT Table -------\nTS ID = %04X\n"), pCurSection->GetTableIdExtension());
#endif

	for (WORD wPos = 0 ; wPos < wDataSize ; wPos += 4, pHexData += 4) {
		TAG_PATITEM PatItem;

		PatItem.wProgramID	= ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];				// +1,2
		PatItem.wPID		= ((WORD)(pHexData[2] & 0x1FU) << 8) | (WORD)pHexData[3];	// +3,4

		if (PatItem.wProgramID == 0) {
			// NIT��PID
#ifdef _DEBUG
			if (m_bDebugTrace)
				TRACE(TEXT("NIT #%u [ID:%04X][PID:%04X]\n"),
					  m_NitPIDArray.size(), PatItem.wProgramID, PatItem.wPID);
#endif
			m_NitPIDArray.push_back(PatItem.wPID);
		} else {
			// PMT��PID
#ifdef _DEBUG
			if (m_bDebugTrace)
				TRACE(TEXT("PMT #%u [ID:%04X][PID:%04X]\n"),
					  m_PmtPIDArray.size(), PatItem.wProgramID, PatItem.wPID);
#endif
			m_PmtPIDArray.push_back(PatItem);
		}
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// CAT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CCatTable::CCatTable()
	: CPsiSingleTable()
{
	Reset();
}

CCatTable::~CCatTable(void)
{
}

CCatTable  & CCatTable::operator = (const CCatTable &Operand)
{
	if (this != &Operand) {
		CPsiSingleTable::operator=(Operand);
		m_DescBlock = Operand.m_DescBlock;
	}
	return *this;
}

void CCatTable::Reset(void)
{
	// ��Ԃ��N���A����
	m_DescBlock.Reset();

	CPsiSingleTable::Reset();
}

const CCaMethodDesc * CCatTable::GetCaDescBySystemID(const WORD SystemID) const
{
	for (WORD i = 0 ; i < m_DescBlock.GetDescNum() ; i++) {
		const CBaseDesc *pDesc = m_DescBlock.GetDescByIndex(i);

		if (pDesc != NULL && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
			const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

			if (pCaDesc != NULL && pCaDesc->GetCaMethodID() == SystemID)
				return pCaDesc;
		}
	}

	return NULL;
}

WORD CCatTable::GetEmmPID() const
{
	const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(m_DescBlock.GetDescByTag(CCaMethodDesc::DESC_TAG));

	if (pCaDesc != NULL)
		return pCaDesc->GetCaPID();

	return 0xFFFF;
}

WORD CCatTable::GetEmmPID(const WORD CASystemID) const
{
	const CCaMethodDesc *pCaDesc = GetCaDescBySystemID(CASystemID);

	if (pCaDesc != NULL)
		return pCaDesc->GetCaPID();

	return 0xFFFF;
}

const CDescBlock * CCatTable::GetCatDesc(void) const
{
	// �L�q�q�̈��Ԃ�
	return &m_DescBlock;
}

const bool CCatTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	if (pCurSection->GetTableID() != 0x01
			|| pCurSection->GetSectionLength() > 1021
			|| pCurSection->GetSectionNumber() != 0x00
			|| pCurSection->GetLastSectionNumber() != 0x00)
		return false;

	TRACE(TEXT("\n------- CAT Table -------\n"));

	// �e�[�u������͂���
	m_DescBlock.ParseBlock(pCurSection->GetPayloadData(), pCurSection->GetPayloadSize());

#ifdef _DEBUG
	for (WORD i = 0 ; i < m_DescBlock.GetDescNum() ; i++) {
		const CBaseDesc *pBaseDesc = m_DescBlock.GetDescByIndex(i);
		TRACE(TEXT("[%lu] TAG = 0x%02X LEN = %lu\n"), i, pBaseDesc->GetTag(), pBaseDesc->GetLength());
	}
#endif

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// PMT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPmtTable::CPmtTable(
#ifdef _DEBUG
	bool bTrace
#endif
)
	: CPsiSingleTable()
#ifdef _DEBUG
	, m_bDebugTrace(bTrace)
#endif
{
	Reset();
}

CPmtTable::CPmtTable(const CPmtTable &Operand)
{
	*this = Operand;
}

CPmtTable & CPmtTable::operator = (const CPmtTable &Operand)
{
	if (this != &Operand) {
		CPsiSingleTable::operator=(Operand);
		m_wPcrPID = Operand.m_wPcrPID;
		m_TableDescBlock = Operand.m_TableDescBlock;
		m_EsInfoArray = Operand.m_EsInfoArray;
	}

	return *this;
}

void CPmtTable::Reset(void)
{
	// ��Ԃ��N���A����
	CPsiSingleTable::Reset();

	m_wPcrPID = 0xFFFFU;
	m_TableDescBlock.Reset();
	m_EsInfoArray.clear();
}

const WORD CPmtTable::GetProgramNumberID(void) const
{
	return m_CurSection.GetTableIdExtension();
}

const WORD CPmtTable::GetPcrPID(void) const
{
	// PCR_PID ��Ԃ�
	return m_wPcrPID;
}

const CDescBlock * CPmtTable::GetTableDesc(void) const
{
	// �e�[�u���̋L�q�q�u���b�N��Ԃ�
	return &m_TableDescBlock;
}

const WORD CPmtTable::GetEcmPID(void) const
{
	// ECM��PID��Ԃ�
	const CCaMethodDesc *pCaMethodDesc = dynamic_cast<const CCaMethodDesc *>(m_TableDescBlock.GetDescByTag(CCaMethodDesc::DESC_TAG));

	return (pCaMethodDesc)? pCaMethodDesc->GetCaPID() : 0xFFFFU;
}

const WORD CPmtTable::GetEcmPID(const WORD CASystemID) const
{
	// �w�肳�ꂽCA_system_id�ɑΉ�����ECM��PID��Ԃ�
	for (WORD i = 0 ; i < m_TableDescBlock.GetDescNum() ; i++) {
		const CBaseDesc *pDesc = m_TableDescBlock.GetDescByIndex(i);

		if (pDesc != NULL && pDesc->GetTag() == CCaMethodDesc::DESC_TAG) {
			const CCaMethodDesc *pCaDesc = dynamic_cast<const CCaMethodDesc*>(pDesc);

			if (pCaDesc != NULL && pCaDesc->GetCaMethodID() == CASystemID)
				return pCaDesc->GetCaPID();
		}
	}

	return 0xFFFF;
}

const WORD CPmtTable::GetEsInfoNum(void) const
{
	// ES���̐���Ԃ�
	return (WORD)m_EsInfoArray.size();
}

const BYTE CPmtTable::GetStreamTypeID(const WORD wIndex) const
{
	// Stream Type ID ��Ԃ�
	return (wIndex < m_EsInfoArray.size())? m_EsInfoArray[wIndex].byStreamTypeID : 0x00U;	// 0x00�͖���`��ID
}

const WORD CPmtTable::GetEsPID(const WORD wIndex) const
{
	// Elementary Stream PID ��Ԃ�
	return (wIndex < m_EsInfoArray.size())? m_EsInfoArray[wIndex].wEsPID : 0xFFFFU;			// 0xFFFF�͖���`��PID
}

const CDescBlock * CPmtTable::GetItemDesc(const WORD wIndex) const
{
	// �A�C�e���̋L�q�q�u���b�N��Ԃ�
	return (wIndex < m_EsInfoArray.size())? &m_EsInfoArray[wIndex].DescBlock : NULL;
}

const bool CPmtTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 4)
		return false;

	if(pCurSection->GetTableID() != 0x02U)return false;	// �e�[�u��ID���s��

	// ��Ԃ��N���A����
	m_wPcrPID = 0xFFFFU;
	m_EsInfoArray.clear();

	// �e�[�u������͂���
	m_wPcrPID = ((WORD)(pHexData[0] & 0x1FU) << 8) | (WORD)pHexData[1];				// +0,1
	WORD wDescLen = ((WORD)(pHexData[2] & 0x0FU) << 8) | (WORD)pHexData[3];
	if (4 + wDescLen > wDataSize)
		return false;

	// �L�q�q�u���b�N
	m_TableDescBlock.ParseBlock(&pHexData[4], wDescLen);

#ifdef _DEBUG
	if (m_bDebugTrace)
		TRACE(TEXT("\n------- PMT Table -------\nProgram Number ID = %04X(%d)\nPCR PID = %04X\nECM PID = %04X\n"),
			pCurSection->GetTableIdExtension(), pCurSection->GetTableIdExtension(), m_wPcrPID , (m_TableDescBlock.GetDescByTag(CCaMethodDesc::DESC_TAG))? dynamic_cast<const CCaMethodDesc *>(m_TableDescBlock.GetDescByTag(CCaMethodDesc::DESC_TAG))->GetCaPID() : 0xFFFFU);
#endif

	// �X�g���[���������
	for (WORD wPos = wDescLen + 4 ; wPos + 5 <= wDataSize ; wPos += 5 + wDescLen) {
		TAG_PMTITEM PmtItem;

		PmtItem.byStreamTypeID = pHexData[wPos + 0];													// +0
		PmtItem.wEsPID = ((WORD)(pHexData[wPos + 1] & 0x1FU) << 8) | (WORD)pHexData[wPos + 2];			// +1,2
		wDescLen = ((WORD)(pHexData[wPos + 3] & 0x0FU) << 8) | (WORD)pHexData[wPos + 4];				// +3,4
		if (wPos + 5 + wDescLen > wDataSize)
			break;

		// �L�q�q�u���b�N
		PmtItem.DescBlock.ParseBlock(&pHexData[wPos + 5], wDescLen);

#ifdef _DEBUG
		if (m_bDebugTrace)
			TRACE(TEXT("[%u] Stream Type ID = %02X  PID = %04X\n"),
				  m_EsInfoArray.size(), PmtItem.byStreamTypeID, PmtItem.wEsPID);
#endif

		// �e�[�u���ɒǉ�����
		m_EsInfoArray.push_back(PmtItem);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// SDT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CSdtTable::CSdtTable()
	: CPsiSingleTable()
{

}

CSdtTable::CSdtTable(const CSdtTable &Operand)
{
	*this = Operand;
}

CSdtTable & CSdtTable::operator = (const CSdtTable &Operand)
{
	if (this != &Operand) {
		CPsiSingleTable::operator = (Operand);
		m_wNetworkID = Operand.m_wNetworkID;
		m_ServiceInfoArray = Operand.m_ServiceInfoArray;
	}

	return *this;
}

void CSdtTable::Reset(void)
{
	// ��Ԃ��N���A����
	CPsiSingleTable::Reset();

	m_wNetworkID = 0xFFFF;
	m_ServiceInfoArray.clear();
}

const WORD CSdtTable::GetNetworkID(void) const
{
	return m_wNetworkID;
}

const WORD CSdtTable::GetServiceNum(void) const
{
	// �T�[�r�X����Ԃ�
	return (WORD)m_ServiceInfoArray.size();
}

const WORD CSdtTable::GetServiceIndexByID(const WORD wServiceID)
{
	// �T�[�r�XID����C���f�b�N�X��Ԃ�
	for (WORD wIndex = 0U ; wIndex < GetServiceNum() ; wIndex++) {
		if (m_ServiceInfoArray[wIndex].wServiceID == wServiceID) {
			return wIndex;
		}
	}

	// �T�[�r�XID��������Ȃ�
	return 0xFFFFU;
}

const WORD CSdtTable::GetServiceID(const WORD wIndex) const
{
	// 	�T�[�r�XID��Ԃ�
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].wServiceID : 0xFFFFU;
}

const bool CSdtTable::GetEITScheduleFlag(const WORD wIndex) const
{
	// EIT Schedule Flag��Ԃ�
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].bEITScheduleFlag : false;
}

const bool CSdtTable::GetEITPresentFollowingFlag(const WORD wIndex) const
{
	// EIT Present Following��Ԃ�
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].bEITPresentFollowingFlag : false;
}

const BYTE CSdtTable::GetRunningStatus(const WORD wIndex) const
{
	// Running Status��Ԃ�
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].byRunningStatus : 0xFFU;
}

const bool CSdtTable::GetFreeCaMode(const WORD wIndex) const
{
	// Free CA Mode��Ԃ�
	return (wIndex < GetServiceNum())? m_ServiceInfoArray[wIndex].bFreeCaMode : false;
}

const CDescBlock * CSdtTable::GetItemDesc(const WORD wIndex) const
{
	// �A�C�e���̋L�q�q�u���b�N��Ԃ�
	return (wIndex < m_ServiceInfoArray.size())? &m_ServiceInfoArray[wIndex].DescBlock : NULL;
}

void CSdtTable::OnPsiSection(const CPsiSectionParser *pPsiSectionParser, const CPsiSection *pSection)
{
	if(pSection->GetTableID() != 0x42U)return;

	// Actual Stream��������������
	CPsiSingleTable::OnPsiSection(pPsiSectionParser, pSection);
}

const bool CSdtTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 3)
		return false;
	if (pCurSection->GetTableID() != 0x42		// actual
		&& pCurSection->GetTableID() != 0x46)	// other
		return false;

	// ��Ԃ��N���A����
	m_ServiceInfoArray.clear();

	m_wNetworkID = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];

	TRACE(TEXT("\n------- SDT Table -------\nTransport Stream ID = %04X\nOriginal Network ID = %04X\n"), pCurSection->GetTableIdExtension(), m_wNetworkID);

	// �e�[�u������͂���
	WORD wDescLen;
	for (WORD wPos = 3 ; wPos + 5 <= wDataSize ; wPos += 5 + wDescLen) {
		TAG_SDTITEM SdtItem;

		SdtItem.wServiceID					= ((WORD)pHexData[wPos + 0] << 8) | (WORD)pHexData[wPos + 1];
		SdtItem.bEITScheduleFlag			= (pHexData[wPos + 2] & 0x02) != 0;
		SdtItem.bEITPresentFollowingFlag	= (pHexData[wPos + 2] & 0x01) != 0;
		SdtItem.byRunningStatus				= pHexData[wPos + 3] >> 5;
		SdtItem.bFreeCaMode					= (pHexData[wPos + 3] & 0x10U)? true : false;

		// Service Descriptor
		wDescLen = ((WORD)(pHexData[wPos + 3] & 0x0FU) << 8) | (WORD)pHexData[wPos + 4];

		if (wPos + 5 + wDescLen > wDataSize)
			break;

		// �L�q�q�u���b�N
		SdtItem.DescBlock.ParseBlock(&pHexData[wPos + 5], wDescLen);

		// �e�[�u���ǉ�
		m_ServiceInfoArray.push_back(SdtItem);

#ifdef _DEBUG
		// �f�o�b�O�p��������
		const CServiceDesc *pServiceDesc = dynamic_cast<const CServiceDesc *>(SdtItem.DescBlock.GetDescByTag(CServiceDesc::DESC_TAG));
		if (pServiceDesc) {
			TCHAR szServiceName[1024] = {TEXT('\0')};
			pServiceDesc->GetServiceName(szServiceName,1024);
			TRACE(TEXT("[%u] Service ID = %04X  Running Status = %01X  Free CA Mode = %u  Service Type = %02X  Service Name = %s\n"), m_ServiceInfoArray.size(), SdtItem.wServiceID, SdtItem.byRunningStatus, SdtItem.bFreeCaMode, pServiceDesc->GetServiceType(), szServiceName);
		} else {
			TRACE(TEXT("[%u] Service ID = %04X  Running Status = %01X  Free CA Mode = %u  ���T�[�r�X�L�q�q�Ȃ�\n"), m_ServiceInfoArray.size(), SdtItem.wServiceID, SdtItem.byRunningStatus, SdtItem.bFreeCaMode);
		}
		// �����܂�
#endif
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// NIT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CNitTable::CNitTable()
	: CPsiSingleTable()
{
	Reset();
}

CNitTable::CNitTable(const CNitTable &Operand)
{
	*this = Operand;
}

CNitTable & CNitTable::operator = (const CNitTable &Operand)
{
	if (this != &Operand) {
		CPsiSingleTable::operator = (Operand);
		m_wNetworkID = Operand.m_wNetworkID;
		m_NetworkDescBlock = Operand.m_NetworkDescBlock;
		m_TransportStreamArray = Operand.m_TransportStreamArray;
	}

	return *this;
}

void CNitTable::Reset(void)
{
	CPsiSingleTable::Reset();
	m_wNetworkID = 0xFFFF;
	m_NetworkDescBlock.Reset();
	m_TransportStreamArray.clear();
}

const WORD CNitTable::GetNetworkID(void) const
{
	return m_wNetworkID;
}

const CDescBlock * CNitTable::GetNetworkDesc(void) const
{
	return &m_NetworkDescBlock;
}

const WORD CNitTable::GetTransportStreamNum(void) const
{
	return (WORD)m_TransportStreamArray.size();
}

const WORD CNitTable::GetTransportStreamID(const WORD wIndex) const
{
	if (wIndex >= GetTransportStreamNum())
		return 0xFFFF;
	return m_TransportStreamArray[wIndex].wTransportStreamID;
}

const WORD CNitTable::GetOriginalNetworkID(const WORD wIndex) const
{
	if (wIndex >= GetTransportStreamNum())
		return 0xFFFF;
	return m_TransportStreamArray[wIndex].wOriginalNetworkID;
}

const CDescBlock * CNitTable::GetItemDesc(const WORD wIndex) const
{
	if (wIndex >= GetTransportStreamNum())
		return NULL;
	return &m_TransportStreamArray[wIndex].DescBlock;
}

const bool CNitTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 2)
		return false;
	if (pCurSection->GetTableID() != 0x40)
		return false;

	m_TransportStreamArray.clear();

	m_wNetworkID = pCurSection->GetTableIdExtension();

	WORD DescLength = ((WORD)(pHexData[0] & 0x0F) << 8) | pHexData[1];
	WORD Pos = 2;
	if (Pos + DescLength > wDataSize)
		return false;
	m_NetworkDescBlock.ParseBlock(&pHexData[Pos], DescLength);
	Pos += DescLength;

	WORD StreamLoopLength = ((WORD)(pHexData[Pos] & 0x0F) << 8) | pHexData[Pos + 1];
	Pos += 2;
	if (Pos + StreamLoopLength > wDataSize)
		return false;
	for (WORD i = 0 ; i < StreamLoopLength ; i += 6 + DescLength) {
		TAG_NITITEM NitItem;

		if (Pos + 6 > wDataSize)
			return false;
		NitItem.wTransportStreamID = ((WORD)pHexData[Pos + 0] << 8) | pHexData[Pos + 1];
		NitItem.wOriginalNetworkID = ((WORD)pHexData[Pos + 2] << 8) | pHexData[Pos + 3];
		DescLength = ((WORD)(pHexData[Pos + 4] & 0x0F) << 8) | pHexData[Pos + 5];
		Pos += 6;
		if (Pos + DescLength > wDataSize)
			return false;
		NitItem.DescBlock.ParseBlock(&pHexData[Pos], DescLength);
		Pos += DescLength;
		m_TransportStreamArray.push_back(NitItem);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// EIT[p/f]�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CEitPfTable::CEitPfTable()
	: CPsiStreamTable()
{

}

CEitPfTable::CEitPfTable(const CEitPfTable &Operand)
{
	*this = Operand;
}

CEitPfTable & CEitPfTable::operator = (const CEitPfTable &Operand)
{
	if (this != &Operand) {
		CPsiStreamTable::operator = (Operand);
		m_ServiceList = Operand.m_ServiceList;
	}

	return *this;
}

void CEitPfTable::Reset(void)
{
	// ��Ԃ��N���A����
	CPsiStreamTable::Reset();

	m_ServiceList.clear();
}

const DWORD CEitPfTable::GetServiceNum(void) const
{
	// �T�[�r�X����Ԃ�
	return (DWORD)m_ServiceList.size();
}

const int CEitPfTable::GetServiceIndexByID(WORD ServiceID) const
{
	// �T�[�r�XID����C���f�b�N�X��Ԃ�
	for (DWORD Index = 0 ; Index < GetServiceNum() ; Index++) {
		if (m_ServiceList[Index].ServiceID == ServiceID) {
			return Index;
		}
	}

	// �T�[�r�XID��������Ȃ�
	return -1;
}

const WORD CEitPfTable::GetServiceID(DWORD Index) const
{
	// �T�[�r�XID��Ԃ�
	if (Index >= GetServiceNum())
		return 0xFFFF;
	return m_ServiceList[Index].ServiceID;
}

const WORD CEitPfTable::GetTransportStreamID(DWORD Index) const
{
	if (Index >= GetServiceNum())
		return 0;
	return m_ServiceList[Index].TransportStreamID;
}

const WORD CEitPfTable::GetOriginalNetworkID(DWORD Index) const
{
	if (Index >= GetServiceNum())
		return 0;
	return m_ServiceList[Index].OriginalNetworkID;
}

const WORD CEitPfTable::GetEventID(DWORD Index,DWORD EventIndex) const
{
	if (Index >= GetServiceNum() || EventIndex > 1
			|| !m_ServiceList[Index].EventList[EventIndex].bEnable)
		return 0;
	return m_ServiceList[Index].EventList[EventIndex].EventID;
}

const SYSTEMTIME *CEitPfTable::GetStartTime(DWORD Index,DWORD EventIndex) const
{
	if (Index >= GetServiceNum() || EventIndex > 1
			|| !m_ServiceList[Index].EventList[EventIndex].bEnable
			|| !m_ServiceList[Index].EventList[EventIndex].bValidStartTime)
		return NULL;
	return &m_ServiceList[Index].EventList[EventIndex].StartTime;
}

const DWORD CEitPfTable::GetDuration(DWORD Index,DWORD EventIndex) const
{
	if (Index >= GetServiceNum() || EventIndex > 1
			|| !m_ServiceList[Index].EventList[EventIndex].bEnable)
		return 0;
	return m_ServiceList[Index].EventList[EventIndex].Duration;
}

const BYTE CEitPfTable::GetRunningStatus(DWORD Index,DWORD EventIndex) const
{
	if (Index >= GetServiceNum() || EventIndex > 1
			|| !m_ServiceList[Index].EventList[EventIndex].bEnable)
		return 0xFF;
	return m_ServiceList[Index].EventList[EventIndex].RunningStatus;
}

const bool CEitPfTable::GetFreeCaMode(DWORD Index,DWORD EventIndex) const
{
	// Free CA Mode��Ԃ�
	if (Index >= GetServiceNum() || EventIndex > 1
			|| !m_ServiceList[Index].EventList[EventIndex].bEnable)
		return false;
	return m_ServiceList[Index].EventList[EventIndex].FreeCaMode;
}

const CDescBlock * CEitPfTable::GetItemDesc(DWORD Index,DWORD EventIndex) const
{
	// �A�C�e���̋L�q�q�u���b�N��Ԃ�
	if (Index >= GetServiceNum() || EventIndex > 1
			|| !m_ServiceList[Index].EventList[EventIndex].bEnable)
		return NULL;
	return &m_ServiceList[Index].EventList[EventIndex].DescBlock;
}

const bool CEitPfTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	if (pCurSection->GetTableID() != 0x4E
			|| pCurSection->GetSectionNumber() > 0x01)
		return false;

	const WORD wDataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (wDataSize < 18)
		return false;

	WORD ServiceID, TransportStreamID, OriginalNetworkID;
	ServiceID = pCurSection->GetTableIdExtension();
	TransportStreamID = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];
	OriginalNetworkID = ((WORD)pHexData[2] << 8) | (WORD)pHexData[3];

	/*
	if (pHexData[4] != 0x01)	// segment_last_section_number
		return false;
	*/
	if (pHexData[5] != 0x4E)	// last_table_id
		return false;

	/*
	TRACE(TEXT("------- EIT[p/f] Table -------\nSID = %04X / TSID = %04X / ONID = %04X\n"),
		  ServiceID, TransportStreamID, OriginalNetworkID);
	*/

	int Index = GetServiceIndexByID(ServiceID);
	if (Index < 0) {
		ServiceInfo Service;

		Service.ServiceID = ServiceID;
		Service.TransportStreamID = TransportStreamID;
		Service.OriginalNetworkID = OriginalNetworkID;
		m_ServiceList.push_back(Service);
		Index = (int)m_ServiceList.size() - 1;
	}

	EventInfo &Info = m_ServiceList[Index].EventList[pCurSection->GetSectionNumber()];

	Info.EventID = ((WORD)pHexData[6] << 8) | (WORD)pHexData[7];
	Info.bValidStartTime = CAribTime::AribToSystemTime(&pHexData[8], &Info.StartTime);
	Info.Duration = CAribTime::AribBcdToSecond(&pHexData[13]);
	Info.RunningStatus = pHexData[16] >> 5;
	Info.FreeCaMode = (pHexData[16] & 0x10) != 0;
	WORD DescLength = (((WORD)pHexData[16] & 0x0F) << 8) | (WORD)pHexData[17];
	if (DescLength > 0 && DescLength <= wDataSize - 18)
		Info.DescBlock.ParseBlock(&pHexData[18], DescLength);
	else
		Info.DescBlock.Reset();
	Info.bEnable = true;

	/*
	TRACE(TEXT("EventID = %02X / %04d/%d/%d %d:%02d %lu sec\n"),
		  Info.EventID,Info.StartTime.wYear,Info.StartTime.wMonth,Info.StartTime.wDay,Info.StartTime.wHour,Info.StartTime.wMinute,Info.Duration);
	*/

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// TOT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CTotTable::CTotTable()
	: CPsiSingleTable(false)
	, m_bValidDateTime(false)
{
}

CTotTable::~CTotTable()
{
}

void CTotTable::Reset(void)
{
	CPsiSingleTable::Reset();
	m_bValidDateTime = false;
	m_DescBlock.Reset();
}

const bool CTotTable::GetDateTime(SYSTEMTIME *pTime) const
{
	if (pTime == NULL || !m_bValidDateTime)
		return false;
#if 0
	const int Offset = GetLocalTimeOffset();
	if (Offset == 0) {
		*pTime = m_DateTime;
	} else {
		CDateTime Time(m_DateTime);

		Time.Offset(CDateTime::MINUTES(Offset));
		Time.Get(pTime);
	}
#else
	*pTime = m_DateTime;
#endif
	return true;
}

const int CTotTable::GetLocalTimeOffset() const
{
	const CLocalTimeOffsetDesc *pLocalTimeOffset = dynamic_cast<const CLocalTimeOffsetDesc*>(m_DescBlock.GetDescByTag(CLocalTimeOffsetDesc::DESC_TAG));

	if (pLocalTimeOffset && pLocalTimeOffset->IsValid()) {
		return pLocalTimeOffset->GetLocalTimeOffset();
	}
	return 0;
}

const CDescBlock * CTotTable::GetTotDesc(void) const
{
	return &m_DescBlock;
}

const bool CTotTable::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_bValidDateTime = CAribTime::AribToSystemTime(pHexData, &m_DateTime);

	WORD DescLength = (((WORD)pHexData[5] & 0x0F) << 8) | (WORD)pHexData[6];
	if (DescLength > 0 && DescLength <= DataSize - 7)
		m_DescBlock.ParseBlock(&pHexData[7], DescLength);
	else
		m_DescBlock.Reset();

/*
#ifdef _DEBUG
	if (m_bValidDateTime) {
		TRACE(TEXT("TOT : %04d/%02d/%02d %02d:%02d:%02d\n"),
			  m_DateTime.wYear, m_DateTime.wMonth, m_DateTime.wDay,
			  m_DateTime.wHour, m_DateTime.wMinute, m_DateTime.wSecond);
	}
#endif
*/

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// CDT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CCdtTable::CCdtTable(ISectionHandler *pHandler)
	: CPsiStreamTable(pHandler)
{
	Reset();
}

CCdtTable::~CCdtTable()
{
}

void CCdtTable::Reset(void)
{
	CPsiStreamTable::Reset();
	m_OriginalNetworkId = 0xFFFF;
	m_DataType = DATATYPE_INVALID;
	m_DescBlock.Reset();
	m_ModuleData.ClearSize();
}

const WORD CCdtTable::GetOriginalNetworkId() const
{
	return m_OriginalNetworkId;
}

const BYTE CCdtTable::GetDataType() const
{
	return m_DataType;
}

const CDescBlock * CCdtTable::GetDesc(void) const
{
	return &m_DescBlock;
}

const WORD CCdtTable::GetDataModuleSize() const
{
	return (WORD)m_ModuleData.GetSize();
}

const BYTE * CCdtTable::GetDataModuleByte() const
{
	if (m_ModuleData.GetSize() == 0)
		return NULL;

	return m_ModuleData.GetData();
}

const bool CCdtTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (DataSize < 5)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_OriginalNetworkId = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];
	m_DataType = pHexData[2];

	WORD DescLength = (((WORD)pHexData[3] & 0x0F) << 8) | (WORD)pHexData[4];
	if (5 + DescLength <= DataSize) {
		if (DescLength > 0)
			m_DescBlock.ParseBlock(&pHexData[7], DescLength);
		m_ModuleData.SetData(&pHexData[5 + DescLength], DataSize - (5 + DescLength));
	} else {
		m_DescBlock.Reset();
		m_ModuleData.ClearSize();
	}

#ifdef _DEBUG
	/*
	TRACE(TEXT("CDT : Data ID %04X / Network ID %04X / Data type %02X / Size %lu\n"),
		  pCurSection->GetTableIdExtension(), m_OriginalNetworkId, m_DataType, (ULONG)m_ModuleData.GetSize());
	*/
#endif

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// SDTT�e�[�u�����ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CSdttTable::CSdttTable(ISectionHandler *pHandler)
	: CPsiStreamTable(pHandler)
{
	Reset();
}

CSdttTable::~CSdttTable()
{
}

void CSdttTable::Reset(void)
{
	CPsiStreamTable::Reset();
	m_MakerID = 0;
	m_ModelID = 0;
	m_TransportStreamID = 0xFFFF;
	m_OriginalNetworkID = 0xFFFF;
	m_ServiceID = 0xFFFF;
	m_ContentList.clear();
}

const BYTE CSdttTable::GetMakerID() const
{
	return m_MakerID;
}

const BYTE CSdttTable::GetModelID() const
{
	return m_ModelID;
}

const bool CSdttTable::IsCommon() const
{
	return m_MakerID == 0xFF && m_ModelID == 0xFE;
}

const WORD CSdttTable::GetTransportStreamID() const
{
	return m_TransportStreamID;
}

const WORD CSdttTable::GetOriginalNetworkID() const
{
	return m_OriginalNetworkID;
}

const WORD CSdttTable::GetServiceID() const
{
	return m_ServiceID;
}

const BYTE CSdttTable::GetNumOfContents() const
{
	return (BYTE)m_ContentList.size();
}

const CSdttTable::ContentInfo * CSdttTable::GetContentInfo(const BYTE Index) const
{
	if ((size_t)Index >= m_ContentList.size())
		return NULL;

	return &m_ContentList[Index];
}

const bool CSdttTable::IsSchedule(DWORD Index) const
{
	if (Index >= m_ContentList.size())
		return false;

	return !m_ContentList[Index].ScheduleList.empty();
}

const CDescBlock * CSdttTable::GetContentDesc(DWORD Index) const
{
	if (Index >= m_ContentList.size())
		return NULL;

	return &m_ContentList[Index].DescBlock;
}

const bool CSdttTable::OnTableUpdate(const CPsiSection *pCurSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pHexData = pCurSection->GetPayloadData();

	if (DataSize < 7)
		return false;
	if (pCurSection->GetTableID() != TABLE_ID)
		return false;

	m_MakerID = (BYTE)(pCurSection->GetTableIdExtension() >> 8);
	m_ModelID = (BYTE)(pCurSection->GetTableIdExtension() & 0xFF);
	m_TransportStreamID = ((WORD)pHexData[0] << 8) | (WORD)pHexData[1];
	m_OriginalNetworkID = ((WORD)pHexData[2] << 8) | (WORD)pHexData[3];
	m_ServiceID = ((WORD)pHexData[4] << 8) | (WORD)pHexData[5];

	/*
	TRACE(TEXT("\n------- SDTT -------\nMaker 0x%02x / Model 0x%02x / TSID 0x%04x / NID %d / SID 0x%04x\n"),
		  m_MakerID, m_ModelID, m_TransportStreamID, m_OriginalNetworkID, m_ServiceID);
	*/

	m_ContentList.clear();
	const int NumOfContents = pHexData[6];
	DWORD Pos = 7;
	for (int i = 0; i < NumOfContents; i++) {
		if (Pos + 8 > DataSize)
			break;

		ContentInfo Content;

		Content.GroupID = pHexData[Pos] >> 4;
		Content.TargetVersion = ((WORD)(pHexData[Pos] & 0x0F) << 8) | (WORD)pHexData[Pos + 1];
		Content.NewVersion = ((WORD)pHexData[Pos + 2] << 4) | (WORD)(pHexData[Pos + 3] >> 4);
		Content.DownloadLevel = (pHexData[Pos + 3] >> 2) & 0x03;
		Content.VersionIndicator = pHexData[Pos + 3] & 0x03;

		const WORD ContentDescLength = ((WORD)pHexData[Pos + 4] << 4) | (WORD)(pHexData[Pos + 5] >> 4);
		const WORD ScheduleDescLength = ((WORD)pHexData[Pos + 6] << 4) | (WORD)(pHexData[Pos + 7] >> 4);
		if (ContentDescLength < ScheduleDescLength
				|| Pos + ContentDescLength > DataSize)
			break;

		Content.ScheduleTimeShiftInformation = pHexData[Pos + 7] & 0x0F;

		Pos += 8;

		if (ScheduleDescLength) {
			for (DWORD j = 0; j + 8 <= ScheduleDescLength; j += 8) {
				ScheduleDescription Schedule;

				CAribTime::AribToSystemTime(&pHexData[Pos + j], &Schedule.StartTime);
				Schedule.Duration = CAribTime::AribBcdToSecond(&pHexData[Pos + j + 5]);

				Content.ScheduleList.push_back(Schedule);
			}

			Pos += ScheduleDescLength;
		}

		const WORD DescLength = ContentDescLength - ScheduleDescLength;
		if (DescLength > 0) {
			Content.DescBlock.ParseBlock(&pHexData[Pos], DescLength);
			Pos += DescLength;
		}

		m_ContentList.push_back(Content);

		/*
		TRACE(TEXT("[%d/%d] : Group 0x%02x / Target ver. 0x%03x / New ver. 0x%03x / Download level %d / Version indicator %d\n"),
			  i + 1, NumOfContents,
			  Content.GroupID, Content.TargetVersion, Content.NewVersion, Content.DownloadLevel, Content.VersionIndicator);
		*/
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// PCR���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CPcrTable::CPcrTable(WORD wServiceIndex)
	: CPsiNullTable()
	, m_ui64_Pcr(0)
{
	m_ServiceIndex.push_back(wServiceIndex);
}

CPcrTable::CPcrTable(const CPcrTable &Operand)
{
	*this = Operand;
}

CPcrTable & CPcrTable::operator = (const CPcrTable &Operand)
{
	if (this != &Operand) {
		CPsiNullTable::operator = (Operand);
		m_ServiceIndex = Operand.m_ServiceIndex;
		m_ui64_Pcr = Operand.m_ui64_Pcr;
	}

	return *this;
}

const bool CPcrTable::StorePacket(const CTsPacket *pPacket)
{
	if(!pPacket)return false;

	if(pPacket->m_AdaptationField.bPcrFlag){
		if (pPacket->m_AdaptationField.byOptionSize < 5)
			return false;
		m_ui64_Pcr = ((unsigned __int64)pPacket->m_AdaptationField.pOptionData[0] << 25 ) |
			((unsigned __int64)pPacket->m_AdaptationField.pOptionData[1] << 17 ) |
			((unsigned __int64)pPacket->m_AdaptationField.pOptionData[2] << 9 ) |
			((unsigned __int64)pPacket->m_AdaptationField.pOptionData[3] << 1 ) |
			((unsigned __int64)pPacket->m_AdaptationField.pOptionData[4] >> 7 );
		}

	return true;
}

void CPcrTable::AddServiceIndex(WORD wServiceIndex)
{
	m_ServiceIndex.push_back(wServiceIndex);
}

const WORD CPcrTable::GetServiceIndex(WORD *pwServiceIndex, WORD wIndex)
{
	if (wIndex < m_ServiceIndex.size() && pwServiceIndex) {
		*pwServiceIndex = m_ServiceIndex[wIndex];
		return true;
	}
	return false;
}

const unsigned __int64 CPcrTable::GetPcrTimeStamp()
{
	return m_ui64_Pcr;
}
