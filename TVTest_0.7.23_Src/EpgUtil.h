#ifndef EPG_UTIL_H
#define EPG_UTIL_H


#include "EpgProgramList.h"
#include "DrawUtil.h"


class CEpgIcons : public DrawUtil::CBitmap
{
public:
	enum {
		ICON_WIDTH	=11,
		ICON_HEIGHT	=11
	};

	enum {
		ICON_HD,
		ICON_SD,
		ICON_5_1CH,
		ICON_MULTILINGUAL,
		ICON_SUB,
		ICON_FREE,
		ICON_PAY,
		ICON_LAST=ICON_PAY
	};

	static UINT IconFlag(int Icon) { return 1<<Icon; }

	bool Load();
	static bool Draw(HDC hdcDst,int DstX,int DstY,
					 HDC hdcSrc,int Icon,int Width,int Height,BYTE Opacity=255);
	static unsigned int GetEventIcons(const CEventInfoData *pEventInfo);
};


#endif
