// FileWriter.h: CFileWriter �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "NCachedFile.h"


/////////////////////////////////////////////////////////////////////////////
// �ėp�t�@�C���o��(CMediaData�����̂܂܃t�@�C���ɏ����o��)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		�������݃f�[�^
/////////////////////////////////////////////////////////////////////////////

class CFileWriter : public CMediaDecoder
{
public:
	enum EVENTID
	{
		EID_WRITE_ERROR
	};

	CFileWriter(IEventHandler *pEventHandler = NULL);
	virtual ~CFileWriter();

// IMediaDecoder
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CFileWriter
	// Modified by HDUSTest�̒��̐l
	// �t���O���w��ł���悤�ɂ���
	// const bool OpenFile(LPCTSTR lpszFileName);
	const bool OpenFile(LPCTSTR lpszFileName, BYTE bFlags = 0);
	void CloseFile(void);
	const bool IsOpen() const;

	const LONGLONG GetWriteSize(void) const;
	const LONGLONG GetWriteCount(void) const;

	// Append by HDUSTest�̒��̐l
	bool SetBufferSize(DWORD Size);
	DWORD GetBufferSize() const { return m_BufferSize; }
	bool Pause();
	bool Resume();
	LPCTSTR GetFileName() const;
protected:
	CNCachedFile m_OutFile;
	DWORD m_BufferSize;

	LONGLONG m_llWriteSize;
	LONGLONG m_llWriteCount;

	bool m_bWriteError;
	bool m_bPause;
};
