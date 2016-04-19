// FileWriter.cpp: CFileWriter �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FileReader.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// �\�z/����
//////////////////////////////////////////////////////////////////////

CFileReader::CFileReader(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 0UL, 1UL)
	, m_ReadBuffer((BYTE)0x00U, DEF_READSIZE)
	, m_hReadAnsyncThread(NULL)
	, m_dwReadAnsyncThreadID(0UL)
	, m_bKillSignal(true)
	, m_bIsAnsyncRead(false)
{

}

CFileReader::~CFileReader()
{
	CloseFile();

	if(m_hReadAnsyncThread)::CloseHandle(m_hReadAnsyncThread);
}

const bool CFileReader::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	// �\�[�X�f�R�[�_�̂��ߏ�ɃG���[��Ԃ�
	return false;
}

const bool CFileReader::OpenFile(LPCTSTR lpszFileName)
{
	// ��U����
	CloseFile();

	// �t�@�C�����J��
	return (m_InFile.Open(lpszFileName, CNCachedFile::CNF_READ | CNCachedFile::CNF_SHAREREAD | CNCachedFile::CNF_SHAREWRITE))? true : false;
}

void CFileReader::CloseFile(void)
{
	// �t�@�C�������
	StopReadAnsync();
	m_InFile.Close();
}

const bool CFileReader::IsOpen() const
{
	return m_InFile.IsOpen();
}

const DWORD CFileReader::ReadSync(const DWORD dwReadSize)
{
	// �ǂݍ��݃T�C�Y�v�Z
	ULONGLONG llRemainSize = m_InFile.GetSize() - m_InFile.GetPos();
	const DWORD dwReqSize = (llRemainSize > (ULONGLONG)dwReadSize)? dwReadSize : (DWORD)llRemainSize;
	if(!dwReqSize)return 0UL;

	// �o�b�t�@�m��
	m_ReadBuffer.SetSize(dwReqSize);

	// �t�@�C���ǂݍ���
	if(m_InFile.Read(m_ReadBuffer.GetData(), dwReqSize) != dwReqSize)return 0UL;

	// �f�[�^�o��
	OutputMedia(&m_ReadBuffer);

	return dwReqSize;
}

const DWORD CFileReader::ReadSync(const DWORD dwReadSize, const ULONGLONG llReadPos)
{
	// �t�@�C���V�[�N
	if(!m_InFile.SetPos(llReadPos))return 0UL;

	// �f�[�^�o��
	return ReadSync(dwReadSize);
}

const bool CFileReader::StartReadAnsync(const DWORD dwReadSize, const ULONGLONG llReadPos)
{
	if(!m_bKillSignal)return false;
	
	if(m_hReadAnsyncThread){
		::CloseHandle(m_hReadAnsyncThread);
		m_hReadAnsyncThread = NULL;
		}

	// �t�@�C���V�[�N
	if(!m_InFile.SetPos(llReadPos))return false;

	// �񓯊����[�h�X���b�h�N��
	m_dwReadAnsyncThreadID = 0UL;
	m_bKillSignal = false;
	m_bIsAnsyncRead = false;

	if(!(m_hReadAnsyncThread = ::CreateThread(NULL, 0UL, CFileReader::ReadAnsyncThread, (LPVOID)this, 0UL, &m_dwReadAnsyncThreadID))){
		return false;
		}

	return true;
}

void CFileReader::StopReadAnsync(void)
{
	// �񓯊����[�h��~
	m_bKillSignal = true;
	if(m_hReadAnsyncThread)
	{
		if(WaitForSingleObject(m_hReadAnsyncThread,5000)!=WAIT_OBJECT_0)
		{
			// �X���b�h���������
			::TerminateThread(m_hReadAnsyncThread,-1);
			m_hReadAnsyncThread=NULL;
		}
	}
}

const bool CFileReader::IsAnsyncReadBusy(void) const
{
	// �񓯊����[�h���L����Ԃ�
	return m_bIsAnsyncRead;
}

const ULONGLONG CFileReader::GetReadPos(void)
{
	// �t�@�C���|�W�V������Ԃ�
	return m_InFile.GetPos();
}

const ULONGLONG CFileReader::GetFileSize(void)
{
	// �t�@�C���T�C�Y��Ԃ�
	return m_InFile.GetSize();
}

const bool CFileReader::SetReadPos(const ULONGLONG llReadPos)
{
	// �t�@�C���V�[�N
	return m_InFile.SetPos(llReadPos);
}

DWORD WINAPI CFileReader::ReadAnsyncThread(LPVOID pParam)
{
	// �񓯊����[�h�X���b�h(�t�@�C�����[�h�ƃO���t������ʃX���b�h�ɂ���Ƃ�萫�\�����シ��)
	CFileReader *pThis = static_cast<CFileReader *>(pParam);

	pThis->m_bIsAnsyncRead = true;

	// �u�񓯊����[�h�J�n�v�C�x���g�ʒm
	pThis->SendDecoderEvent(EID_READ_ASYNC_START);

	while(!pThis->m_bKillSignal && (pThis->m_InFile.GetPos() < pThis->m_InFile.GetSize())){
		
		// �u�񓯊����[�h�O�v�C�x���g�ʒm
		if(pThis->SendDecoderEvent(EID_READ_ASYNC_PREREAD))break;
		
		// �t�@�C���������[�h
		pThis->ReadSync();
			
		// �u�񓯊����[�h��v�C�x���g�ʒm
		if(pThis->SendDecoderEvent(EID_READ_ASYNC_POSTREAD))break;
		}

	// �u�񓯊����[�h�I���v�C�x���g�ʒm
	pThis->SendDecoderEvent(EID_READ_ASYNC_END);

	pThis->m_bKillSignal = true;
	pThis->m_bIsAnsyncRead = false;

	return 0UL;
}
