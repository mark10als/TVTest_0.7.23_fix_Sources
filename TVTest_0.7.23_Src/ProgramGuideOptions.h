#ifndef PROGRAM_GUIDE_OPTIONS_H
#define PROGRAM_GUIDE_OPTIONS_H


#include "ProgramGuide.h"
#include "Options.h"
#include "Plugin.h"
#include "Tooltip.h"


class CProgramGuideOptions : public COptions
{
	CProgramGuide *m_pProgramGuide;
	CPluginManager *m_pPluginManager;
	bool m_fOnScreen;
	int m_BeginHour;
	enum { MIN_VIEW_HOURS=1, MAX_VIEW_HOURS=24*7 };
	int m_ViewHours;
	int m_ItemWidth;
	int m_LinesPerHour;
	int m_LineMargin;
	int m_WheelScrollLines;
	LOGFONT m_Font;
	LOGFONT m_CurSettingFont;
	UINT m_VisibleEventIcons;
	CProgramGuideToolList m_ToolList;
	HIMAGELIST m_himlEventIcons;
	CDynamicString m_ProgramLDoubleClickCommand;
	CTooltip m_Tooltip;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	void SetDlgItemState();
	void DeleteAllTools();

public:
	enum {
		UPDATE_EVENTICONS	=0x00000001UL
	};

	CProgramGuideOptions(CProgramGuide *pProgramGuide,CPluginManager *pPluginManager);
	~CProgramGuideOptions();
// CSettingsBase
	bool LoadSettings(CSettings &Settings) override;
	bool SaveSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CProgramGuideOptions
	bool GetTimeRange(SYSTEMTIME *pstFirst,SYSTEMTIME *pstLast);
	bool GetOnScreen() const { return m_fOnScreen; }
	UINT GetVisibleEventIcons() const { return m_VisibleEventIcons; }
	LPCTSTR GetProgramLDoubleClickCommand() const { return m_ProgramLDoubleClickCommand.Get(); }
	int ParseCommand(LPCTSTR pszCommand) const;
};


#endif
