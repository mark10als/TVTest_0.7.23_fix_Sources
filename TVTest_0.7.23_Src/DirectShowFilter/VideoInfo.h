#ifndef VIDEO_INFO_H
#define VIDEO_INFO_H


class CMpeg2VideoInfo
{
public:
	WORD m_OrigWidth,m_OrigHeight;
	WORD m_DisplayWidth,m_DisplayHeight;
	WORD m_PosX,m_PosY;
	BYTE m_AspectRatioX,m_AspectRatioY;
	struct FrameRate {
		DWORD Num,Denom;
	};
	FrameRate m_FrameRate;
	CMpeg2VideoInfo()
	{
		Reset();
	}
	CMpeg2VideoInfo(WORD OrigWidth,WORD OrigHeight,WORD DisplayWidth,WORD DisplayHeight,BYTE AspectX,BYTE AspectY)
	{
		m_OrigWidth=OrigWidth;
		m_OrigHeight=OrigHeight;
		m_DisplayWidth=DisplayWidth;
		m_DisplayHeight=DisplayHeight;
		m_PosX=(OrigWidth-DisplayWidth)/2;
		m_PosY=(OrigHeight-DisplayHeight)/2;
		m_AspectRatioX=AspectX;
		m_AspectRatioY=AspectY;
		m_FrameRate.Num=0;
		m_FrameRate.Denom=0;
	}
	bool operator==(const CMpeg2VideoInfo &Info) const
	{
		return m_OrigWidth==Info.m_OrigWidth
			&& m_OrigHeight==Info.m_OrigHeight
			&& m_DisplayWidth==Info.m_DisplayWidth
			&& m_DisplayHeight==Info.m_DisplayHeight
			&& m_AspectRatioX==Info.m_AspectRatioX
			&& m_AspectRatioY==Info.m_AspectRatioY
			&& m_FrameRate.Num==Info.m_FrameRate.Num
			&& m_FrameRate.Denom==Info.m_FrameRate.Denom;
	}
	bool operator!=(const CMpeg2VideoInfo &Info) const
	{
		return !(*this==Info);
	}
	void Reset()
	{
		m_OrigWidth=0;
		m_OrigHeight=0;
		m_DisplayWidth=0;
		m_DisplayHeight=0;
		m_PosX=0;
		m_PosY=0;
		m_AspectRatioX=0;
		m_AspectRatioY=0;
		m_FrameRate.Num=0;
		m_FrameRate.Denom=0;
	}
};


#endif
