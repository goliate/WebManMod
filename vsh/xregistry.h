// Mysis xRegistry.h v0.1
typedef struct
{
	int (*GetProductCode)(void);
	int (*GetProductSubCode)(void); // Model Type, Mobo Id
	int (*GetUnk1)(void *) ; // uint8_t [0x1C] { hdmi, ieee802.11, msslot, sdslot, cfslot }
	int (*SetUnk2)(void *) ;
	int (*GetEnterButtonAssign)(int *) ;
	int (*SetEnterButtonAssign)(int);
	int (*GetLicenseArea)(int *) ;
	int (*SetSystemInitialize)(int);
	int (*GetSystemInitialize)(int *);
	int (*SetSystemLanguage)(int) ;
	int (*GetSystemLanguage)(int *);
	int (*SetSystemNickname)(char *);
	int (*GetSystemNickname)(char *,int *); // nick, length
	int (*SetSystemCharacterCodeOem)(int) ;
	int (*GetSystemCharacterCodeOem)(int *) ;
	int (*SetSystemCharacterCodeOemValue)(int) ;
	int (*GetSystemCharacterCodeOemValue)(int *) ;
	int (*SetsystemCharacterCodeAnsi)(int) ;
	int (*GetSystemCharacterCodeAnsi)(int *) ;
	int (*ResetNicknameDvdRegionDlnaFlag)(void) ;
	int (*SetSystemNotificationEnabled)(int) ;
	int (*GetSystemNotificationEnabled)(int *) ;
	int (*SetSystemDiscBootFirstEnabled)(int) ;
	int (*GetSystemDiscBootFirstEnabled)(int *) ;
	int (*SetSystemSoundEffectEnabled)(int) ;
	int (*GetSystemSoundEffectEnabled)(int *) ;
	int (*unk_new)(void *, void *) ;
	int (*unk_delete)(void *, void *) ;
} xsetting_AF1F161_class;

typedef struct
{
} xsetting_154430FC_class;

typedef struct
{
	 int (*SetBdMenuLanguage)(int);
	 int (*GetBdMenuLanguage)(int *);
	 int (*SetBdSoundLanguage)(int);
	 int (*GetBdSoundLanguage)(int *);
	 int (*SetBdCaptionLanguage)(int);
	 int (*GetBdCaptionLanguage)(int *);
	 int (*Set_unk6)(int);
	 int (*Get_unk7)(int *);
	 int (*SetDvdMenuLanguage)(int);
	 int (*GetDvdMenuLanguage)(int *);
	 int (*SetDvdSoundLanguage)(int);
	 int (*GetDvdSoundLanguage)(int *);
	 int (*SetDvdCaptionLanguage)(int);
	 int (*GetDvdCaptionLanguage)(int *);
	 int (*Set_unk14)(int);
	 int (*Get_unk15)(int *);
	 int (*SetFnr)(int);
	 int (*GetFnr)(int *);
	 int (*SetBnr)(int);
	 int (*GetBnr)(int *);
	 int (*SetMnr)(int);
	 int (*GetMnr)(int *);
	 int (*SetFnrForDvdRom)(int);
	 int (*GetFnrForDvdRom)(int *);
	 int (*SetBnrForDvdRom)(int);
	 int (*GetBnrForDvdRom)(int *);
	 int (*SetMnrForDvdRom)(int);
	 int (*GetMnrForDvdRom)(int *);
	 int (*SetCinemaConversion)(int);
	 int (*GetCinemaConversion)(int *);
	 int (*SetVolume)(int);
	 int (*GetVolume)(int *);
	 int (*SetDvdWideScreen)(int);
	 int (*GetDvdWideScreen)(int *);
	 int (*SetDvdRegionCode)(int);
	 int (*GetDvdRegionCode)(int *);
	 int (*SetBdRegionCode)(int);
	 int (*GetBdRegionCode)(int *);
	 int (*SetDvdTvSystem)(int);
	 int (*GetDvdTvSystem)(int *);
	 int (*SetDvdUpConvert)(int);
	 int (*GetDvdUpConvert)(int *);
	 int (*SetDrc)(int);
	 int (*GetDrc)(int *);
	 int (*GetColorButtonAssign)(int *);
	 int (*SetNetworkConnect)(int);
	 int (*GetNetworkConnect)(int *);
	 int (*GetSoundFormatHdmi)(int *);
	 int (*SetSoundFormatHdmi)(int);
	 int (*GetVideoFormatHdmiColorSpace)(int *);
	 int (*SetVideoFormatHdmiColorSpace)(int);
	 int (*GetVideoFormatHdmi1080_24p)(int *);
	 int (*SetVideoFormatHdmi1080_24p)(int);
} xsetting_16A8A805_class;

class xsetting_1D6D60D4_class
{public:};
class xsetting_2FD1B113_class
{public:};
class xsetting_43F98936_class
{public:};
class xsetting_4712F276_class
{public:};

class xsetting_58560CA4_class
{
public:
	 int (*GetVideoScreenMode)(int *) ;
	 int (*SetVideoScreenMode)(int) ;
	 int (*GetVideoOutputChannel)(int *) ;
	 int (*SetVideoOutputChannel)(int) ;
	 int (*GetVideoPlayerVolume)(int *) ;
	 int (*SetVideoPlayerVolume)(int) ;
};

class xsetting_5FB90B89_class
{public:};

class xsetting_660ECC35_class
{
public:
	 int (*SetPhotoNormalScenario)(int) ;
	 int (*GetPhotoNormalScenario)(int *) ;
	 int (*SetPhotoSlideScenario)(int) ;
	 int (*GetPhotoSlideScenario)(int *) ;
	 int (*SetPhotoSlideInterval)(int) ;
	 int (*GetPhotoSlideInterval)(int *) ;
	 int (*SetPhotoSlideRepeat)(int) ;
	 int (*GetPhotoSlideRepeat)(int *) ;
};

class xsetting_69C19C7F_class
{public:};

class xsetting_7125FEB5_class
{
public:
	 int (*SaveMusicCodec)(int) ;
	 int (*loadMusicCodec)(int *) ;
	 int (*SaveBitRateAac)(int) ;
	 int (*loadBitRateAac)(int *) ;
	 int (*SaveBitRateMp3)(int) ;
	 int (*loadBitRateMp3)(int* ) ;
	 int (*SaveBitRateAtrac)(int) ;
	 int (*loadBitRateAtrac)(int* ) ;
	 int (*SaveCrossFadePlay)(int) ;
	 int (*loadCrossFadePlay)(int* ) ;
	 int (*SaveRepeatMode)(int) ;
	 int (*loadRepeatMode)(int* ) ;
	 int (*SaveShuffleMode)(int) ;
	 int (*loadShuffleMode)(int* ) ;
	 int (*SaveVisualizerSelect)(int) ;
	 int (*loadVisualizerSelect)(int* ) ;
	 int (*SaveCddaEula)(int) ;
	 int (*loadCddaEula)(int* ) ;
	 int (*SaveCddaServer)(int) ;
	 int (*loadCddaServer)(int* ) ;
	 int (*SaveAudioplayerVolume)(int) ;
	 int (*loadAudioplayerVolume)(int* ) ;
	 int (*SaveSacdPlayerVolume)(int) ;
	 int (*loadSacdPlayerVolume)(int* ) ;
	 int (*SaveAtracActivation)(int) ;
	 int (*loadAtracActivation)(int* ) ;
	 int (*SaveWmaActivation)(int) ;
	 int (*loadWmaActivation)(int* ) ;
	 int (*SaveCdOutputFs)(int) ;
	 int (*loadCdOutputFs)(int* ) ;
	 int (*SaveDitherSetting)(int) ;
	 int (*loadDitherSetting)(int* ) ;
	 int (*SaveBitmappingSetting)(int) ;
	 int (*loadBitmappingSetting)(int* ) ;
};

class xsetting_7EDDAD29_class
{
public:
	 int (*loadParentalInformation)(void *);// uint8_t [0x20] { parental passwordNumber, bdPc, bdPcAge, dvdRegion, dvdLevel, gameLevel, browserStartControl }
	 int (*SaveDvdRegion)(char *) ;
	 int (*SaveBdPc)(int) ;
	 int (*SaveBdPcAge)(int) ;
	 int (*SaveDvdRegionWithCheck)(int) ;
	 int (*SaveDvdLevel)(int) ;
	 int (*SaveGameLevel)(int) ;
	 int (*SaveBrowserStartControl)(int) ;
};

class xsetting_8B69F85A_class
{
public:
	 int (*loadDateTimeInformation)(void *);//uint8_t [0x18] {diffTime, dateFormat, timeFormat, timeZone, summerTime }
     int (*SaveDiffTime)(int);
     int (*SaveDateFormat)(int);
	 int (*SaveTimeFormat)(int);
	 int (*SaveTimeZone)(int);
	 int (*SaveSummerTime)(int);
	//and more
};

class xsetting_9EE60B4E_class
{public:};

class xsetting_C1008335_class
{public:};

class xsetting_CC56EB2D_class
{
public:
	int (*DoUnk0)(void);
	int (*DoUnk1)(void);
	int (*DoUnk2)(void);
	int (*DoUnk3)(void);
	int (*DoUnk4)(void);
	int (*DoUnk5)(void);
	int (*DoUnk6)(void);
	int (*DoUnk7)(void);
	int (*DoUnk8)(void);
	int (*DoUnk9)(void);
	int (*DoUnk10)(void);
	int (*DoUnk11)(void);
	int (*GetCurrentUserNumber)(void);
	int (*DoUnk13)(void);
	int (*DoUnk14)(void);
	int (*DoUnk15)(void);
	int (*DoUnk16_GetRegistryValue)(int userid,int packetid,int * value);
	int (*DoUnk17_GetRegistryString)(int userid,int packetid,char * value, int maxlen);
	int (*DoUnk18_SetRegistryValue)(int userid,int packetid,int value);
	int (*DoUnk19_SetRegistryString)(int userid,int packetid,char * value, int maxlen);
	int (*DoUnk20)(void);
	int (*DoUnk21)(void);
	int (*DoUnk22)(void);
	int (*DoUnk23)(void);
	int (*DoUnk24)(void);
	int (*DoUnk25)(void);
	int (*DoUnk26)(void);
	int (*DoUnk27)(void);
	int (*DoUnk28)(void);
	int (*DoUnk29)(void);
	int (*DoUnk30)(void);
	int (*DoUnk31)(void);
	int (*DoUnk32_GetRegistryHomePath)(int userid,char * path);
	int (*DoUnk33)(void);
	int (*DoUnk34)(void);
	int (*DoUnk35)(void);
	int (*DoUnk36)(void);
	int (*DoUnk37)(void);
	int (*DoUnk38)(void);
	int (*DoUnk39)(void);
	int (*DoUnk40)(void);
	int (*DoUnk41)(void);
	int (*DoUnk42)(void);

	int GetRegistryKeyboardType()	{int v; DoUnk16_GetRegistryValue(0,0x14, &v);return v;}
	int GetRegistryKeyboardJapaneseInput()	{int v; DoUnk16_GetRegistryValue(0,0x15, &v);return v;}
	int GetRegistryKeyboardTradChineseInput()	{int v; DoUnk16_GetRegistryValue(0,0x16, &v);return v;}
	int GetRegistryKeyboardBind()	{int v; DoUnk16_GetRegistryValue(0,0x17, &v);return v;}
	int GetRegistryKeyboardRepeatStartingTime()	{int v; DoUnk16_GetRegistryValue(0,0x18, &v);return v;}
	int GetRegistryKeyboardRepeadSpeed()	{int v; DoUnk16_GetRegistryValue(0,0x19, &v);return v;}
	int GetRegistryMouseType()	{int v; DoUnk16_GetRegistryValue(0,0x1E, &v);return v;}
	int GetRegistryMouseSpeed()	{int v; DoUnk16_GetRegistryValue(0,0x1F, &v);return v;}
	int GetRegistryBrowserHomeUri(char * uri)	{return DoUnk17_GetRegistryString(0,0x28,uri,0x80);}
	int GetRegistryBrowserCookieMode()	{int v; DoUnk16_GetRegistryValue(0,0x29, &v);return v;}
	int GetRegistryBrowserJavascript()	{int v; DoUnk16_GetRegistryValue(0,0x2A, &v);return v;}
	int GetRegistryBrowserDisplayMode()	{int v; DoUnk16_GetRegistryValue(0,0x2B, &v);return v;}
	int GetRegistryBrowserExitConfirmation()	{int v; DoUnk16_GetRegistryValue(0,0x2C, &v);return v;}
	int GetRegistryBrowserOverscan()	{int v; DoUnk16_GetRegistryValue(0,0x2D, &v);return v;}
	int GetRegistryBrowserWindowSize()	{int v; DoUnk16_GetRegistryValue(0,0x2E, &v);return v;}
	int GetRegistryBrowserFontSize()	{int v; DoUnk16_GetRegistryValue(0,0x2F, &v);return v;}
	int GetRegistryBrowserEncoding()	{int v; DoUnk16_GetRegistryValue(0,0x30, &v);return v;}
	int GetRegistryBrowserCacheSize()	{int v; DoUnk16_GetRegistryValue(0,0x31, &v);return v;}
	int GetRegistryBrowserRestoreWindow()	{int v; DoUnk16_GetRegistryValue(0,0x32, &v);return v;}
	int GetRegistryBrowserPopupBlock()	{int v; DoUnk16_GetRegistryValue(0,0x33, &v);return v;}
	int GetRegistryBrowserDpi()	{int v; DoUnk16_GetRegistryValue(0,0x34, &v);return v;}
	int GetRegistryBrowserInterlaceFilter()	{int v; DoUnk16_GetRegistryValue(0,0x35, &v);return v;}
	int GetRegistryBrowserIfilter()	{int v; DoUnk16_GetRegistryValue(0,0x36, &v);return v;}
	int GetRegistryBrowserIfilterAddress()	{int v; DoUnk16_GetRegistryValue(0,0x37, &v);return v;}
	int GetRegistryBrowserIfilterPort()	{int v; DoUnk16_GetRegistryValue(0,0x38, &v);return v;}
	int GetRegistryBrowserIfilterUsername()	{int v; DoUnk16_GetRegistryValue(0,0x39, &v);return v;}
	int GetRegistryBrowserIfilterPassword()	{int v; DoUnk16_GetRegistryValue(0,0x3A, &v);return v;}
	int GetRegistryBrowserIfilterBase64()	{int v; DoUnk16_GetRegistryValue(0,0x3B, &v);return v;}
	int GetRegistryBrowserTrendEula()	{int v; DoUnk16_GetRegistryValue(0,0x3C, &v);return v;}
	int GetRegistryBrowserTrendEnable()	{int v; DoUnk16_GetRegistryValue(0,0x3D, &v);return v;}
	int GetRegistryInputForecastDictionary()	{int v; DoUnk16_GetRegistryValue(0,0x46, &v);return v;}
	int GetRegistryInputTenKeyInputType()	{int v; DoUnk16_GetRegistryValue(0,0x47, &v);return v;}
	int GetRegistryInputForecastDictionaryCh()	{int v; DoUnk16_GetRegistryValue(0,0x48, &v);return v;}
	int GetRegistryInputKeyLayoutType()	{int v; DoUnk16_GetRegistryValue(0,0x49, &v);return v;}
	int GetRegistryEdyEula()	{int v; DoUnk16_GetRegistryValue(0,0x50, &v);return v;}
	int GetRegistryThemeWallpaper()	{int v; DoUnk16_GetRegistryValue(0,0x5A, &v);return v;}
	int GetRegistryThemeFont()	{int v; DoUnk16_GetRegistryValue(0,0x5B, &v);return v;}
	int GetRegistryThemeFontEu()	{int v; DoUnk16_GetRegistryValue(0,0x5C, &v);return v;}
	int GetRegistryThemeFontRu()	{int v; DoUnk16_GetRegistryValue(0,0x5D, &v);return v;}
	int GetRegistryThemeFile(char * f)	{return DoUnk17_GetRegistryString(0,0x5E,f,0x80);}
	int GetRegistryThemeColor()	{int v; DoUnk16_GetRegistryValue(0,0x5F, &v);return v;}
	int GetRegistryThemeEndarkbg()	{int v; DoUnk16_GetRegistryValue(0,0x60, &v);return v;}
	int GetRegistryMicPitchshift()	{int v; DoUnk16_GetRegistryValue(0,0x64, &v);return v;}
	int GetRegistryUserRestoreSignInPassword(char * p)	{return DoUnk17_GetRegistryString(0,0x6E,p,0x80);}
	int GetRegistryUserRestoreSignInStatus()	{int v; DoUnk16_GetRegistryValue(0,0x6F, &v);return v;}
	int GetRegistryYtaccountAccountid(char * a)	{return DoUnk17_GetRegistryString(0,0x78,a,0x80);}
	int GetRegistryYtaccountPassword(char * p)	{return DoUnk17_GetRegistryString(0,0x79,p,0x80);}
	int GetRegistryNpGuestCountry(char * c)	{return DoUnk17_GetRegistryString(0,0x82,c,0x80);}
	int GetRegistryNpGuestLang(char * l)	{return DoUnk17_GetRegistryString(0,0x83,l,0x80);}
	int GetRegistryNpGuestBirth()	{int v; DoUnk16_GetRegistryValue(0,0x84, &v);return v;}
	int GetRegistryWBoardEnable()	{int v; DoUnk16_GetRegistryValue(0,0x8C, &v);return v;}
	int GetRegistryWBoardFocusMask(){int v; DoUnk16_GetRegistryValue(0,0x8D, &v);return v;}
	int GetRegistryNetOnlineFlag()	{int v; DoUnk16_GetRegistryValue(0,0x96, &v);return v;}
	int GetRegistryFacebookAccount(char * a)	{return DoUnk17_GetRegistryString(0,0xC8,a,0x80);}
	int GetRegistryFacebookSessionKey()	{int v; DoUnk16_GetRegistryValue(0,0xC9, &v);return v;}
	int GetRegistryFacebookSessionSecret()	{int v; DoUnk16_GetRegistryValue(0,0xCA, &v);return v;}
	int GetRegistryFacebookUid()	{int v; DoUnk16_GetRegistryValue(0,0xCB, &v);return v;}
	int GetRegistryFacebookTrophy()	{int v; DoUnk16_GetRegistryValue(0,0xCC, &v);return v;}
	int GetRegistryFacebookPurchase()	{int v; DoUnk16_GetRegistryValue(0,0xCD, &v);return v;}
	int GetRegistryFacebookGameEvent()	{int v; DoUnk16_GetRegistryValue(0,0xCE, &v);return v;}
	int GetRegistryFacebookHknwEnable()	{int v; DoUnk16_GetRegistryValue(0,0xCF, &v);return v;}
	int GetRegistryFacebookAccessToken()	{int v; DoUnk16_GetRegistryValue(0,0xD0, &v);return v;}
	int GetRegistryFacebookGameIntegrations()	{int v; DoUnk16_GetRegistryValue(0,0xD1, &v);return v;}
	int GetRegistryPicasaMail(char * m)	{return DoUnk17_GetRegistryString(0,0xD2,m,0x80);}
	int GetRegistryPicasaPassword(char * p)	{return DoUnk17_GetRegistryString(0,0xD3,p,0x80);}
	int GetRegistryPicasaHknwEnable()	{int v; DoUnk16_GetRegistryValue(0,0xD4, &v);return v;}
	int GetRegistryFlickrMail(char * m)	{return DoUnk17_GetRegistryString(0,0xDC,m,0x80);}
	int GetRegistryFlickrPassword(char * p)	{return DoUnk17_GetRegistryString(0,0xDD,p,0x80);}
	int GetRegistryFacebookRating()	{int v; DoUnk16_GetRegistryValue(0,0xE6, &v);return v;}
	int SetRegistryWBoardFocusMask(int v){ return DoUnk18_SetRegistryValue(0,0x8D, v);}
};

class xsetting_CE27E884_class
{
public:
	 int (*savePremoPSPInformation)(void *) ; // uint8_t [0x4B8]
	 int (*loadPremoPSPInformation)(void *) ; // uint8_t [0x4B8]
	 int (*saveRemoteBoot)(int) ;
	 int (*loadRemoteBoot)(int *) ;
	 int (*saveBootCount)(int) ;
	 int (*loadBootCount)(int *) ;
	 int (*savePowerOffTime)(void *) ;
	 int (*loadPowerOffTime)(void *) ;
	 int (*saveAudioConfig)(int) ;
	 int (*loadAudioConfig)(int *) ;
};

class xsetting_D0261D72_class
{
public:
	int (*saveAvcInitialCameraMode)(int) ;
	int (*loadAvcInitialCameraMode)(int *) ;
	int (*saveNpEnvironment)(char *,int *) ; // env, len
	int (*loadNpEnvironment)(char *,int *) ; // env, len
	int (*saveRegistryIntValue)(int, int) ; // id, value
	int (*loadRegistryIntValue)(int, int*) ; // id, value
	int (*saveRegistryStringValue)(int,char *, int) ; // id, string, len
	int (*loadRegistryStringValue)(int,char *, int) ; // id, string, len
	int (*Setunk1)(int) ;
	int (*Getunk2)(int, int *) ;
	int (*Setunk3)(int, int) ;

	int loadRegistryAvcVgaLastBitrate()	{int v; loadRegistryIntValue(0x00, &v);return v;}
	int loadRegistryGameLevel0Control()	{int v; loadRegistryIntValue(0x01, &v);return v;}
	int loadRegistryNetworkServiceControl()	{int v; loadRegistryIntValue(0x02, &v);return v;}
	int loadRegistryCddaServer()	{int v; loadRegistryIntValue(0x03, &v);return v;}
	int loadRegistryGameBgmPlayback()	{int v; loadRegistryIntValue(0x04, &v);return v;}
	int loadRegistryGameBgmVolume()	{int v; loadRegistryIntValue(0x05, &v);return v;}
	int loadRegistryDummyBgmPlayer()	{int v; loadRegistryIntValue(0x06, &v);return v;}
	int loadRegistryDynamicNormalizer()	{int v; loadRegistryIntValue(0x07, &v);return v;}
	int loadRegistryNpDebug()	{int v; loadRegistryIntValue(0x08, &v);return v;}
	int loadRegistryNpTitleId(char * titleid,int max_len)	{return loadRegistryStringValue(0x09,titleid,max_len);}
	int loadRegistryNavOnly()	{int v; loadRegistryIntValue(0x0A, &v);return v;}
	int loadRegistryNpAdClockDiff()	{int v; loadRegistryIntValue(0x0B, &v);return v;}
	int loadRegistryDebugDrmError()	{int v; loadRegistryIntValue(0x0C, &v);return v;}
	int loadRegistryDebugDrmClock()	{int v; loadRegistryIntValue(0x0D, &v);return v;}
	int loadRegistryDebugConsoleBind()	{int v; loadRegistryIntValue(0x0E, &v);return v;}
	int loadRegistryDebugIngameCommerce2()	{int v; loadRegistryIntValue(0x0F, &v);return v;}
	int loadRegistryDebugSFForce()	{int v; loadRegistryIntValue(0x10, &v);return v;}
	int loadRegistryNpGeoFiltering()	{int v; loadRegistryIntValue(0x11, &v);return v;}
	int loadRegistryGameUpdateImposeTest()	{int v; loadRegistryIntValue(0x12, &v);return v;}
	int loadRegistryGameUpdateForceOverwrite()	{int v; loadRegistryIntValue(0x13, &v);return v;}
	int loadRegistryFakeNpSnsThrottle()	{int v; loadRegistryIntValue(0x14, &v);return v;}
	int loadRegistryFakeNpSnsThrottleWaitSeconds()	{int v; loadRegistryIntValue(0x15, &v);return v;}
	int loadRegistryTppsProxyFlag()	{int v; loadRegistryIntValue(0x16, &v);return v;}
	int loadRegistryTppsProxyServer()	{int v; loadRegistryIntValue(0x17, &v);return v;} // questionable
	int loadRegistryTppsProxyPort()	{int v; loadRegistryIntValue(0x18, &v);return v;}
	int loadRegistryTppsProxyUserName(char * username,int max_len)	{return loadRegistryStringValue(0x19,username,max_len);}
	int loadRegistryTppsProxyPassword(char * password,int max_len)	{return loadRegistryStringValue(0x1A,password,max_len);}
	int loadRegistryRegion()	{int v; loadRegistryIntValue(0x1B, &v);return v;}
	int loadRegistryLicenseArea()	{int v; loadRegistryIntValue(0x1C, &v);return v;}
	int loadRegistryHddSerial(char * hddserial)	{return loadRegistryStringValue(0x1D,hddserial,0x3D);}
	int loadRegistryCoreDump()	{int v; loadRegistryIntValue(0x1E, &v);return v;}
	int loadRegistryCoreDumpOptionTrigger()	{int v; loadRegistryIntValue(0x1F, &v);return v;}
	int loadRegistryCoreDumpOptionFileGen()	{int v; loadRegistryIntValue(0x20, &v);return v;}
	int loadRegistryCoreDumpOptionExeCtrl()	{int v; loadRegistryIntValue(0x21, &v);return v;}
	int loadRegistryMatEnable()	{int v; loadRegistryIntValue(0x22, &v);return v;}
	int loadRegistryUpdateServerUrl(char * url,int max_len)	{return loadRegistryStringValue(0x23,url,max_len);}
	int loadRegistryFakeLimitSize()	{int v; loadRegistryIntValue(0x24, &v);return v;}
	int loadRegistryFakeFreeSpace()	{int v; loadRegistryIntValue(0x25, &v);return v;}
	int loadRegistryFakeSavedataOwner()	{int v; loadRegistryIntValue(0x26, &v);return v;}
	int loadRegistryFakeHddSpeed()	{int v; loadRegistryIntValue(0x27, &v);return v;}
	int loadRegistryDebugGameType()	{int v; loadRegistryIntValue(0x28, &v);return v;}
	int loadRegistryDebugBootPath()	{int v; loadRegistryIntValue(0x29, &v);return v;}
	int loadRegistryDebugDirName(char * path,int max_len)	{return loadRegistryStringValue(0x2A,path,max_len);}
	int loadRegistryAppHomeBootPath()	{int v; loadRegistryIntValue(0x2B, &v);return v;}
	int loadRegistryWolDex()	{int v; loadRegistryIntValue(0x2C, &v);return v;}
	int loadRegistryDispHddSpace()	{int v; loadRegistryIntValue(0x2D, &v);return v;}
	int loadRegistryAutoNetworkUpdate()	{int v; loadRegistryIntValue(0x2E, &v);return v;}
	int loadRegistryAutoPowerOff()	{int v; loadRegistryIntValue(0x2F, &v);return v;}
	int loadRegistryAutoPowerOffEx()	{int v; loadRegistryIntValue(0x30, &v);return v;}
	int loadRegistryAutoPowerOffDebug()	{int v; loadRegistryIntValue(0x31, &v);return v;}
	int loadRegistryHdmiControl()	{int v; loadRegistryIntValue(0x32, &v);return v;}
	int loadRegistryHdmiControlEx()	{int v; loadRegistryIntValue(0x33, &v);return v;}
	int loadRegistryPowerOnDiscBoot()	{int v; loadRegistryIntValue(0x34, &v);return v;}
	int loadRegistryPowerOnReset()	{int v; loadRegistryIntValue(0x35, &v);return v;}
	int loadRegistryDisable15Timeout()	{int v; loadRegistryIntValue(0x36, &v);return v;}
	int loadRegistryDebugSystemUpdate()	{int v; loadRegistryIntValue(0x37, &v);return v;}
	int loadRegistryFakePlus()	{int v; loadRegistryIntValue(0x38, &v);return v;}
	int loadRegistryTurnOffWarning()	{int v; loadRegistryIntValue(0x39, &v);return v;}
	int loadRegistryBootMode(char * bootmode,int max_len)	{return loadRegistryStringValue(0x3A,bootmode,max_len);}
	int loadRegistryCrashreportCrepo()	{int v; loadRegistryIntValue(0x3B, &v);return v;}
	int loadRegistryCrashreportReporterStatus()	{int v; loadRegistryIntValue(0x3C, &v);return v;}
	int loadRegistryCrashreportVshGeneratorEnableFlag()	{int v; loadRegistryIntValue(0x3D, &v);return v;}
	int loadRegistryDateTimeAutoCorrection()	{int v; loadRegistryIntValue(0x3E, &v);return v;}
	int loadRegistryAutobootStartTime()	{int v; loadRegistryIntValue(0x3F, &v);return v;}
	int loadRegistryEdyDebug()	{int v; loadRegistryIntValue(0x40, &v);return v;}
	int loadRegistryUpConvert()	{int v; loadRegistryIntValue(0x41, &v);return v;}
	int loadRegistryFnrLevel()	{int v; loadRegistryIntValue(0x42, &v);return v;}
	int loadRegistryBnrLevel()	{int v; loadRegistryIntValue(0x43, &v);return v;}
	int loadRegistryMnrLevel()	{int v; loadRegistryIntValue(0x44, &v);return v;}
	int loadRegistrySequentialPlay()	{int v; loadRegistryIntValue(0x45, &v);return v;}
	int loadRegistryHD50HzOutput()	{int v; loadRegistryIntValue(0x46, &v);return v;}
	int loadRegistryOutputExtMenu()	{int v; loadRegistryIntValue(0x47, &v);return v;}
	int loadRegistryOutputExtFunc()	{int v; loadRegistryIntValue(0x48, &v);return v;}
	int loadRegistryDtcpIpSettingMenu()	{int v; loadRegistryIntValue(0x49, &v);return v;}
	int loadRegistryDefaultSubTitleLanguage()	{int v; loadRegistryIntValue(0x4A, &v);return v;}
	int loadRegistryDefaultAudioLanguage()	{int v; loadRegistryIntValue(0x4B, &v);return v;}
	int loadRegistryEmuUpConvert()	{int v; loadRegistryIntValue(0x4C, &v);return v;}
	int loadRegistryEmuSmoothing()	{int v; loadRegistryIntValue(0x4D, &v);return v;}
	int loadRegistryMinisUpConvert()	{int v; loadRegistryIntValue(0x4E, &v);return v;}
	int loadRegistryPspemuViewmode()	{int v; loadRegistryIntValue(0x4F, &v);return v;}
	int loadRegistryPspemu3dDisplay()	{int v; loadRegistryIntValue(0x50, &v);return v;}
	int loadRegistryPspemu3dDepthAdjust()	{int v; loadRegistryIntValue(0x51, &v);return v;}
	int loadRegistryPspemu3dMenu()	{int v; loadRegistryIntValue(0x52, &v);return v;}
	int loadRegistryPspemuAdhocModeWlan()	{int v; loadRegistryIntValue(0x53, &v);return v;}
	int loadRegistryPspemuAdhocModeCh()	{int v; loadRegistryIntValue(0x54, &v);return v;}
	int loadRegistryPs2emuSaveUtility()	{int v; loadRegistryIntValue(0x55, &v);return v;}
	int loadRegistryPs2softemuFunc()	{int v; loadRegistryIntValue(0x56, &v);return v;}
	int loadRegistryPs2BgCaution()	{int v; loadRegistryIntValue(0x57, &v);return v;}
	int loadRegistryCameraPlfreq()	{int v; loadRegistryIntValue(0x58, &v);return v;}
	int loadRegistryTvCategory()	{int v; loadRegistryIntValue(0x59, &v);return v;}
	int loadRegistryHomeInstaller()	{int v; loadRegistryIntValue(0x5A, &v);return v;}
	int loadRegistryHomeQAMode()	{int v; loadRegistryIntValue(0x5B, &v);return v;}
	int loadRegistryDummyInGameXMB()	{int v; loadRegistryIntValue(0x5C, &v);return v;}
	int loadRegistryYconExplained()	{int v; loadRegistryIntValue(0x5D, &v);return v;}
	int loadRegistryXaiDebugFlag()	{int v; loadRegistryIntValue(0x5E, &v);return v;}
	int loadRegistryAdServerURL(char * url,int max_len)	{return loadRegistryStringValue(0x5F,url,max_len);}
	int loadRegistryAdCatalogVersion(char * version,int max_len)	{return loadRegistryStringValue(0x60,version,max_len);}
	int loadRegistryAdEnableNotification()	{int v; loadRegistryIntValue(0x61, &v);return v;}
	int loadRegistryUploadDebug()	{int v; loadRegistryIntValue(0x62, &v);return v;}
	int loadRegistryNetAutoDlDebug()	{int v; loadRegistryIntValue(0x63, &v);return v;}
	int loadRegistryNetAutoDlFlag()	{int v; loadRegistryIntValue(0x64, &v);return v;}
	int loadRegistryNetAutoDlTime()	{int v; loadRegistryIntValue(0x65, &v);return v;}
	int loadRegistryNetAutoDlFunc()	{int v; loadRegistryIntValue(0x66, &v);return v;}
	int loadRegistryNetEmulationType()	{int v; loadRegistryIntValue(0x67, &v);return v;}   // questionable
	int loadRegistryNetAdhocSsidPrefix(char * prefix,int max_len)	{return loadRegistryStringValue(0x68,prefix,max_len);}
	int loadRegistryPadVibrationEnable()	{int v; loadRegistryIntValue(0x69, &v);return v;}
	int loadRegistryPadAutoPowerOff()	{int v; loadRegistryIntValue(0x6A, &v);return v;}
	int loadRegistryPadMagnetometer()	{int v; loadRegistryIntValue(0x6B, &v);return v;}
	int loadRegistrySound0Initial()	{int v; loadRegistryIntValue(0x6C, &v);return v;}
	int loadRegistrySound1UsbHeadSetSound()	{int v; loadRegistryIntValue(0x6D, &v);return v;}   // questionable
	int loadRegistryDlnaFlag()	{int v; loadRegistryIntValue(0x6E, &v);return v;}
	int loadRegistryDlnaDtcpipDevCert()	{int v; loadRegistryIntValue(0x6F, &v);return v;}   // questionable
	int loadRegistryBrowserTrendEula()	{int v; loadRegistryIntValue(0x70, &v);return v;}
	int loadRegistryBrowserTrendEnable()	{int v; loadRegistryIntValue(0x71, &v);return v;}
	int loadRegistryBrowserTrendLastTime()	{int v; loadRegistryIntValue(0x72, &v);return v;}
	int loadRegistryBrowserTrendTtl()	{int v; loadRegistryIntValue(0x73, &v);return v;}
	int loadRegistryBrowserTrendRegistered()	{int v; loadRegistryIntValue(0x74, &v);return v;}
	int loadRegistryBrowserHeapSize()	{int v; loadRegistryIntValue(0x75, &v);return v;}
	int loadRegistryBrowserDebugMenu()	{int v; loadRegistryIntValue(0x76, &v);return v;}
	int loadRegistryBrowserType()	{int v; loadRegistryIntValue(0x77, &v);return v;}
	int loadRegistryWboardBaseUri(char * uri,int max_len)	{return loadRegistryStringValue(0x68,uri,max_len);}
	int loadRegistrySmssTargetServer()	{int v; loadRegistryIntValue(0x79, &v);return v;}
	int loadRegistrySmssResultOutput()	{int v; loadRegistryIntValue(0x7A, &v);return v;}
	int loadRegistryDisplayForceEnable3D()	{int v; loadRegistryIntValue(0x7B, &v);return v;}
	int loadRegistryDisplayScreenSize()	{int v; loadRegistryIntValue(0x7C, &v);return v;}
	int loadRegistryDisplayDeepColor()	{int v; loadRegistryIntValue(0x7D, &v);return v;}
};

typedef struct
{
} xsetting_ED5B559F_class;

typedef struct
{
} xsetting_F48C0548_class;

typedef struct
{
} xsetting_FFAF9B19_class;

// xRegistry
xsetting_AF1F161_class* (*xsetting_AF1F161)() = 0;
xsetting_16A8A805_class* (*xsetting_16A8A805)() = 0;
xsetting_43F98936_class* (*xsetting_43F98936)() = 0;
xsetting_4712F276_class* (*xsetting_4712F276)() = 0;
xsetting_58560CA4_class* (*xsetting_58560CA4)() = 0;
xsetting_660ECC35_class* (*xsetting_660ECC35)() = 0;
xsetting_7125FEB5_class* (*xsetting_7125FEB5)() = 0;
xsetting_7EDDAD29_class* (*xsetting_7EDDAD29)() = 0;
xsetting_8B69F85A_class* (*xsetting_8B69F85A)() = 0;
xsetting_CC56EB2D_class* (*xsetting_CC56EB2D)() = 0;
xsetting_CE27E884_class* (*xsetting_CE27E884)() = 0;
xsetting_D0261D72_class* (*xsetting_D0261D72)() = 0;

