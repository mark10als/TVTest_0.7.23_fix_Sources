#ifndef DRIVER_MANAGER_H
#define DRIVER_MANAGER_H


#include <vector>
#include "ChannelList.h"


class CDriverInfo
{
	CDynamicString m_FileName;
	CDynamicString m_TunerName;
	bool m_fChannelFileLoaded;
	CTuningSpaceList m_TuningSpaceList;
	bool m_fDriverSpaceLoaded;
	CTuningSpaceList m_DriverSpaceList;

public:
	CDriverInfo(LPCTSTR pszFileName);
	~CDriverInfo();
	LPCTSTR GetFileName() const { return m_FileName.Get(); }
	LPCTSTR GetTunerName() const { return m_TunerName.Get(); }
	enum LoadTuningSpaceListMode {
		LOADTUNINGSPACE_DEFAULT,
		LOADTUNINGSPACE_NOLOADDRIVER,
		LOADTUNINGSPACE_USEDRIVER,
		LOADTUNINGSPACE_USEDRIVER_NOOPEN,
	};
	bool LoadTuningSpaceList(LoadTuningSpaceListMode Mode=LOADTUNINGSPACE_DEFAULT);
	bool IsChannelFileLoaded() const { return m_fChannelFileLoaded; }
	bool IsDriverChannelLoaded() const { return m_fDriverSpaceLoaded; }
	bool IsTuningSpaceListLoaded() const {
		return m_fChannelFileLoaded || m_fDriverSpaceLoaded;
	}
	const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
	const CTuningSpaceList *GetDriverSpaceList() const { return &m_DriverSpaceList; }
	const CTuningSpaceList *GetAvailableTuningSpaceList() const;
	const CChannelList *GetChannelList(int Space) const;
	int NumDriverSpaces() const { return m_DriverSpaceList.NumSpaces(); }
};

class CDriverManager
{
	std::vector<CDriverInfo*> m_DriverList;
	CDynamicString m_BaseDirectory;

	static bool CompareDriverFileName(const CDriverInfo *pDriver1,const CDriverInfo *pDriver2);

public:
	CDriverManager();
	~CDriverManager();
	void Clear();
	int NumDrivers() const { return (int)m_DriverList.size(); }
	bool Find(LPCTSTR pszDirectory);
	CDriverInfo *GetDriverInfo(int Index);
	const CDriverInfo *GetDriverInfo(int Index) const;
	LPCTSTR GetBaseDirectory() const { return m_BaseDirectory.Get(); }
};


#endif
