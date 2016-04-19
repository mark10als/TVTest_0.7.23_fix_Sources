#ifndef CARD_READER_DIALOG_H
#define CARD_READER_DIALOG_H


#include "Dialog.h"
#include "CoreEngine.h"


class CCardReaderErrorDialog : public CBasicDialog
{
public:
	CCardReaderErrorDialog();
	~CCardReaderErrorDialog();
	bool Show(HWND hwndOwner) override;
	bool SetMessage(LPCTSTR pszMessage);
	CCoreEngine::CardReaderType GetReaderType() const { return m_ReaderType; }
	LPCTSTR GetReaderName() const { return m_ReaderName.Get(); }

private:
	CDynamicString m_Message;
	CCoreEngine::CardReaderType m_ReaderType;
	CDynamicString m_ReaderName;

	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;
};


#endif
