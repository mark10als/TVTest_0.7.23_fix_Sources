#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CardReaderDialog.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// サービスAPI用
#pragma comment(lib, "advapi32.lib")


// スマートカードサービスが有効か調べる
enum SCardQueryResult {
	SCARD_QUERY_ENABLED,
	SCARD_QUERY_DISABLED,
	SCARD_QUERY_ERR_SERVICENOTFOUND,
	SCARD_QUERY_ERR_OPENMANAGER,
	SCARD_QUERY_ERR_OPENSERVICE,
	SCARD_QUERY_ERR_QUERYCONFIG
};
static SCardQueryResult IsSmartCardServiceEnabled()
{
	SCardQueryResult Result;

	SC_HANDLE hManager=::OpenSCManager(NULL,NULL,GENERIC_READ);
	if (hManager==NULL)
		return SCARD_QUERY_ERR_OPENMANAGER;

	TCHAR szName[256];
	DWORD Length=lengthof(szName);
	if (!::GetServiceKeyName(hManager,TEXT("Smart Card"),szName,&Length))
		::lstrcpy(szName,TEXT("SCardSvr"));

	SC_HANDLE hService=::OpenService(hManager,szName,SERVICE_QUERY_CONFIG);
	if (hService==NULL) {
		if (::GetLastError()==ERROR_SERVICE_DOES_NOT_EXIST)
			Result=SCARD_QUERY_ERR_SERVICENOTFOUND;
		else
			Result=SCARD_QUERY_ERR_OPENSERVICE;
		::CloseServiceHandle(hManager);
		return Result;
	}

	Result=SCARD_QUERY_ERR_QUERYCONFIG;
	DWORD Size=0;
	::QueryServiceConfig(hService,NULL,0,&Size);
	if (Size>0) {
		BYTE *pBuffer=new BYTE[Size];
		QUERY_SERVICE_CONFIG *pConfig=(QUERY_SERVICE_CONFIG*)pBuffer;

		if (::QueryServiceConfig(hService,pConfig,Size,&Size)) {
			if (pConfig->dwStartType==SERVICE_DISABLED)
				Result=SCARD_QUERY_DISABLED;
			else
				Result=SCARD_QUERY_ENABLED;
		}
		delete [] pBuffer;
	}

	::CloseServiceHandle(hService);
	::CloseServiceHandle(hManager);

	return Result;
}




CCardReaderErrorDialog::CCardReaderErrorDialog()
	: m_ReaderType(CCoreEngine::CARDREADER_SCARD)
{
}


CCardReaderErrorDialog::~CCardReaderErrorDialog()
{
}


bool CCardReaderErrorDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,
					  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_CARDREADER))==IDOK;
}


bool CCardReaderErrorDialog::SetMessage(LPCTSTR pszMessage)
{
	return m_Message.Set(pszMessage);
}


static bool SearchReaders(HWND hDlg)
{
	CCardReader *pCardReader;
	bool fFound=false;
	LRESULT Index;

	pCardReader=CCardReader::CreateCardReader(CCardReader::READER_SCARD);
	if (pCardReader!=NULL) {
		if (pCardReader->NumReaders()>0) {
			for (int i=0;i<pCardReader->NumReaders();i++) {
				LPCTSTR pszReaderName=pCardReader->EnumReader(i);
				if (pszReaderName!=NULL) {
					Index=DlgListBox_AddString(hDlg,IDC_CARDREADER_READERLIST,pszReaderName);
					DlgListBox_SetItemData(hDlg,IDC_CARDREADER_READERLIST,Index,(LPARAM)CCoreEngine::CARDREADER_SCARD);
					fFound=true;
				}
			}
		}
		delete pCardReader;
	}

	pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
	if (pCardReader!=NULL) {
		if (pCardReader->Open()) {
			Index=DlgListBox_AddString(hDlg,IDC_CARDREADER_READERLIST,TEXT("HDUS内蔵カードリーダ"));
			DlgListBox_SetItemData(hDlg,IDC_CARDREADER_READERLIST,Index,(LPARAM)CCoreEngine::CARDREADER_HDUS);
			pCardReader->Close();
			fFound=true;
		}
		delete pCardReader;
	}

	return fFound;
}


INT_PTR CCardReaderErrorDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			::SendDlgItemMessage(hDlg,IDC_CARDREADER_ICON,STM_SETICON,
				reinterpret_cast<WPARAM>(::LoadIcon(NULL,IDI_WARNING)),0);
			if (!m_Message.IsEmpty())
				::SetDlgItemText(hDlg,IDC_CARDREADER_MESSAGE,m_Message.Get());
			bool fFound=SearchReaders(hDlg);
			EnableDlgItem(hDlg,IDC_CARDREADER_RETRY,fFound);
			::CheckRadioButton(hDlg,IDC_CARDREADER_RETRY,IDC_CARDREADER_NOREADER,
							   fFound && m_ReaderType!=CCoreEngine::CARDREADER_NONE?
							   IDC_CARDREADER_RETRY:IDC_CARDREADER_NOREADER);
			EnableDlgItem(hDlg,IDC_CARDREADER_READERLIST,
						  fFound && m_ReaderType!=CCoreEngine::CARDREADER_NONE);
			if (fFound)
				DlgListBox_SetCurSel(hDlg,IDC_CARDREADER_READERLIST,0);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CARDREADER_RETRY:
		case IDC_CARDREADER_NOREADER:
			EnableDlgItem(hDlg,IDC_CARDREADER_READERLIST,
						  DlgRadioButton_IsChecked(hDlg,IDC_CARDREADER_RETRY));
			return TRUE;

		case IDC_CARDREADER_SEARCH:
			{
				DlgListBox_Clear(hDlg,IDC_CARDREADER_READERLIST);
				HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				bool fFound=SearchReaders(hDlg);
				::SetCursor(hcurOld);
				EnableDlgItem(hDlg,IDC_CARDREADER_RETRY,fFound);
				::CheckRadioButton(hDlg,IDC_CARDREADER_RETRY,IDC_CARDREADER_NOREADER,
								   fFound?IDC_CARDREADER_RETRY:IDC_CARDREADER_NOREADER);
				EnableDlgItem(hDlg,IDC_CARDREADER_READERLIST,
							  fFound && DlgRadioButton_IsChecked(hDlg,IDC_CARDREADER_RETRY));
				if (fFound) {
					DlgListBox_SetCurSel(hDlg,IDC_CARDREADER_READERLIST,0);
				} else {
					TCHAR szMessage[1024];

					::lstrcpy(szMessage,TEXT("カードリーダが見付かりません。"));
					switch (IsSmartCardServiceEnabled()) {
					case SCARD_QUERY_DISABLED:
						::lstrcat(szMessage,TEXT("\nSmart Card サービスが無効になっています。"));
						break;
					case SCARD_QUERY_ERR_SERVICENOTFOUND:
						::lstrcat(szMessage,TEXT("\nSmart Card サービスが見付かりません。"));
						break;
					}
					::MessageBox(hDlg,szMessage,TEXT("結果"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;

		case IDOK:
			{
				if (DlgRadioButton_IsChecked(hDlg,IDC_CARDREADER_RETRY)) {
					LRESULT Sel=DlgListBox_GetCurSel(hDlg,IDC_CARDREADER_READERLIST);

					if (Sel<0) {
						::MessageBox(hDlg,TEXT("カードリーダを選択してください。"),TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
						return TRUE;
					}
					m_ReaderType=(CCoreEngine::CardReaderType)DlgListBox_GetItemData(hDlg,IDC_CARDREADER_READERLIST,Sel);
					LRESULT Length=DlgListBox_GetStringLength(hDlg,IDC_CARDREADER_READERLIST,Sel);
					if (Length>0) {
						LPTSTR pszName=new TCHAR[Length+1];
						DlgListBox_GetString(hDlg,IDC_CARDREADER_READERLIST,Sel,pszName);
						m_ReaderName.Set(pszName);
						delete [] pszName;
					} else {
						m_ReaderName.Clear();
					}
				} else {
					m_ReaderType=CCoreEngine::CARDREADER_NONE;
					m_ReaderName.Clear();
				}
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}

	return FALSE;
}
