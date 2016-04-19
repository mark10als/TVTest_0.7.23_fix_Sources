#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Logger.h"
#include "DialogUtil.h"
#include "StdUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CLogItem::CLogItem(LPCTSTR pszText)
	: m_Text(pszText)
{
	::GetSystemTimeAsFileTime(&m_Time);
}


CLogItem::~CLogItem()
{
}


void CLogItem::GetTime(SYSTEMTIME *pTime) const
{
	SYSTEMTIME stUTC;

	::FileTimeToSystemTime(&m_Time,&stUTC);
	::SystemTimeToTzSpecificLocalTime(NULL,&stUTC,pTime);
}


int CLogItem::Format(char *pszText,int MaxLength) const
{
	SYSTEMTIME st;
	int Length;

	GetTime(&st);
	Length=::GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,
											NULL,pszText,MaxLength-1);
	pszText[Length-1]=' ';
	Length+=::GetTimeFormatA(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,
								NULL,pszText+Length,MaxLength-Length);
	pszText[Length-1]='>';
	Length+=::WideCharToMultiByte(CP_ACP,0,m_Text.Get(),m_Text.Length(),
									pszText+Length,MaxLength-Length-1,NULL,NULL);
	pszText[Length]='\0';
	return Length;
}




CLogger::CLogger()
	: m_fOutputToFile(false)
{
}


CLogger::~CLogger()
{
	Destroy();
	Clear();
}


bool CLogger::ReadSettings(CSettings &Settings)
{
	CBlockLock Lock(&m_Lock);

	Settings.Read(TEXT("OutputLogToFile"),&m_fOutputToFile);
	if (m_fOutputToFile && m_LogList.size()>0) {
		TCHAR szFileName[MAX_PATH];

		GetDefaultLogFileName(szFileName);
		SaveToFile(szFileName,true);
	}
	return true;
}


bool CLogger::WriteSettings(CSettings &Settings)
{
	Settings.Write(TEXT("OutputLogToFile"),m_fOutputToFile);
	return true;
}


bool CLogger::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_LOG));
}


bool CLogger::AddLog(LPCTSTR pszText, ...)
{
	if (pszText==NULL)
		return false;

	va_list Args;
	va_start(Args,pszText);
	AddLogV(pszText,Args);
	va_end(Args);
	return true;
}


bool CLogger::AddLogV(LPCTSTR pszText,va_list Args)
{
	if (pszText==NULL)
		return false;

	CBlockLock Lock(&m_Lock);

	TCHAR szText[1024];
	StdUtil::vsnprintf(szText,lengthof(szText),pszText,Args);
	CLogItem *pLogItem=new CLogItem(szText);
	m_LogList.push_back(pLogItem);
	TRACE(TEXT("Log : %s\n"),szText);

	if (m_fOutputToFile) {
		TCHAR szFileName[MAX_PATH];
		HANDLE hFile;

		GetDefaultLogFileName(szFileName);
		hFile=::CreateFile(szFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
						   OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			char szText[1024];
			DWORD Length,Write;

			::SetFilePointer(hFile,0,NULL,FILE_END);
			Length=pLogItem->Format(szText,lengthof(szText)-1);
			szText[Length++]='\r';
			szText[Length++]='\n';
			::WriteFile(hFile,szText,Length,&Write,NULL);
			::CloseHandle(hFile);
		}
	}

	return true;
}


void CLogger::Clear()
{
	CBlockLock Lock(&m_Lock);

	for (size_t i=0;i<m_LogList.size();i++)
		delete m_LogList[i];
	m_LogList.clear();
}


bool CLogger::SetOutputToFile(bool fOutput)
{
	CBlockLock Lock(&m_Lock);

	m_fOutputToFile=fOutput;
	return true;
}


bool CLogger::SaveToFile(LPCTSTR pszFileName,bool fAppend)
{
	HANDLE hFile;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
					   fAppend?OPEN_ALWAYS:CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (fAppend)
		::SetFilePointer(hFile,0,NULL,FILE_END);

	m_Lock.Lock();

	for (size_t i=0;i<m_LogList.size();i++) {
		char szText[1024];
		DWORD Length,Write;

		Length=m_LogList[i]->Format(szText,lengthof(szText)-1);
		szText[Length++]='\r';
		szText[Length++]='\n';
		::WriteFile(hFile,szText,Length,&Write,NULL);
	}

	m_Lock.Unlock();

	::CloseHandle(hFile);
	return true;
}


void CLogger::GetDefaultLogFileName(LPTSTR pszFileName) const
{
	::GetModuleFileName(NULL,pszFileName,MAX_PATH);
	::PathRenameExtension(pszFileName,TEXT(".log"));
}


INT_PTR CLogger::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HWND hwndList=GetDlgItem(hDlg,IDC_LOG_LIST);
			LV_COLUMN lvc;
			LV_ITEM lvi;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=80;
			lvc.pszText=TEXT("����");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("���e");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvi.mask=LVIF_TEXT;
			m_Lock.Lock();
			ListView_SetItemCount(hwndList,(int)m_LogList.size());
			for (size_t i=0;i<m_LogList.size();i++) {
				const CLogItem *pLogItem=m_LogList[i];
				SYSTEMTIME st;
				int Length;
				TCHAR szTime[64];

				lvi.iItem=(int)i;
				lvi.iSubItem=0;
				pLogItem->GetTime(&st);
				Length=::GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,
											NULL,szTime,lengthof(szTime)-1);
				szTime[Length-1]=_T(' ');
				::GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,
								NULL,szTime+Length,lengthof(szTime)-Length);
				lvi.pszText=szTime;
				ListView_InsertItem(hwndList,&lvi);
				lvi.iSubItem=1;
				lvi.pszText=(LPTSTR)pLogItem->GetText();
				ListView_SetItem(hwndList,&lvi);
			}
			for (int i=0;i<2;i++)
				ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
			if (m_LogList.size()>0)
				ListView_EnsureVisible(hwndList,(int)m_LogList.size()-1,FALSE);
			m_Lock.Unlock();

			DlgCheckBox_Check(hDlg,IDC_LOG_OUTPUTTOFILE,m_fOutputToFile);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LOG_CLEAR:
			ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_LOG_LIST));
			Clear();
			return TRUE;

		case IDC_LOG_SAVE:
			{
				TCHAR szFileName[MAX_PATH];

				GetDefaultLogFileName(szFileName);
				if (!SaveToFile(szFileName,false)) {
					::MessageBox(hDlg,TEXT("�ۑ����ł��܂���B"),NULL,MB_OK | MB_ICONEXCLAMATION);
				} else {
					TCHAR szMessage[MAX_PATH+64];

					StdUtil::snprintf(szMessage,lengthof(szMessage),
									  TEXT("���O�� \"%s\" �ɕۑ����܂����B"),szFileName);
					::MessageBox(hDlg,szMessage,TEXT("���O�ۑ�"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				bool fOutput=DlgCheckBox_IsChecked(hDlg,IDC_LOG_OUTPUTTOFILE);

				if (fOutput!=m_fOutputToFile) {
					CBlockLock Lock(&m_Lock);

					if (fOutput && m_LogList.size()>0) {
						TCHAR szFileName[MAX_PATH];

						GetDefaultLogFileName(szFileName);
						SaveToFile(szFileName,true);
					}
					m_fOutputToFile=fOutput;

					m_fChanged=true;
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}


void CLogger::OnTrace(LPCTSTR pszOutput)
{
	AddLog(pszOutput);
}
