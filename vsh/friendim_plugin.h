// Mysis friend_plugin.h v0.1
typedef struct
{
	int (*SetFriendimExit)();
	int (*PluginRecvLaunch)();
	int (*PluginSendLaunch)();
	int (*PluginNewMessageLaunch)();
	int (*PluginFriendAddNormalLaunch)();
	int (*PluginFriendAddIdLaunch)();
	int (*PluginUpdateCommentLaunch)();
	int (*PluginSentEdtiLaunch)();
	int (*DoUnk8)();
	int (*DoUnk9)();
	int (*DoUnk10)();
	int (*DoUnk11)();
	int (*DoUnk12)();
	int (*DoUnk13)();
	int (*DoUnk14)();
	int (*DoUnk15)();
	int (*DoUnk16)();
	int (*sendFriendAddRequestBySceNpUserInfoLaunch)();
	int (*DoUnk18)();
	int (*sendMessageBySceNpUserInfoNoCallbackLaunch)();
	int (*DoUnk20)();
	int (*DoUnk21)();
	int (*DoUnk22)();
	int (*DoUnk23)();
	int (*DoUnk24)();
	int (*DoUnk25)();
	int (*AbortGui)();
	int (*DoUnk27)();
	int (*DoUnk28)();
	int (*DoUnk29)();
	int (*RecommendGame)(char * contentid, char * title, int);
	int (*sendBrowserWakeup)();
	int (*DoUnk32)();
	int (*DoUnk33)();
	int (*DoUnk34)();
	int (*DoUnk35)();
	int (*DoUnk36)();
	int (*sendEtcLaunch)();
	int (*DoUnk38)();
	int (*DoUnk39)();
	int (*TellFriend)();
} friendim_plugin_interface;

friendim_plugin_interface * friendim_interface;
