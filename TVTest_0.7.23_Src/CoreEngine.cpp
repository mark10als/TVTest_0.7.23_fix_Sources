#include "stdafx.h"
#include "TVTest.h"
#include "CoreEngine.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CCoreEngine::CCoreEngine()
	: m_hDriverLib(NULL)
	, m_DriverType(DRIVER_UNKNOWN)
	, m_fDescramble(true)
	, m_CardReaderType(CARDREADER_SCARD)
	, m_fPacketBuffering(false)
	, m_PacketBufferLength(m_DtvEngine.m_MediaBuffer.GetBufferLength())
	, m_PacketBufferPoolPercentage(m_DtvEngine.m_MediaBuffer.GetPoolPercentage())
	, m_OriginalVideoWidth(0)
	, m_OriginalVideoHeight(0)
	, m_DisplayVideoWidth(0)
	, m_DisplayVideoHeight(0)
	, m_NumAudioChannels(CMediaViewer::AUDIO_CHANNEL_INVALID)
	, m_NumAudioStreams(0)
	, m_AudioComponentType(0)
	, m_fMute(false)
	, m_Volume(50)
	, m_AudioGain(100)
	, m_SurroundAudioGain(100)
	, m_StereoMode(STEREOMODE_STEREO)
	, m_AutoStereoMode(STEREOMODE_LEFT)
	, m_fDownMixSurround(true)
	, m_fSpdifPassthrough(false)
	, m_EventID(0)
	, m_ErrorPacketCount(0)
	, m_ContinuityErrorPacketCount(0)
	, m_ScramblePacketCount(0)
	, m_SignalLevel(0.0)
	, m_BitRate(0)
	, m_PacketBufferUsedCount(0)
	, m_StreamRemain(0)
	, m_TimerResolution(0)
	, m_fNoEpg(false)
{
	m_szDriverDirectory[0]='\0';
	m_szDriverFileName[0]='\0';
}


CCoreEngine::~CCoreEngine()
{
	Close();
	if (m_TimerResolution!=0)
		::timeEndPeriod(m_TimerResolution);
}


void CCoreEngine::Close()
{
	m_DtvEngine.CloseEngine();
	UnloadDriver();
}


bool CCoreEngine::BuildDtvEngine(CDtvEngine::CEventHandler *pEventHandler)
{
	if (!m_DtvEngine.BuildEngine(pEventHandler,
			m_fDescramble?m_CardReaderType!=CARDREADER_NONE:false,
			true/*m_fPacketBuffering*/,!m_fNoEpg)) {
		return false;
	}
	return true;
}


bool CCoreEngine::GetDriverDirectory(LPTSTR pszDirectory) const
{
	if (m_szDriverDirectory[0]!='\0') {
		if (::PathIsRelative(m_szDriverDirectory)) {
			TCHAR szTemp[MAX_PATH];

			::GetModuleFileName(NULL,szTemp,lengthof(szTemp));
			::PathRemoveFileSpec(szTemp);
			::PathAppend(szTemp,m_szDriverDirectory);
			::PathCanonicalize(pszDirectory,szTemp);
		} else {
			::lstrcpy(pszDirectory,m_szDriverDirectory);
		}
	} else {
		::GetModuleFileName(NULL,pszDirectory,MAX_PATH);
		::PathRemoveFileSpec(pszDirectory);
	}
	return true;
}


bool CCoreEngine::SetDriverDirectory(LPCTSTR pszDirectory)
{
	if (pszDirectory==NULL) {
		m_szDriverDirectory[0]='\0';
	} else {
		if (::lstrlen(pszDirectory)>=MAX_PATH)
			return false;
		::lstrcpy(m_szDriverDirectory,pszDirectory);
	}
	return true;
}


bool CCoreEngine::SetDriverFileName(LPCTSTR pszFileName)
{
	if (pszFileName==NULL || ::lstrlen(pszFileName)>=MAX_PATH)
		return false;
	::lstrcpy(m_szDriverFileName,pszFileName);
	return true;
}


bool CCoreEngine::GetDriverPath(LPTSTR pszPath) const
{
	if (m_szDriverFileName[0]=='\0' || pszPath==NULL)
		return false;
	if (::PathIsRelative(m_szDriverFileName)) {
		TCHAR szTemp[MAX_PATH];

		GetDriverDirectory(szTemp);
		::PathAppend(szTemp,m_szDriverFileName);
		::PathCanonicalize(pszPath,szTemp);
	} else {
		::lstrcpy(pszPath,m_szDriverFileName);
	}
	return true;
}


bool CCoreEngine::LoadDriver()
{
	UnloadDriver();
	TCHAR szDriverPath[MAX_PATH];
	GetDriverPath(szDriverPath);
	m_hDriverLib=::LoadLibrary(szDriverPath);
	if (m_hDriverLib==NULL) {
		int ErrorCode=::GetLastError();
		TCHAR szText[MAX_PATH+64];

		StdUtil::snprintf(szText,lengthof(szText),
						  TEXT("\"%s\" ���ǂݍ��߂܂���B"),m_szDriverFileName);
		SetError(szText);
		switch (ErrorCode) {
		case ERROR_MOD_NOT_FOUND:
			SetErrorAdvise(TEXT("�t�@�C����������܂���B"));
			break;
		case ERROR_BAD_EXE_FORMAT:
			SetErrorAdvise(
#ifndef WIN64
				TEXT("32")
#else
				TEXT("64")
#endif
				TEXT("�r�b�g�p��BonDriver�ł͂Ȃ����A�t�@�C�����j�����Ă���\��������܂��B"));
			break;
		case ERROR_SXS_CANT_GEN_ACTCTX:
			SetErrorAdvise(TEXT("����BonDriver�ɕK�v�ȃ����^�C�����C���X�g�[������Ă��Ȃ��\��������܂��B"));
			break;
		default:
			StdUtil::snprintf(szText,lengthof(szText),TEXT("�G���[�R�[�h: %d"),ErrorCode);
			SetErrorAdvise(szText);
		}
		if (GetErrorText(ErrorCode,szText,lengthof(szText)))
			SetErrorSystemMessage(szText);
		return false;
	}
	return true;
}


bool CCoreEngine::UnloadDriver()
{
	if (m_hDriverLib) {
		CloseDriver();
		::FreeLibrary(m_hDriverLib);
		m_hDriverLib=NULL;
	}
	return true;
}


bool CCoreEngine::OpenDriver()
{
	if (m_hDriverLib==NULL)
		return false;
	if (m_DtvEngine.IsSrcFilterOpen())
		return false;
	if (!m_DtvEngine.OpenSrcFilter_BonDriver(m_hDriverLib)) {
		SetError(m_DtvEngine.GetLastErrorException());
		return false;
	}
	LPCTSTR pszName=m_DtvEngine.m_BonSrcDecoder.GetTunerName();
	m_DriverType=DRIVER_UNKNOWN;
	if (pszName!=NULL) {
		if (::_tcsncmp(pszName,TEXT("UDP/"),4)==0)
			m_DriverType=DRIVER_UDP;
		else if (::_tcsncmp(pszName,TEXT("TCP"),3)==0)
			m_DriverType=DRIVER_TCP;
	}
	return true;
}


bool CCoreEngine::CloseDriver()
{
	return m_DtvEngine.ReleaseSrcFilter();
}


bool CCoreEngine::IsDriverOpen() const
{
	return m_DtvEngine.IsSrcFilterOpen();
}


bool CCoreEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	if (!m_DtvEngine.m_MediaViewer.IsOpen()) {
		if (!m_DtvEngine.BuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,
										  pszMpeg2Decoder,pszAudioDevice)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	} else {
		if (!m_DtvEngine.RebuildMediaViewer(hwndHost,hwndMessage,VideoRenderer,
											pszMpeg2Decoder,pszAudioDevice)) {
			SetError(m_DtvEngine.GetLastErrorException());
			return false;
		}
	}
	m_DtvEngine.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	m_DtvEngine.m_MediaViewer.SetAudioGainControl(
		m_AudioGain!=100 || m_SurroundAudioGain!=100,
		(float)m_AudioGain/100.0f,(float)m_SurroundAudioGain/100.0f);
	m_DtvEngine.SetStereoMode(m_StereoMode);
	m_DtvEngine.m_MediaViewer.SetAutoStereoMode(m_AutoStereoMode);
	m_DtvEngine.m_MediaViewer.SetDownMixSurround(m_fDownMixSurround);
	m_DtvEngine.m_MediaViewer.SetSpdifOptions(&m_SpdifOptions);
	return true;
}


bool CCoreEngine::CloseMediaViewer()
{
	return m_DtvEngine.CloseMediaViewer();
}


bool CCoreEngine::OpenCardReader(CardReaderType Type,LPCTSTR pszReaderName)
{
	if (Type==CARDREADER_SCARD && pszReaderName==NULL && IsDriverSpecified()) {
		// ���݂� BonDriver ��p�� winscard.dll ������΂���𗘗p����
		TCHAR szFileName[MAX_PATH];

		GetDriverPath(szFileName);
		::PathRenameExtension(szFileName,TEXT(".scard"));
		if (::PathFileExists(szFileName)) {
			if (!m_DtvEngine.OpenBcasCard(CCardReader::READER_SCARD_DYNAMIC,szFileName)) {
				SetError(m_DtvEngine.GetLastErrorException());
				return false;
			}
			return true;
		}
	}
	if (!m_DtvEngine.OpenBcasCard(
			Type==CARDREADER_SCARD?CCardReader::READER_SCARD:
			Type==CARDREADER_HDUS?CCardReader::READER_HDUS:
			Type==CARDREADER_BONCASCLIENT?CCardReader::READER_BONCASCLIENT:
			CCardReader::READER_NONE,
			pszReaderName)) {
		SetError(m_DtvEngine.GetLastErrorException());
		return false;
	}
	return true;
}


bool CCoreEngine::OpenBcasCard()
{
	if (m_fDescramble) {
		if (!OpenCardReader(m_CardReaderType))
			return false;
	}
	return true;
}


bool CCoreEngine::CloseBcasCard()
{
	return m_DtvEngine.CloseBcasCard();
}


bool CCoreEngine::IsBcasCardOpen() const
{
	return m_DtvEngine.m_TsDescrambler.IsBcasCardOpen();
}


// ���� BonDriver ��p�� winscard.dll ���g�p���Ă��邩�擾����
bool CCoreEngine::IsDriverCardReader() const
{
	return m_DtvEngine.m_TsDescrambler.GetCardReaderType()==CCardReader::READER_SCARD_DYNAMIC;
}


bool CCoreEngine::IsBuildComplete() const
{
	return m_DtvEngine.IsBuildComplete();
}


bool CCoreEngine::EnablePreview(bool fPreview)
{
	if (!m_DtvEngine.EnablePreview(fPreview))
		return false;
	if (fPreview)
		m_DtvEngine.SetVolume(m_fMute?-100.0f:LevelToDeciBel(m_Volume));
	return true;
}


bool CCoreEngine::SetDescramble(bool fDescramble)
{
	if (m_fDescramble!=fDescramble) {
		if (m_DtvEngine.IsEngineBuild()) {
			if (!m_DtvEngine.SetDescramble(fDescramble)) {
				SetError(m_DtvEngine.GetLastErrorException());
				return false;
			}
		}
		m_fDescramble=fDescramble;
	}
	return true;
}


bool CCoreEngine::SetCardReaderType(CardReaderType Type,LPCTSTR pszReaderName)
{
	if (Type<CARDREADER_NONE || Type>CARDREADER_LAST)
		return false;
	/*if (m_CardReaderType!=Type)*/ {
		if (m_DtvEngine.IsEngineBuild()) {
			if (!SetDescramble(Type!=CARDREADER_NONE))
				return false;
			if (!OpenCardReader(Type,pszReaderName)) {
				m_CardReaderType=CARDREADER_NONE;
				return false;
			}
		}
		m_CardReaderType=Type;
	}
	return true;
}


bool CCoreEngine::IsNetworkDriverFileName(LPCTSTR pszFileName)
{
	if (pszFileName==NULL)
		return false;

	LPCTSTR pszName=::PathFindFileName(pszFileName);

	if (::lstrcmpi(pszName,TEXT("BonDriver_UDP.dll"))==0
			|| ::lstrcmpi(pszName,TEXT("BonDriver_TCP.dll"))==0)
		return true;
	return false;
}


bool CCoreEngine::SetPacketBuffering(bool fBuffering)
{
	if (!m_DtvEngine.m_MediaBuffer.EnableBuffering(fBuffering))
		return false;
	m_fPacketBuffering=fBuffering;
	return true;
}


bool CCoreEngine::SetPacketBufferLength(DWORD BufferLength)
{
	if (!m_DtvEngine.m_MediaBuffer.SetBufferLength(BufferLength))
		return false;
	m_PacketBufferLength=BufferLength;
	return true;
}


bool CCoreEngine::SetPacketBufferPoolPercentage(int Percentage)
{
	if (!m_DtvEngine.m_MediaBuffer.SetPoolPercentage(Percentage))
		return false;
	m_PacketBufferPoolPercentage=Percentage;
	return true;
}


bool CCoreEngine::GetVideoViewSize(int *pWidth,int *pHeight)
{
	WORD Width,Height;

	if (m_DtvEngine.m_MediaViewer.GetCroppedVideoSize(&Width,&Height)
			&& Width>0 && Height>0) {
		BYTE XAspect,YAspect;

		if (m_DtvEngine.m_MediaViewer.GetEffectiveAspectRatio(&XAspect,&YAspect)) {
			Width=Height*XAspect/YAspect;
		}
		if (pWidth)
			*pWidth=Width;
		if (pHeight)
			*pHeight=Height;
		return true;
	}
	return false;
}


bool CCoreEngine::SetVolume(int Volume)
{
	if (Volume<0 || Volume>MAX_VOLUME)
		return false;
	m_DtvEngine.SetVolume(LevelToDeciBel(Volume));
	m_Volume=Volume;
	m_fMute=false;
	return true;
}


bool CCoreEngine::SetMute(bool fMute)
{
	if (fMute!=m_fMute) {
		m_DtvEngine.SetVolume(fMute?-100.0f:LevelToDeciBel(m_Volume));
		m_fMute=fMute;
	}
	return true;
}


bool CCoreEngine::SetAudioGainControl(int Gain,int SurroundGain)
{
	if (Gain<0 || SurroundGain<0)
		return false;
	if (Gain!=m_AudioGain || SurroundGain!=m_SurroundAudioGain) {
		m_DtvEngine.m_MediaViewer.SetAudioGainControl(
			Gain!=100 || SurroundGain!=100,
			(float)Gain/100.0f,(float)SurroundGain/100.0f);
		m_AudioGain=Gain;
		m_SurroundAudioGain=SurroundGain;
	}
	return true;
}


bool CCoreEngine::GetAudioGainControl(int *pGain,int *pSurroundGain) const
{
	if (pGain)
		*pGain=m_AudioGain;
	if (pSurroundGain)
		*pSurroundGain=m_SurroundAudioGain;
	return true;
}


bool CCoreEngine::SetStereoMode(int Mode)
{
	if (Mode<0 || Mode>2)
		return false;
	/*if (Mode!=m_StereoMode)*/ {
		m_DtvEngine.SetStereoMode(Mode);
		m_StereoMode=Mode;
	}
	return true;
}


bool CCoreEngine::SetAutoStereoMode(int Mode)
{
	if (Mode<0 || Mode>2)
		return false;
	if (Mode!=m_AutoStereoMode) {
		m_DtvEngine.m_MediaViewer.SetAutoStereoMode(Mode);
		m_AutoStereoMode=Mode;
	}
	return true;
}


bool CCoreEngine::SetDownMixSurround(bool fDownMix)
{
	if (fDownMix!=m_fDownMixSurround) {
		m_DtvEngine.m_MediaViewer.SetDownMixSurround(fDownMix);
		m_fDownMixSurround=fDownMix;
	}
	return true;
}


bool CCoreEngine::SetSpdifOptions(const CAacDecFilter::SpdifOptions &Options)
{
	m_SpdifOptions=Options;
	m_DtvEngine.m_MediaViewer.SetSpdifOptions(&Options);
	return true;
}


bool CCoreEngine::GetSpdifOptions(CAacDecFilter::SpdifOptions *pOptions) const
{
	if (pOptions==NULL)
		return false;
	*pOptions=m_SpdifOptions;
	return true;
}


// TODO: �ω����������ꍇ DtvEngine ������ʒm����悤�ɂ���
DWORD CCoreEngine::UpdateAsyncStatus()
{
	DWORD Updated=0;

	WORD Width,Height;
	if (m_DtvEngine.m_MediaViewer.GetOriginalVideoSize(&Width,&Height)) {
		if (Width!=m_OriginalVideoWidth || Height!=m_OriginalVideoHeight) {
			m_OriginalVideoWidth=Width;
			m_OriginalVideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}

	if (m_DtvEngine.m_MediaViewer.GetCroppedVideoSize(&Width,&Height)) {
		if (Width!=m_DisplayVideoWidth || Height!=m_DisplayVideoHeight) {
			m_DisplayVideoWidth=Width;
			m_DisplayVideoHeight=Height;
			Updated|=STATUS_VIDEOSIZE;
		}
	}

	int NumAudioChannels=m_DtvEngine.GetAudioChannelNum();
	if (NumAudioChannels!=m_NumAudioChannels) {
		m_NumAudioChannels=NumAudioChannels;
		Updated|=STATUS_AUDIOCHANNELS;
		TRACE(TEXT("Audio channels = %dch\n"),NumAudioChannels);
	}

	int NumAudioStreams=m_DtvEngine.GetAudioStreamNum();
	if (NumAudioStreams!=m_NumAudioStreams) {
		m_NumAudioStreams=NumAudioStreams;
		Updated|=STATUS_AUDIOSTREAMS;
		TRACE(TEXT("Audio streams = %dch\n"),NumAudioChannels);
	}

	BYTE AudioComponentType=m_DtvEngine.GetAudioComponentType();
	if (AudioComponentType!=m_AudioComponentType) {
		m_AudioComponentType=AudioComponentType;
		Updated|=STATUS_AUDIOCOMPONENTTYPE;
		TRACE(TEXT("AudioComponentType = %x\n"),AudioComponentType);
	}

	bool fSpdifPassthrough=m_DtvEngine.m_MediaViewer.IsSpdifPassthrough();
	if (fSpdifPassthrough!=m_fSpdifPassthrough) {
		m_fSpdifPassthrough=fSpdifPassthrough;
		Updated|=STATUS_SPDIFPASSTHROUGH;
		TRACE(TEXT("S/PDIF passthrough %s\n"),fSpdifPassthrough?TEXT("ON"):TEXT("OFF"));
	}

	WORD EventID=m_DtvEngine.GetEventID();
	if (EventID!=m_EventID) {
		m_EventID=EventID;
		Updated|=STATUS_EVENTID;
		TRACE(TEXT("EventID = %d\n"),EventID);
	}

	return Updated;
}


DWORD CCoreEngine::UpdateStatistics()
{
	DWORD Updated=0;

	DWORD ErrorCount=m_DtvEngine.m_TsPacketParser.GetErrorPacketCount();
	if (ErrorCount!=m_ErrorPacketCount) {
		m_ErrorPacketCount=ErrorCount;
		Updated|=STATISTIC_ERRORPACKETCOUNT;
	}
	DWORD ContinuityErrorCount=m_DtvEngine.m_TsPacketParser.GetContinuityErrorPacketCount();
	if (ContinuityErrorCount!=m_ContinuityErrorPacketCount) {
		m_ContinuityErrorPacketCount=ContinuityErrorCount;
		Updated|=STATISTIC_CONTINUITYERRORPACKETCOUNT;
	}
	DWORD ScrambleCount=m_DtvEngine.m_TsDescrambler.GetScramblePacketCount();
	if (ScrambleCount!=m_ScramblePacketCount) {
		m_ScramblePacketCount=ScrambleCount;
		Updated|=STATISTIC_SCRAMBLEPACKETCOUNT;
	}
	float SignalLevel=m_DtvEngine.m_BonSrcDecoder.GetSignalLevel();
	DWORD BitRate=m_DtvEngine.m_BonSrcDecoder.GetBitRate();
	DWORD StreamRemain=m_DtvEngine.m_BonSrcDecoder.GetStreamRemain();
	if (SignalLevel!=m_SignalLevel) {
		m_SignalLevel=SignalLevel;
		Updated|=STATISTIC_SIGNALLEVEL;
	}
	if (BitRate!=m_BitRate) {
		m_BitRate=BitRate;
		Updated|=STATISTIC_BITRATE;
	}
	if (StreamRemain!=m_StreamRemain) {
		m_StreamRemain=StreamRemain;
		Updated|=STATISTIC_STREAMREMAIN;
	}
	DWORD BufferUsedCount=m_DtvEngine.m_MediaBuffer.GetUsedBufferCount();
	if (BufferUsedCount!=m_PacketBufferUsedCount) {
		m_PacketBufferUsedCount=BufferUsedCount;
		Updated|=STATISTIC_PACKETBUFFERRATE;
	}
	return Updated;
}


void CCoreEngine::ResetErrorCount()
{
	m_DtvEngine.m_TsPacketParser.ResetErrorPacketCount();
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
	m_DtvEngine.m_TsDescrambler.ResetScramblePacketCount();
	m_ScramblePacketCount=0;
}


int CCoreEngine::GetPacketBufferUsedPercentage()
{
	return m_DtvEngine.m_MediaBuffer.GetUsedBufferCount()*100/
								m_DtvEngine.m_MediaBuffer.GetBufferLength();
}


bool CCoreEngine::GetCurrentEventInfo(CEventInfoData *pInfo,WORD ServiceID,bool fNext)
{
	if (pInfo==NULL)
		return false;

	CTsAnalyzer::EventInfo EventInfo;
	TCHAR szEventName[256],szEventText[256],szEventExtText[2048];

	EventInfo.pszEventName=szEventName;
	EventInfo.MaxEventName=lengthof(szEventName);
	EventInfo.pszEventText=szEventText;
	EventInfo.MaxEventText=lengthof(szEventText);
	EventInfo.pszEventExtendedText=szEventExtText;
	EventInfo.MaxEventExtendedText=lengthof(szEventExtText);
	if (ServiceID==0xFFFF) {
		if (!m_DtvEngine.GetServiceID(&ServiceID))
			return false;
	}
	int ServiceIndex=m_DtvEngine.m_TsAnalyzer.GetServiceIndexByID(ServiceID);
	if (ServiceIndex<0
			|| !m_DtvEngine.m_TsAnalyzer.GetEventInfo(ServiceIndex ,&EventInfo, true, fNext))
		return false;

	pInfo->m_NetworkID=m_DtvEngine.m_TsAnalyzer.GetNetworkID();
	pInfo->m_TSID=m_DtvEngine.m_TsAnalyzer.GetTransportStreamID();
	m_DtvEngine.GetServiceID(&pInfo->m_ServiceID);
	pInfo->m_EventID=EventInfo.EventID;
	pInfo->m_fValidStartTime=EventInfo.bValidStartTime;
	if (EventInfo.bValidStartTime)
		pInfo->m_stStartTime=EventInfo.StartTime;
	pInfo->m_DurationSec=EventInfo.Duration;
	pInfo->m_RunningStatus=EventInfo.RunningStatus;
	pInfo->m_FreeCaMode=EventInfo.bFreeCaMode?
		CEventInfoData::FREE_CA_MODE_SCRAMBLED:CEventInfoData::FREE_CA_MODE_UNSCRAMBLED;
	pInfo->m_VideoInfo.StreamContent=EventInfo.Video.StreamContent;
	pInfo->m_VideoInfo.ComponentType=EventInfo.Video.ComponentType;
	pInfo->m_VideoInfo.ComponentTag=EventInfo.Video.ComponentTag;
	pInfo->m_VideoInfo.LanguageCode=EventInfo.Video.LanguageCode;
	::lstrcpyn(pInfo->m_VideoInfo.szText,EventInfo.Video.szText,CEventInfoData::VideoInfo::MAX_TEXT);
	pInfo->m_AudioList.resize(EventInfo.Audio.size());
	for (size_t i=0;i<EventInfo.Audio.size();i++) {
		pInfo->m_AudioList[i].StreamContent=EventInfo.Audio[i].StreamContent;
		pInfo->m_AudioList[i].ComponentType=EventInfo.Audio[i].ComponentType;
		pInfo->m_AudioList[i].ComponentTag=EventInfo.Audio[i].ComponentTag;
		pInfo->m_AudioList[i].SimulcastGroupTag=EventInfo.Audio[i].SimulcastGroupTag;
		pInfo->m_AudioList[i].bESMultiLingualFlag=EventInfo.Audio[i].bESMultiLingualFlag;
		pInfo->m_AudioList[i].bMainComponentFlag=EventInfo.Audio[i].bMainComponentFlag;
		pInfo->m_AudioList[i].QualityIndicator=EventInfo.Audio[i].QualityIndicator;
		pInfo->m_AudioList[i].SamplingRate=EventInfo.Audio[i].SamplingRate;
		pInfo->m_AudioList[i].LanguageCode=EventInfo.Audio[i].LanguageCode;
		pInfo->m_AudioList[i].LanguageCode2=EventInfo.Audio[i].LanguageCode2;
		::lstrcpyn(pInfo->m_AudioList[i].szText,EventInfo.Audio[i].szText,CEventInfoData::AudioInfo::MAX_TEXT);
	}
	pInfo->SetEventName(szEventName[0]!='\0'?szEventName:NULL);
	pInfo->SetEventText(szEventText[0]!='\0'?szEventText:NULL);
	pInfo->SetEventExtText(szEventExtText[0]!='\0'?szEventExtText:NULL);
	pInfo->m_ContentNibble.NibbleCount=EventInfo.ContentNibble.NibbleCount;
	for (int i=0;i<EventInfo.ContentNibble.NibbleCount;i++)
		pInfo->m_ContentNibble.NibbleList[i]=EventInfo.ContentNibble.NibbleList[i];

	return true;
}


void *CCoreEngine::GetCurrentImage()
{
	BYTE *pDib;

	bool fPause=m_DtvEngine.m_MediaViewer.GetVideoRendererType()==CVideoRenderer::RENDERER_DEFAULT;

	if (fPause)
		m_DtvEngine.m_MediaViewer.Pause();
	if (!m_DtvEngine.m_MediaViewer.GetCurrentImage(&pDib))
		pDib=NULL;
	if (fPause)
		m_DtvEngine.m_MediaViewer.Play();
	return pDib;
}


bool CCoreEngine::SetMinTimerResolution(bool fMin)
{
	if ((m_TimerResolution!=0)!=fMin) {
		if (fMin) {
			TIMECAPS tc;

			if (::timeGetDevCaps(&tc,sizeof(tc))!=TIMERR_NOERROR)
				tc.wPeriodMin=1;
			if (::timeBeginPeriod(tc.wPeriodMin)!=TIMERR_NOERROR)
				return false;
			m_TimerResolution=tc.wPeriodMin;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Set %u\n"),m_TimerResolution);
		} else {
			::timeEndPeriod(m_TimerResolution);
			m_TimerResolution=0;
			TRACE(TEXT("CCoreEngine::SetMinTimerResolution() Reset\n"));
		}
	}
	return true;
}


LPCTSTR CCoreEngine::GetCardReaderSettingName(CardReaderType Type)
{
	static const LPCTSTR CardReaderList[] = {
		TEXT("�Ȃ� (�X�N�����u���������Ȃ�)"),
		TEXT("�X�}�[�g�J�[�h���[�_"),
		TEXT("HDUS�����J�[�h���[�_"),
		TEXT("BonCasClient"),
	};

	if (Type<CARDREADER_NONE || Type>CARDREADER_LAST)
		return NULL;
	return CardReaderList[Type];
}
