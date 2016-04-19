// MediaGrabber.h: CMediaGrabber �N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"


/////////////////////////////////////////////////////////////////////////////
// �T���v���O���o(�O���t��ʉ߂���CMediaData�ɃA�N�Z�X�����i��񋟂���)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		���̓f�[�^
// Output	#0	: CMediaData		�o�̓f�[�^
/////////////////////////////////////////////////////////////////////////////

class CMediaGrabber : public CMediaDecoder  
{
public:
	typedef bool (CALLBACK * MEDIAGRABFUNC)(const CMediaData *pMediaData, const PVOID pParam);	// ���f�B�A�󂯎��R�[���o�b�N�^
	typedef void (CALLBACK * RESETGRABFUNC)(const PVOID pParam);								// ���Z�b�g�󂯎��R�[���o�b�N�^

	CMediaGrabber(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaGrabber();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaGrabber
	void SetMediaGrabCallback(const MEDIAGRABFUNC pCallback, const PVOID pParam = NULL);
	void SetResetGrabCallback(const RESETGRABFUNC pCallback, const PVOID pParam = NULL);

protected:
	MEDIAGRABFUNC m_pfnMediaGrabFunc;
	RESETGRABFUNC m_pfnResetGrabFunc;

	PVOID m_pMediaGrabParam;
	PVOID m_pResetGrabParam;
};
