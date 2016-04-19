// FileReader.h: CFileReader �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "NCachedFile.h"


/////////////////////////////////////////////////////////////////////////////
// �ėp�t�@�C������(�t�@�C������ǂݍ��񂾃f�[�^���f�R�[�_�O���t�ɓ��͂���)
/////////////////////////////////////////////////////////////////////////////
// Output	#0	: CMediaData		�ǂݍ��݃f�[�^
/////////////////////////////////////////////////////////////////////////////


#define DEF_READSIZE	0x00200000UL		// 2MB


class CFileReader : public CMediaDecoder  
{
public:
	enum EVENTID
	{
		EID_READ_ASYNC_START,		// �񓯊����[�h�J�n
		EID_READ_ASYNC_END,			// �񓯊����[�h�I��
		EID_READ_ASYNC_PREREAD,		// �񓯊����[�h�O
		EID_READ_ASYNC_POSTREAD		// �񓯊����[�h��
	};

	CFileReader(IEventHandler *pEventHandler = NULL);
	virtual ~CFileReader();

// IMediaDecoder
	//virtual void Reset(void);
	const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex);

// CFileReader
	const bool OpenFile(LPCTSTR lpszFileName);
	void CloseFile(void);
	const bool IsOpen() const;

	const DWORD ReadSync(const DWORD dwReadSize = DEF_READSIZE);
	const DWORD ReadSync(const DWORD dwReadSize, const ULONGLONG llReadPos);

	const bool StartReadAnsync(const DWORD dwReadSize = DEF_READSIZE, const ULONGLONG llReadPos = 0ULL);
	void StopReadAnsync(void);
	const bool IsAnsyncReadBusy(void) const;

	const ULONGLONG GetReadPos(void);
	const ULONGLONG GetFileSize(void);
	const bool SetReadPos(const ULONGLONG llReadPos);

protected:
//	CNFile m_InFile;
	CNCachedFile m_InFile;
	CMediaData m_ReadBuffer;

	HANDLE m_hReadAnsyncThread;
	DWORD m_dwReadAnsyncThreadID;

	bool m_bKillSignal;
	bool m_bIsAnsyncRead;

private:
	static DWORD WINAPI ReadAnsyncThread(LPVOID pParam);
};
