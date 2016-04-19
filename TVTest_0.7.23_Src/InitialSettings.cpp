#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "InitialSettings.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Aero.h"
#include "Help.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CInitialSettings::CInitialSettings(const CDriverManager *pDriverManager)
	: m_pDriverManager(pDriverManager)
	, m_VideoRenderer(CVideoRenderer::RENDERER_DEFAULT)
	, m_CardReader(
#ifndef TVH264_FOR_1SEG
		CCoreEngine::CARDREADER_SCARD
#else
		CCoreEngine::CARDREADER_NONE
#endif
		)
{
	m_szDriverFileName[0]='\0';
	m_szMpeg2DecoderName[0]='\0';
#if 0
	// Vista�ł̓r�f�I�����_���̃f�t�H���g��EVR�ɂ���
	// ...�Ɩ�肪�o���������݂���
	{
		OSVERSIONINFO osvi;

		osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion>=6)
			m_VideoRenderer=CVideoRenderer::RENDERER_EVR;
	}
#endif
	if (!::SHGetSpecialFolderPath(NULL,m_szRecordFolder,CSIDL_MYVIDEO,FALSE)
			&& !::SHGetSpecialFolderPath(NULL,m_szRecordFolder,CSIDL_PERSONAL,FALSE))
		m_szRecordFolder[0]='\0';
}


CInitialSettings::~CInitialSettings()
{
	Destroy();
}


bool CInitialSettings::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_INITIALSETTINGS))==IDOK;
}


bool CInitialSettings::GetDriverFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (::lstrlen(m_szDriverFileName)>=MaxLength)
		return false;
	::lstrcpy(pszFileName,m_szDriverFileName);
	return true;
}


bool CInitialSettings::GetMpeg2DecoderName(LPTSTR pszDecoderName,int MaxLength) const
{
	if (::lstrlen(m_szMpeg2DecoderName)>=MaxLength)
		return false;
	::lstrcpy(pszDecoderName,m_szMpeg2DecoderName);
	return true;
}


INT_PTR CInitialSettings::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			{
				HWND hwndLogo=::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO);
				RECT rc;

				::GetWindowRect(hwndLogo,&rc);
				::SetRect(&rc,rc.right-rc.left,0,0,0);
				if (m_AeroGlass.ApplyAeroGlass(hDlg,&rc)) {
					m_GdiPlus.Initialize();
					m_LogoImage.LoadFromResource(GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_LOGO32),TEXT("PNG"));
					::ShowWindow(hwndLogo,SW_HIDE);
				} else {
					HBITMAP hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),
											 MAKEINTRESOURCE(IDB_LOGO));
					::SendMessage(hwndLogo,STM_SETIMAGE,
								  IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
				}
			}

			// BonDriver
			{
				int NormalDriverCount=0;

				DlgComboBox_LimitText(hDlg,IDC_INITIALSETTINGS_DRIVER,MAX_PATH-1);
				for (int i=0;i<m_pDriverManager->NumDrivers();i++) {
					const CDriverInfo *pDriverInfo=m_pDriverManager->GetDriverInfo(i);
					int Index;

					if (CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName())) {
						Index=i;
					} else {
						Index=NormalDriverCount++;
					}
					DlgComboBox_InsertString(hDlg,IDC_INITIALSETTINGS_DRIVER,
											 Index,pDriverInfo->GetFileName());
				}
				if (m_pDriverManager->NumDrivers()>0) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_DRIVER,
											0,m_szDriverFileName);
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,m_szDriverFileName);
				}
			}

			// MPEG-2 or H.264 decoder
			{
				CDirectShowFilterFinder FilterFinder;
				WCHAR szFilterName[MAX_DECODER_NAME];
				int Sel=0,Count=0;

				if (FilterFinder.FindFilter(&MEDIATYPE_Video,
#ifndef TVH264
											&MEDIASUBTYPE_MPEG2_VIDEO
#else
											&MEDIASUBTYPE_H264
#endif
											)) {
					for (int i=0;i<FilterFinder.GetFilterCount();i++) {
						if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
							int Index=(int)DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,szFilterName);
							if (::lstrcmpi(szFilterName,m_szMpeg2DecoderName)==0)
								Sel=Index;
							Count++;
						}
					}
				}
				DlgComboBox_InsertString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,
					0,Count>0?TEXT("�f�t�H���g"):TEXT("<�f�R�[�_�����t����܂���>"));
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,Sel);
			}

			// Video renderer
			{
				LPCTSTR pszName;

				DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,TEXT("�f�t�H���g"));
				for (int i=1;(pszName=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
					DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,pszName);
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,
									  m_VideoRenderer);
			}

			// �J�[�h���[�_�[
			{
				CCardReader *pCardReader;

				for (int i=0;i<=CCoreEngine::CARDREADER_LAST;i++)
					DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_CARDREADER,
										  CCoreEngine::GetCardReaderSettingName((CCoreEngine::CardReaderType)i));
				m_CardReader=CCoreEngine::CARDREADER_NONE;
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_SCARD);
				if (pCardReader!=NULL) {
					if (pCardReader->NumReaders()>0)
						m_CardReader=CCoreEngine::CARDREADER_SCARD;
					delete pCardReader;
				}
				if (m_CardReader==CCoreEngine::CARDREADER_NONE) {
					pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
					if (pCardReader!=NULL) {
						if (pCardReader->Open()) {
							m_CardReader=CCoreEngine::CARDREADER_HDUS;
							pCardReader->Close();
						}
						delete pCardReader;
					}
				}
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_CARDREADER,m_CardReader);
			}

			// �^��t�H���_
			::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,m_szRecordFolder);
			::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);

			AdjustDialogPos(NULL,hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_INITIALSETTINGS_DRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("���ׂẴt�@�C��\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriver�̑I��");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,szFileName);
				}
			}
			return TRUE;

		case IDC_INITIALSETTINGS_SEARCHCARDREADER:
			{
				CCoreEngine::CardReaderType ReaderType=CCoreEngine::CARDREADER_NONE;
				CCardReader *pCardReader;
				TCHAR szText[1024];
				int Length;

				::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				Length=StdUtil::snprintf(szText,lengthof(szText),
										 TEXT("�ȉ��̃J�[�h���[�_�����t����܂����B\n"));
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_SCARD);
				if (pCardReader!=NULL) {
					if (pCardReader->NumReaders()>0) {
						for (int i=0;i<pCardReader->NumReaders();i++) {
							LPCTSTR pszReaderName=pCardReader->EnumReader(i);
							if (pszReaderName!=NULL) {
								Length+=StdUtil::snprintf(
									szText+Length,lengthof(szText)-Length,
									TEXT("\"%s\"\n"),pszReaderName);
							}
						}
						ReaderType=CCoreEngine::CARDREADER_SCARD;
					}
					delete pCardReader;
				}
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
				if (pCardReader!=NULL) {
					if (pCardReader->Open()) {
						/*Length+=*/StdUtil::snprintf(
							szText+Length,lengthof(szText)-Length,
							TEXT("\"HDUS�����J�[�h���[�_\"\n"));
						pCardReader->Close();
						if (ReaderType==CCoreEngine::CARDREADER_NONE)
							ReaderType=CCoreEngine::CARDREADER_HDUS;
					}
					delete pCardReader;
				}
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
				if (ReaderType==CCoreEngine::CARDREADER_NONE) {
					::MessageBox(hDlg,TEXT("�J�[�h���[�_�͌��t����܂���ł����B"),TEXT("��������"),MB_OK | MB_ICONINFORMATION);
				} else {
					DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_CARDREADER,(WPARAM)ReaderType);
					::MessageBox(hDlg,szText,TEXT("��������"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;

		case IDC_INITIALSETTINGS_RECORDFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,szFolder,lengthof(szFolder));
				if (BrowseFolderDialog(hDlg,szFolder,
										TEXT("�^��t�@�C���̕ۑ���t�H���_:")))
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,szFolder);
			}
			return TRUE;

		case IDC_INITIALSETTINGS_HELP:
			GetAppClass().ShowHelpContent(HELP_ID_INITIALSETTINGS);
			return TRUE;

		case IDOK:
			{
				TCHAR szMpeg2Decoder[MAX_DECODER_NAME];
				CVideoRenderer::RendererType VideoRenderer;

				int SelDecoder=(int)DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER);
				if (SelDecoder>0) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,SelDecoder,szMpeg2Decoder);
				} else if (DlgComboBox_GetCount(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER)>1) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,1,szMpeg2Decoder);
				} else {
					::MessageBox(hDlg,
						TEXT("�f�R�[�_�����t����Ȃ����߁A�Đ����s�����Ƃ��ł��܂���B\n")
						TEXT("�f�����Đ�����ɂ̓f�R�[�_���C���X�g�[�����Ă��������B"),
						TEXT("���m�点"),
						MB_OK | MB_ICONINFORMATION);
					szMpeg2Decoder[0]='\0';
				}
				VideoRenderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER);
#ifndef TVH264
				// �����̈����g�ݍ��킹�ɑ΂��Ē��ӂ�\������
				static const struct {
					LPCTSTR pszDecoder;
					CVideoRenderer::RendererType Renderer;
					LPCTSTR pszMessage;
				} ConflictList[] = {
					{TEXT("CyberLink"),	CVideoRenderer::RENDERER_DEFAULT,
						TEXT("CyberLink �̃f�R�[�_�ƃf�t�H���g�����_���̑g�ݍ��킹�ŁA\n")
						TEXT("�ꕔ�̔ԑg�Ŕ䗦�����������Ȃ錻�ۂ��o�鎖�����邽�߁A\n")
						TEXT("�����_�����f�t�H���g�ȊO�ɂ��邱�Ƃ������߂��܂��B\n")
						TEXT("���݂̐ݒ��ύX���܂���?")},
				};
				for (int i=0;i<lengthof(ConflictList);i++) {
					if (::StrCmpNI(szMpeg2Decoder,ConflictList[i].pszDecoder,::lstrlen(ConflictList[i].pszDecoder))==0
							&& VideoRenderer==ConflictList[i].Renderer) {

						if (::MessageBox(hDlg,ConflictList[i].pszMessage,TEXT("����"),
										 MB_YESNO | MB_ICONINFORMATION)==IDYES)
							return TRUE;
						break;
					}
				}
#endif
				if (!CVideoRenderer::IsAvailable(VideoRenderer)) {
					::MessageBox(hDlg,TEXT("�I�����ꂽ�����_���͂��̊��ŗ��p�\�ɂȂ��Ă��܂���B"),
								 NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,
								 m_szDriverFileName,MAX_PATH);
				if (SelDecoder>0)
					::lstrcpy(m_szMpeg2DecoderName,szMpeg2Decoder);
				else
					m_szMpeg2DecoderName[0]='\0';
				m_VideoRenderer=VideoRenderer;
				m_CardReader=(CCoreEngine::CardReaderType)
					DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_CARDREADER);

				TCHAR szRecordFolder[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,
								 szRecordFolder,lengthof(szRecordFolder));
				if (szRecordFolder[0]!='\0'
						&& !::PathIsDirectory(szRecordFolder)) {
					TCHAR szMessage[MAX_PATH+64];

					StdUtil::snprintf(szMessage,lengthof(szMessage),
						TEXT("�^��t�@�C���̕ۑ���t�H���_ \"%s\" ������܂���B\n")
						TEXT("�쐬���܂���?"),szRecordFolder);
					if (::MessageBox(hDlg,szMessage,TEXT("�t�H���_�쐬�̊m�F"),
									 MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szRecordFolder,NULL);
						if (Result!=ERROR_SUCCESS
								&& Result!=ERROR_ALREADY_EXISTS) {
							::MessageBox(hDlg,TEXT("�t�H���_���쐬�ł��܂���B"),
										 NULL,MB_OK | MB_ICONEXCLAMATION);
							return TRUE;
						}
					}
				}
				::lstrcpy(m_szRecordFolder,szRecordFolder);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_PAINT:
		if (m_GdiPlus.IsInitialized()) {
			PAINTSTRUCT ps;

			::BeginPaint(hDlg,&ps);
			{
				CGdiPlus::CCanvas Canvas(ps.hdc);
				CGdiPlus::CBrush Brush(::GetSysColor(COLOR_3DFACE));
				RECT rc,rcClient;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO),&rc);
				::OffsetRect(&rc,-rc.left,-rc.top);
				Canvas.Clear(0,0,0,0);
				::GetClientRect(hDlg,&rcClient);
				rcClient.left=rc.right;
				m_GdiPlus.FillRect(&Canvas,&Brush,&rcClient);
				m_GdiPlus.DrawImage(&Canvas,&m_LogoImage,
					(rc.right-m_LogoImage.GetWidth())/2,
					(rc.bottom-m_LogoImage.GetHeight())/2);
			}
			::EndPaint(hDlg,&ps);
			return TRUE;
		}
		break;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO))
			return reinterpret_cast<INT_PTR>(::GetStockObject(WHITE_BRUSH));
		break;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
				m_GdiPlus.Finalize();
			}
		}
		return TRUE;
	}
	return FALSE;
}
