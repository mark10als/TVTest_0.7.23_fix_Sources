#include "stdafx.h"
#include "BonTsEngine/TsDescrambler.h"
#include "BonTsEngine/Multi2Decoder.h"
#include "TVTest.h"
#include "AppMain.h"
#include "GeneralOptions.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CGeneralOptions::CGeneralOptions()
	: m_DefaultDriverType(DEFAULT_DRIVER_LAST)
	, m_VideoRendererType(CVideoRenderer::RENDERER_DEFAULT)
	, m_CardReaderType(CCoreEngine::CARDREADER_SCARD)
	, m_fTemporaryNoDescramble(false)
	, m_fResident(false)
	, m_fKeepSingleTask(false)
	, m_DescrambleInstruction(
		CTsDescrambler::IsSSSE3Available()?CTsDescrambler::INSTRUCTION_SSSE3:
		CTsDescrambler::IsSSE2Available()?CTsDescrambler::INSTRUCTION_SSE2:
		CTsDescrambler::INSTRUCTION_NORMAL)
	, m_fDescrambleCurServiceOnly(false)
	, m_fEnableEmmProcess(false)
{
	m_szDriverDirectory[0]='\0';
	m_szDefaultDriverName[0]='\0';
	m_szLastDriverName[0]='\0';
}


CGeneralOptions::~CGeneralOptions()
{
	Destroy();
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();
	CCoreEngine *pCoreEngine=AppMain.GetCoreEngine();

	if ((Flags&UPDATE_CARDREADER)!=0) {
		if (!pCoreEngine->SetCardReaderType(m_CardReaderType)) {
			AppMain.AddLog(pCoreEngine->GetLastErrorText());
			AppMain.GetUICore()->GetSkin()->ShowErrorMessage(pCoreEngine);
		}
	}

	if ((Flags&UPDATE_RESIDENT)!=0) {
		AppMain.GetUICore()->SetResident(m_fResident);
	}

	if ((Flags&UPDATE_DESCRAMBLECURONLY)!=0) {
		if (!AppMain.GetRecordManager()->IsRecording())
			pCoreEngine->m_DtvEngine.SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	}

	if ((Flags&UPDATE_ENABLEEMMPROCESS)!=0) {
		pCoreEngine->m_DtvEngine.m_TsDescrambler.EnableEmmProcess(m_fEnableEmmProcess);
	}

	return true;
}


bool CGeneralOptions::ReadSettings(CSettings &Settings)
{
	int Value;

	TCHAR szDirectory[MAX_PATH];
	if (Settings.Read(TEXT("DriverDirectory"),szDirectory,lengthof(szDirectory))
			&& szDirectory[0]!='\0') {
		::lstrcpy(m_szDriverDirectory,szDirectory);
		GetAppClass().GetCoreEngine()->SetDriverDirectory(szDirectory);
	}
	if (Settings.Read(TEXT("DefaultDriverType"),&Value)
			&& Value>=DEFAULT_DRIVER_NONE && Value<=DEFAULT_DRIVER_CUSTOM)
		m_DefaultDriverType=(DefaultDriverType)Value;
	Settings.Read(TEXT("DefaultDriver"),
				  m_szDefaultDriverName,lengthof(m_szDefaultDriverName));
	Settings.Read(TEXT("Driver"),
				  m_szLastDriverName,lengthof(m_szLastDriverName));
	TCHAR szDecoder[MAX_MPEG2_DECODER_NAME];
	if (Settings.Read(TEXT("Mpeg2Decoder"),szDecoder,lengthof(szDecoder)))
		m_Mpeg2DecoderName.Set(szDecoder);
	TCHAR szRenderer[16];
	if (Settings.Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
		if (szRenderer[0]=='\0') {
			m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		} else {
			CVideoRenderer::RendererType Renderer=CVideoRenderer::ParseName(szRenderer);
			if (Renderer!=CVideoRenderer::RENDERER_UNDEFINED)
				m_VideoRendererType=Renderer;
		}
	}
	if (Settings.Read(TEXT("CardReader"),&Value)) {
		if (Value>=CCoreEngine::CARDREADER_NONE && Value<=CCoreEngine::CARDREADER_LAST)
			m_CardReaderType=(CCoreEngine::CardReaderType)Value;
	} else {
		// 以前のバージョンとの互換用
		bool fNoDescramble;
		if (Settings.Read(TEXT("NoDescramble"),&fNoDescramble) && fNoDescramble)
			m_CardReaderType=CCoreEngine::CARDREADER_NONE;
	}

	bool f;
	if (Settings.Read(TEXT("DescrambleSSSE3"),&f) && f && CTsDescrambler::IsSSSE3Available()) {
		m_DescrambleInstruction=CTsDescrambler::INSTRUCTION_SSSE3;
	} else if (Settings.Read(TEXT("DescrambleSSE2"),&f)) {
		if (f && CTsDescrambler::IsSSE2Available())
			m_DescrambleInstruction=CTsDescrambler::INSTRUCTION_SSE2;
		else
			m_DescrambleInstruction=CTsDescrambler::INSTRUCTION_NORMAL;
	}

	Settings.Read(TEXT("DescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	Settings.Read(TEXT("ProcessEMM"),&m_fEnableEmmProcess);

	Settings.Read(TEXT("Resident"),&m_fResident);
	Settings.Read(TEXT("KeepSingleTask"),&m_fKeepSingleTask);
	return true;
}


bool CGeneralOptions::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("DriverDirectory"),m_szDriverDirectory);
	Settings.Write(TEXT("DefaultDriverType"),(int)m_DefaultDriverType);
	Settings.Write(TEXT("DefaultDriver"),m_szDefaultDriverName);
	Settings.Write(TEXT("Driver"),GetAppClass().GetCoreEngine()->GetDriverFileName());
	Settings.Write(TEXT("Mpeg2Decoder"),m_Mpeg2DecoderName.GetSafe());
	Settings.Write(TEXT("Renderer"),
				   CVideoRenderer::EnumRendererName((int)m_VideoRendererType));
	//Settings.Write(TEXT("NoDescramble"),m_CardReaderType==CCoreEngine::CARDREADER_NONE);
	Settings.Write(TEXT("CardReader"),(int)m_CardReaderType);
	Settings.Write(TEXT("DescrambleSSE2"),m_DescrambleInstruction!=CTsDescrambler::INSTRUCTION_NORMAL);
	Settings.Write(TEXT("DescrambleSSSE3"),m_DescrambleInstruction==CTsDescrambler::INSTRUCTION_SSSE3);
	Settings.Write(TEXT("DescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	Settings.Write(TEXT("ProcessEMM"),m_fEnableEmmProcess);
	Settings.Write(TEXT("Resident"),m_fResident);
	Settings.Write(TEXT("KeepSingleTask"),m_fKeepSingleTask);
	return true;
}


bool CGeneralOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_GENERAL));
}


CGeneralOptions::DefaultDriverType CGeneralOptions::GetDefaultDriverType() const
{
	return m_DefaultDriverType;
}


LPCTSTR CGeneralOptions::GetDefaultDriverName() const
{
	return m_szDefaultDriverName;
}


bool CGeneralOptions::SetDefaultDriverName(LPCTSTR pszDriverName)
{
	if (pszDriverName==NULL)
		m_szDefaultDriverName[0]='\0';
	else
		::lstrcpy(m_szDefaultDriverName,pszDriverName);
	return true;
}


bool CGeneralOptions::GetFirstDriverName(LPTSTR pszDriverName) const
{
	switch (m_DefaultDriverType) {
	case DEFAULT_DRIVER_NONE:
		pszDriverName[0]='\0';
		break;
	case DEFAULT_DRIVER_LAST:
		::lstrcpy(pszDriverName,m_szLastDriverName);
		break;
	case DEFAULT_DRIVER_CUSTOM:
		::lstrcpy(pszDriverName,m_szDefaultDriverName);
		break;
	default:
		return false;
	}
	return true;
}


LPCTSTR CGeneralOptions::GetMpeg2DecoderName() const
{
	return m_Mpeg2DecoderName.Get();
}


bool CGeneralOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	return m_Mpeg2DecoderName.Set(pszDecoderName);
}


CVideoRenderer::RendererType CGeneralOptions::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CGeneralOptions::SetVideoRendererType(CVideoRenderer::RendererType Renderer)
{
	if (CVideoRenderer::EnumRendererName((int)Renderer)==NULL)
		return false;
	m_VideoRendererType=Renderer;
	return true;
}


CCoreEngine::CardReaderType CGeneralOptions::GetCardReaderType() const
{
	return m_CardReaderType;
}


bool CGeneralOptions::SetCardReaderType(CCoreEngine::CardReaderType CardReader)
{
	if (CardReader<CCoreEngine::CARDREADER_NONE || CardReader>CCoreEngine::CARDREADER_LAST)
		return false;
	m_CardReaderType=CardReader;
	return true;
}


void CGeneralOptions::SetTemporaryNoDescramble(bool fNoDescramble)
{
	m_fTemporaryNoDescramble=fNoDescramble;
}


bool CGeneralOptions::GetResident() const
{
	return m_fResident;
}


bool CGeneralOptions::GetKeepSingleTask() const
{
	return m_fKeepSingleTask;
}


bool CGeneralOptions::GetDescrambleCurServiceOnly() const
{
	return m_fDescrambleCurServiceOnly;
}


bool CGeneralOptions::GetEnableEmmProcess() const
{
	return m_fEnableEmmProcess;
}


INT_PTR CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &AppMain=GetAppClass();

			::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,m_szDriverDirectory);

			// BonDriver
			::CheckRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
									IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
							   (int)m_DefaultDriverType+IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
						   m_DefaultDriverType==DEFAULT_DRIVER_CUSTOM);

			const CDriverManager *pDriverManager=AppMain.GetDriverManager();
			DlgComboBox_LimitText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,MAX_PATH-1);
			/*
			TCHAR szDirectory[MAX_PATH];
			AppMain.GetDriverDirectory(szDirectory);
			pDriverManager->Find(szDirectory);
			AppMain.UpdateDriverMenu();
			*/
			for (int i=0;i<pDriverManager->NumDrivers();i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									  pDriverManager->GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,m_szDefaultDriverName);

			// MPEG-2 decoder
			CDirectShowFilterFinder FilterFinder;
			int Count=0;
			if (FilterFinder.FindFilter(&MEDIATYPE_Video,
#ifndef TVH264
										&MEDIASUBTYPE_MPEG2_VIDEO
#else
										&MEDIASUBTYPE_H264
#endif
					)) {
				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					WCHAR szFilterName[MAX_MPEG2_DECODER_NAME];

					if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
						DlgComboBox_AddString(hDlg,IDC_OPTIONS_DECODER,szFilterName);
						Count++;
					}
				}
			}
			int Sel=0;
			if (Count==0) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DECODER,TEXT("<デコーダが見付かりません>"));
			} else {
				CMediaViewer &MediaViewer=GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer;
				TCHAR szText[32+MAX_MPEG2_DECODER_NAME];

				::lstrcpy(szText,TEXT("デフォルト"));
				if (!m_Mpeg2DecoderName.IsEmpty()) {
					Sel=(int)DlgComboBox_FindStringExact(hDlg,IDC_OPTIONS_DECODER,-1,
														 m_Mpeg2DecoderName.Get())+1;
				} else if (MediaViewer.IsOpen()) {
					::lstrcat(szText,TEXT(" ("));
					MediaViewer.GetVideoDecoderName(szText+::lstrlen(szText),MAX_MPEG2_DECODER_NAME);
					::lstrcat(szText,TEXT(")"));
				}
				DlgComboBox_InsertString(hDlg,IDC_OPTIONS_DECODER,0,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_DECODER,Sel);

			// Renderer
			LPCTSTR pszRenderer;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,TEXT("デフォルト"));
			for (int i=1;(pszRenderer=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,pszRenderer);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RENDERER,m_VideoRendererType);

			// Card reader
			for (int i=0;i<=CCoreEngine::CARDREADER_LAST;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_CARDREADER,
									  CCoreEngine::GetCardReaderSettingName((CCoreEngine::CardReaderType)i));
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_CARDREADER,
				(int)(m_fTemporaryNoDescramble?CCoreEngine::CARDREADER_NONE:m_CardReaderType));

			DlgComboBox_AddString(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,TEXT("なし"));
			if (CTsDescrambler::IsSSE2Available()) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,TEXT("SSE2"));
				if (CTsDescrambler::IsSSSE3Available())
					DlgComboBox_AddString(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,TEXT("SSSE3"));
				DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,(int)m_DescrambleInstruction);
			} else {
				DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION,0);
			}

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY,m_fDescrambleCurServiceOnly);
			if (m_CardReaderType==CCoreEngine::CARDREADER_NONE)
				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLE_FIRST,
									IDC_OPTIONS_DESCRAMBLE_LAST,false);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS,m_fEnableEmmProcess);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESIDENT,m_fResident);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_KEEPSINGLETASK,m_fKeepSingleTask);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVERDIRECTORY_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH];

				if (::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory,lengthof(szDirectory))>0) {
					if (::PathIsRelative(szDirectory)) {
						TCHAR szTemp[MAX_PATH];

						GetAppClass().GetAppDirectory(szTemp);
						::PathAppend(szTemp,szDirectory);
						::PathCanonicalize(szDirectory,szTemp);
					}
				} else {
					GetAppClass().GetAppDirectory(szDirectory);
				}
				if (BrowseFolderDialog(hDlg,szDirectory,TEXT("ドライバの検索フォルダを選択してください。")))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory);
			}
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_NONE:
		case IDC_OPTIONS_DEFAULTDRIVER_LAST:
		case IDC_OPTIONS_DEFAULTDRIVER_CUSTOM:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									   IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
									   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM);
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName,lengthof(szFileName));
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
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriverの選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName);
			}
			return TRUE;

		case IDC_OPTIONS_CARDREADER:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				bool fBcas=DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER)>0;

				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLE_FIRST,
									IDC_OPTIONS_DESCRAMBLE_LAST,fBcas);
			}
			return TRUE;

		case IDC_OPTIONS_DESCRAMBLEBENCHMARK:
			if (::MessageBox(hDlg,
					TEXT("ベンチマークテストを開始します。\n")
					TEXT("終了するまで操作は行わないようにしてください。\n")
					TEXT("結果はばらつきがありますので、数回実行してください。"),
					TEXT("ベンチマークテスト"),
					MB_OKCANCEL | MB_ICONINFORMATION)==IDOK) {
				DescrambleBenchmarkTest(hDlg);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RENDERER);
				if (Renderer!=m_VideoRendererType) {
					if (!CVideoRenderer::IsAvailable(Renderer)) {
						SettingError();
						::MessageBox(hDlg,TEXT("選択されたレンダラはこの環境で利用可能になっていません。"),
									 NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					m_VideoRendererType=Renderer;
					SetUpdateFlag(UPDATE_RENDERER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,
								 m_szDriverDirectory,lengthof(m_szDriverDirectory));
				m_DefaultDriverType=(DefaultDriverType)
					(GetCheckedRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
										   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM)-
					IDC_OPTIONS_DEFAULTDRIVER_NONE);
				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								 m_szDefaultDriverName,lengthof(m_szDefaultDriverName));

				TCHAR szDecoder[MAX_MPEG2_DECODER_NAME];
				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_DECODER);
				if (Sel>0)
					DlgComboBox_GetLBString(hDlg,IDC_OPTIONS_DECODER,Sel,szDecoder);
				else
					szDecoder[0]='\0';
				if (::lstrcmpi(szDecoder,m_Mpeg2DecoderName.GetSafe())!=0) {
					m_Mpeg2DecoderName.Set(szDecoder);
					SetUpdateFlag(UPDATE_DECODER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				CCoreEngine::CardReaderType CardReader=(CCoreEngine::CardReaderType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER);
				if ((m_fTemporaryNoDescramble && CardReader!=CCoreEngine::CARDREADER_NONE)
						|| (!m_fTemporaryNoDescramble && CardReader!=m_CardReaderType)) {
					m_CardReaderType=CardReader;
					m_fTemporaryNoDescramble=false;
					SetUpdateFlag(UPDATE_CARDREADER);
				}

				Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_DESCRAMBLEINSTRUCTION);
				if (Sel>=0)
					m_DescrambleInstruction=(CTsDescrambler::InstructionType)Sel;

				bool fCurOnly=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY);
				if (fCurOnly!=m_fDescrambleCurServiceOnly) {
					m_fDescrambleCurServiceOnly=fCurOnly;
					SetUpdateFlag(UPDATE_DESCRAMBLECURONLY);
				}

				bool fEmm=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS);
				if (fEmm!=m_fEnableEmmProcess) {
					m_fEnableEmmProcess=fEmm;
					SetUpdateFlag(UPDATE_ENABLEEMMPROCESS);
				}

				bool fResident=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESIDENT);
				if (fResident!=m_fResident) {
					m_fResident=fResident;
					SetUpdateFlag(UPDATE_RESIDENT);
				}

				m_fKeepSingleTask=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_KEEPSINGLETASK);

				m_fChanged=true;
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


static void FillRandomData(BYTE *pData,size_t Size)
{
	for (size_t i=0;i<Size;i++)
		pData[i]=::rand()&0xFF;
}

// ベンチマークテスト
void CGeneralOptions::DescrambleBenchmarkTest(HWND hwndOwner)
{
	static const DWORD BENCHMARK_ROUND=200000;
	static const DWORD PACKETS_PER_SECOND=10000;
	HCURSOR hcurOld=::SetCursor(LoadCursor(NULL,IDC_WAIT));

	BYTE SystemKey[32],InitialCbc[8],ScrambleKey[4][16];
	static const DWORD PACKET_DATA_SIZE=184;
	BYTE PacketData[PACKET_DATA_SIZE];

	FillRandomData(SystemKey,sizeof(SystemKey));
	FillRandomData(InitialCbc,sizeof(InitialCbc));
	FillRandomData(PacketData,sizeof(PacketData));
	FillRandomData(&ScrambleKey[0][0],sizeof(ScrambleKey));

	CMulti2Decoder Multi2DecoderNormal(
#ifdef MULTI2_SIMD
		CMulti2Decoder::INSTRUCTION_NORMAL
#endif
		);
	Multi2DecoderNormal.Initialize(SystemKey,InitialCbc);
#ifdef MULTI2_SSE2
	CMulti2Decoder Multi2DecoderSSE2(CMulti2Decoder::INSTRUCTION_SSE2);
	__declspec(align(16)) BYTE SSE2PacketData[192];
	bool fSSE2=CMulti2Decoder::IsSSE2Available();
	if (fSSE2) {
		Multi2DecoderSSE2.Initialize(SystemKey,InitialCbc);
		::CopyMemory(SSE2PacketData,PacketData,PACKET_DATA_SIZE);
	}
#endif
#ifdef MULTI2_SSSE3
	CMulti2Decoder Multi2DecoderSSSE3(CMulti2Decoder::INSTRUCTION_SSSE3);
	__declspec(align(16)) BYTE SSSE3PacketData[192];
	bool fSSSE3=CMulti2Decoder::IsSSSE3Available();
	if (fSSSE3) {
		Multi2DecoderSSSE3.Initialize(SystemKey,InitialCbc);
		::CopyMemory(SSSE3PacketData,PacketData,PACKET_DATA_SIZE);
	}
#endif

	DWORD BenchmarkCount=0,NormalTime=0;
#ifdef MULTI2_SSE2
	DWORD SSE2Time=0;
#endif
#ifdef MULTI2_SSSE3
	DWORD SSSE3Time=0;
#endif

	do {
		BYTE ScramblingCtrl=2;
		DWORD StartTime=::timeGetTime();
		for (int i=0;i<BENCHMARK_ROUND;i++) {
			if (i%PACKETS_PER_SECOND==0) {
				Multi2DecoderNormal.SetScrambleKey(
					ScrambleKey[(i/PACKETS_PER_SECOND)%lengthof(ScrambleKey)]);
				ScramblingCtrl=ScramblingCtrl==2?3:2;
			}
			Multi2DecoderNormal.Decode(PacketData,PACKET_DATA_SIZE,ScramblingCtrl);
		}
		NormalTime+=TickTimeSpan(StartTime,::timeGetTime());

#ifdef MULTI2_SSE2
		if (fSSE2) {
			ScramblingCtrl=2;
			StartTime=::timeGetTime();
			for (int i=0;i<BENCHMARK_ROUND;i++) {
				if (i%PACKETS_PER_SECOND==0) {
					Multi2DecoderSSE2.SetScrambleKey(
						ScrambleKey[(i/PACKETS_PER_SECOND)%lengthof(ScrambleKey)]);
					ScramblingCtrl=ScramblingCtrl==2?3:2;
				}
				Multi2DecoderSSE2.Decode(SSE2PacketData,PACKET_DATA_SIZE,ScramblingCtrl);
			}
			SSE2Time+=TickTimeSpan(StartTime,::timeGetTime());

			// 計算結果のチェック
			if (::memcmp(PacketData,SSE2PacketData,PACKET_DATA_SIZE)!=0) {
				::SetCursor(hcurOld);
				::MessageBox(hwndOwner,TEXT("SSE2での計算結果が不正です。"),NULL,MB_OK | MB_ICONSTOP);
				return;
			}
		}
#endif

#ifdef MULTI2_SSSE3
		if (fSSSE3) {
			ScramblingCtrl=2;
			StartTime=::timeGetTime();
			for (int i=0;i<BENCHMARK_ROUND;i++) {
				if (i%PACKETS_PER_SECOND==0) {
					Multi2DecoderSSSE3.SetScrambleKey(
						ScrambleKey[(i/PACKETS_PER_SECOND)%lengthof(ScrambleKey)]);
					ScramblingCtrl=ScramblingCtrl==2?3:2;
				}
				Multi2DecoderSSSE3.Decode(SSSE3PacketData,PACKET_DATA_SIZE,ScramblingCtrl);
			}
			SSSE3Time+=TickTimeSpan(StartTime,::timeGetTime());

			// 計算結果のチェック
			if (::memcmp(PacketData,SSSE3PacketData,PACKET_DATA_SIZE)!=0) {
				::SetCursor(hcurOld);
				::MessageBox(hwndOwner,TEXT("SSSE3での計算結果が不正です。"),NULL,MB_OK | MB_ICONSTOP);
				return;
			}
		}
#endif

		BenchmarkCount+=BENCHMARK_ROUND;
	} while (NormalTime<1500);

	::SetCursor(hcurOld);
	TCHAR szText[1024];
	CStaticStringFormatter Formatter(szText,lengthof(szText));
	Formatter.AppendFormat(
		TEXT("%u 回の実行に掛かった時間\n")
		TEXT("拡張命令なし : %u ms (%d パケット/秒)\n"),
		BenchmarkCount,
		NormalTime,::MulDiv(BenchmarkCount,1000,NormalTime));
#ifdef MULTI2_SSE2
	int SSE2Percentage;
	if (fSSE2) {
		if (NormalTime>=SSE2Time)
			SSE2Percentage=(int)(NormalTime*100/SSE2Time)-100;
		else
			SSE2Percentage=-(int)((SSE2Time*100/NormalTime)-100);
		Formatter.AppendFormat(
			TEXT("SSE2 : %u ms (%d パケット/秒) (高速化される割合 %d %%)\n"),
			SSE2Time,::MulDiv(BenchmarkCount,1000,SSE2Time),SSE2Percentage);
	}
#endif
#ifdef MULTI2_SSSE3
	int SSSE3Percentage;
	if (fSSSE3) {
		if (NormalTime>=SSSE3Time)
			SSSE3Percentage=(int)(NormalTime*100/SSSE3Time)-100;
		else
			SSSE3Percentage=-(int)((SSSE3Time*100/NormalTime)-100);
		Formatter.AppendFormat(
			TEXT("SSSE3 : %u ms (%d パケット/秒) (高速化される割合 %d %%)\n"),
			SSSE3Time,::MulDiv(BenchmarkCount,1000,SSSE3Time),SSSE3Percentage);
	}
#endif

#if defined(MULTI2_SSE2) || defined(MULTI2_SSSE3)
	// 表示されるメッセージに深い意味はない
	Formatter.AppendFormat(
		TEXT("\n%s"),
#ifdef MULTI2_SSSE3
		fSSSE3 && SSSE3Percentage>=10
#ifdef MULTI2_SSE2
			&& SSSE3Time<SSE2Time
#endif
			?
			TEXT("SSSE3にすることをお勧めします。"):
#endif
#ifdef MULTI2_SSE2
		fSSE2 && SSE2Percentage>=10?
			TEXT("SSE2にすることをお勧めします。"):
		fSSE2 && SSE2Percentage>=5?
			TEXT("微妙…"):
#endif
		TEXT("<なし> にすることをお勧めします。")
		);
#endif

	CMessageDialog MessageDialog;
	MessageDialog.Show(hwndOwner,CMessageDialog::TYPE_INFO,Formatter.GetString(),
					   TEXT("ベンチマークテスト結果"),NULL,TEXT("ベンチマークテスト"));
}
