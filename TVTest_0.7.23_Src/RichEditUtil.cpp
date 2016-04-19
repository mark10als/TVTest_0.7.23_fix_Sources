#include "stdafx.h"
#include "TVTest.h"
#include "RichEditUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CRichEditUtil::CRichEditUtil()
	: m_hLib(NULL)
{
}


CRichEditUtil::~CRichEditUtil()
{
	UnloadRichEditLib();
}


bool CRichEditUtil::LoadRichEditLib()
{
	if (m_hLib==NULL) {
		m_hLib=::LoadLibrary(TEXT("Riched32.dll"));
		if (m_hLib==NULL)
			return false;
	}
	return true;
}


void CRichEditUtil::UnloadRichEditLib()
{
	if (m_hLib!=NULL) {
		::FreeLibrary(m_hLib);
		m_hLib=NULL;
	}
}


bool CRichEditUtil::LogFontToCharFormat(HDC hdc,const LOGFONT *plf,CHARFORMAT *pcf)
{
	if (hdc==NULL || plf==NULL || pcf==NULL)
		return false;

	pcf->cbSize=sizeof(CHARFORMAT);
	pcf->dwMask=CFM_BOLD | CFM_CHARSET | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT | CFM_UNDERLINE;
	pcf->dwEffects=0;
	if (plf->lfWeight>=FW_BOLD)
		pcf->dwEffects|=CFE_BOLD;
	if (plf->lfItalic)
		pcf->dwEffects|=CFE_ITALIC;
	if (plf->lfUnderline)
		pcf->dwEffects|=CFE_UNDERLINE;
	if (plf->lfStrikeOut)
		pcf->dwEffects|=CFE_STRIKEOUT;
	pcf->yHeight=abs(plf->lfHeight)*72*20/::GetDeviceCaps(hdc,LOGPIXELSY);
	pcf->crTextColor=::GetSysColor(COLOR_WINDOWTEXT);
	pcf->bPitchAndFamily=plf->lfPitchAndFamily;
	pcf->bCharSet=plf->lfCharSet;
	::lstrcpy(pcf->szFaceName,plf->lfFaceName);

	return true;
}


bool CRichEditUtil::LogFontToCharFormat2(HDC hdc,const LOGFONT *plf,CHARFORMAT2 *pcf)
{
	if (hdc==NULL || plf==NULL || pcf==NULL)
		return false;

	::ZeroMemory(pcf,sizeof(CHARFORMAT2));
	pcf->cbSize=sizeof(CHARFORMAT2);
	pcf->dwMask=CFM_CHARSET | CFM_COLOR | CFM_FACE | CFM_ITALIC | CFM_SIZE | CFM_STRIKEOUT | CFM_UNDERLINE | CFM_WEIGHT;
	pcf->dwEffects=0;
	if (plf->lfItalic)
		pcf->dwEffects|=CFE_ITALIC;
	if (plf->lfUnderline)
		pcf->dwEffects|=CFE_UNDERLINE;
	if (plf->lfStrikeOut)
		pcf->dwEffects|=CFE_STRIKEOUT;
	pcf->yHeight=abs(plf->lfHeight)*72*20/::GetDeviceCaps(hdc,LOGPIXELSY);
	pcf->crTextColor=::GetSysColor(COLOR_WINDOWTEXT);
	pcf->bPitchAndFamily=plf->lfPitchAndFamily;
	pcf->bCharSet=plf->lfCharSet;
	::lstrcpy(pcf->szFaceName,plf->lfFaceName);
	pcf->wWeight=(WORD)plf->lfWeight;

	return true;
}


void CRichEditUtil::CharFormatToCharFormat2(const CHARFORMAT *pcf,CHARFORMAT2 *pcf2)
{
	::CopyMemory(pcf2,pcf,sizeof(CHARFORMAT));
	pcf2->cbSize=sizeof(CHARFORMAT2);
}


bool CRichEditUtil::AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT *pcf)
{
	if (hwndEdit==NULL || pszText==NULL)
		return false;

	CHARRANGE cr,crOld;

	::SendMessage(hwndEdit,WM_SETREDRAW,FALSE,0);
	::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	cr.cpMin=0;
	cr.cpMax=-1;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&cr));
	cr.cpMin=cr.cpMax;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
	if (pcf!=NULL) {
		::SendMessage(hwndEdit,EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(pcf));
	}
	::SendMessage(hwndEdit,EM_REPLACESEL,0,reinterpret_cast<LPARAM>(pszText));
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	::SendMessage(hwndEdit,WM_SETREDRAW,TRUE,0);
	::InvalidateRect(hwndEdit,NULL,TRUE);

	return true;
}


bool CRichEditUtil::AppendText(HWND hwndEdit,LPCTSTR pszText,const CHARFORMAT2 *pcf)
{
	return AppendText(hwndEdit,pszText,(const CHARFORMAT*)pcf);
}


bool CRichEditUtil::CopyAllText(HWND hwndEdit)
{
	CHARRANGE cr,crOld;

	::SendMessage(hwndEdit,EM_HIDESELECTION,TRUE,0);
	::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	cr.cpMin=0;
	cr.cpMax=-1;
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
	::SendMessage(hwndEdit,WM_COPY,0,0);
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	::SendMessage(hwndEdit,EM_HIDESELECTION,FALSE,0);
	return true;
}


void CRichEditUtil::SelectAll(HWND hwndEdit)
{
	CHARRANGE cr={0,-1};

	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
}


bool CRichEditUtil::IsSelected(HWND hwndEdit)
{
	return hwndEdit!=NULL
		&& ::SendMessage(hwndEdit,EM_SELECTIONTYPE,0,0)!=SEL_EMPTY;
}


LPTSTR CRichEditUtil::GetSelectedText(HWND hwndEdit)
{
	if (hwndEdit!=NULL) {
		CHARRANGE cr={0,0};

		::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&cr));
		if (cr.cpMax>cr.cpMin) {
			LPTSTR pszText=new TCHAR[cr.cpMax-cr.cpMin+1];

			if (::SendMessage(hwndEdit,EM_GETSELTEXT,0,reinterpret_cast<LPARAM>(pszText))>0)
				return pszText;
			delete [] pszText;
		}
	}
	return NULL;
}


int CRichEditUtil::GetMaxLineWidth(HWND hwndEdit)
{
	int NumLines=(int)::SendMessage(hwndEdit,EM_GETLINECOUNT,0,0);
	int MaxWidth=0;

	for (int i=0;i<NumLines;i++) {
		int Index=(int)::SendMessage(hwndEdit,EM_LINEINDEX,i,0);
		POINTL pt;
		::SendMessage(hwndEdit,EM_POSFROMCHAR,
					  reinterpret_cast<WPARAM>(&pt),
					  Index+::SendMessage(hwndEdit,EM_LINELENGTH,Index,0));
		if (pt.x>MaxWidth)
			MaxWidth=pt.x;
	}
	return MaxWidth;
}


bool CRichEditUtil::DetectURL(HWND hwndEdit,const CHARFORMAT *pcf,int FirstLine,int LastLine)
{
	const int LineCount=(int)::SendMessage(hwndEdit,EM_GETLINECOUNT,0,0);
	if (LastLine<0 || LastLine>LineCount)
		LastLine=LineCount;
	CHARFORMAT2 cfLink;
	CharFormatToCharFormat2(pcf,&cfLink);
	cfLink.dwMask|=CFM_UNDERLINE | CFM_LINK | CFM_COLOR;
	cfLink.dwEffects|=CFE_UNDERLINE | CFE_LINK;
	cfLink.crTextColor=RGB(0,0,255);
	CHARRANGE cr,crOld;
	bool fDetect=false;

	::SendMessage(hwndEdit,WM_SETREDRAW,FALSE,0);
	::SendMessage(hwndEdit,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	for (int i=FirstLine;i<LastLine;) {
		const int LineIndex=(int)::SendMessage(hwndEdit,EM_LINEINDEX,i,0);
		TCHAR szText[2048],*p;
		int TotalLength=0,Length;

		p=szText;
		while (i<LastLine) {
#ifdef UNICODE
			p[0]=(WORD)(lengthof(szText)-2-TotalLength);
#else
			*(WORD*)p=(WORD)(sizeof(szText)-sizeof(WORD)-1-TotalLength);
#endif
			Length=(int)::SendMessage(hwndEdit,EM_GETLINE,i,reinterpret_cast<LPARAM>(p));
			i++;
			if (Length<1)
				break;
			p+=Length;
			TotalLength+=Length;
			if (*(p-1)=='\r' || *(p-1)=='\n')
				break;
		}
		if (TotalLength>0) {
			szText[TotalLength]='\0';
			LPCTSTR q=szText;
			while (SearchNextURL(&q,&Length)) {
				cr.cpMin=LineIndex+(LONG)(q-szText);
				cr.cpMax=cr.cpMin+Length;
				::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&cr));
				::SendMessage(hwndEdit,EM_SETCHARFORMAT,SCF_SELECTION,reinterpret_cast<LPARAM>(&cfLink));
				fDetect=true;
				q+=Length;
			}
		}
	}
	::SendMessage(hwndEdit,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&crOld));
	::SendMessage(hwndEdit,WM_SETREDRAW,TRUE,0);
	::InvalidateRect(hwndEdit,NULL,TRUE);
	return fDetect;
}


bool CRichEditUtil::SearchNextURL(LPCTSTR *ppszText,int *pLength)
{
	LPCTSTR p=*ppszText;
	int Length=::lstrlen(p);

	for (int i=0;i<Length-7;i++) {
		if (::CompareString(LOCALE_USER_DEFAULT,NORM_IGNOREWIDTH,
							&p[i],7,TEXT("http://"),7)==CSTR_EQUAL
				|| ::CompareString(LOCALE_USER_DEFAULT,NORM_IGNOREWIDTH,
							&p[i],4,TEXT("www."),4)==CSTR_EQUAL) {
			int URLLength=4;
			if (p[i]<0x0080) {
				while (i+URLLength<Length && p[i+URLLength]>0x0020 && p[i+URLLength]<0x0080)
					URLLength++;
			} else {
				while (i+URLLength<Length && p[i+URLLength]>0xFF00 && p[i+URLLength]<=0xFF5E)
					URLLength++;
			}
			*ppszText=p+i;
			*pLength=URLLength;
			return true;
		}
	}
	return false;
}


bool CRichEditUtil::HandleLinkClick(const ENLINK *penl)
{
	if (penl==NULL || penl->chrg.cpMax-penl->chrg.cpMin>=256)
		return false;

	TCHAR szText[256];
	int Length;

	TEXTRANGE tr;
	tr.chrg=penl->chrg;
	tr.lpstrText=szText;
	Length=(int)::SendMessage(penl->nmhdr.hwndFrom,EM_GETTEXTRANGE,0,reinterpret_cast<LPARAM>(&tr));
	if (Length<=0)
		return false;

	TCHAR szURL[256+7];
	Length=::LCMapString(LOCALE_USER_DEFAULT,LCMAP_HALFWIDTH,
						 szText,Length,szURL,lengthof(szURL));
	szURL[Length]='\0';
	if (::StrCmpN(szURL,TEXT("www."),4)==0) {
		::lstrcpy(szText,szURL);
		::wsprintf(szURL,TEXT("http://%s"),szText);
	}
	::ShellExecute(NULL,TEXT("open"),szURL,NULL,NULL,SW_SHOWNORMAL);
	return true;
}
