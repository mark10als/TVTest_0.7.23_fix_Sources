/*
	TVTest �v���O�C���T���v��

	�ȈՃX�y�N�g�����E�A�i���C�U�[

	fft4g.c �� http://www.kurims.kyoto-u.ac.jp/~ooura/fft-j.html �Ŕz�z�����
	������̂ŁA��Y��Ǝ��ɒ��쌠������܂��B
	Copyright Takuya OOURA, 1996-2001
*/


#include <windows.h>
#include <math.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT	// �N���X�Ƃ��Ď���
#include "TVTestPlugin.h"


#define SAFE_DELETE_ARRAY(p) if (p) { delete [] p; p=NULL; }


// fft4g.c
extern "C" void rdft(int n, int isgn, double *a, int *ip, double *w);

// FFT �������s���T���v����(2�̗ݏ�ɂ���)
#define FFT_SIZE 4096

#define FREQ_UNIT ((48000.0/2.0)/(double)(FFT_SIZE/2+1))

#define POW2(v) pow(v,2.0)


// �E�B���h�E�N���X��
#define SPECTRUM_ANALYZER_WINDOW_CLASS TEXT("TVTest Spectrum Analyzer Window")




// �v���O�C���N���X
class CSpectrumAnalyzer : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	struct Position {
		int Left,Top,Width,Height;
		Position(int x,int y,int w,int h) : Left(x),Top(y),Width(w),Height(h) {}
	};
	Position m_WindowPosition;
	COLORREF m_crBackColor;
	COLORREF m_crGridColor;
	COLORREF m_crSpectrumColor;

	short *m_pBuffer;
	double *m_pFFTBuffer;
	int *m_pFFTWorkBuffer;
	double *m_pFFTSinTable;
	DWORD m_BufferUsed;
	DWORD m_BufferPos;
	double *m_pPower;

	class CCriticalLock
	{
		CRITICAL_SECTION m_CriticalSection;

	public:
		CCriticalLock() {
			::InitializeCriticalSection(&m_CriticalSection);
		}
		virtual ~CCriticalLock() {
			::DeleteCriticalSection(&m_CriticalSection);
		}
		void Lock() {
			::EnterCriticalSection(&m_CriticalSection);
		}
		void Unlock() {
			::LeaveCriticalSection(&m_CriticalSection);
		}
	};

	CCriticalLock m_FFTLock;

	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static LRESULT CALLBACK AudioCallback(short *pData,DWORD Samples,int Channels,void *pClientData);
	static CSpectrumAnalyzer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CSpectrumAnalyzer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CSpectrumAnalyzer::CSpectrumAnalyzer()
	: m_hwnd(NULL)
	, m_WindowPosition(100,100,200,200)
	, m_crBackColor(RGB(0,0,0))
	, m_crGridColor(RGB(0,128,0))
	, m_crSpectrumColor(RGB(0,224,0))
	, m_pBuffer(NULL)
	, m_pFFTBuffer(NULL)
	, m_pFFTWorkBuffer(NULL)
	, m_pFFTSinTable(NULL)
	, m_BufferUsed(0)
	, m_BufferPos(0)
	, m_pPower(NULL)
{
}


bool CSpectrumAnalyzer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// �v���O�C���̏���Ԃ�
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"Spectrum Analyzer";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"�ȈՃX�y�N�g�����E�A�i���C�U�[";
	return true;
}


bool CSpectrumAnalyzer::Initialize()
{
	// ����������

	// �E�B���h�E�N���X�o�^
	WNDCLASS wc;
	wc.style=CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=g_hinstDLL;
	wc.hIcon=NULL;
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.hbrBackground=NULL;
	wc.lpszMenuName=NULL;
	wc.lpszClassName=SPECTRUM_ANALYZER_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	// �C�x���g�R�[���o�b�N�֐���o�^
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


bool CSpectrumAnalyzer::Finalize()
{
	// �I������

	// �E�B���h�E���쐬����Ă�����j������
	if (m_hwnd != NULL)
		::DestroyWindow(m_hwnd);

	return true;
}


// �C�x���g�R�[���o�b�N�֐�
// �����C�x���g���N����ƌĂ΂��
LRESULT CALLBACK CSpectrumAnalyzer::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CSpectrumAnalyzer *pThis=static_cast<CSpectrumAnalyzer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// �v���O�C���̗L����Ԃ��ω�����
		if (lParam1) {
			// �v���O�C�����L���ɂ��ꂽ�̂ŃE�B���h�E���쐬����
			if (pThis->m_hwnd==NULL) {
				if (::CreateWindowEx(WS_EX_TOOLWINDOW,SPECTRUM_ANALYZER_WINDOW_CLASS,
									 TEXT("Spectrum Analyzer"),
									 WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME,
									 pThis->m_WindowPosition.Left,
									 pThis->m_WindowPosition.Top,
									 pThis->m_WindowPosition.Width,
									 pThis->m_WindowPosition.Height,
									pThis->m_pApp->GetAppWindow(),NULL,g_hinstDLL,pThis)==NULL)
					return FALSE;
			}
			::ShowWindow(pThis->m_hwnd,SW_SHOWNORMAL);
			::UpdateWindow(pThis->m_hwnd);
		} else {
			// �v���O�C���������ɂ��ꂽ�̂ŃE�B���h�E��j������
			if (pThis->m_hwnd!=NULL)
				::DestroyWindow(pThis->m_hwnd);
		}
		return TRUE;

	case TVTest::EVENT_STANDBY:
		// �ҋ@��Ԃ��ω�����
		if (pThis->m_pApp->IsPluginEnabled()) {
			// �ҋ@��Ԃ̎��̓E�B���h�E���B��
			::ShowWindow(pThis->m_hwnd,lParam1!=0?SW_HIDE:SW_SHOW);
		}
		return TRUE;
	}
	return 0;
}


// �����R�[���o�b�N�֐�
LRESULT CALLBACK CSpectrumAnalyzer::AudioCallback(short *pData,DWORD Samples,int Channels,void *pClientData)
{
	CSpectrumAnalyzer *pThis=static_cast<CSpectrumAnalyzer*>(pClientData);

	DWORD BufferUsed=pThis->m_BufferUsed;
	DWORD j=pThis->m_BufferPos;
	for (DWORD i=0;i<Samples;i++) {
		if (j==FFT_SIZE)
			j=0;
		pThis->m_pBuffer[j]=pData[i*Channels];
		j++;
		BufferUsed++;

		if (BufferUsed==FFT_SIZE) {
			pThis->m_FFTLock.Lock();
			// double�ɕϊ�
			for (int k=0;k<FFT_SIZE;k++) {
				pThis->m_pFFTBuffer[k]=
					(double)pThis->m_pBuffer[(j+k)%FFT_SIZE]/32768.0;
			}

			// FFT�������s
			rdft(FFT_SIZE,1,pThis->m_pFFTBuffer,
				 pThis->m_pFFTWorkBuffer,pThis->m_pFFTSinTable);
			pThis->m_FFTLock.Unlock();

			BufferUsed=0;
		}
	}

	pThis->m_BufferUsed=BufferUsed;
	pThis->m_BufferPos=j;

	return 0;
}


CSpectrumAnalyzer *CSpectrumAnalyzer::GetThis(HWND hwnd)
{
	// �E�B���h�E�n���h������this���擾
	return reinterpret_cast<CSpectrumAnalyzer*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CSpectrumAnalyzer::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSpectrumAnalyzer *pThis=static_cast<CSpectrumAnalyzer*>(pcs->lpCreateParams);

			// �E�B���h�E�n���h������this���擾�ł���悤�ɂ���
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;

			// �������m��
			pThis->m_pBuffer=new short[FFT_SIZE];
			pThis->m_pFFTBuffer=new double[FFT_SIZE];
			for (int i=0;i<FFT_SIZE;i++)
				pThis->m_pFFTBuffer[i]=0.0;
			pThis->m_pFFTWorkBuffer=new int[2+(int)sqrt((double)(FFT_SIZE/2))+10];
			pThis->m_pFFTWorkBuffer[0]=0;
			pThis->m_pFFTSinTable=new double[FFT_SIZE/2];
			pThis->m_pPower=new double[FFT_SIZE/2+1];

			pThis->m_BufferUsed=0;
			pThis->m_BufferPos=0;

			// �����R�[���o�b�N��o�^
			pThis->m_pApp->SetAudioCallback(AudioCallback,pThis);

			// �\���X�V�p�^�C�}�̐ݒ�
			::SetTimer(hwnd,1,200,NULL);
		}
		return TRUE;

	case WM_PAINT:
		{
			CSpectrumAnalyzer *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HPEN hpen,hpenOld;
			int x,y,i;

			::BeginPaint(hwnd,&ps);

			HBRUSH hbr=::CreateSolidBrush(pThis->m_crBackColor);
			::FillRect(ps.hdc,&ps.rcPaint,hbr);
			::DeleteObject(hbr);

			RECT rc;
			::GetClientRect(hwnd,&rc);

			// �O���b�h��`��
			hpen=::CreatePen(PS_SOLID,1,pThis->m_crGridColor);
			hpenOld=static_cast<HPEN>(::SelectObject(ps.hdc,hpen));
			for (i=1;i<10;i++) {
				y=rc.bottom*i/10;
				::MoveToEx(ps.hdc,ps.rcPaint.left,y,NULL);
				::LineTo(ps.hdc,ps.rcPaint.right,y);
			}

			// db�l�v�Z
			const double EPSILON = 0.0000001;
			const int LEVELS=100;
			const int Width=(int)(12000.0/LEVELS/FREQ_UNIT);
			double dbTable[LEVELS];

			pThis->m_FFTLock.Lock();
			pThis->m_pPower[0]=POW2(pThis->m_pFFTBuffer[0]);
			for (int i=1;i<FFT_SIZE/2;i++) {
				pThis->m_pPower[i]=POW2(pThis->m_pFFTBuffer[i*2])+
								   POW2(pThis->m_pFFTBuffer[i*2+1]);
			}
			pThis->m_pPower[FFT_SIZE/2]=POW2(pThis->m_pFFTBuffer[1]);
			for (int i=0;i<LEVELS;i++) {
				double db=0.0;
				for (int j=0;j<Width;j++) {
					db+=pThis->m_pPower[i*Width+j];
				}
				db/=Width;
				if (db<EPSILON)
					db=EPSILON;
				db=log10(db)*10.0;
				if (db<0.0)
					db=0.0;
				dbTable[i]=db;
			}
			pThis->m_FFTLock.Unlock();

			// ���b�N���鎞�Ԃ��o���邾���Z�����邽�߂ɕ`�揈���͌�񂵂ɂ���

			// �X�y�N�g������`��
			hpen=::CreatePen(PS_SOLID,1,pThis->m_crSpectrumColor);
			::DeleteObject(::SelectObject(ps.hdc,hpen));
			for (int i=0;i<LEVELS;i++) {
				x=rc.right*i/(LEVELS-1);
				y=rc.bottom-1-(int)((double)rc.bottom*dbTable[i]/40.0);
				if (i==0)
					::MoveToEx(ps.hdc,x,y,NULL);
				else
					::LineTo(ps.hdc,x,y);
			}

			::DeleteObject(::SelectObject(ps.hdc,hpenOld));
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		::InvalidateRect(hwnd,NULL,TRUE);
		return 0;

	case WM_SYSCOMMAND:
		if ((wParam&0xFFF0)==SC_CLOSE) {
			// ���鎞�̓v���O�C���𖳌��ɂ���
			CSpectrumAnalyzer *pThis=GetThis(hwnd);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CSpectrumAnalyzer *pThis=GetThis(hwnd);

			// �����R�[���o�b�N��o�^����
			pThis->m_pApp->SetAudioCallback(NULL);

			// �������J��
			SAFE_DELETE_ARRAY(pThis->m_pBuffer);
			SAFE_DELETE_ARRAY(pThis->m_pFFTBuffer);
			SAFE_DELETE_ARRAY(pThis->m_pFFTWorkBuffer);
			SAFE_DELETE_ARRAY(pThis->m_pFFTSinTable);
			SAFE_DELETE_ARRAY(pThis->m_pPower);

			// �E�B���h�E�ʒu�ۑ�
			RECT rc;
			::GetWindowRect(hwnd,&rc);
			pThis->m_WindowPosition.Left=rc.left;
			pThis->m_WindowPosition.Top=rc.top;
			pThis->m_WindowPosition.Width=rc.right-rc.left;
			pThis->m_WindowPosition.Height=rc.bottom-rc.top;

			::KillTimer(hwnd,1);	// �ʂɂ��Ȃ��Ă���������...
			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSpectrumAnalyzer;
}
