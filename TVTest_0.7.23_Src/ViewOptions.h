#ifndef VIEW_OPTIONS_H
#define VIEW_OPTIONS_H


#include "Options.h"
#include "MediaViewer.h"


class CViewOptions : public COptions
{
public:
	enum {
		UPDATE_MASKCUTAREA				=0x00000001UL,
		UPDATE_IGNOREDISPLAYEXTENSION	=0x00000002UL,
		UPDATE_LOGO						=0x00000004UL
	};

	CViewOptions();
	~CViewOptions();
// COptions
	bool Apply(DWORD Flags) override;
	bool ReadSettings(CSettings &Settings) override;
	bool WriteSettings(CSettings &Settings) override;
// CBasicDialog
	bool Create(HWND hwndOwner) override;
// CViewOptions
	bool GetAdjustAspectResizing() const { return m_fAdjustAspectResizing; }
	bool GetSnapAtWindowEdge() const { return m_fSnapAtWindowEdge; }
	int GetSnapAtWindowEdgeMargin() const { return m_SnapAtWindowEdgeMargin; }
	bool GetNearCornerResizeOrigin() const { return m_fNearCornerResizeOrigin; }
	bool GetPanScanNoResizeWindow() const { return m_fPanScanNoResizeWindow; }
	bool GetResetPanScanEventChange() const { return m_fResetPanScanEventChange; }
	bool GetNoMaskSideCut() const { return m_fNoMaskSideCut; }
	CMediaViewer::ViewStretchMode GetFullscreenStretchMode() const { return m_FullscreenStretchMode; }
	CMediaViewer::ViewStretchMode GetMaximizeStretchMode() const { return m_MaximizeStretchMode; }
	bool GetMinimizeToTray() const { return m_fMinimizeToTray; }
	bool GetDisablePreviewWhenMinimized() const { return m_fDisablePreviewWhenMinimized; }
	bool GetUseLogoIcon() const { return m_fUseLogoIcon; }
	bool GetIgnoreDisplayExtension() const { return m_fIgnoreDisplayExtension; }
	bool GetNoScreenSaver() const { return m_fNoScreenSaver; }
	bool GetNoMonitorLowPower() const { return m_fNoMonitorLowPower; }
	bool GetNoMonitorLowPowerActiveOnly() const { return m_fNoMonitorLowPowerActiveOnly; }
	bool GetShowLogo() const { return m_fShowLogo; }
	LPCTSTR GetLogoFileName() const { return m_szLogoFileName; }

private:
// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam) override;

	bool m_fAdjustAspectResizing;
	bool m_fSnapAtWindowEdge;
	int m_SnapAtWindowEdgeMargin;
	bool m_fNearCornerResizeOrigin;
	bool m_fPanScanNoResizeWindow;
	bool m_fResetPanScanEventChange;
	bool m_fNoMaskSideCut;
	CMediaViewer::ViewStretchMode m_FullscreenStretchMode;
	CMediaViewer::ViewStretchMode m_MaximizeStretchMode;
	bool m_fMinimizeToTray;
	bool m_fDisablePreviewWhenMinimized;
	bool m_fUseLogoIcon;
	bool m_fIgnoreDisplayExtension;
	bool m_fNoScreenSaver;
	bool m_fNoMonitorLowPower;
	bool m_fNoMonitorLowPowerActiveOnly;
	bool m_fShowLogo;
	TCHAR m_szLogoFileName[MAX_PATH];
};


#endif
