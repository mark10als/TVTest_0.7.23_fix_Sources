#ifndef TVTEST_OSD_MANAGER_H
#define TVTEST_OSD_MANAGER_H


#include "OSDOptions.h"
#include "PseudoOSD.h"
#include "ChannelList.h"


class COSDManager
{
public:
	class ABSTRACT_CLASS(CEventHandler)
	{
	public:
		virtual ~CEventHandler() {}
		virtual bool GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD)=0;
		virtual bool SetOSDHideTimer(DWORD Delay)=0;
	};

	COSDManager(const COSDOptions *pOptions);
	~COSDManager();
	void SetEventHandler(CEventHandler *pEventHandler);
	void Reset();
	void ClearOSD();
	void OnParentMove();
	bool ShowOSD(LPCTSTR pszText);
	void HideOSD();
	bool ShowChannelOSD(const CChannelInfo *pInfo,bool fChanging=false);
	void HideChannelOSD();
	bool ShowVolumeOSD(int Volume);
	void HideVolumeOSD();

private:
	const COSDOptions *m_pOptions;
	CEventHandler *m_pEventHandler;
	CPseudoOSD m_OSD;
	CPseudoOSD m_VolumeOSD;
};


#endif
