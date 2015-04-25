#include <sdk_version.h>
#include <cellstatus.h>
#include <cell/cell_fs.h>
#include <cell/rtc.h>
#include <cell/gcm.h>
#include <cell/pad.h>
#include <sys/vm.h>
#include <sysutil/sysutil_common.h>

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/memory.h>
#include <sys/timer.h>
#include <sys/process.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netex/net.h>
#include <netex/errno.h>
#include <netex/libnetctl.h>
#include <netex/sockinfo.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "flags.h"

#include "types.h"
#include "common.h"
#include "cobra/cobra.h"
#include "cobra/storage.h"
#include "vsh/game_plugin.h"

#ifdef EXTRA_FEAT
 #include "vsh/system_plugin.h"
#endif

#define _game_TitleID  _game_info+0x04
#define _game_Title    _game_info+0x14

static char _game_info[0x120];
static char search_url[50] = "http://google.com/search?q=";

#ifdef COBRA_ONLY
 #include "cobra/netiso.h"

 #ifdef LITE_EDITION
    #define EDITION " [Lite]"
 #else
  #ifdef PS3MAPI
	#ifdef REX_ONLY
		#define EDITION " [Rebug-PS3MAPI]"
	#else
		#define EDITION " [PS3MAPI]"
	#endif
  #else
   #ifdef REX_ONLY
    #define EDITION " [Rebug]"
   #else
    #define EDITION ""
   #endif
  #endif
 #endif
#else
 #ifdef CCAPI
    #define EDITION " [CCAPI]"
 #else
    #define EDITION " [nonCobra]"
 #endif
#endif

SYS_MODULE_INFO(WWWD, 0, 1, 0);
SYS_MODULE_START(wwwd_start);
SYS_MODULE_STOP(wwwd_stop);

#define VSH_MODULE_PATH 	"/dev_blind/vsh/module/"
#define VSH_ETC_PATH		"/dev_blind/vsh/etc/"
#define PS2_EMU_PATH		"/dev_blind/ps2emu/"
#define REBUG_COBRA_PATH	"/dev_blind/rebug/cobra/"
#define HABIB_COBRA_PATH	"/dev_blind/habib/cobra/"
#define SYS_COBRA_PATH		"/dev_blind/sys/"
#define PS2_CLASSIC_TOGGLER "/dev_hdd0/classic_ps2"
#define REBUG_TOOLBOX		"/dev_hdd0/game/RBGTLBOX2/USRDIR/"
#define COLDBOOT_PATH		"/dev_blind/vsh/resource/coldboot.raf"
#define ORG_LIBFS_PATH		"/dev_flash/sys/external/libfs.sprx"
#define NEW_LIBFS_PATH		"/dev_hdd0/tmp/libfs.sprx"

#define PS2_CLASSIC_PLACEHOLDER  "/dev_hdd0/game/PS2U10000/USRDIR"
#define PS2_CLASSIC_ISO_PATH     "/dev_hdd0/game/PS2U10000/USRDIR/ISO.BIN.ENC"
#define PS2_CLASSIC_ISO_ICON     "/dev_hdd0/game/PS2U10000/ICON0.PNG"

#define WM_VERSION			"1.41.41 MOD"						// webMAN version
#define MM_ROOT_STD			"/dev_hdd0/game/BLES80608/USRDIR"	// multiMAN root folder
#define MM_ROOT_SSTL		"/dev_hdd0/game/NPEA00374/USRDIR"	// multiman SingStar® Stealth root folder
#define MM_ROOT_STL			"/dev_hdd0/tmp/game_repo/main"		// stealthMAN root folder

#define WMCONFIG			"/dev_hdd0/tmp/wmconfig.bin"		// webMAN config file
#define WMTMP				"/dev_hdd0/tmp/wmtmp"				// webMAN work/temp folder
#define WM_ICONS_PATH		"/dev_hdd0/tmp/wm_icons/"			// webMAN icons path
#define WMNOSCAN			"/dev_hdd0/tmp/wm_noscan"			// webMAN config file

#ifdef WEB_CHAT
 #define WMCHATFILE			"/dev_hdd0/tmp/wmtmp/wmchat.htm"
 #define DELETE_TURNOFF		{do_umount(false); cellFsUnlink((char*)"/dev_hdd0/tmp/turnoff"); cellFsUnlink((char*)WMCHATFILE);}
#else
 #define DELETE_TURNOFF		{do_umount(false); cellFsUnlink((char*)"/dev_hdd0/tmp/turnoff");}
#endif

#define THREAD_NAME 		"wwwdt"
#define THREAD_NAME_FTP 	"ftpdt"
#define THREAD_NAME_NET 	"netiso"
#define THREAD_NAME_NTFS 	"ntfsd"

#define STOP_THREAD_NAME 	"wwwds"

////////////

#define HTML_BASE_PATH			"/dev_hdd0/xmlhost/game_plugin"

#define FB_XML					HTML_BASE_PATH "/fb.xml"
#define MY_GAMES_XML			HTML_BASE_PATH "/mygames.xml"
#define MOBILE_HTML				HTML_BASE_PATH "/mobile.html"
#define GAMELIST_JS				HTML_BASE_PATH "/gamelist.js"

#define DELETE_CACHED_GAMES		{cellFsUnlink((char*)WMTMP "/games.html"); cellFsUnlink((char*)GAMELIST_JS);}

#define XML_HEADER				"<?xml version=\"1.0\" encoding=\"UTF-8\"?><XMBML version=\"1.0\">"
#define XML_PAIR(key, value) 	"<Pair key=\"" key "\"><String>" value "</String></Pair>"
#define QUERY_XMB(key, src) 	"<Query class=\"type:x-xmb/folder-pixmap\" key=\"" key "\" attr=\"" key "\" src=\"" src "\"/>"
#define ADD_XMB_ITEM(key)		"<Item class=\"type:x-xmb/module-action\" key=\"" key "\" attr=\"" key "\"/>"

#define ITEM_CHECKED			" checked=\"checked\""
#define ITEM_SELECTED			" selected=\"selected\""

#define WEB_LINK_PAIR			XML_PAIR("module_name", "webbrowser_plugin")
#define STR_NOITEM_PAIR			XML_PAIR("str_noitem", "msg_error_no_content") "</Table>"

#define HTML_DIR				"&lt;dir&gt;"
#define HTML_BUTTON_FMT			"%s%s\" %s'%s';\">"
#define HTML_BUTTON				" <input type=\"button\" value=\""
#define HTML_ONCLICK			"onclick=\"window.location.href="
#define HTML_INPUT(n, v, m, s)	"<input name=\"" n "\" type=\"text\" value=\"" v "\" maxlength=\"" m "\" size=\"" s "\">"

////////////

#define SC_PEEK_LV2 					(6)
#define SC_POKE_LV2 					(7)
#define SC_PEEK_LV1 					(8)
#define SC_POKE_LV1 					(9)
#define SC_COBRA_SYSCALL8				(8)

#define SC_GET_FREE_MEM 				(352)
#define SC_SYS_CONTROL_LED				(386)
#define SC_GET_PLATFORM_INFO			(387)
#define SC_RING_BUZZER  				(392)

#define SC_SET_FAN_POLICY				(389)
#define SC_GET_FAN_POLICY				(409)
#define SC_GET_TEMPERATURE				(383)

#define SC_STORAGE_OPEN 				(600)
#define SC_STORAGE_CLOSE 				(601)
#define SC_STORAGE_INSERT_EJECT			(616)
#define SC_FS_LINK						(810)
#define SC_FS_MOUNT  					(837)
#define SC_FS_UMOUNT 					(838)
#define SC_GET_IDPS 					(870)
#define SC_GET_PSID 					(872)
#define SC_GET_CONSOLE_TYPE				(985)

#define SC_GET_PRX_MODULE_BY_ADDRESS	(461)
#define SC_STOP_PRX_MODULE 				(482)
#define SC_UNLOAD_PRX_MODULE 			(483)
#define SC_PPU_THREAD_EXIT				(41)

#define SC_SYS_POWER 					(379)
#define SYS_SOFT_REBOOT 				0x0200
#define SYS_HARD_REBOOT					0x1200
#define SYS_REBOOT						0x8201 /*load LPAR id 1*/
#define SYS_SHUTDOWN					0x1100

#define SYS_NET_EURUS_POST_COMMAND		(726)
#define CMD_GET_MAC_ADDRESS				0x103f

#define BEEP1 { system_call_3(SC_RING_BUZZER, 0x1004, 0x4,   0x6); }
#define BEEP2 { system_call_3(SC_RING_BUZZER, 0x1004, 0x7,  0x36); }
#define BEEP3 { system_call_3(SC_RING_BUZZER, 0x1004, 0xa, 0x1b6); }

#ifdef PS3MAPI

///////////// PS3MAPI BEGIN //////////////

#define PS3MAPI_SERVER_VERSION						0x0120
#define PS3MAPI_SERVER_MINVERSION					0x0120

#define PS3MAPI_WEBUI_VERSION						0x0121
#define PS3MAPI_WEBUI_MINVERSION					0x0120

#define PS3MAPI_CORE_MINVERSION						0x0111

#define SYSCALL8_OPCODE_PS3MAPI			 			0x7777

#define PS3MAPI_OPCODE_GET_CORE_VERSION				0x0011
#define PS3MAPI_OPCODE_GET_CORE_MINVERSION			0x0012
#define PS3MAPI_OPCODE_GET_FW_TYPE					0x0013
#define PS3MAPI_OPCODE_GET_FW_VERSION				0x0014
#define PS3MAPI_OPCODE_GET_ALL_PROC_PID				0x0021
#define PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID			0x0022
#define PS3MAPI_OPCODE_GET_PROC_BY_PID				0x0023
#define PS3MAPI_OPCODE_GET_CURRENT_PROC				0x0024
#define PS3MAPI_OPCODE_GET_CURRENT_PROC_CRIT		0x0025
#define PS3MAPI_OPCODE_GET_PROC_MEM					0x0031
#define PS3MAPI_OPCODE_SET_PROC_MEM					0x0032
#define PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID		0x0041
#define PS3MAPI_OPCODE_GET_PROC_MODULE_NAME			0x0042
#define PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME		0x0043
#define PS3MAPI_OPCODE_LOAD_PROC_MODULE				0x0044
#define PS3MAPI_OPCODE_UNLOAD_PROC_MODULE			0x0045
#define PS3MAPI_OPCODE_UNLOAD_VSH_PLUGIN			0x0046
#define PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO			0x0047
#define PS3MAPI_OPCODE_GET_IDPS 					0x0081
#define PS3MAPI_OPCODE_SET_IDPS 					0x0082
#define PS3MAPI_OPCODE_GET_PSID 					0x0083
#define PS3MAPI_OPCODE_SET_PSID						0x0084
#define PS3MAPI_OPCODE_CHECK_SYSCALL				0x0091
#define PS3MAPI_OPCODE_DISABLE_SYSCALL				0x0092
#define PS3MAPI_OPCODE_PDISABLE_SYSCALL8 			0x0093
#define PS3MAPI_OPCODE_PCHECK_SYSCALL8 				0x0094
#define PS3MAPI_OPCODE_REMOVE_HOOK					0x0101

///////////// PS3MAPI END //////////////

#endif

#define WWWPORT			(80)
#define FTPPORT			(21)

#define ssend(socket, str) send(socket, str, strlen(str), 0)
#define getPort(p1x, p2x) ((p1x * 256) + p2x)

#define KB			   1024UL
#define   _4KB_		   4096UL
#define   _8KB_		   8192UL
#define  _32KB_		  32768UL
#define  _64KB_		  65536UL
#define _128KB_		 131072UL
#define _192KB_		 196608UL
#define _256KB_		 262144UL
#define  _1MB_		1048576UL
#define _32MB_		33554432UL

#define MIN_MEM		_192KB_

#define MODE		0777

#define LINELEN			512 // file listing
#define MAX_LINE_LEN	640 // html games
#define MAX_PATH_LEN	512 // do not change!

#define FAILED		-1

#define FTP_RECV_SIZE  1024
#define HTML_RECV_SIZE 2048

static u32 BUFFER_SIZE_FTP	= ( _128KB_);

static u32 BUFFER_SIZE		= ( 448*KB);
static u32 BUFFER_SIZE_PSX	= ( 160*KB);
static u32 BUFFER_SIZE_PSP	= (  _32KB_);
static u32 BUFFER_SIZE_PS2	= (  _64KB_);
static u32 BUFFER_SIZE_DVD	= ( _192KB_);
static u32 BUFFER_SIZE_ALL	= ( 896*KB);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
 static sys_ppu_thread_t thread_id_net	=-1;
 #endif
 static sys_ppu_thread_t thread_id_ntfs	=-1;
#endif
static sys_ppu_thread_t thread_id_poll	=-1;
static sys_ppu_thread_t thread_id_ftp	=-1;
static sys_ppu_thread_t thread_id		=-1;

#define SUFIX(a)	((a==1)? "_1" :(a==2)? "_2" :(a==3)? "_3" :(a==4)?"_4":"")
#define SUFIX2(a)	((a==1)?" (1)":(a==2)?" (2)":(a==3)?" (3)":(a==4)?" (4)":"")
#define SUFIX3(a)	((a==1)?" (1).ntfs[":(a==2)?" (2).ntfs[":(a==3)?" (3).ntfs[":(a==4)?" (4).ntfs[":"")

#define IS_ISO_FOLDER (f1>1 && f1<10)
#define IS_PS3_FOLDER (f1<3 || f1>=10)
#define IS_BLU_FOLDER (f1==3)
#define IS_DVD_FOLDER (f1==4)
#define IS_PS2_FOLDER (f1==5)
#define IS_PSX_FOLDER (f1==6 || f1==7)
#define IS_PSP_FOLDER (f1==8 || f1==9)

#define MIN(a, b)	((a) <= (b) ? (a) : (b))
#define ABS(a)		(((a) < 0) ? -(a) : (a))
#define RANGE(a, b, c)	((a) <= (b) ? (b) : (a) >= (c) ? (c) : (a))

#define CD_CACHE_SIZE			(64)

#define ATA_HDD				0x101000000000007ULL
#define BDVD_DRIVE			0x101000000000006ULL
#define PATA0_HDD_DRIVE		0x101000000000008ULL
#define PATA0_BDVD_DRIVE	BDVD_DRIVE
#define PATA1_HDD_DRIVE		ATA_HDD
#define PATA1_BDVD_DRIVE	0x101000000000009ULL
#define BUILTIN_FLASH		0x100000000000001ULL
#define MEMORY_STICK		0x103000000000010ULL
#define SD_CARD				0x103000100000010ULL
#define COMPACT_FLASH		0x103000200000010ULL

#define USB_MASS_STORAGE_1(n)	(0x10300000000000AULL+n) /* For 0-5 */
#define USB_MASS_STORAGE_2(n)	(0x10300000000001FULL+(n-6)) /* For 6-127 */

#define	HDD_PARTITION(n)	(ATA_HDD | ((uint64_t)n<<32))
#define FLASH_PARTITION(n)	(BUILTIN_FLASH | ((uint64_t)n<<32))

#define DEVICE_TYPE_PS3_DVD	0xFF70
#define DEVICE_TYPE_PS3_BD	0xFF71
#define DEVICE_TYPE_PS2_CD	0xFF60
#define DEVICE_TYPE_PS2_DVD	0xFF61
#define DEVICE_TYPE_PSX_CD	0xFF50
#define DEVICE_TYPE_BDROM	0x40
#define DEVICE_TYPE_BDMR_SR	0x41 /* Sequential record */
#define DEVICE_TYPE_BDMR_RR 0x42 /* Random record */
#define DEVICE_TYPE_BDMRE	0x43
#define DEVICE_TYPE_DVD		0x10 /* DVD-ROM, DVD+-R, DVD+-RW etc, they are differenced by booktype field in some scsi command */
#define DEVICE_TYPE_CD		0x08 /* CD-ROM, CD-DA, CD-R, CD-RW, etc, they are differenced somehow with scsi commands */
#define DEVICE_TYPE_USB		0x00

#define IS_COPY				9
#define COPY_WHOLE_FILE		0

#ifdef EXT_GDATA
 #define MOUNT_EXT_GDATA		2
#endif

#define START_DAEMON		(0xC0FEBABE)
#define REFRESH_CONTENT		(0xC0FEBAB0)

#define LV1_UPPER_MEMORY	0x8000000010000000ULL
#define LV2_UPPER_MEMORY	0x8000000000800000ULL

enum FIX_GAME_MODES
{
	FIX_GAME_AUTO,
	FIX_GAME_QUICK,
	FIX_GAME_FORCED,
	FIX_GAME_DISABLED
};

enum STORAGE_COMMAND
{
	CMD_READ_ISO,
	CMD_READ_DISC,
	CMD_READ_CD_ISO_2352,
	CMD_FAKE_STORAGE_EVENT,
	CMD_GET_PSX_VIDEO_MODE
};

#ifdef COBRA_ONLY

#ifndef LITE_EDITION
typedef struct
{
	char server[0x40];
	char path[0x420];
	uint32_t emu_mode;
	uint32_t num_tracks;
	uint16_t port;
	uint8_t pad[6];
	ScsiTrackDescriptor tracks[1];
} __attribute__((packed)) netiso_args;

int connect_to_remote_server(u8 server_id);
#endif

typedef struct
{
	uint64_t device;
        uint32_t emu_mode;
        uint32_t num_sections;
        uint32_t num_tracks;
} __attribute__((packed)) rawseciso_args;


static sys_device_info_t disc_info;
static uint32_t *sections, *sections_size;
static uint32_t num_sections;
uint64_t sector_size = 0x200;
uint32_t handle = -1;
static sys_event_queue_t command_queue_ntfs = -1;
static u8 netiso_loaded=0;
static u8 rawseciso_loaded=0;
#endif

typedef struct {
	uint32_t total;
	uint32_t avail;
} _meminfo;

//static bool is_rebug = false;
static u8 profile = 0;

#ifdef EXT_GDATA
 static u8 extgd = 0;       //external gameDATA
#endif

static u8 loading_html = 0;
static u8 loading_games = 0;
static u8 init_running = 0;

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
  static int g_socket = -1;
  static sys_event_queue_t command_queue = -1;
 #endif

 #define CD_SECTOR_SIZE_2048   2048
 static u32 CD_SECTOR_SIZE_2352 = 2352;

 static uint64_t discsize=0;
 static int is_cd2352=0;
 static uint8_t *cd_cache=0;
 static uint32_t cached_cd_sector=0x80000000;
#endif

#define NTFS 		 	(10)

#define MIN_FANSPEED	(20)
#define DEFAULT_MIN_FANSPEED	(25)
#define MAX_FANSPEED	(0xE6)
#define MY_TEMP 		(68)
static u8 fan_speed=0x33;
static u8 old_fan=0x33;
static u32 max_temp=MY_TEMP;
static bool fan_ps2_mode=false; // temporary disable dynamic fan control

#define MAX_LAST_GAMES (5)
typedef struct
{
	uint8_t last;
	char game[MAX_LAST_GAMES][MAX_PATH_LEN];
} __attribute__((packed)) _lastgames;

#ifdef USE_DEBUG
 static int debug_s=-1;
 static char debug[256];
#endif
static volatile u8 wm_unload_combo = 0;
static volatile u8 working = 1;
static u8 cobra_mode=0;
static u8 max_mapped=0;

static float c_firmware=0.0f;
static u8 dex_mode=0;

static u64 SYSCALL_TABLE = 0;

#ifndef COBRA_ONLY
 static u64 base_addr=0;
 static u64 open_hook=0;

 typedef struct
 {
	char src[384];
	char dst[384];
 } redir_files_struct;
 static redir_files_struct file_to_map[10];
#endif


typedef struct
{
	uint8_t usb0;
	uint8_t usb1;
	uint8_t usb2;
	uint8_t usb3;
	uint8_t usb6;
	uint8_t usb7;
	uint8_t netd0;
	uint8_t lastp;
	uint8_t autob;
	uint8_t delay;
	uint8_t bootd;
	uint8_t boots;
	uint8_t blind;
	uint8_t nogrp;
	uint8_t noset;
	uint8_t cmask;
	uint32_t netp0;
	char neth0[16];
	uint8_t poll;
	uint8_t ftpd;
	uint8_t warn;
	uint8_t fanc;
	uint8_t temp1;
	uint8_t rxvid;
	uint8_t bind;
	uint8_t refr;
	uint8_t manu;
	uint8_t temp0;
	uint8_t netd1;
	uint32_t netp1;
	char neth1[16];
	uint8_t foot;
	uint8_t nopad;
	uint8_t nocov;
	uint8_t nospoof;
	uint8_t ps2temp;
	uint8_t pspl;
	uint8_t minfan;
	uint16_t combo;
	uint8_t sidps;
	uint8_t spsid;
	uint8_t spp;
	uint8_t lang;
	char vIDPS1[17];
	char vIDPS2[17];
	char vPSID1[17];
	char vPSID2[17];
	uint8_t tid;
	uint8_t wmdn;
	char autoboot_path[256];
	uint8_t ps2l;
	uint32_t combo2;
	uint8_t homeb;
	char home_url[256];
	uint8_t netd2;
	uint32_t netp2;
	char neth2[16];
	uint8_t profile;
	char uaccount[9];
	char allow_ip[16];
	uint8_t noss;
	uint8_t fixgame;
	uint8_t bus;
	uint8_t dev_sd;
	uint8_t dev_ms;
	uint8_t dev_cf;
	uint8_t ps1emu;
} __attribute__((packed)) WebmanCfg;

//combo
#define FAIL_SAFE (1<<0)
#define SHOW_TEMP (1<<1)
#define PREV_GAME (1<<2)
#define NEXT_GAME (1<<3)
#define SHUT_DOWN (1<<4)
#define RESTARTPS (1<<5)
#define UNLOAD_WM (1<<6)
#define MANUALFAN (1<<7)
#define SHOW_IDPS (1<<8)
#define DISABLESH (1<<9)
#define DISABLEFC (1<<10)
#define MINDYNFAN (1<<11)
#define DISACOBRA (1<<12)

//combo2
#define EXTGAMDAT (1<<0)
#define MOUNTNET0 (1<<1)
#define MOUNTNET1 (1<<2)
#define PS2TOGGLE (1<<3)
#define PS2SWITCH (1<<4)
#define BLOCKSVRS (1<<5)
#define XMLREFRSH (1<<6)
#define UMNT_GAME (1<<7)
#define VIDRECORD (1<<8)

#define REBUGMODE (1<<13)
#define NORMAMODE (1<<14)
#define DEBUGMENU (1<<15)

#define AUTOBOOT_PATH            "/dev_hdd0/PS3ISO/AUTOBOOT.ISO"

#ifdef COBRA_ONLY
 #define DEFAULT_AUTOBOOT_PATH   "/dev_hdd0/PS3ISO/AUTOBOOT.ISO"
#else
 #define DEFAULT_AUTOBOOT_PATH   "/dev_hdd0/GAMES/AUTOBOOT"
#endif

#define ISO_EXTENSIONS           ".iso.cue.img.mdf.bin"

uint64_t idps_offset1=0;
uint64_t idps_offset2=0;
uint64_t psid_offset=0;

uint64_t get_fan_policy_offset=0;
uint64_t set_fan_policy_offset=0;

static CellRtcTick rTick, gTick;

static int set_gamedata_status(u8 status, bool do_mount);
static void set_buffer_sizes(int footprint);
static void get_idps_psid(void);
static void enable_dev_blind(char *msg);

#ifdef NOSINGSTAR
static void no_singstar_icon(void);
#endif
#ifdef COBRA_ONLY
static void select_ps1emu(void);
uint64_t getlba(const char *s1, u16 n1, const char *s2, u16 n2, u16 start);
void fix_iso(char *iso_file, uint64_t maxbytes, bool patch_update);
#endif

int extcmp(const char *s1, const char *s2, size_t n);
int extcasecmp(const char *s1, const char *s2, size_t n);
char *strcasestr(const char *s1, const char *s2);

static void refresh_xml(char *msg);

static void reset_settings(void);
static int save_settings(void);
static u64 backup[6];

static u8 wmconfig[sizeof(WebmanCfg)];
static WebmanCfg *webman_config = (WebmanCfg*) wmconfig;

static bool gmobile_mode = false;

static char ftp_password[20]="";
static char html_base_path[MAX_PATH_LEN]="";

static char smonth[12][4]={"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static char drives[14][12]={"/dev_hdd0", "/dev_usb000", "/dev_usb001", "/dev_usb002", "/dev_usb003", "/dev_usb006", "/dev_usb007", "/net0", "/net1", "/net2", "/ext", "/dev_sd", "/dev_ms", "/dev_cf"};
static char paths [11][12]={"GAMES", "GAMEZ", "PS3ISO", "BDISO", "DVDISO", "PS2ISO", "PSXISO", "PSXGAMES", "PSPISO", "ISO", "video"};

static char wm_icons[12][60]={WM_ICONS_PATH "icon_wm_album_ps3.png", //024.png  [0]
                              WM_ICONS_PATH "icon_wm_album_psx.png", //026.png  [1]
                              WM_ICONS_PATH "icon_wm_album_ps2.png", //025.png  [2]
                              WM_ICONS_PATH "icon_wm_album_psp.png", //022.png  [3]
                              WM_ICONS_PATH "icon_wm_album_dvd.png", //023.png  [4]

                              WM_ICONS_PATH "icon_wm_ps3.png",       //024.png  [5]
                              WM_ICONS_PATH "icon_wm_psx.png",       //026.png  [6]
                              WM_ICONS_PATH "icon_wm_ps2.png",       //025.png  [7]
                              WM_ICONS_PATH "icon_wm_psp.png",       //022.png  [8]
                              WM_ICONS_PATH "icon_wm_dvd.png",       //023.png  [9]

                              WM_ICONS_PATH "icon_wm_settings.png",  //icon/icon_home.png  [10]
                              WM_ICONS_PATH "icon_wm_eject.png"      //icon/icon_home.png  [11]
                             };

static bool covers_exist[7];
static char local_ip[16] = "127.0.0.1";

uint64_t convertH(char *val);

//uint64_t find_syscall();
//uint64_t search64(uint64_t val);
//uint64_t find_syscall_table();

#define SYSCALLS_UNAVAILABLE    0xFFFFFFFF80010003ULL

uint64_t IDPS[2] = {0, 0};
uint64_t PSID[2] = {0, 0};

int lang_pos, fh;

#ifdef ENGLISH_ONLY

static char STR_HOME[8] = "Home";

#define STR_TRADBY	"<br>"

#define STR_FILES		"Files"
#define STR_GAMES		"Games"
#define STR_SETUP		"Setup"

#define STR_EJECT		"Eject"
#define STR_INSERT		"Insert"
#define STR_UNMOUNT		"Unmount"
#define STR_COPY		"Copy Folder"
#define STR_REFRESH		"Refresh"
#define STR_SHUTDOWN	"Shutdown"
#define STR_RESTART		"Restart"

#define STR_BYTE		"b"
#define STR_KILOBYTE	"KB"
#define STR_MEGABYTE	"MB"
#define STR_GIGABYTE	"GB"

#define STR_COPYING		"Copying"
#define STR_CPYDEST		"Destination"
#define STR_CPYFINISH	"Copy Finished!"
#define STR_CPYABORT	"Copy aborted!"
#define STR_DELETE		"Delete"

#define STR_SCAN1		"Scan these devices"
#define STR_SCAN2		"Scan for content"
#define STR_PSPL		"Show PSP Launcher"
#define STR_PS2L		"Show PS2 Classic Launcher"
#define STR_RXVID		"Show Video sub-folder"
#define STR_VIDLG		"Video"
#define STR_LPG			"Load last-played game on startup"
#define STR_AUTOB		"Check for /dev_hdd0/PS3ISO/AUTOBOOT.ISO on startup"
#define STR_DELAYAB		"Delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)"
#define STR_DEVBL		"Enable /dev_blind (writable /dev_flash) on startup"
#define STR_CONTSCAN	"Disable content scan on startup"
#define STR_USBPOLL		"Disable USB polling"
#define STR_FTPSVC		"Disable FTP service"
#define STR_FIXGAME		"Disable auto-fix game"
#define STR_COMBOS		"Disable all PAD shortcuts"
#define STR_MMCOVERS	"Disable multiMAN covers"
#define STR_ACCESS		"Disable remote access to FTP/WWW services"
#define STR_NOSETUP		"Disable webMAN Setup entry in \"webMAN Games\""
#define STR_NOSPOOF		"Disable firmware version spoofing"
#define STR_NOGRP		"Disable grouping of content in \"webMAN Games\""
#define STR_NOWMDN		"Disable startup notification of WebMAN on the XMB"
#ifdef NOSINGSTAR
 #define STR_NOSINGSTAR	"Remove SingStar icon"
#endif
#define STR_RESET_USB	"Disable Reset USB Bus"
#define STR_TITLEID		"Include the ID as part of the title of the game"
#define STR_FANCTRL		"Enable dynamic fan control"
#define STR_NOWARN		"Disable temperature warnings"
#define STR_AUTOAT		"Auto at"
#define STR_LOWEST		"Lowest"
#define STR_FANSPEED	"fan speed"
#define STR_MANUAL		"Manual"
#define STR_PS2EMU		"PS2 Emulator"
#define STR_LANGAMES	"Scan for LAN games/videos"
#define STR_ANYUSB		"Wait for any USB device to be ready"
#define STR_ADDUSB		"Wait additionally for each selected USB device to be ready"
#define STR_SPOOFID		"Change idps and psid in lv2 memory at system startup"
#define STR_DELCFWSYS	"Disable lv1&lv2 peek&poke syscalls (6,7,9,10,36) and delete history files at system startup"
#define STR_MEMUSAGE	"Plugin memory usage"
#define STR_PLANG		"Plugin language"
#define STR_PROFILE		"Profile"
#define STR_DEFAULT		"Default"
#define STR_COMBOS2		"XMB/In-Game PAD SHORTCUTS"
#define STR_FAILSAFE	"FAIL SAFE"
#define STR_SHOWTEMP	"SHOW TEMP"
#define STR_SHOWIDPS	"SHOW IDPS"
#define STR_PREVGAME	"PREV GAME"
#define STR_NEXTGAME	"NEXT GAME"
#define STR_SHUTDOWN2	"SHUTDOWN "
#define STR_RESTART2	"RESTART&nbsp; "
#define STR_DELCFWSYS2	"DEL CFW SYSCALLS"
#define STR_UNLOADWM	"UNLOAD WM"
#define STR_FANCTRL2	"CTRL FAN"
#define STR_FANCTRL4	"CTRL DYN FAN"
#define STR_FANCTRL5	"CTRL MIN FAN"
#define STR_UPDN		"&#8593;/&#8595;" //↑/↓
#define STR_LFRG		"&#8592;/&#8594;" //←/→

#ifdef COBRA_ONLY
 #define STR_DISCOBRA	"COBRA TOGGLE"
#endif

#ifdef REX_ONLY
 #define STR_RBGMODE	"RBG MODE TOGGLE"
 #define STR_RBGNORM	"NORM MODE TOGGLE"
 #define STR_RBGMENU 	"MENU TOGGLE"
#endif

#define STR_SAVE		"Save"
#define STR_SETTINGSUPD	"Settings updated.<br><br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system."
#define STR_ERROR		"Error!"

#define STR_MYGAMES		"webMAN Games"
#define STR_LOADGAMES	"Load games with webMAN"
#define STR_FIXING		"Fixing"

#define STR_WMSETUP		"webMAN Setup"
#define STR_WMSETUP2	"Setup webMAN options"

#define STR_EJECTDISC	"Eject Disc"
#define STR_UNMOUNTGAME	"Unmount current game"

#define STR_WMSTART		"webMAN loaded!"
#define STR_WMUNL		"webMAN unloaded!"
#define STR_CFWSYSALRD	"CFW Syscalls already disabled"
#define STR_CFWSYSRIP	"Removal History files & CFW Syscalls in progress..."
#define STR_RMVCFWSYS	"History files & CFW Syscalls deleted OK!"
#define STR_RMVCFWSYSF	"Failed to remove CFW Syscalls"

#define STR_RMVWMCFG	"webMAN config reset in progress..."
#define STR_RMVWMCFGOK	"Done! Restart within 3 seconds"

#define STR_PS3FORMAT	"PS3 format games"
#define STR_PS2FORMAT	"PS2 format games"
#define STR_PS1FORMAT	"PSOne format games"
#define STR_PSPFORMAT	"PSP\xE2\x84\xA2 format games"

#define STR_VIDFORMAT	"Blu-ray\xE2\x84\xA2 and DVD"
#define STR_VIDEO		"Video content"

#define STR_LAUNCHPSP	"Launch PSP ISO mounted through webMAN or mmCM"
#define STR_LAUNCHPS2	"Launch PS2 Classic"

#define STR_GAMEUM		"Game unmounted."

#define STR_EJECTED		"Disc ejected."
#define STR_LOADED		"Disc inserted."

#define STR_GAMETOM		"Game to mount"
#define STR_GAMELOADED	"Game loaded successfully. Start the game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry.<hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the game."
#define STR_PSPLOADED	"Game loaded successfully. Start the game using <b>PSP Launcher</b>.<hr>"
#define STR_PS2LOADED	"Game loaded successfully. Start the game using <b>PS2 Classic Launcher</b>.<hr>"
#define STR_LOADED2		"loaded   "

#define STR_MOVIETOM	"Movie to mount"
#define STR_MOVIELOADED	"Movie loaded successfully. Start the movie from the disc icon<br>under the Video column.<hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the movie."

#define STR_XMLRF		"Game list refreshed (<a href=\"" MY_GAMES_XML "\">mygames.xml</a>).<br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system now."

#define STR_STORAGE		"System storage"
#define STR_MEMORY		"Memory available"
#define STR_MBFREE		"MB free"
#define STR_KBFREE		"KB free"

#define STR_FANCTRL3	"Fan control:"

#define STR_ENABLED 	"Enabled"
#define STR_DISABLED	"Disabled"

#define STR_FANCH0		"Fan setting changed:"
#define STR_FANCH1		"MAX TEMP: "
#define STR_FANCH2		"FAN SPEED: "
#define STR_FANCH3		"MIN FAN SPEED: "

#define STR_OVERHEAT	"System overheat warning!"
#define STR_OVERHEAT2	"  OVERHEAT DANGER!\r\nFAN SPEED INCREASED!"

#define STR_NOTFOUND	"Not found!"

#else
static char lang_code[3]			= "";

static char STR_TRADBY[150]			= "<br>";

static char STR_FILES[30]			= "Files";
static char STR_GAMES[30]			= "Games";
static char STR_SETUP[30]			= "Setup";
static char STR_HOME[30]			= "Home";
static char STR_EJECT[50]			= "Eject";
static char STR_INSERT[50]			= "Insert";
static char STR_UNMOUNT[50]			= "Unmount";
static char STR_COPY[50]			= "Copy Folder";
static char STR_REFRESH[50]			= "Refresh";
static char STR_SHUTDOWN[50]		= "Shutdown";
static char STR_RESTART[50]			= "Restart";

static char STR_BYTE[10]			= "b";
static char STR_KILOBYTE[10]		= "KB";
static char STR_MEGABYTE[10]		= "MB";
static char STR_GIGABYTE[10]		= "GB";

static char STR_COPYING[30]			= "Copying";
static char STR_CPYDEST[30]			= "Destination";
static char STR_CPYFINISH[30]		= "Copy Finished!";
static char STR_CPYABORT[50]		= "Copy aborted!";
static char STR_DELETE[50]			= "Delete";

static char STR_SCAN1[100]			= "Scan these devices";
static char STR_SCAN2[100]			= "Scan for content";
static char STR_PSPL[100]			= "Show PSP Launcher";
static char STR_PS2L[100]			= "Show PS2 Classic Launcher";
static char STR_RXVID[100]			= "Show Video sub-folder";
static char STR_VIDLG[30]			= "Video";
static char STR_LPG[100]			= "Load last-played game on startup";
static char STR_AUTOB[150]			= "Check for /dev_hdd0/PS3ISO/AUTOBOOT.ISO on startup";
static char STR_DELAYAB[200]		= "Delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)";
static char STR_DEVBL[150]			= "Enable /dev_blind (writable /dev_flash) on startup";
static char STR_CONTSCAN[150]		= "Disable content scan on startup";
static char STR_USBPOLL[100]		= "Disable USB polling";
static char STR_FTPSVC[100]			= "Disable FTP service";
static char STR_FIXGAME[100]		= "Disable auto-fix game";
static char STR_COMBOS[100]			= "Disable all PAD shortcuts";
static char STR_MMCOVERS[100]		= "Disable multiMAN covers";
static char STR_ACCESS[100]			= "Disable remote access to FTP/WWW services";
static char STR_NOSETUP[150]		= "Disable webMAN Setup entry in \"webMAN Games\"";
static char STR_NOSPOOF[100]		= "Disable firmware version spoofing";
static char STR_NOGRP[100]			= "Disable grouping of content in \"webMAN Games\"";
static char STR_NOWMDN[200]			= "Disable startup notification of WebMAN on the XMB";
#ifdef NOSINGSTAR
static char STR_NOSINGSTAR[100]		= "Remove SingStar icon";
#endif
static char STR_RESET_USB[100]		= "Disable Reset USB Bus";
static char STR_TITLEID[200]		= "Include the ID as part of the title of the game";
static char STR_FANCTRL[120]		= "Enable dynamic fan control";
static char STR_NOWARN[120]			= "Disable temperature warnings";
static char STR_AUTOAT[100]			= "Auto at";
static char STR_LOWEST[30]			= "Lowest";
static char STR_FANSPEED[80]		= "fan speed";
static char STR_MANUAL[30]			= "Manual";
static char STR_PS2EMU[100]			= "PS2 Emulator";
static char STR_LANGAMES[100]		= "Scan for LAN games/videos";
static char STR_ANYUSB[100]			= "Wait for any USB device to be ready";
static char STR_ADDUSB[150]			= "Wait additionally for each selected USB device to be ready";
static char STR_SPOOFID[150]		= "Change idps and psid in lv2 memory at system startup";
static char STR_DELCFWSYS[200]		= "Disable lv1&lv2 peek&poke syscalls (6,7,9,10,11,36) and delete history files at system startup";
static char STR_MEMUSAGE[100]		= "Plugin memory usage";
static char STR_PLANG[100]			= "Plugin language";
static char STR_PROFILE[30]			= "Profile";
static char STR_DEFAULT[30]			= "Default";
static char STR_COMBOS2[100]		= "XMB/In-Game PAD SHORTCUTS";
static char STR_FAILSAFE[100]		= "FAIL SAFE";
static char STR_SHOWTEMP[100]		= "SHOW TEMP";
static char STR_SHOWIDPS[100]		= "SHOW IDPS";
static char STR_PREVGAME[100]		= "PREV GAME";
static char STR_NEXTGAME[100]		= "NEXT GAME";
static char STR_SHUTDOWN2[100]		= "SHUTDOWN ";
static char STR_RESTART2[100]		= "RESTART&nbsp; ";
#ifdef REMOVE_SYSCALLS
static char STR_DELCFWSYS2[100] 	= "DEL CFW SYSCALLS";
#endif
static char STR_UNLOADWM[100]		= "UNLOAD WM";
static char STR_FANCTRL2[100]		= "CTRL FAN";
static char STR_FANCTRL4[100]		= "CTRL DYN FAN";
static char STR_FANCTRL5[100]		= "CTRL MIN FAN";
static char STR_UPDN[20]			= "&#8593;/&#8595;"; //↑/↓
static char STR_LFRG[20]			= "&#8592;/&#8594;"; //←/→
#ifdef COBRA_ONLY
static char STR_DISCOBRA[100]		= "COBRA TOGGLE";
#endif
#ifdef REX_ONLY
static char STR_RBGMODE[100]		= "RBG MODE TOGGLE";
static char STR_RBGNORM[100]		= "NORM MODE TOGGLE";
static char STR_RBGMENU[100] 		= "MENU TOGGLE";
#endif
static char STR_SAVE[30]			= "Save";
static char STR_SETTINGSUPD[250]	= "Settings updated.<br><br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system.";
static char STR_ERROR[30]			= "Error!";

static char STR_MYGAMES[50]			= "webMAN Games";
static char STR_LOADGAMES[80]		= "Load games with webMAN";
static char STR_FIXING[50]			= "Fixing";

static char STR_WMSETUP[50]			= "webMAN Setup";
static char STR_WMSETUP2[50]		= "Setup webMAN options";

static char STR_EJECTDISC[50]		= "Eject Disc";
static char STR_UNMOUNTGAME[100]	= "Unmount current game";

static char STR_WMSTART[50]			= "webMAN loaded!";
static char STR_WMUNL[80]			= "webMAN unloaded!";
static char STR_CFWSYSALRD[130]		= "CFW Syscalls already disabled";
static char STR_CFWSYSRIP[130]		= "Removal History files & CFW Syscalls in progress...";
static char STR_RMVCFWSYS[130]		= "History files & CFW Syscalls deleted OK!";
static char STR_RMVCFWSYSF[130]		= "Failed to remove CFW Syscalls";

static char STR_RMVWMCFG[130]		= "webMAN config reset in progress...";
static char STR_RMVWMCFGOK[130]		= "Done! Restart within 3 seconds";

static char STR_PS3FORMAT[50]		= "PS3 format games";
static char STR_PS2FORMAT[50]		= "PS2 format games";
static char STR_PS1FORMAT[50]		= "PSOne format games";
static char STR_PSPFORMAT[50]		= "PSP\xE2\x84\xA2 format games";

static char STR_VIDFORMAT[50]		= "Blu-ray\xE2\x84\xA2 and DVD";
static char STR_VIDEO[50]			= "Video content";

static char STR_LAUNCHPSP[100]		= "Launch PSP ISO mounted through webMAN or mmCM";
static char STR_LAUNCHPS2[100]		= "Launch PS2 Classic";

static char STR_GAMEUM[50]			= "Game unmounted.";

static char STR_EJECTED[50]			= "Disc ejected.";
static char STR_LOADED[50]			= "Disc inserted.";

static char STR_GAMETOM[50]			= "Game to mount";
static char STR_GAMELOADED[250]		= "Game loaded successfully. Start the game from the disc icon<br>or from <b>/app_home</b>&nbsp;XMB entry.<hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the game.";
static char STR_PSPLOADED[230]		= "Game loaded successfully. Start the game using <b>PSP Launcher</b>.<hr>";
static char STR_PS2LOADED[230]		= "Game loaded successfully. Start the game using <b>PS2 Classic Launcher</b>.<hr>";
static char STR_LOADED2[50]			= "loaded   ";

static char STR_MOVIETOM[50]		= "Movie to mount";
static char STR_MOVIELOADED[250]	= "Movie loaded successfully. Start the movie from the disc icon<br>under the Video column.<hr>Click <a href=\"/mount.ps3/unmount\">here</a> to unmount the movie.";

static char STR_XMLRF[200]			= "Game list refreshed (<a href=\"" MY_GAMES_XML "\">mygames.xml</a>).<br>Click <a href=\"/restart.ps3\">here</a> to restart your PLAYSTATION®3 system now.";

static char STR_STORAGE[50]			= "System storage";
static char STR_MEMORY[50]			= "Memory available";
static char STR_MBFREE[50]			= "MB free";
static char STR_KBFREE[50]			= "KB free";

static char STR_FANCTRL3[50]		= "Fan control:";
static char STR_ENABLED[50]			= "Enabled";
static char STR_DISABLED[50]		= "Disabled";

static char STR_FANCH0[50]			= "Fan setting changed:";
static char STR_FANCH1[50]			= "MAX TEMP: ";
static char STR_FANCH2[50]			= "FAN SPEED: ";
static char STR_FANCH3[50]			= "MIN FAN SPEED: ";

static char STR_OVERHEAT[100]		= "System overheat warning!";
static char STR_OVERHEAT2[100]		= "  OVERHEAT DANGER!\r\nFAN SPEED INCREASED!";

static char STR_NOTFOUND[50]		= "Not found!";

static char COVERS_PATH[100]		= "";
#endif

int wwwd_start(uint64_t arg);
int wwwd_stop(void);
static void stop_prx_module(void);
static void unload_prx_module(void);

static void get_temperature(u32 _dev, u32 *_temp);
static void fan_control(u8 temp0, u8 maxtemp);
static void led(u64 color, u64 mode);
static void restore_fan(u8 set_ps2_temp);

static int savefile(char *file, char *mem, u64 size);
static int filecopy(char *file1, char *file2, uint64_t maxbytes);
static int folder_copy(char *path1, char *path2);

static void absPath(char* absPath_s, const char* path, const char* cwd);
static int isDir(const char* path);
static int ssplit(const char* str, char* left, int lmaxlen, char* right, int rmaxlen);
static int slisten(int port, int backlog);
static void sclose(int *socket_e);

static void detect_firmware(void);
#ifndef ENGLISH_ONLY
static bool language(const char *file_str, char *default_str);
static void update_language(void);
uint32_t get_system_language(uint8_t *lang);
#endif
#ifdef REMOVE_SYSCALLS
static void remove_cfw_syscalls(void);
#endif
#ifdef SPOOF_CONSOLEID
static void spoof_idps_psid(void);
#endif
#ifndef LITE_EDITION
static int del(char *path, bool recursive);
#endif
#ifdef COPY_PS3
static void  import_edats(char *path1, char *path2);
#endif

static void delete_history(bool delete_folders);
static void block_online_servers(void);

static void add_query_html(char *buffer, char *param, char *label);
static void add_xmb_entry(char *param, char *tempstr, char *templn, char *skey, u32 key, char *myxml_ps3, char *myxml_ps2, char *myxml_psx, char *myxml_psp, char *myxml_dvd, char *entry_name, u16 *item_count);
static void add_list_entry(char *tempstr, bool is_dir, char *ename, char *templn, char *name, char *fsize, CellRtcDateTime rDate, u16 flen, unsigned long long sz, char *sf, u8 is_net);

static void prepare_header(char *header, char *param, u8 is_binary);
static void get_value(char *text, char *url, u16 size);
static void handleclient(u64 conn_s_p);
static void handleclient_ftp(u64 conn_s_ftp_p);

static char h2a(char hex);
static void urlenc(char *dst, char *src);
static void utf8enc(char *dst, char *src);
static void htmlenc(char *dst, char *src, u8 cpy2src);
static int val(const char *c);

#ifdef PS3MAPI
static int Char2Int(char input);
static void Hex2Bin(const char* src, char* target);
#endif

#ifdef PS2_DISC
static void do_umount_ps2disc(bool mount);
static bool mount_ps2disc(char *path);
#endif

static void enable_classic_ps2_mode(void);
static void disable_classic_ps2_mode(void);

static void mount_autoboot(void);
static void do_umount(bool clean);
static bool mount_with_mm(const char *_path, u8 do_eject);
static void eject_insert(u8 eject, u8 insert);

#ifdef COBRA_ONLY
static void do_umount_iso(void);
 #ifndef LITE_EDITION
static int remote_stat(int s, char *path, int *is_directory, int64_t *file_size, uint64_t *mtime, uint64_t *ctime, uint64_t *atime, int *abort_connection);
 #endif
#endif

static bool from_reboot = false;
static bool copy_in_progress = false;
static bool copy_aborted = false;
static bool fix_in_progress = false;
static bool fix_aborted = false;
static bool is_busy = false;
static bool is_mounting = false;
static u32 copied_count = 0;

#ifdef COPY_PS3
static char current_file[MAX_PATH_LEN];
#endif

static char* get_game_info(void);
static void show_msg(char* msg);
//void show_msg2(char* msg);

//int (*_cellGcmIoOffsetToAddress)(uint32_t, void**) = NULL;
int (*vshtask_notify)(int, const char *) = NULL;

#ifdef SYS_BGM
uint32_t (*BgmPlaybackDisable)(int, void *) = NULL;
uint32_t (*BgmPlaybackEnable)(int, void *) = NULL;
#endif

int (*vshmain_is_ss_enabled)(void) = NULL;
int (*set_SSHT_)(int) = NULL;

int opd[2] = {0, 0};

void * getNIDfunc(const char * vsh_module, uint32_t fnid, int32_t offset);

void * getNIDfunc(const char * vsh_module, uint32_t fnid, int32_t offset)
{
	// 0x10000 = ELF
	// 0x10080 = segment 2 start
	// 0x10200 = code start

	uint32_t table = (*(uint32_t*)0x1008C) + 0x984; // vsh table address
//	uint32_t table = (*(uint32_t*)0x1002C) + 0x214 - 0x10000; // vsh table address
//	uint32_t table = 0x63A9D4;


	while(((uint32_t)*(uint32_t*)table) != 0)
	{
		uint32_t* export_stru_ptr = (uint32_t*)*(uint32_t*)table; // ptr to export stub, size 2C, "sys_io" usually... Exports:0000000000635BC0 stru_635BC0:    ExportStub_s <0x1C00, 1, 9, 0x39, 0, 0x2000000, aSys_io, ExportFNIDTable_sys_io, ExportStubTable_sys_io>

		const char* lib_name_ptr =  (const char*)*(uint32_t*)((char*)export_stru_ptr + 0x10);

		if(strncmp(vsh_module, lib_name_ptr, strlen(lib_name_ptr))==0)
		{
			// we got the proper export struct
			uint32_t lib_fnid_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x14);
			uint32_t lib_func_ptr = *(uint32_t*)((char*)export_stru_ptr + 0x18);
			uint16_t count = *(uint16_t*)((char*)export_stru_ptr + 6); // number of exports
			for(int i=0;i<count;i++)
			{
				if(fnid == *(uint32_t*)((char*)lib_fnid_ptr + i*4))
				{
					// take address from OPD
					return (void**)*((uint32_t*)(lib_func_ptr) + i) + offset;
				}
			}
		}
		table=table+4;
	}
	return 0;
}

static void show_msg(char* msg)
{
	if(!vshtask_notify)
		vshtask_notify = (void*)((int)getNIDfunc("vshtask", 0xA02D46E7, 0));

	if(strlen(msg)>200) msg[200]=0;

	if(vshtask_notify)
		vshtask_notify(0, msg);
}

static char* get_game_info(void)
{
    int h = View_Find("game_plugin");

	if(h)
	{
		game_interface = (game_plugin_interface *)plugin_GetInterface(h,1);
		game_interface->gameInfo(_game_info);
	}

    return (char*)h;
}


/*
#include "vsh/xmb_plugin.h"

static void show_msg2(char* msg) // usage: show_msg2(L"text");
{
	if(View_Find("xmb_plugin") != 0)
	{
		xmb2_interface = (xmb_plugin_xmb2 *)plugin_GetInterface(View_Find("xmb_plugin"),'XMB2');
		xmb2_interface->showMsg(msg);
	}
}
*/

#ifdef EXTRA_FEAT
static void saveBMP()
{
	if(View_Find("game_plugin")==0) //XMB
	{
		system_interface = (system_plugin_interface *)plugin_GetInterface(View_Find("system_plugin"),1); // 1=regular xmb, 3=ingame xmb (doesnt work)

		CellRtcDateTime t;
		cellRtcGetCurrentClockLocalTime(&t);

		cellFsMkdir((char*)"/dev_hdd0/PICTURE", MODE);

		char bmp[0x50];
		vsh_sprintf(bmp,"/dev_hdd0/PICTURE/ScreenShot_%04d.%02d.%02d_%02d_%02d_%02d.bmp",t.year,t.month,t.day,t.hour,t.minute,t.second);
		system_interface->saveBMP(bmp);

		show_msg(bmp);
	}
}
#endif

/*
void add_log(const char *fmt, char *text)
{
	char buffer[4096];

	sprintf(buffer, fmt, text);

	console_write(buffer);
	int fd;

	if(cellFsOpen("/dev_hdd0/plugins/webMAN_MOD.log", CELL_FS_O_RDWR|CELL_FS_O_CREAT|CELL_FS_O_APPEND, &fd, NULL, 0) == CELL_OK)
	{
		uint64_t nrw; int size = strlen(buffer);
		cellFsWrite(fd, buffer, size, &nrw);
	}
	cellFsClose(fd);
}
*/

#ifndef COBRA_ONLY
static void string_to_lv2(char* path, u64 addr);
static void add_to_map(char *path1, char *path2);
#endif

static inline uint64_t peekq(uint64_t addr);
static inline void pokeq( uint64_t addr, uint64_t val);
static inline uint64_t peek_lv1(uint64_t addr);
static inline void poke_lv1( uint64_t addr, uint64_t val);

//static u32 lv2peek32(u64 addr);
static void lv2poke32(u64 addr, u32 value);

#ifdef DEBUG_MEM
static void dump_mem(char *file, uint64_t start, uint32_t size_mb);
#endif

/*
static u32 lv2peek32(u64 addr)
{
    u32 ret = (u32) (peekq(addr) >> 32ULL);
    return ret;
}
*/

static void lv2poke32(u64 addr, u32 value)
{
    pokeq(addr, (((u64) value) <<32) | (peekq(addr) & 0xffffffffULL));
}

#define PS3 (1<<0)
#define PS2 (1<<1)
#define PS1 (1<<2)
#define PSP (1<<3)
#define BLU (1<<4)
#define DVD (1<<5)

#define GREEN	1
#define RED		2
#define YELLOW	2 //RED+GREEN (RED alias due green is already on)

#define OFF			0
#define ON			1
#define BLINK_FAST	2
#define BLINK_SLOW	3

static void led(u64 color, u64 mode)
{
	system_call_2(SC_SYS_CONTROL_LED, (u64)color, (u64)mode);
}

static void get_temperature(u32 _dev, u32 *_temp)
{
	system_call_2(SC_GET_TEMPERATURE, (uint64_t)(u32) _dev, (uint64_t)(u32) _temp);
}

static int sys_sm_set_fan_policy(u8 arg0, u8 arg1, u8 arg2)
{
    system_call_3(SC_SET_FAN_POLICY, (u64) arg0, (u64) arg1, (u64) arg2);
    return_to_user_prog(int);
}

static int sys_sm_get_fan_policy(u8 id, u8 *st, u8 *mode, u8 *speed, u8 *unknown)
{
    system_call_5(SC_GET_FAN_POLICY, (u64) id, (u64)(u32) st, (u64)(u32) mode, (u64)(u32) speed, (u64)(u32) unknown);
    return_to_user_prog(int);
}

static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(SC_PPU_THREAD_EXIT, val); // prxloader = mandatory; cobra = optional; ccapi = don't use !!!
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(SC_GET_PRX_MODULE_BY_ADDRESS, (uint64_t)(uint32_t)addr);
	return (int)p1;
}

static int sysLv2FsLink(const char *oldpath,const char *newpath)
{
	system_call_2(SC_FS_LINK, (u64)(u32)oldpath, (u64)(u32)newpath);
	return_to_user_prog(int);
}

/*
s32 lv2_get_platform_info(struct platform_info *info)
{
	system_call_1(SC_GET_PLATFORM_INFO, (uint64_t) info);
	return_to_user_prog(s32);
}

static u32 in_cobra(u32 *mode)
{
	system_call_2(SC_COBRA_SYSCALL8, (uint32_t) 0x7000, (uint32_t)mode);
	return_to_user_prog(uint32_t);
}
*/
static int isDir(const char* path)
{
	struct CellFsStat s;
	if(cellFsStat(path, &s)==CELL_FS_SUCCEEDED)
		return ((s.st_mode & CELL_FS_S_IFDIR) != 0);
	else
		return 0;
}

static int savefile(char *file, char *mem, u64 size)
{
	u64 written; int fd=0;
	cellFsChmod(file, MODE);
	if(cellFsOpen(file, CELL_FS_O_CREAT| CELL_FS_O_TRUNC |CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		if(size) cellFsWrite(fd, (void *)mem, size, &written);
		cellFsClose(fd);
		cellFsChmod(file, MODE);
        return CELL_FS_SUCCEEDED;
	}
    else
        return FAILED;
}

static int filecopy(char *file1, char *file2, uint64_t maxbytes)
{
    struct CellFsStat buf;
    int fd1, fd2;
    int ret=FAILED;

    uint64_t chunk_size=_64KB_; //64K

	if(cellFsStat(file1, &buf)!=CELL_FS_SUCCEEDED) return ret;

#ifdef COPY_PS3
	sprintf(current_file, "%s", file2);
#endif

	if(!memcmp(file1, "/dev_hdd0/", 10) && !memcmp(file2, "/dev_hdd0/", 10))
	{
		if(strcmp(file1, file2)==0) return ret;

		cellFsUnlink(file2); copied_count++;
		return sysLv2FsLink(file1, file2);
	}

	if(cellFsOpen((char*)file1, CELL_FS_O_RDONLY, &fd1, NULL, 0)==CELL_FS_SUCCEEDED)
	{
		uint64_t size = buf.st_size;
		if(maxbytes>0 && size>maxbytes) size=maxbytes;

		sys_addr_t buf1=0;
		if(sys_memory_allocate(chunk_size, SYS_MEMORY_PAGE_SIZE_64K, &buf1)==0)
		{	// copy_file

			if(cellFsOpen(file2, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd2, 0, 0)==CELL_FS_SUCCEEDED)
			{
				char *chunk=(char*)buf1;
				uint64_t msiz1 = 0, msiz2 = 0, pos=0;
				copy_aborted = false;

				while(size>0ULL)
        	    {
					if(copy_aborted) break;

					cellFsLseek(fd1, pos, CELL_FS_SEEK_SET, &msiz1);
					cellFsRead(fd1, chunk, chunk_size, &msiz1);

					cellFsWrite(fd2, chunk, msiz1, &msiz2);
					if(!msiz2) {break;}

					pos+=msiz2;
					size-=msiz2;
					if(chunk_size>size) chunk_size=(int) size;

					sys_timer_usleep(1);
				}
				cellFsClose(fd2);

				if(copy_aborted)
					cellFsUnlink(file2); //remove incomplete file
				else
					{cellFsChmod(file2, MODE); copied_count++;}

				ret=size;
			}
			sys_memory_free(buf1);
		}
		cellFsClose(fd1);
	}

	return ret;
}

#ifndef LITE_EDITION
static int del(char *path, bool recursive)
{
	if(!isDir(path)) {return cellFsUnlink(path);}
	if(strlen(path)<11 || !memcmp(path, "/dev_bdvd", 9) || !memcmp(path, "/dev_flash", 10) || !memcmp(path, "/dev_blind", 10)) return FAILED;

	int fd;
	u64 read;
	CellFsDirent dir;
	char entry[MAX_PATH_LEN];

	copy_aborted=false;

	if(cellFsOpendir(path, &fd) == CELL_FS_SUCCEEDED)
	{
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || copy_aborted) break;
			if(dir.d_name[0]=='.' && (dir.d_name[1]=='.' || dir.d_name[1]==0)) continue;

			sprintf(entry, "%s/%s", path, dir.d_name);

			if(isDir(entry))
				{if(recursive) del(entry, recursive);}
			else
				cellFsUnlink(entry);
		}
		cellFsClosedir(fd);

		if(copy_aborted) return FAILED;
	}
	else
		return FAILED;

	if(recursive) cellFsRmdir(path);

	return CELL_FS_SUCCEEDED;
}
#endif

static int folder_copy(char *path1, char *path2)
{
	int fd;
	u64 read;
	CellFsDirent dir;
	char source[MAX_PATH_LEN];
	char target[MAX_PATH_LEN];

	cellFsMkdir((char*)path2, MODE);
    copy_aborted=false;

	if(cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || copy_aborted) break;
			if(dir.d_name[0]=='.' && (dir.d_name[1]=='.' || dir.d_name[1]==0)) continue;

			sprintf(source, "%s/%s", path1, dir.d_name);
			sprintf(target, "%s/%s", path2, dir.d_name);

			if(isDir(source))
				folder_copy(source, target);
			else
				filecopy(source, target, COPY_WHOLE_FILE);
		}
		cellFsClosedir(fd);

		if(copy_aborted) return FAILED;
	}
	else
		return FAILED;

	return CELL_FS_SUCCEEDED;
}

#ifdef COPY_PS3
static void import_edats(char *path1, char *path2)
{
	int fd; bool from_usb;
	u64 read;
	CellFsDirent dir;
	struct CellFsStat buf;

	char source[MAX_PATH_LEN];
	char target[MAX_PATH_LEN];

	cellFsMkdir((char*)path2, MODE);
	if(cellFsStat(path2, &buf)!=CELL_FS_SUCCEEDED) return;

	copy_aborted=false;
	from_usb=(strstr(path1,"/dev_usb")!=NULL);

	if(cellFsOpendir(path1, &fd) == CELL_FS_SUCCEEDED)
	{
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || copy_aborted) break;
			if(strstr(dir.d_name, ".edat")==NULL || strstr(dir.d_name, ".bak")) continue;

			sprintf(source, "%s/%s", path1, dir.d_name);
			sprintf(target, "%s/%s", path2, dir.d_name);

			if(cellFsStat(target, &buf)!=CELL_FS_SUCCEEDED)
				filecopy(source, target, COPY_WHOLE_FILE);
			if(from_usb && cellFsStat(target, &buf)==CELL_FS_SUCCEEDED)
                {sprintf(target, "%s.bak", source); cellFsRename(source, target);}
		}
		cellFsClosedir(fd);
	}
	else
		return;

	return;
}
#endif

static char h2a(char hex)
{
	char c = hex;
	if(c>=0 && c<=9)
		c += '0';
	else if(c>=10 && c<=15)
		c += 0x57; //a-f
	return c;
}

static void urlenc(char *dst, char *src)
{
	size_t j=0;
    size_t n=strlen(src);
	for(size_t i=0; i<n; i++,j++)
	{
		if(src[i]==' ') {dst[j++] = '%'; dst[j++] = '2'; dst[j] = '0';}
		else if(src[i] & 0x80)
		{
			dst[j++] = '%';
			dst[j++] = h2a((unsigned char)src[i]>>4);
			dst[j] = h2a(src[i] & 0xf);
		}
		else if(src[i]==34) {dst[j++] = '%'; dst[j++] = '2'; dst[j] = '2';}
		else if(gmobile_mode && src[i]==39) {dst[j++] = '%'; dst[j++] = '2'; dst[j] = '7';}
		else dst[j] = src[i];
	}
	dst[j] = '\0';
}

static void utf8enc(char *dst, char *src)
{
	size_t j=0, n=strlen(src); u16 c;
	for(size_t i=0; i<n; i++)
	{
		c=(src[i]&0xFF);

		if(!(c & 0xff80)) dst[j++]=c;
		else //if(!(c & 0xf800))
		{
			dst[j++]=0xC0|(c>>6);
			dst[j++]=0x80|(0x3F&c);
		}
/*
		else
		{
			dst[j++]=0xE0|(0x0F&(c>>12));
			dst[j++]=0x80|(0x3F&(c>>06));
			dst[j++]=0x80|(0x3F&(c    ));
		}
*/
	}
	dst[j] = '\0';
	strncpy(src, dst, MAX_LINE_LEN);
}

static void htmlenc(char *dst, char *src, u8 cpy2src)
{
	size_t j=0;
    size_t n=strlen(src); char tmp[8]; memset(dst, 4*n, 0); u8 t, c;
	for(size_t i=0; i<n; i++)
	{
		if(src[i] & 0x80)
		{
			dst[j++] = '&';
			dst[j++] = '#';
			sprintf(tmp, "%i;", (int)(unsigned char)src[i]); t=strlen(tmp); c=0;
			while(t--) {dst[j++] = tmp[c++];}
		}
		else dst[j++] = src[i];
	}
	dst[j] = '\0';

	if(cpy2src) strncpy(src, dst, MAX_LINE_LEN);
}

/*
static void utf8dec(char *dst, char *src)
{
	size_t j=0;
	size_t n=strlen(src); u8 c;
	for(size_t i=0; i<n; i++)
	{
		c=(src[i]&0xFF);
		if(c<0x80)
			dst[j++]=c;
		else if(c & 0x20)
			dst[j++]=(((src[i++] & 0x1F)<<6)+(c & 0x3F));
		else
			dst[j++]=(((src[i++] & 0xF)<<12)+((src[i++] & 0x3F)<<6)+(c & 0x3F));
	}
	dst[j] = '\0';
}
*/

static int val(const char *c)
{
    int previous_result=0, result=0;
    int multiplier=1;

    if(c && *c == '-')
    {
        multiplier = -1;
        c++;
    }

    while(*c)
    {
        if(*c < '0' || *c > '9') return result * multiplier;

        result *= 10;
        if(result < previous_result)
            return(0);
        else
            previous_result *= 10;

        result += *c - '0';
        if(result < previous_result)
            return(0);
        else
            previous_result += *c - '0';

        c++;
    }
    return(result * multiplier);
}

#ifdef PS3MAPI

static int Char2Int(char input)
{
  if(input >= '0' && input <= '9')
    return input - '0';
  if(input >= 'A' && input <= 'F')
    return input - 'A' + 10;
  if(input >= 'a' && input <= 'f')
    return input - 'a' + 10;
  return NULL;
}

static void Hex2Bin(const char* src, char* target)
{
  while(*src && src[1])
  {
    *(target++) = Char2Int(*src)*16 + Char2Int(src[1]);
    src += 2;
  }
}

#endif

static int connect_to_server(char *server, uint16_t port)
{
	struct sockaddr_in sin;
	unsigned int temp;
	int s;

	if((temp = inet_addr(server)) != (unsigned int)-1)
	{
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = temp;
	}
	else
	{
		struct hostent *hp;

		if((hp = gethostbyname(server)) == NULL)
		{
			return FAILED;
		}

		sin.sin_family = hp->h_addrtype;
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
	}

	sin.sin_port = htons(port);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(s < 0)
	{
		return FAILED;
	}

	struct timeval tv;
	tv.tv_usec = 0;

	tv.tv_sec = 3;
	//setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		return FAILED;
	}

	tv.tv_sec = 60;
	//setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	return s;
}


#ifdef COBRA_ONLY

#ifndef LITE_EDITION
static int remote_stat(int s, char *path, int *is_directory, int64_t *file_size, uint64_t *mtime, uint64_t *ctime, uint64_t *atime, int *abort_connection)
{
	netiso_stat_cmd cmd;
	netiso_stat_result res;
	int len;

	*abort_connection = 1;

	len = strlen(path);
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_STAT_FILE);
	cmd.fp_len = (len);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (remote_stat) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(send(s, path, len, 0) != len)
	{
		//DPRINTF("send failed (remote_stat) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (remote_stat) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	*abort_connection = 0;

	*file_size = (res.file_size);
	if(*file_size == -1)
		return FAILED;

	*is_directory = res.is_directory;
	*mtime = (res.mtime);
	*ctime = (res.ctime);
	*atime = (res.atime);

	return 0;
}

static int read_remote_file(int s, void *buf, uint64_t offset, uint32_t size, int *abort_connection)
{
	netiso_read_file_cmd cmd;
	netiso_read_file_result res;

	*abort_connection = 1;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_READ_FILE);
	cmd.offset = (offset);
	cmd.num_bytes = (size);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (read_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (read_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	*abort_connection = 0;

	int bytes_read = (res.bytes_read);
	if(bytes_read <= 0)
		return bytes_read;

	if(recv(s, buf, bytes_read, MSG_WAITALL) != bytes_read)
	{
		//DPRINTF("recv failed (read_remote_file) (errno=%d)!\n", get_network_error());
		*abort_connection = 1;
		return FAILED;
	}

	return bytes_read;
}

static int64_t open_remote_file_2(int s, char *path, int *abort_connection)
{
	netiso_open_cmd cmd;
	netiso_open_result res;
	int len;

	*abort_connection = 1;

	len = strlen(path);
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = BE16(NETISO_CMD_OPEN_FILE);
	cmd.fp_len = BE16(len);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (open_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(send(s, path, len, 0) != len)
	{
		//DPRINTF("send failed (open_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (open_remote_file) (errno=%d)!\n", get_network_error());
		return FAILED;
	}

	*abort_connection = 0;

	return (res.file_size);
}

static int64_t open_remote_file(char *path)
{
	netiso_open_cmd cmd;
	netiso_open_result res;
	int len;

	len = strlen(path);

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = NETISO_CMD_OPEN_FILE;
	cmd.fp_len = len;

	if(send(g_socket, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (open_file) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(send(g_socket, path, len, 0) != len)
	{
		//DPRINTF("send failed (open_file) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(recv(g_socket, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (open_file) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(res.file_size == -1)
	{
		//DPRINTF("Remote file %s doesn't exist!\n", path);
		return FAILED;
	}

	//DPRINTF("Remote file %s opened. Size = %x%08lx bytes\n", path, (res.file_size>>32), res.file_size&0xFFFFFFFF);
	return res.file_size;
}

static int read_remote_file_critical(uint64_t offset, void *buf, uint32_t size)
{
	netiso_read_file_critical_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = NETISO_CMD_READ_FILE_CRITICAL;
	cmd.num_bytes = size;
	cmd.offset = offset;

	if(send(g_socket, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (read file) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(recv(g_socket, buf, size, MSG_WAITALL) != (int)size)
	{
		//DPRINTF("recv failed (recv file)  (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	return 0;
}

static int process_read_cd_2048_cmd(uint8_t *buf, uint32_t start_sector, uint32_t sector_count)
{
	netiso_read_cd_2048_critical_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = NETISO_CMD_READ_CD_2048_CRITICAL;
	cmd.start_sector = start_sector;
	cmd.sector_count = sector_count;

	if(send(g_socket, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (read 2048) (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	if(recv(g_socket, buf, sector_count * CD_SECTOR_SIZE_2048, MSG_WAITALL) != (int)(sector_count * CD_SECTOR_SIZE_2048))
	{
		//DPRINTF("recv failed (read 2048)  (errno=%d)!\n", sys_net_errno);
		return FAILED;
	}

	return 0;
}

static int process_read_iso_cmd(uint8_t *buf, uint64_t offset, uint32_t size)
{
	uint64_t read_end;

	//DPRINTF("read iso: %p %lx %x\n", buf, offset, size);
	read_end = offset + size;

	if(read_end >= discsize)
	{
		//DPRINTF("Read beyond limits: %llx %x (discsize=%llx)!\n", offset, size, discsize);

		if(offset >= discsize)
		{
			memset(buf, 0, size);
			return 0;
		}

		memset(buf+(discsize-offset), 0, read_end-discsize);
		size = discsize-offset;
	}

	return read_remote_file_critical(offset, buf, size);
}

static int process_read_cd_2352_cmd(uint8_t *buf, uint32_t sector, uint32_t remaining)
{
	int cache = 0;

	if(remaining <= CD_CACHE_SIZE)
	{
		int dif = (int)cached_cd_sector-sector;

		if(ABS(dif) < CD_CACHE_SIZE)
		{
			uint8_t *copy_ptr = NULL;
			uint32_t copy_offset = 0;
			uint32_t copy_size = 0;

			if(dif > 0)
			{
				if(dif < (int)remaining)
				{
					copy_ptr = cd_cache;
					copy_offset = dif;
					copy_size = remaining-dif;
				}
			}
			else
			{

				copy_ptr = cd_cache+((-dif) * CD_SECTOR_SIZE_2352);
				copy_size = MIN((int)remaining, CD_CACHE_SIZE+dif);
			}

			if(copy_ptr)
			{
				memcpy(buf+(copy_offset * CD_SECTOR_SIZE_2352), copy_ptr, copy_size * CD_SECTOR_SIZE_2352);

				if(remaining == copy_size)
				{
					return 0;
				}

				remaining -= copy_size;

				if(dif <= 0)
				{
					uint32_t newsector = cached_cd_sector + CD_CACHE_SIZE;
					buf += ((newsector-sector) * CD_SECTOR_SIZE_2352);
					sector = newsector;
				}
			}
		}

		cache = 1;
	}

	if(!cache)
	{
		return process_read_iso_cmd(buf, sector * CD_SECTOR_SIZE_2352, remaining * CD_SECTOR_SIZE_2352);
	}

	if(!cd_cache)
	{
		sys_addr_t addr=0;

		int ret = sys_memory_allocate(_192KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr);
		if(ret != 0)
		{
			//DPRINTF("sys_memory_allocate failed: %x\n", ret);
			return ret;
		}

		cd_cache = (uint8_t *)addr;
	}

	if(process_read_iso_cmd(cd_cache, sector * CD_SECTOR_SIZE_2352, CD_CACHE_SIZE * CD_SECTOR_SIZE_2352) != 0)
		return FAILED;

	memcpy(buf, cd_cache, remaining * CD_SECTOR_SIZE_2352);
	cached_cd_sector = sector;
	return 0;
}
#endif //#ifndef LITE_EDITION

static int sys_storage_ext_mount_discfile_proxy(sys_event_port_t result_port, sys_event_queue_t command_queue, int emu_type, uint64_t disc_size_bytes, uint32_t read_size, unsigned int trackscount, ScsiTrackDescriptor *tracks)
{
	system_call_8(SC_COBRA_SYSCALL8, SYSCALL8_OPCODE_MOUNT_DISCFILE_PROXY, result_port, command_queue, emu_type, disc_size_bytes, read_size, trackscount, (uint64_t)(uint32_t)tracks);
	return (int)p1;
}

static int fake_insert_event(uint64_t devicetype, uint64_t disctype)
{
	uint64_t param = (uint64_t)(disctype) << 32ULL;
	sys_storage_ext_fake_storage_event(7, 0, devicetype);
	return sys_storage_ext_fake_storage_event(3, param, devicetype);
}

static int fake_eject_event(uint64_t devicetype)
{
	sys_storage_ext_fake_storage_event(4, 0, devicetype);
	return sys_storage_ext_fake_storage_event(8, 0, devicetype);
}


#ifndef LITE_EDITION
static void netiso_thread(uint64_t arg)
{
	netiso_args *args;
	sys_event_port_t result_port;
	sys_event_queue_attribute_t queue_attr;
	unsigned int real_disctype;
	int64_t ret64;
	int ret;
	ScsiTrackDescriptor *tracks;
	int emu_mode, num_tracks;
	unsigned int cd_sector_size_param = 0;

	args = (netiso_args *)(uint32_t)arg;

	sector_size = 0x200;

	//DPRINTF("Hello VSH\n");

	g_socket = connect_to_server(args->server, args->port);
	if(g_socket < 0)
	{
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(0);
	}

	ret64 = open_remote_file(args->path);
	if(ret64 < 0)
	{
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(0);
	}

	discsize = (uint64_t)ret64;

	ret = sys_event_port_create(&result_port, 1, SYS_EVENT_PORT_NO_NAME);
	if(ret != 0)
	{
		//DPRINTF("sys_event_port_create failed: %x\n", ret);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	sys_event_queue_attribute_initialize(queue_attr);
	ret = sys_event_queue_create(&command_queue, &queue_attr, 0, 1);
	if(ret != 0)
	{
		//DPRINTF("sys_event_queue_create failed: %x\n", ret);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	CD_SECTOR_SIZE_2352 = 2352;

	emu_mode = args->emu_mode & 0xF;
	if(emu_mode == EMU_PSX)
	{
		num_tracks = args->num_tracks;
		tracks = &args->tracks[0];
		is_cd2352 = 1;
	}
	else
	{
		num_tracks = 0;
		tracks = NULL;
		is_cd2352 = 0;
	}

	sys_memory_free((sys_addr_t)args);
	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

	if(real_disctype != 0)
	{
		fake_eject_event(BDVD_DRIVE);
	}

	if(discsize % CD_SECTOR_SIZE_2352)
	{
		discsize = discsize - (discsize % CD_SECTOR_SIZE_2352);
	}

	ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue, emu_mode, discsize, _256KB_, (num_tracks | cd_sector_size_param), tracks);
	//DPRINTF("mount = %x\n", ret);

	fake_insert_event(BDVD_DRIVE, real_disctype);

	if(ret != 0)
	{
		sys_event_port_destroy(result_port);
		sys_ppu_thread_exit(0);
	}

	netiso_loaded=1;
	while(netiso_loaded)
	{
		sys_event_t event;

		ret = sys_event_queue_receive(command_queue, &event, 0);
		if(ret != 0)
		{
			//DPRINTF("sys_event_queue_receive failed: %x\n", ret);
			break;
		}

		void *buf = (void *)(uint32_t)(event.data3>>32ULL);
		uint64_t offset = event.data2;
		uint32_t size = event.data3&0xFFFFFFFF;

		switch(event.data1)
		{
			case CMD_READ_ISO:
			{
				if(is_cd2352)
				{
					ret = process_read_cd_2048_cmd(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048);
				}
				else
				{
					ret = process_read_iso_cmd(buf, offset, size);
				}
			}
			break;

			case CMD_READ_CD_ISO_2352:
			{
				ret = process_read_cd_2352_cmd(buf, offset / CD_SECTOR_SIZE_2352, size / CD_SECTOR_SIZE_2352);
			}
			break;
		}

		ret = sys_event_port_send(result_port, ret, 0, 0);
		if(ret != 0)
		{
			//DPRINTF("sys_event_port_send failed: %x\n", ret);
			break;
		}
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
	fake_eject_event(BDVD_DRIVE);
	sys_storage_ext_umount_discfile();

	if(real_disctype != 0)
	{
		fake_insert_event(BDVD_DRIVE, real_disctype);
	}

	if(cd_cache)
	{
		sys_memory_free((sys_addr_t)cd_cache);
	}

	if(g_socket >= 0)
	{
		shutdown(g_socket, SHUT_RDWR);
		socketclose(g_socket);
		g_socket = -1;
	}

	sys_event_port_disconnect(result_port);
	if(sys_event_port_destroy(result_port) != 0)
	{
		//DPRINTF("Error destroyng result_port\n");
	}

	//DPRINTF("Exiting main thread!\n");
	netiso_loaded = 0;
	sys_ppu_thread_exit(0);
}

static void netiso_stop_thread(uint64_t arg)
{
	uint64_t exit_code;
	netiso_loaded = 1;

	if(g_socket >= 0)
	{
		shutdown(g_socket, SHUT_RDWR);
		socketclose(g_socket);
		g_socket = -1;
	}

	if(command_queue != (sys_event_queue_t)-1)
	{
		if(sys_event_queue_destroy(command_queue, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
		{
			//DPRINTF("Failed in destroying command_queue\n");
		}
	}

	if(thread_id_net != (sys_ppu_thread_t)-1)
	{
		sys_ppu_thread_join(thread_id_net, &exit_code);
	}

	netiso_loaded = 0;
	sys_ppu_thread_exit(0);
}
#endif //#ifndef LITE_EDITION

static inline void get_next_read(uint64_t discoffset, uint64_t bufsize, uint64_t *offset, uint64_t *readsize, int *idx)
{
	uint64_t base = 0;
	*idx = -1;
	*readsize = bufsize;
	*offset = 0;

	for(uint32_t i = 0; i < num_sections; i++)
	{
		uint64_t last = base + ((uint64_t)sections_size[i] * sector_size);

		if(discoffset >= base && discoffset < last)
		{
			uint64_t maxfileread = last-discoffset;

			if(bufsize > maxfileread)
				*readsize = maxfileread;
			else
				*readsize = bufsize;

			*idx = i;
			*offset = discoffset-base;
			return;
		}

		base += ((uint64_t)sections_size[i] * sector_size);
	}

	// We can be here on video blu-ray
	//DPRINTF("Offset or size out of range  %lx%08lx   %lx!!!!!!!!\n", discoffset>>32, discoffset, bufsize);
}

static int process_read_iso_cmd_iso(uint8_t *buf, uint64_t offset, uint64_t size)
{
	uint64_t remaining;

	//DPRINTF("read iso: %p %lx %lx\n", buf, offset, size);
	remaining = size;

	while(remaining > 0)
	{
		uint64_t pos, readsize;
		int idx;
		int ret;
		uint8_t tmp[sector_size];
		uint32_t sector;
		uint32_t r;

		get_next_read(offset, remaining, &pos, &readsize, &idx);

		if(idx == -1 || sections[idx] == 0xFFFFFFFF)
		{
			memset(buf, 0, readsize);
			buf += readsize;
			offset += readsize;
			remaining -= readsize;
			continue;
		}

		if(pos & (sector_size-1))
		{
			uint64_t csize;

			sector = sections[idx] + pos/sector_size;
			ret = sys_storage_read(handle, 0, sector, 1, tmp, &r, 0);
			if(ret != 0 || r != 1)
			{
				//DPRINTF("sys_storage_read failed: %x 1 -> %x\n", sector, ret);
				return FAILED;
			}

			csize = sector_size-(pos&(sector_size-1));

			if(csize > readsize)
				csize = readsize;

			memcpy(buf, tmp+(pos&(sector_size-1)), csize);
			buf += csize;
			offset += csize;
			pos += csize;
			remaining -= csize;
			readsize -= csize;
		}

		if(readsize > 0)
		{
			uint32_t n = readsize / sector_size;

			if(n > 0)
			{
				uint64_t s;

				sector = sections[idx] + pos/sector_size;
				ret = sys_storage_read(handle, 0, sector, n, buf, &r, 0);
				if(ret != 0 || r != n)
				{
					//DPRINTF("sys_storage_read failed: %x %x -> %x\n", sector, n, ret);
					return FAILED;
				}

				s = n * sector_size;
				buf += s;
				offset += s;
				pos += s;
				remaining -= s;
				readsize -= s;
			}

			if(readsize > 0)
			{
				sector = sections[idx] + pos/sector_size;
				ret = sys_storage_read(handle, 0, sector, 1, tmp, &r, 0);
				if(ret != 0 || r != 1)
				{
					//DPRINTF("sys_storage_read failed: %x 1 -> %x\n", sector, ret);
					return FAILED;
				}

				memcpy(buf, tmp, readsize);
				buf += readsize;
				offset += readsize;
				remaining -= readsize;
			}
		}
	}

	return 0;
}

static inline void my_memcpy(uint8_t *dst, uint8_t *src, uint16_t size)
{
	for(uint16_t i = 0; i < size; i++)
		dst[i] = src[i];
}

static int process_read_cd_2048_cmd_iso(uint8_t *buf, uint32_t start_sector, uint32_t sector_count)
{
	uint32_t capacity = sector_count * CD_SECTOR_SIZE_2048;
	uint32_t fit = capacity / CD_SECTOR_SIZE_2352;
	uint32_t rem = (sector_count-fit);
	uint32_t i;
	uint8_t *in = buf;
	uint8_t *out = buf;

	if(fit > 0)
	{
		process_read_iso_cmd_iso(buf, start_sector * CD_SECTOR_SIZE_2352, fit * CD_SECTOR_SIZE_2352);

		for(i = 0; i < fit; i++)
		{
			my_memcpy(out, in+24, CD_SECTOR_SIZE_2048);
			in  += CD_SECTOR_SIZE_2352;
			out += CD_SECTOR_SIZE_2048;
			start_sector++;
		}
	}

	for(i = 0; i < rem; i++)
	{
		process_read_iso_cmd_iso(out, (start_sector * CD_SECTOR_SIZE_2352) + 24, CD_SECTOR_SIZE_2048);
		out += CD_SECTOR_SIZE_2048;
		start_sector++;
	}

	return 0;
}

static int process_read_cd_2352_cmd_iso(uint8_t *buf, uint32_t sector, uint32_t remaining)
{
	int cache = 0;

	if(remaining <= CD_CACHE_SIZE)
	{
		int dif = (int)cached_cd_sector-sector;

		if(ABS(dif) < CD_CACHE_SIZE)
		{
			uint8_t *copy_ptr = NULL;
			uint32_t copy_offset = 0;
			uint32_t copy_size = 0;

			if(dif > 0)
			{
				if(dif < (int)remaining)
				{
					copy_ptr = cd_cache;
					copy_offset = dif;
					copy_size = remaining-dif;
				}
			}
			else
			{
				copy_ptr = cd_cache+((-dif) * CD_SECTOR_SIZE_2352);
				copy_size = MIN((int)remaining, CD_CACHE_SIZE+dif);
			}

			if(copy_ptr)
			{
				memcpy(buf+(copy_offset * CD_SECTOR_SIZE_2352), copy_ptr, copy_size * CD_SECTOR_SIZE_2352);

				if(remaining == copy_size)
				{
					return 0;
				}

				remaining -= copy_size;

				if(dif <= 0)
				{
					uint32_t newsector = cached_cd_sector + CD_CACHE_SIZE;
					buf += ((newsector-sector) * CD_SECTOR_SIZE_2352);
					sector = newsector;
				}
			}
		}

		cache = 1;
	}

	if(!cache)
	{
		return process_read_iso_cmd_iso(buf, sector * CD_SECTOR_SIZE_2352, remaining * CD_SECTOR_SIZE_2352);
	}

	if(!cd_cache)
	{
		sys_addr_t addr=0;

		int ret = sys_memory_allocate(_192KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr);
		if(ret != 0)
		{
			//DPRINTF("sys_memory_allocate failed: %x\n", ret);
			return ret;
		}

		cd_cache = (uint8_t *)addr;
	}

	if(process_read_iso_cmd_iso(cd_cache, sector * CD_SECTOR_SIZE_2352, CD_CACHE_SIZE * CD_SECTOR_SIZE_2352) != 0)
		return FAILED;

	memcpy(buf, cd_cache, remaining * CD_SECTOR_SIZE_2352);
	cached_cd_sector = sector;
	return 0;
}

static void rawseciso_stop_thread(uint64_t arg)
{
	uint64_t exit_code;
	rawseciso_loaded = 1;

	if(command_queue_ntfs != (sys_event_queue_t)-1)
	{
		if(sys_event_queue_destroy(command_queue_ntfs, SYS_EVENT_QUEUE_DESTROY_FORCE) != 0)
		{
			//DPRINTF("Failed in destroying command_queue\n");
		}
	}

	if(thread_id_ntfs != (sys_ppu_thread_t)-1)
	{
		sys_ppu_thread_join(thread_id_ntfs, &exit_code);
	}

	rawseciso_loaded = 0;
	sys_ppu_thread_exit(0);
}

static void rawseciso_thread(uint64_t arg)
{
	rawseciso_args *args;
	sys_event_port_t result_port;
	sys_event_queue_attribute_t queue_attr;
	unsigned int real_disctype;
	int ret;
	ScsiTrackDescriptor *tracks;
	int emu_mode, num_tracks;
	int cd_sector_size_param = 0;

	CD_SECTOR_SIZE_2352 = 2352;

	args = (rawseciso_args *)(uint32_t)arg;

	num_sections = args->num_sections;
	sections = (uint32_t *)(args+1);
	sections_size = sections + num_sections;

	sector_size = 0x200;
	if(args->device!=0)
	{
		for(u8 retry = 0; retry < 16; retry++)
		{
			if(sys_storage_get_device_info(args->device, &disc_info) == 0) {sector_size = (uint32_t) disc_info.sector_size; break;}
			sys_timer_usleep(500000);
		}
	}

	discsize = 0;

	for(uint32_t i = 0; i < num_sections; i++)
	{
		discsize += sections_size[i];
	}

	discsize = discsize * sector_size;

	emu_mode = args->emu_mode & 0xF;
	if(emu_mode == EMU_PSX)
	{
		num_tracks = args->num_tracks;

        if(num_tracks > 100)
        {
            CD_SECTOR_SIZE_2352 = (num_tracks & 0xffff00)>>4;
            if(CD_SECTOR_SIZE_2352 != 2352 && CD_SECTOR_SIZE_2352 != 2048 && CD_SECTOR_SIZE_2352 != 2336 && CD_SECTOR_SIZE_2352 != 2448) CD_SECTOR_SIZE_2352 = 2352;
        }

        if(CD_SECTOR_SIZE_2352 != 2352) cd_sector_size_param = CD_SECTOR_SIZE_2352<<4;

		num_tracks &= 0xff;

		tracks = (ScsiTrackDescriptor *)(sections_size + num_sections);
		is_cd2352 = 1;
	}
	else
	{
		num_tracks = 0;
		tracks = NULL;
		is_cd2352 = 0;
	}

	//DPRINTF("discsize = %lx%08lx\n", discsize>>32, discsize);

	ret = sys_storage_open(args->device, 0, &handle, 0);
	if(ret != 0)
	{
		//DPRINTF("sys_storage_open failed: %x\n", ret);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	ret = sys_event_port_create(&result_port, 1, SYS_EVENT_PORT_NO_NAME);
	if(ret != 0)
	{
		//DPRINTF("sys_event_port_create failed: %x\n", ret);
		sys_storage_close(handle);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	sys_event_queue_attribute_initialize(queue_attr);
	ret = sys_event_queue_create(&command_queue_ntfs, &queue_attr, 0, 1);
	if(ret != 0)
	{
		//DPRINTF("sys_event_queue_create failed: %x\n", ret);
		sys_event_port_destroy(result_port);
		sys_storage_close(handle);
		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(ret);
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);

	if(real_disctype != 0)
	{
		fake_eject_event(BDVD_DRIVE);
	}

	if(is_cd2352)
	{
		if(discsize % CD_SECTOR_SIZE_2352)
		{
			discsize = discsize - (discsize % CD_SECTOR_SIZE_2352);
		}
	}

	ret = sys_storage_ext_mount_discfile_proxy(result_port, command_queue_ntfs, emu_mode, discsize, _256KB_, (num_tracks | cd_sector_size_param), tracks);
	//DPRINTF("mount = %x\n", ret);


	fake_insert_event(BDVD_DRIVE, real_disctype);

	if(ret != 0)
	{
		sys_event_port_disconnect(result_port);
		// Queue destroyed in stop thread sys_event_queue_destroy(command_queue_ntfs);
		sys_event_port_destroy(result_port);
		sys_storage_close(handle);

		sys_memory_free((sys_addr_t)args);
		sys_ppu_thread_exit(0);
	}

	rawseciso_loaded=1;
	while(rawseciso_loaded)
	{
		sys_event_t event;

		ret = sys_event_queue_receive(command_queue_ntfs, &event, 0);
		if(ret != 0)
		{
			//DPRINTF("sys_event_queue_receive failed: %x\n", ret);
			break;
		}

		void *buf = (void *)(uint32_t)(event.data3>>32ULL);
		uint64_t offset = event.data2;
		uint32_t size = event.data3&0xFFFFFFFF;

		switch(event.data1)
		{
			case CMD_READ_ISO:
			{
				if(is_cd2352)
				{
					ret = process_read_cd_2048_cmd_iso(buf, offset / CD_SECTOR_SIZE_2048, size / CD_SECTOR_SIZE_2048);
				}
				else
				{
					ret = process_read_iso_cmd_iso(buf, offset, size);
				}
			}
			break;

			case CMD_READ_CD_ISO_2352:
			{
				ret = process_read_cd_2352_cmd_iso(buf, offset / CD_SECTOR_SIZE_2352, size / CD_SECTOR_SIZE_2352);
			}
			break;
		}

		ret = sys_event_port_send(result_port, ret, 0, 0);
		if(ret != 0)
		{
			//DPRINTF("sys_event_port_send failed: %x\n", ret);
			break;
		}
	}

	sys_storage_ext_get_disc_type(&real_disctype, NULL, NULL);
	fake_eject_event(BDVD_DRIVE);
	sys_storage_ext_umount_discfile();

	if(real_disctype != 0)
	{
		fake_insert_event(BDVD_DRIVE, real_disctype);
	}

	if(cd_cache)
	{
		sys_memory_free((sys_addr_t)cd_cache);
	}

	sys_storage_close(handle);

	sys_event_port_disconnect(result_port);
	if(sys_event_port_destroy(result_port) != 0)
	{
		//DPRINTF("Error destroyng result_port\n");
	}

	// queue destroyed in stop thread

	//DPRINTF("Exiting main thread!\n");
	rawseciso_loaded = 0;

	sys_memory_free((sys_addr_t)args);
	sys_ppu_thread_exit(0);
}
#endif //#ifdef COBRA_ONLY

static void absPath(char* absPath_s, const char* path, const char* cwd)
{
	if(path[0] == '/') strcpy(absPath_s, path);
	else
	{
		strcpy(absPath_s, cwd);

		if(cwd[strlen(cwd) - 1] != '/') strcat(absPath_s, "/");

		strcat(absPath_s, path);
	}

	if(strstr(absPath_s, "/dev_blind") && !isDir("/dev_blind")) enable_dev_blind(NULL);
}

static inline uint64_t peekq(uint64_t addr) //lv2
{
	system_call_1(SC_PEEK_LV2, addr);
	return_to_user_prog(uint64_t);
}

static inline void pokeq( uint64_t addr, uint64_t val) //lv2
{
	system_call_2(SC_POKE_LV2, addr, val);
}

static inline uint64_t peek_lv1(uint64_t addr)
{
	system_call_1(SC_PEEK_LV1, (uint64_t) addr);
	return (uint64_t) p1;
}

static inline void poke_lv1( uint64_t addr, uint64_t val)
{
	system_call_2(SC_POKE_LV1, addr, val);
}


//////////////////////////////////////////////////////////////////////////////
#ifdef VIDEO_REC

#include "vsh/vsh.h"
#include "vsh/vshmain.h"
#include "vsh/rec_plugin.h"

#define PLUGIN_NAME "rec_plugin"

void toggle_video_rec(void);

bool recording = false;

uint32_t *recOpt = NULL;              // recording utility vsh options struct
int32_t (*reco_open)(int32_t) = NULL; // base pointer

bool rec_start(void);

bool rec_start()
{
	recOpt[1] = 0x4660;//CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS | 0x2100; //CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS
	recOpt[2] = 0x0000; //CELL_REC_PARAM_AUDIO_FMT_AAC_96K
	recOpt[5] = (vsh_memory_container_by_id(1) == -1 ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(1);
	recOpt[0x208] = 0x80; // 0x90 show XMB || reduce memsize // 0x80; // allow show XMB

	CellRtcDateTime t;
	cellRtcGetCurrentClockLocalTime(&t);

	char g[0x120];
	game_interface = (game_plugin_interface *)plugin_GetInterface(View_Find("game_plugin"), 1);

	game_interface->gameInfo(g);

	cellFsMkdir((char*)"/dev_hdd0/VIDEO", 0777);
	sprintf((char*)&recOpt[0x6], "/dev_hdd0/VIDEO/%s_%04d.%02d.%02d_%02d_%02d_%02d.mp4",
								   g+0x04, t.year, t.month, t.day, t.hour, t.minute, t.second);

	reco_open(-1); // memory container
	sys_timer_sleep(4);


	if(View_Find("rec_plugin") != 0)
	{
		rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

		if(rec_interface != 0)
		{
			rec_interface->start();
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		reco_open(-1); //reco_open((vsh_memory_container_by_id(1) == -1 ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(1));
		sys_timer_sleep(3);

		if(View_Find("rec_plugin") != 0)
		{
			rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

			rec_interface->start();
			return true;
		}
		else
		{
			show_msg((char*)"No rec_plugin view found.");
			return false;
		}
	}
}

void toggle_video_rec(void)
{
	if(View_Find("game_plugin") != 0)    // if game_plugin is loaded -> there is a game/app running and we can recording...
	{
		if(!reco_open)
		{
			// get functions pointer for sub_163EB0() aka reco_open()
			reco_open = vshmain_BEF63A14; // base pointer, the export nearest to sub_163EB0()

			reco_open -= (50*8); // reco_open_opd (50 opd's above vshmain_BEF63A14_opd)

			// fetch recording utility vsh options struct (build address from instructions...)
			uint32_t addr = (*(uint32_t*)(*(uint32_t*)reco_open+0xC) & 0x0000FFFF) -1;
			recOpt = (uint32_t*)((addr << 16) + ((*(uint32_t*)(*(uint32_t*)reco_open+0x14)) & 0x0000FFFF)); // (uint32_t*)0x72EEC0;
		}

		if(recording == false)
		{
		  // not recording yet
			show_msg((char*)"Recording started");

			if(rec_start() == false)
			{
				show_msg((char*)"Recording Error!");
			}
			else
			{
				recording = true;
			}
		}
		else
		{
			// we are already recording
			rec_interface->stop();
			rec_interface->close(0);
			show_msg((char*)"Recording finished");
			recording = false;
		}
	}
	else recording = false;
}
#endif // #ifdef VIDEO_REC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ENGLISH_ONLY
uint32_t get_system_language(uint8_t *lang)
{
	int reg = -1;
	u32 val_lang = 0; // default english
	u16 off_string, len_data, len_string;
	u64 read, pos, off_val_data;
	CellFsStat stat;
	char string[256];

	if(cellFsOpen("/dev_flash2/etc/xRegistry.sys", CELL_FS_O_RDONLY, &reg, NULL, 0) != CELL_FS_SUCCEEDED || reg == -1)
	{
		*lang = 0;
		return val_lang;
	}

	cellFsStat("/dev_flash2/etc/xRegistry.sys", &stat);

	u64 entry_name = 0x10000;

	while(true)
	{
	/** Data entries **/
		//unk
		entry_name += 2;

		//relative entry offset
		cellFsLseek(reg, entry_name, 0, &pos);
		cellFsRead(reg, &off_string, 2, &read);
		entry_name += 2;

		//unk2
		entry_name += 2;

		//data lenght
		cellFsLseek(reg, entry_name, 0, &pos);
		cellFsRead(reg, &len_data, 2, &read);
		entry_name += 2;

		//data type
		entry_name += 1;

		//data
		off_val_data = entry_name;
		entry_name += len_data;

		//Separator 0x0
		entry_name += 1;

	/** String Entries **/
		off_string += 0x10;

		//unk
		off_string += 2;

		//string length
		cellFsLseek(reg, off_string, 0, &pos);
		cellFsRead(reg, &len_string, 2, &read);
		off_string += 2;

		//string type
		off_string += 1;

		//string
		memset(string, 0, sizeof(string));
		cellFsLseek(reg, off_string, 0, &pos);
		cellFsRead(reg, string, len_string, &read);

		//Find language
		if(strcmp(string, "/setting/system/language") == 0)
		{
			cellFsLseek(reg, off_val_data, 0, &pos);
			cellFsRead(reg, &val_lang, 4, &read);
			break;
		}

		if(off_string == 0xCCDD || entry_name >= stat.st_size) break;
	}

	switch(val_lang)
	{
		case 0x0:
			*lang = 4;		//ger;
			break;
		//case 0x1:
		//	lang = 0;		//eng-us
		//	break;
		case 0x2:
			*lang = 3;		//spa
			break;
		case 0x3:
			*lang = 1;		//fre
			break;
		case 0x4:
			*lang = 2;		//ita
			break;
		case 0x5:
			*lang = 5;		//dut //Olandese
			break;
		case 0x6:
			*lang = 6;		//por-por
			break;
		case 0x7:
			*lang = 7;		//rus
			break;
		case 0x8:
			*lang = 18;		//jap
			break;
		case 0x9:
			*lang = 17;		//kor
			break;
		case 0xA:
		case 0xB:
			*lang = 19;		//chi-tra / chi-sim
			break;
		//case 0xC:
		//	*lang = 0;		//fin /** missing **/
		//	break;
		//case 0xD:
		//	*lang = 0;		//swe /** missing**/
		//	break;
		case 0xE:
			*lang = 20;		//dan
			break;
		//case 0xF:
		//	*lang = 0;		//nor /** missing**/
		//	break;
		case 0x10:
			*lang = 9;		//pol
			break;
		case 0x11:
			*lang = 12;		//por-bra
			break;
		//case 0x12:
		//	*lang = 0;		//eng-uk
		//	break;
		default:
			*lang = 0;
			break;
	}
	cellFsClose(reg);

	return val_lang;
}

static bool language(const char *file_str, char *default_str)
{
	uint64_t siz = 0;
	uint8_t i;
	int f=0;
	char temp[1];

	bool do_retry=true;

	if(fh) f=fh; //file is already open
    else
    {
		if(webman_config->lang>22 && webman_config->lang!=99) return false;

		const char lang_codes[24][3]={"EN", "FR", "IT", "ES", "DE", "NL", "PT", "RU", "HU", "PL", "GR", "HR", "BG", "IN", "TR", "AR", "CN", "KR", "JP", "ZH", "DK", "SK", "CZ", "XX"};
		char lang_path[34];

		i=webman_config->lang; if(i>23) i=23;

		sprintf(lang_code, "_%s", lang_codes[i]);
		sprintf(lang_path, "/dev_hdd0/tmp/wm_lang/LANG%s.TXT", lang_code);

		if(cellFsOpen(lang_path, CELL_FS_O_RDONLY, &f, NULL, 0) != CELL_FS_SUCCEEDED) return false;

		fh = f;

retry:
		cellFsLseek(f, lang_pos, CELL_FS_SEEK_SET, &siz);
	}

	do {
		cellFsRead(f, (void *)&temp, 0x01, &siz);
		lang_pos++;
		for(i=0; i < strlen(file_str); i++)
		{
			if(temp[0] != file_str[i]) break;
			else if(i==strlen(file_str)-1)
			{
				while(siz && temp[0] != '[')
				{
					cellFsRead(f, (void *)&temp, 0x01, &siz);
					lang_pos++;
				}
				int str_len = 0;
				while(siz)
				{
					cellFsRead(f, (void *)&temp, 0x01, &siz);
					lang_pos++;
					if(temp[0] == ']')
					{
						default_str[str_len] = NULL;
						return true;
					}
					default_str[str_len] = temp[0];
					str_len++;
				}
			}
			else
			{
				cellFsRead(f, (void *)&temp, 0x01, &siz);
				lang_pos++;
			}
		}
	} while(siz != 0);

	if(do_retry) {do_retry=false; lang_pos=0; goto retry;}

	return true;
}

static void update_language(void)
{
	lang_pos=fh=0;

	if(language("STR_TRADBY", STR_TRADBY))
	{
		language("STR_FILES", STR_FILES);
		language("STR_GAMES", STR_GAMES);
		language("STR_SETUP", STR_SETUP);
		language("STR_HOME", STR_HOME);
		language("STR_EJECT", STR_EJECT);
		language("STR_INSERT", STR_INSERT);
		language("STR_UNMOUNT", STR_UNMOUNT);
		language("STR_COPY", STR_COPY);
		language("STR_REFRESH", STR_REFRESH);
		language("STR_SHUTDOWN", STR_SHUTDOWN);
		language("STR_RESTART", STR_RESTART);

		language("STR_BYTE", STR_BYTE);
		language("STR_KILOBYTE", STR_KILOBYTE);
		language("STR_MEGABYTE", STR_MEGABYTE);
		language("STR_GIGABYTE", STR_GIGABYTE);

		language("STR_COPYING", STR_COPYING);
		language("STR_CPYDEST", STR_CPYDEST);
		language("STR_CPYFINISH", STR_CPYFINISH);
		language("STR_CPYABORT", STR_CPYABORT);
		language("STR_DELETE", STR_DELETE);

		language("STR_SCAN1", STR_SCAN1);
		language("STR_SCAN2", STR_SCAN2);
		language("STR_PSPL", STR_PSPL);
		language("STR_PS2L", STR_PS2L);
		language("STR_RXVID", STR_RXVID);
		language("STR_VIDLG", STR_VIDLG	);
		language("STR_LPG", STR_LPG);
		language("STR_AUTOB", STR_AUTOB);
		language("STR_DELAYAB", STR_DELAYAB);
		language("STR_DEVBL", STR_DEVBL);
		language("STR_CONTSCAN", STR_CONTSCAN);
		language("STR_USBPOLL", STR_USBPOLL);
		language("STR_FTPSVC", STR_FTPSVC);
		language("STR_FIXGAME", STR_FIXGAME);
		language("STR_COMBOS", STR_COMBOS);
		language("STR_MMCOVERS", STR_MMCOVERS);
		language("STR_ACCESS", STR_ACCESS);
		language("STR_NOSETUP", STR_NOSETUP);
		language("STR_NOSPOOF", STR_NOSPOOF);
		language("STR_NOGRP", STR_NOGRP);
		language("STR_NOWMDN", STR_NOWMDN);
#ifdef NOSINGSTAR
		language("STR_NOSINGSTAR", STR_NOSINGSTAR);
#endif
		language("STR_RESET_USB", STR_RESET_USB);
		language("STR_TITLEID", STR_TITLEID);
		language("STR_FANCTRL", STR_FANCTRL);
		language("STR_NOWARN", STR_NOWARN);
		language("STR_AUTOAT", STR_AUTOAT);
		language("STR_LOWEST", STR_LOWEST);
		language("STR_FANSPEED", STR_FANSPEED);
		language("STR_MANUAL", STR_MANUAL);
		language("STR_PS2EMU", STR_PS2EMU);
		language("STR_LANGAMES", STR_LANGAMES);
		language("STR_ANYUSB", STR_ANYUSB);
		language("STR_ADDUSB", STR_ADDUSB);
		language("STR_SPOOFID", STR_SPOOFID);
		language("STR_DELCFWSYS", STR_DELCFWSYS);
		language("STR_MEMUSAGE", STR_MEMUSAGE);
		language("STR_PLANG", STR_PLANG);
		language("STR_PROFILE", STR_PROFILE);
		language("STR_DEFAULT", STR_DEFAULT);
		language("STR_COMBOS2", STR_COMBOS2);
		language("STR_FAILSAFE", STR_FAILSAFE);
		language("STR_SHOWTEMP", STR_SHOWTEMP);
		language("STR_SHOWIDPS", STR_SHOWIDPS);
		language("STR_PREVGAME", STR_PREVGAME);
		language("STR_NEXTGAME", STR_NEXTGAME);
		language("STR_SHUTDOWN2", STR_SHUTDOWN2);
		language("STR_RESTART2", STR_RESTART2);
#ifdef REMOVE_SYSCALLS
		language("STR_DELCFWSYS2", STR_DELCFWSYS2);
#endif
		language("STR_UNLOADWM", STR_UNLOADWM);
		language("STR_FANCTRL2", STR_FANCTRL2);
		language("STR_FANCTRL4", STR_FANCTRL4);
		language("STR_FANCTRL5", STR_FANCTRL5);
		language("STR_UPDN", STR_UPDN);
		language("STR_LFRG", STR_LFRG);
#ifdef COBRA_ONLY
		language("STR_DISCOBRA", STR_DISCOBRA);
#endif
#ifdef REX_ONLY
		language("STR_RBGMODE", STR_RBGMODE);
		language("STR_RBGNORM", STR_RBGNORM);
		language("STR_RBGMENU", STR_RBGMENU);
#endif
		language("STR_SAVE", STR_SAVE);
		language("STR_SETTINGSUPD", STR_SETTINGSUPD);
		language("STR_ERROR", STR_ERROR);

		language("STR_MYGAMES", STR_MYGAMES);
		language("STR_LOADGAMES", STR_LOADGAMES);
		language("STR_FIXING", STR_FIXING);

		language("STR_WMSETUP", STR_WMSETUP);
		language("STR_WMSETUP2", STR_WMSETUP2);

		language("STR_EJECTDISC", STR_EJECTDISC);
		language("STR_UNMOUNTGAME", STR_UNMOUNTGAME);

        language("STR_WMSTART", STR_WMSTART);
		language("STR_WMUNL", STR_WMUNL);
		language("STR_CFWSYSALRD", STR_CFWSYSALRD);
		language("STR_CFWSYSRIP", STR_CFWSYSRIP);
		language("STR_RMVCFWSYS", STR_RMVCFWSYS);
		language("STR_RMVCFWSYSF", STR_RMVCFWSYSF);

		language("STR_RMVWMCFG", STR_RMVWMCFG);
		language("STR_RMVWMCFGOK", STR_RMVWMCFGOK);

		language("STR_PS3FORMAT", STR_PS3FORMAT);
		language("STR_PS2FORMAT", STR_PS2FORMAT);
		language("STR_PS1FORMAT", STR_PS1FORMAT);
		language("STR_PSPFORMAT", STR_PSPFORMAT);

		language("STR_VIDFORMAT", STR_VIDFORMAT);
		language("STR_VIDEO", STR_VIDEO);

		language("STR_LAUNCHPSP", STR_LAUNCHPSP);
		language("STR_LAUNCHPS2", STR_LAUNCHPS2);

		language("STR_GAMEUM", STR_GAMEUM);

		language("STR_EJECTED", STR_EJECTED);
		language("STR_LOADED", STR_LOADED);

		language("STR_GAMETOM", STR_GAMETOM);
		language("STR_GAMELOADED", STR_GAMELOADED);
		language("STR_PSPLOADED", STR_PSPLOADED);
		language("STR_PS2LOADED", STR_PS2LOADED);
		language("STR_LOADED2", STR_LOADED2);

		language("STR_MOVIETOM", STR_MOVIETOM);
		language("STR_MOVIELOADED", STR_MOVIELOADED);

		language("STR_XMLRF", STR_XMLRF);

		language("STR_STORAGE", STR_STORAGE);
		language("STR_MEMORY", STR_MEMORY);
		language("STR_MBFREE", STR_MBFREE);
		language("STR_KBFREE", STR_KBFREE);

		language("STR_FANCTRL3", STR_FANCTRL3);
		language("STR_ENABLED", STR_ENABLED);
		language("STR_DISABLED", STR_DISABLED);

		language("STR_FANCH0", STR_FANCH0);
		language("STR_FANCH1", STR_FANCH1);
		language("STR_FANCH2", STR_FANCH2);
		language("STR_FANCH3", STR_FANCH3);

		language("STR_OVERHEAT", STR_OVERHEAT);
		language("STR_OVERHEAT2", STR_OVERHEAT2);

		language("STR_NOTFOUND", STR_NOTFOUND);

		language("COVERS_PATH", COVERS_PATH);
		language("IP_ADDRESS", local_ip);
		language("SEARCH_URL", search_url);
	}

	if(fh) {cellFsClose(fh); lang_pos=fh=0;}
}
#endif //#ifndef ENGLISH_ONLY

uint64_t convertH(char *val)
{
	uint8_t buff;
	uint64_t ret=0;
	int i, n=0;

	for(i = 0; i < 16+n; i++)
	{
		if(val[i]==' ') {n++; continue;}

		if(val[i]>='0' && val[i]<='9') buff=(   val[i]-'0')*0x10; else
		if(val[i]>='A' && val[i]<='F') buff=(10+val[i]-'A')*0x10; else
		if(val[i]>='a' && val[i]<='f') buff=(10+val[i]-'a')*0x10; else
		return ret;

		i++;
		if(val[i]>='0' && val[i]<='9') buff+=(   val[i]-'0'); else
		if(val[i]>='A' && val[i]<='F') buff+=(10+val[i]-'A'); else
		if(val[i]>='a' && val[i]<='f') buff+=(10+val[i]-'a'); else
		{ret=(ret<<4)+(buff>>4); return ret;}

		ret = (ret << 8) | buff;
	}

	return ret;
}

/*
uint64_t find_syscall()
{
	uint64_t i = 0x8000000000600000ULL;
	while(i>0x8000000000000000ULL)	{
		if(peekq(i) == 0x3C60800160630003ULL)
			if(((peekq(i+8) >> 32) & 0xFFFFFFFF) == 0x4E800020)	return i;
		i-=4;
	}
	return 0;
}

uint64_t search64(uint64_t val)
{
	uint64_t i;
	for(i=0x8000000000080200ULL;0x8000000000600000ULL;i+=4)
	{
		if(peekq(i) == val)
			return i;
	}
	return 0;
}

uint64_t find_syscall_table()
{
	uint64_t sc, opd_sc;
	sc = find_syscall();
	opd_sc = search64(sc);
	return search64(opd_sc);
}
*/

static void get_idps_psid(void)
{
	if(c_firmware<=4.53f)
	{
		{system_call_1(SC_GET_IDPS, (uint64_t) IDPS);}
		{system_call_1(SC_GET_PSID, (uint64_t) PSID);}
	}
	else if(peekq(0x8000000000003000ULL)==SYSCALLS_UNAVAILABLE)
		return; // do not update IDPS/PSID if syscalls are removed
	else if(idps_offset2 | psid_offset)
	{
			IDPS[0] = peekq(idps_offset2  );
			IDPS[1] = peekq(idps_offset2+8);
			PSID[0] = peekq(psid_offset   );
			PSID[1] = peekq(psid_offset +8);
	}
}

#ifndef COBRA_ONLY
static void add_to_map(char *path1, char *path2)
{
	if(max_mapped==0) pokeq(MAP_BASE + 0x00, 0x0000000000000000ULL);

	if(max_mapped<10)
	{
		for(u8 n=0; n<max_mapped; n++)
		{
			if(!strcmp(file_to_map[n].src, path1)) goto leave;
		}
		sprintf(file_to_map[max_mapped].src, "%s", path1);
		sprintf(file_to_map[max_mapped].dst, "%s", path2);
		max_mapped++;
	}
leave:
	return;
}

static void string_to_lv2(char* path, u64 addr)
{
	u16 len=(strlen(path)+8)&0x7f8;
	len=RANGE(len, 8, 384);
	u16 len2=strlen(path); if(len2>len) len2=len;

	u8 data2[384];
	u8* data = data2;
	memset(data, 0, 384);
	memcpy(data, path, len2);

	u64 val=0x0000000000000000ULL;
	for(u64 n = 0; n < len; n += 8)
	{
		memcpy(&val, &data[n], 8);
		pokeq(addr+n, val);
	}
}
#endif

#ifdef EXT_GDATA
static int set_gamedata_status(u8 status, bool do_mount)
{
	char msg[100];
	char gamei_path[MAX_PATH_LEN]; u8 n;

#ifndef COBRA_ONLY
	sprintf(gamei_path, "//dev_hdd0/game");
	if(do_mount) max_mapped=0;
#endif

	if(status)
	{
		for(n=0; n<8; n++) {sprintf(gamei_path, "/dev_usb00%i/GAMEI", n); if(isDir(gamei_path)) break;}
		if(n>7)
		{
			for(n=0; n<8; n++) {sprintf(gamei_path, "/dev_usb00%i", n); if(isDir(gamei_path)) break;}
			if(n<8) {sprintf(gamei_path, "/dev_usb00%i/GAMEI", n); if(cellFsMkdir((char*)gamei_path, MODE)==CELL_FS_SUCCEEDED) n=99;}
		}

		if(n<8)
		{
#ifdef COBRA_ONLY

			sys_map_path((char*)"/dev_hdd0/game", gamei_path);
			if(isDir(MM_ROOT_STD)) sys_map_path((char*)MM_ROOT_STD, (char*)"/" MM_ROOT_STD);
#else
			if(isDir(MM_ROOT_STD)) add_to_map((char*)MM_ROOT_STD, (char*)MM_ROOT_STD);
			add_to_map((char*)"/dev_hdd0/game", gamei_path);
#endif
			sprintf(msg, "gameDATA %s (usb00%i)", STR_ENABLED, n);
		}
		else
		{
			extgd = 0;
#ifdef COBRA_ONLY
			sys_map_path((char*)"/dev_hdd0/game", NULL);
#endif
			sprintf(msg, (char*)"gameDATA %s (no usb)", STR_ERROR);
			show_msg((char*) msg);
			return FAILED;
		}
	}
	else
    {
		sprintf(msg, (char*)"gameDATA %s", STR_DISABLED);

#ifdef COBRA_ONLY
		sys_map_path((char*)"/dev_hdd0/game", NULL);
#else
		add_to_map((char*)"/dev_hdd0/game", gamei_path);
#endif
	}

	extgd = status;

	if(do_mount)
    {
		show_msg((char*) msg);
#ifndef COBRA_ONLY
		mount_with_mm(gamei_path, MOUNT_EXT_GDATA);
#endif
	}
	return 0;
}
#endif //#ifdef EXT_GDATA

static void detect_firmware(void)
{
	if(c_firmware>3.40f) return;

	u64 CEX=0x4345580000000000ULL;
	u64 DEX=0x4445580000000000ULL;

	dex_mode=0;

	if(peekq(0x80000000002ED778ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_470;  c_firmware=4.70f;}				else
	if(peekq(0x800000000030F240ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_470D; c_firmware=4.70f; dex_mode=2;}	else
	if(peekq(0x80000000002ED860ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_465;  c_firmware=(peekq(0x80000000002FC938ULL)==0x323031342F31312FULL)?4.66f:4.65f;} else
	if(peekq(0x800000000030F1A8ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_465D; c_firmware=(peekq(0x800000000031EBA8ULL)==0x323031342F31312FULL)?4.66f:4.65f; dex_mode=2;}	else
	if(peekq(0x80000000002ED850ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_460;  c_firmware=4.60f;}				else
	if(peekq(0x80000000002EC5E0ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_455;  c_firmware=4.55f;}				else
	if(peekq(0x80000000002E9D70ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_453;  c_firmware=4.53f;}				else
	if(peekq(0x800000000030AEA8ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_453D; c_firmware=4.53f; dex_mode=2;}	else
	if(peekq(0x80000000002E9BE0ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_450;  c_firmware=4.50f;}				else
	if(peekq(0x80000000002EA9B8ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_446;  c_firmware=4.46f;}				else
	if(peekq(0x8000000000305410ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_446D; c_firmware=4.46f; dex_mode=2;}	else
	if(peekq(0x80000000002E8610ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_421;  c_firmware=4.21f;}				else
	if(peekq(0x8000000000302D88ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_421D; c_firmware=4.21f; dex_mode=2;}	else
	if(peekq(0x80000000002D83D0ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_355;  c_firmware=3.55f;}				else
	if(peekq(0x80000000002EFE20ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_355D; c_firmware=3.55f; dex_mode=2;}	else
//No cobra cfw but as mamba compatibility
	if(peekq(0x800000000030D6A8ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_455D; c_firmware=4.55f; dex_mode=2;}	else
	if(peekq(0x8000000000309698ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_450D; c_firmware=4.50f; dex_mode=2;}	else
	if(peekq(0x8000000000304EF0ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_441D; c_firmware=4.41f; dex_mode=2;}	else
	if(peekq(0x80000000002EA498ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_441;  c_firmware=4.41f;}				else
	if(peekq(0x80000000002EA488ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_440;  c_firmware=4.40f;}				else
	if(peekq(0x80000000002E9F18ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_431;  c_firmware=4.31f;}				else
	if(peekq(0x8000000000304630ULL)==DEX) {SYSCALL_TABLE = SYSCALL_TABLE_430D; c_firmware=4.30f; dex_mode=2;}	else
	if(peekq(0x80000000002E9F08ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_430;  c_firmware=4.30f;}				else
#ifndef COBRA_ONLY
	if(peekq(0x80000000002CFF98ULL)==CEX) {SYSCALL_TABLE = SYSCALL_TABLE_341;  c_firmware=3.41f;}				else
	//if(peekq(0x80000000002E79C8ULL)==DEX) {c_firmware=3.41f; dex_mode=2;}	else
#endif
	{c_firmware=0.00f; return;}

    if(!SYSCALL_TABLE) {c_firmware=0.00f; return;}

#ifndef COBRA_ONLY
	if(!dex_mode)
	{   // CEX
		if(c_firmware==3.41f) {base_addr=0x2B5D30; open_hook=0x2AAFC8;} else
		if(c_firmware==3.55f) {base_addr=0x2BE0D0; open_hook=0x2B3274;} else
		if(c_firmware==4.21f) {base_addr=0x2D0C98; open_hook=0x2C2558;} else
		if(c_firmware==4.30f) {base_addr=0x2D2418; open_hook=0x2C3CD4;} else
		if(c_firmware==4.31f) {base_addr=0x2D2428; open_hook=0x2C3CE0;} else
		if(c_firmware==4.40f) {base_addr=0x2D29A8; open_hook=0x2C4284;} else
		if(c_firmware==4.41f) {base_addr=0x2D29B8; open_hook=0x2C4290;} else
		if(c_firmware==4.46f) {base_addr=0x2D2ED8; open_hook=0x2C47B0;} else
		if(c_firmware==4.50f) {base_addr=0x2D4CB8; open_hook=0x29DD20;} else
		if(c_firmware==4.53f) {base_addr=0x2D4E48; open_hook=0x29DEF8;} else
		if(c_firmware==4.55f) {base_addr=0x2D7660; open_hook=0x29F748;} else
		if(c_firmware==4.60f) {base_addr=0x2D88D0; open_hook=0x2A02BC;} else
		if(c_firmware==4.65f) {base_addr=0x2D88E0; open_hook=0x2A02C8;} else
		if(c_firmware==4.66f) {base_addr=0x2D88E0; open_hook=0x2A02C8;} else
		if(c_firmware==4.70f) {base_addr=0x2D8A70; open_hook=0x2975C0;}
	}
	else
	{   // DEX
		if(c_firmware==3.55f) {base_addr=0x2D5B20; open_hook=0x2C8A94;} else
		if(c_firmware==4.21f) {base_addr=0x2EB418; open_hook=0x2D9718;} else
		if(c_firmware==4.30f) {base_addr=0x2ECB48; open_hook=0x2DAE4C;} else
	  //if(c_firmware==4.31f) {base_addr=0x??????; open_hook=0x??????;} else
	  //if(c_firmware==4.40f) {base_addr=0x??????; open_hook=0x??????;} else
		if(c_firmware==4.41f) {base_addr=0x2ED418; open_hook=0x2DB73C;} else
		if(c_firmware==4.46f) {base_addr=0x2ED938; open_hook=0x2DBC5C;} else
		if(c_firmware==4.50f) {base_addr=0x2F4778; open_hook=0x2B81E8;} else
		if(c_firmware==4.53f) {base_addr=0x2F5F88; open_hook=0x2B83C0;} else
		if(c_firmware==4.55f) {base_addr=0x2F8730; open_hook=0x2B9C14;} else
	  //if(c_firmware==4.60f) {base_addr=0x??????; open_hook=0x??????;} else
		if(c_firmware==4.65f) {base_addr=0x2FA230; open_hook=0x2BB010;} else
		if(c_firmware==4.66f) {base_addr=0x2FA230; open_hook=0x2BB010;} else
		if(c_firmware==4.70f) {base_addr=0x2FA540; open_hook=0x2B2480;}
	}

	base_addr |=0x8000000000000000ULL;
	open_hook |=0x8000000000000000ULL;
#endif

	if(!dex_mode)
	{
		if(c_firmware>=4.55f && c_firmware<=4.70f)
		{
			get_fan_policy_offset=0x8000000000009E38ULL; // sys 409 get_fan_policy  4.55/4.60/4.65/4.70
			set_fan_policy_offset=0x800000000000A334ULL; // sys 389 set_fan_policy

			// idps / psid cex
			if(c_firmware==4.55f)
			{
				idps_offset1 = 0x80000000003E17B0ULL;
				idps_offset2 = 0x8000000000474F1CULL;
				psid_offset  = 0x8000000000474F34ULL;
			}
			else if(c_firmware>=4.60f && c_firmware<=4.66f)
			{
				idps_offset1 = 0x80000000003E2BB0ULL;
				idps_offset2 = 0x8000000000474F1CULL;
				psid_offset  = 0x8000000000474F34ULL;
			}
			else if(c_firmware==4.70f)
			{
				idps_offset1 = 0x80000000003E2DB0ULL;
				idps_offset2 = 0x8000000000474AF4ULL;
				psid_offset  = 0x8000000000474B0CULL;
			}
		}
		else if(c_firmware>=4.21f && c_firmware<=4.53f)
		{
			get_fan_policy_offset=0x8000000000009E28ULL; // sys 409 get_fan_policy  4.21/4.30/4.31/4.40/4.41/4.46/4.50/4.53
			set_fan_policy_offset=0x800000000000A324ULL; // sys 389 set_fan_policy
		}
		else if(c_firmware == 3.55f)
		{
			get_fan_policy_offset=0x8000000000008CBCULL; // sys 409 get_fan_policy
			set_fan_policy_offset=0x80000000000091B8ULL; // sys 389 set_fan_policy
		}
#ifndef COBRA_ONLY
		else if(c_firmware == 3.41f)
		{
			get_fan_policy_offset=0x8000000000008644ULL; // sys 409 get_fan_policy
			set_fan_policy_offset=0x8000000000008B40ULL; // sys 389 set_fan_policy
		}
#endif
	}
	else // DEX
	if(c_firmware>=4.55f && c_firmware<=4.70f)
	{
			get_fan_policy_offset=0x8000000000009EB8ULL; // sys 409 get_fan_policy  4.55/4.60/4.65
			set_fan_policy_offset=0x800000000000A3B4ULL; // sys 389 set_fan_policy

			// idps / psid dex
			if(c_firmware==4.55f)
			{
				idps_offset1 = 0x8000000000407930ULL;
				idps_offset2 = 0x8000000000494F1CULL;
				psid_offset  = 0x8000000000494F34ULL;
			}
			else if(c_firmware>=4.60f && c_firmware<=4.66f)
			{
				idps_offset1 = 0x80000000004095B0ULL;
				idps_offset2 = 0x800000000049CF1CULL;
				psid_offset  = 0x800000000049CF34ULL;
			}
			else if(c_firmware==4.70f)
			{
				idps_offset1 = 0x80000000004098B0ULL;
				idps_offset2 = 0x800000000049CAF4ULL;
				psid_offset  = 0x800000000049CB0CULL;
			}
	}
	else if(c_firmware>=4.21f && c_firmware<=4.53f)
	{
			get_fan_policy_offset=0x8000000000009EA8ULL; // sys 409 get_fan_policy  4.21/4.30/4.31/4.40/4.41/4.46/4.50/4.53
			set_fan_policy_offset=0x800000000000A3A4ULL; // sys 389 set_fan_policy
	}
	else if(c_firmware == 3.55f)
	{
			get_fan_policy_offset=0x8000000000008D3CULL; // sys 409 get_fan_policy
			set_fan_policy_offset=0x8000000000009238ULL; // sys 389 set_fan_policy
	}
}

static void block_online_servers(void)
{
	u64 mem=0;
	for(u64 addr=0x860000; addr<0xFFFFF8ULL; addr+=4)//16MB
	{
	 mem=peek_lv1(addr);
	 if(mem         == 0x733A2F2F61757468ULL)  // s://auth
	  poke_lv1(addr,   0x733A2F2F00000000ULL);
	 else if(mem    == 0x2E7073332E757064ULL)  // .ps3.upd
	  poke_lv1(addr-8, 0x3A2F2F0000000000ULL);
	 else if(mem    == 0x656E612E6E65742EULL)  // ena.net.
	  poke_lv1(addr,   0x0000000000000000ULL);
	}
	for(u64 addr=0x300000; addr<0x7FFFF8ULL; addr+=4)//8MB
	{
	 mem=peekq(addr);
	 if(mem      == 0x733A2F2F6E73782EULL)  // s://nsx.
	  pokeq(addr,   0x733A2F2F00000000ULL);
	 else if(mem == 0x733A2F2F6E73782DULL)  // s://nsx-
	  pokeq(addr,   0x733A2F2F00000000ULL);
	 else if(mem == 0x3A2F2F786D622D65ULL)  // ://xmb-e
	  pokeq(addr,   0x3A2F2F0000000000ULL);
	 else if(mem == 0x2E7073332E757064ULL)  // .ps3.upd
	  pokeq(addr-8, 0x3A2F2F0000000000ULL);
	 else if(mem == 0x702E616470726F78ULL)  // p.adprox
	  pokeq(addr-8, 0x733A2F2F00000000ULL);
	 else if(mem == 0x656E612E6E65742EULL)  // ena.net.
	  pokeq(addr,   0x0000000000000000ULL);
	 else if(mem == 0x702E7374756E2E70ULL)  // p.stun.p
	  pokeq(addr-4, 0x0000000000000000ULL);
	 else if(mem == 0x2E7374756E2E706CULL)  // .stun.pl
	  pokeq(addr-4, 0x0000000000000000ULL);
	 else if(mem == 0x63726565706F2E77ULL)  // creepo.w
	  pokeq(addr  , 0x0000000000000000ULL);
	}
}

#ifdef SPOOF_CONSOLEID
static void spoof_idps_psid(void)
{
	if(webman_config->spsid)
	{
		uint64_t j, newPSID[2] = {0, 0};

		newPSID[0] = convertH(webman_config->vPSID1);
		newPSID[1] = convertH(webman_config->vPSID2);

		//if(newPSID[0] != 0 && newPSID[1] != 0)
		{
			if(c_firmware<=4.53f)
			{
				{system_call_1(SC_GET_PSID, (uint64_t) PSID);}
				for(j = 0x8000000000000000ULL; j < 0x8000000000600000ULL; j+=4) {
					if((peekq(j) == PSID[0]) && (peekq(j+8) == PSID[1])) {
						pokeq(j, newPSID[0]); j+=8;
						pokeq(j, newPSID[1]); j+=8;
					}
				}
			}
			else if(psid_offset)
			{
				pokeq(psid_offset  , newPSID[0]);
				pokeq(psid_offset+8, newPSID[1]);
			}
		}
	}

	if(webman_config->sidps)
	{
		uint64_t j, newIDPS[2] = {0, 0};

		newIDPS[0] = convertH(webman_config->vIDPS1);
		newIDPS[1] = convertH(webman_config->vIDPS2);

		if(newIDPS[0] != 0 && newIDPS[1] != 0)
		{
			if(c_firmware<=4.53f)
			{
				{system_call_1(SC_GET_IDPS, (uint64_t) IDPS);}
				for(j = 0x8000000000000000ULL; j < 0x8000000000600000ULL; j+=4)
				{
					if((peekq(j) == IDPS[0]) && (peekq(j+8) == IDPS[1]))
					{
						pokeq(j, newIDPS[0]); j+=8;
						pokeq(j, newIDPS[1]); j+=8;
					}
				}
			}
			else if(idps_offset1 | idps_offset2)
			{
				pokeq(idps_offset1  , newIDPS[0]);
				pokeq(idps_offset1+8, newIDPS[1]);
				pokeq(idps_offset2  , newIDPS[0]);
				pokeq(idps_offset2+8, newIDPS[1]);
			}
		}
	}

	get_idps_psid();
}
#endif

#ifdef REMOVE_SYSCALLS
static void remove_cfw_syscalls(void)
{
	detect_firmware();

	if(c_firmware==0.00f) return;

    u64 sc_null = peekq(SYSCALL_TABLE);

	get_idps_psid();
	#ifndef COBRA_ONLY
	pokeq(SYSCALL_PTR( 8), sc_null);
	pokeq(SYSCALL_PTR(35), sc_null);
	#endif
	pokeq(SYSCALL_PTR( 9), sc_null);
	pokeq(SYSCALL_PTR(10), sc_null);
	pokeq(SYSCALL_PTR(11), sc_null);
	pokeq(SYSCALL_PTR(36), sc_null);
	pokeq(SYSCALL_PTR( 6), sc_null);
	pokeq(SYSCALL_PTR( 7), sc_null);

	u64 sc_not_impl_pt = peekq(sc_null);
	#ifndef COBRA_ONLY
	u64 sc8  = peekq(SYSCALL_PTR( 8));
	u64 sc35 = peekq(SYSCALL_PTR(35));
	#endif
	u64 sc6  = peekq(SYSCALL_PTR( 6));
	u64 sc7  = peekq(SYSCALL_PTR( 7));
	u64 sc9  = peekq(SYSCALL_PTR( 9));
	u64 sc10 = peekq(SYSCALL_PTR(10));
	u64 sc11 = peekq(SYSCALL_PTR(11));
	u64 sc36 = peekq(SYSCALL_PTR(36));

	#ifndef COBRA_ONLY
	if(sc8 !=sc_null) pokeq(sc8,  sc_not_impl_pt);
	if(sc35!=sc_null) pokeq(sc35, sc_not_impl_pt);
	#endif
	if(sc9 !=sc_null) pokeq(sc9,  sc_not_impl_pt);
	if(sc10!=sc_null) pokeq(sc10, sc_not_impl_pt);
	if(sc11!=sc_null) pokeq(sc11, sc_not_impl_pt);
	if(sc36!=sc_null) pokeq(sc36, sc_not_impl_pt);
	if(sc6 !=sc_null) pokeq(sc6,  sc_not_impl_pt);
	if(sc7 !=sc_null) pokeq(sc7,  sc_not_impl_pt);

	#ifdef PS3MAPI
  //{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 8); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 9); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 10); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 11); }
  //{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 35); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 36); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 6); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 7); }
	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 1); }//Partial disable syscall8 (Keep cobra/mamba+ps3mapi features only)
	#endif
}
#endif

static void delete_history(bool delete_folders)
{
	int fd;

	if(cellFsOpendir("/dev_hdd0/home", &fd) == CELL_FS_SUCCEEDED)
	{
		char path[128];
		CellFsDirent dir; u64 read = sizeof(CellFsDirent);

		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read) break;
			sprintf(path, "%s/%s/etc/boot_history.dat", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
			sprintf(path, "%s/%s/etc/community/CI.TMP", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
			sprintf(path, "%s/%s/community/MI.TMP", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
			sprintf(path, "%s/%s/community/PTL.TMP", "/dev_hdd0/home", dir.d_name);
			cellFsUnlink(path);
		}
		cellFsClosedir(fd);
	}

	cellFsUnlink("/dev_hdd0/vsh/pushlist/game.dat");
	cellFsUnlink("/dev_hdd0/vsh/pushlist/patch.dat");

	if(!delete_folders) return;

	cellFsRmdir("/dev_hdd0/GAMES");
	cellFsRmdir("/dev_hdd0/GAMEZ");
	cellFsRmdir("/dev_hdd0/PS3ISO");
	cellFsRmdir("/dev_hdd0/PS2ISO");
	cellFsRmdir("/dev_hdd0/PSXISO");
	cellFsRmdir("/dev_hdd0/PSXGAMES");
	cellFsRmdir("/dev_hdd0/PSPISO");
	cellFsRmdir("/dev_hdd0/ISO");
	cellFsRmdir("/dev_hdd0/BDISO");
	cellFsRmdir("/dev_hdd0/DVDISO");
	cellFsRmdir("/dev_hdd0/PKG");
}

static int ssplit(const char* str, char* left, int lmaxlen, char* right, int rmaxlen)
{
	int ios = strcspn(str, " ");
	int ret = (ios < (int)strlen(str) - 1);
	int lmax = (ios < lmaxlen) ? ios : lmaxlen;

	strncpy(left, str, lmax);
	left[lmax] = '\0';

	if(ret)
	{
		strncpy(right, str + ios + 1, rmaxlen);
		right[rmaxlen] = '\0';
	}
	else
	{
		right[0] = '\0';
	}

	return ret;
}

static int slisten(int port, int backlog)
{
	int list_s = socket(AF_INET, SOCK_STREAM, 0);
	if(list_s<0) return list_s;

	//int optval = 1;
	//setsockopt(list_s, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	struct sockaddr_in sa;
	socklen_t sin_len = sizeof(sa);
	memset(&sa, 0, sin_len);

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(list_s, (struct sockaddr *)&sa, sin_len);
	listen(list_s, backlog);

	return list_s;
}

static void sclose(int *socket_e)
{
	if(*socket_e != -1)
	{
		shutdown(*socket_e, SHUT_RDWR);
		socketclose(*socket_e);
		*socket_e = -1;
	}
}




/*
u64 ssssocket(void)
{
	system_call_3(713, 2, 1, 0);			//socket
	return_to_user_prog(uint64_t);
}


static int connect_to_server2(char *server, uint16_t port)
{
	struct sockaddr_in sin;
	unsigned int temp;
	int s;

	if((temp = inet_addr(server)) != (unsigned int)-1)
	{
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = temp;
	}
	else {
		struct hostent *hp;

		if((hp = gethostbyname(server)) == NULL)
		{
			return FAILED;
		}

		sin.sin_family = hp->h_addrtype;
		memcpy(&sin.sin_addr, hp->h_addr, hp->h_length);
	}

	sin.sin_port = htons(port);

	memcpy(&sin_global, &sin, 16);

	sprintf(netbuf, "Empty buffer...\r\n");

	pokeq(0x80000000007FD0A0ULL, (u64)&sin_global);
	pokeq(0x80000000007FD0A8ULL, (u64)&netbuf);
	pokeq(0x80000000007FD0B0ULL, (u64)&statbuf);
	pokeq(0x80000000007FD0B8ULL, (u64)&direntry);

	//peek(0x80000000007FD608): 0x0002947AC0A864D1 | ...z..d.

	s = ssssocket();
	{system_call_3(702, s, (u64)&sin, sizeof(sin));}		//connect
	{system_call_6(710, s, (u64)&sin, sizeof(sin), 0, (u64)&sin, sizeof(sin));}	//sendto
	//{system_call_6(707, s, (u64)&sin, sizeof(sin), 8, (u64)&sin, sizeof(sin));}	//recvfrom
	{system_call_1(714, s);}				//close
	{system_call_2(712, s, 2);}				//shutdown

	return 0;
}


static uint64_t syscall_837(const char *device, const char *format, const char *point, u32 a, u32 b, u32 c, void *buffer, u32 len)
{
	system_call_8(SC_FS_MOUNT, (u64)device, (u64)format, (u64)point, a, b, c, (u64)buffer, len);
	return_to_user_prog(uint64_t);
}
*/

#ifdef COBRA_ONLY
#ifndef LITE_EDITION
static int open_remote_dir(int s, char *path, int *abort_connection)
{
	netiso_open_dir_cmd cmd;
	netiso_open_dir_result res;
	int len;

	*abort_connection = 0;

	len = strlen(path);
	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_OPEN_DIR);
	cmd.dp_len = (len);

	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
		//DPRINTF("send failed (open_remote_dir) (errno=%d)!\n", get_network_error());
		*abort_connection = 1;
		return FAILED;
	}

	if(send(s, path, len, 0) != len)
	{
		//DPRINTF("send failed (open_remote_dir) (errno=%d)!\n", get_network_error());
		*abort_connection = 1;
		return FAILED;
	}

	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
		//DPRINTF("recv failed (open_remote_dir) (errno=%d)!\n", get_network_error());
		*abort_connection = 1;
		return FAILED;
	}

	return (res.open_result);
}

static int read_remote_dir(int s, sys_addr_t *data /*netiso_read_dir_result_data **data*/, int *abort_connection)
{
	netiso_read_dir_entry_cmd cmd;
	netiso_read_dir_result res;
	int len;

	*abort_connection = 1;

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = (NETISO_CMD_READ_DIR);

//MM_LOG("Sending request...(%i) ", s);
	if(send(s, &cmd, sizeof(cmd), 0) != sizeof(cmd))
	{
//MM_LOG("FAILED!\n");
		return FAILED;
	}
//MM_LOG("Receiving response...");
	if(recv(s, &res, sizeof(res), MSG_WAITALL) != sizeof(res))
	{
//MM_LOG("FAILED!\n");
		return FAILED;
	}

//MM_LOG("OK (%i entries)\n", res.dir_size );
	if(res.dir_size > 0)
	{
		sys_addr_t data1=0;
		for(int64_t retry=16; retry>0; retry--)
		{
			if(res.dir_size>retry*123) res.dir_size=retry*123;

			len = (sizeof(netiso_read_dir_result_data)*res.dir_size);
			int len2= ((len+_64KB_)/_64KB_)*_64KB_;
			if(sys_memory_allocate(len2, SYS_MEMORY_PAGE_SIZE_64K, &data1)==0)
			{
				*data=data1;
				u8 *data2=(u8*)data1;

				if(recv(s, data2, len, MSG_WAITALL) != len)
				{
					sys_memory_free(data1);
					*data=NULL;
					return FAILED;
				}
				break;
			}
			else
				*data=NULL;
		}
	}
	else
		*data=NULL;

	*abort_connection = 0;

	return (res.dir_size);
}
#endif //#ifndef LITE_EDITION
#endif //#ifdef COBRA_ONLY

static void add_radio_button(const char *name, const char *value, const char *id, const char *label, const char *sufix, bool checked, char *buffer)
{
	char templn[MAX_LINE_LEN];
	sprintf(templn, "<label><input type=\"radio\" name=\"%s\" value=\"%s\" id=\"%s\"%s/> %s%s</label>", name, value, id, checked?ITEM_CHECKED:"", label, (!sufix)?"<br>":sufix);
	strcat(buffer, templn);
}

static void add_check_box(const char *name, const char *value, const char *label, const char *sufix, bool checked, char *buffer)
{
	char templn[MAX_LINE_LEN];
	char clabel[MAX_LINE_LEN];
	char *p;
	strcpy(clabel, label);
	p=strstr(clabel, AUTOBOOT_PATH);
	if(p!=NULL)
	{
		p[0]=0;
		sprintf(templn, HTML_INPUT("autop", "%s", "255", "40"), webman_config->autoboot_path);
		strcat(clabel, templn);
		p=strstr(label, AUTOBOOT_PATH)+strlen(AUTOBOOT_PATH);
		strcat(clabel, p);
	}
	sprintf(templn, "<label><input type=\"checkbox\" name=\"%s\" value=\"%s\"%s/> %s</label>%s", name, value, checked?ITEM_CHECKED:"", clabel, (!sufix)?"<br>":sufix);
	strcat(buffer, templn);
}

static void add_option_item(const char *value, const char *label, bool selected, char *buffer)
{
	char templn[MAX_LINE_LEN];
	sprintf(templn, "<option value=\"%s\"%s/>%s</option>", value, selected?ITEM_SELECTED:"", label);
	strcat(buffer, templn);
}

#define READ_SFO_HEADER() \
	if(!(mem[1]=='P' && mem[2]=='S' && mem[3]=='F')) return false; \
	u16 pos, str, dat, indx=0; \
	str=(mem[0x8]+(mem[0x9]<<8)); \
	dat=pos=(mem[0xc]+(mem[0xd]<<8));

#define READ_SFO_HEADER2() \
	if(!(mem[1]=='P' && mem[2]=='S' && mem[3]=='F')) return; \
	u16 pos, str, dat, indx=0; \
	str=(mem[0x8]+(mem[0x9]<<8)); \
	dat=pos=(mem[0xc]+(mem[0xd]<<8));

#define FOR_EACH_SFO_FIELD() \
	while(str<4090) \
	{ \
		if((mem[str]==0) || (str>=dat)) break;

#define READ_NEXT_SFO_FIELD() \
		while(mem[str]) str++;str++; \
		pos+=(mem[0x1c+indx]+(mem[0x1d+indx]<<8)); \
		indx+=0x10; \
	}

static void parse_param_sfo(unsigned char *mem, char *titleID, char *title)
{
	READ_SFO_HEADER2()

	memset(titleID, 0, 10);
	memset(title, 0, 64);

	u8 fcount=0;

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "TITLE_ID", 8))
		{
			strncpy(titleID, (char *) &mem[pos], 9);
			fcount++; if(fcount>=2) break;
		}
		else
		if(!memcmp((char *) &mem[str], "TITLE", 6))
		{
			strncpy(title, (char *) &mem[pos], 63);
			fcount++; if(fcount>=2) break;
		}

		READ_NEXT_SFO_FIELD()
	}

	if(webman_config->tid && strlen(titleID)==9 && (titleID[0]=='B' || titleID[0]=='N'))
	{
		strcat(title, " ["); strcat(title, titleID); strcat(title, "]");
	}
}

static bool fix_param_sfo(unsigned char *mem, char *titleID, u8 msg)
{
	READ_SFO_HEADER()

	memset(titleID, 0, 10);

#ifdef FIX_GAME
	u8 fcount=0;
#endif

	bool ret=false;

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "TITLE_ID", 8))
		{
			strncpy(titleID, (char *) &mem[pos], 9);
#ifdef FIX_GAME
			fcount++; if(fcount>=2) break;
#else
			break;
#endif
		}
#ifdef FIX_GAME
		else
		if(!memcmp((char *) &mem[str], "PS3_SYSTEM_VER", 14))
		{
			char version[8];
			strncpy(version, (char *) &mem[pos], 7);
			int fw_ver=10000*((version[1] & 0xFF)-'0') + 1000*((version[3] & 0xFF)-'0') + 100*((version[4] & 0xFF)-'0');
			if((fw_ver>(int)(c_firmware*10000.0f)) && c_firmware>=4.20f && c_firmware<4.70f)
			{
				if(msg) {char text[64]; sprintf(text, "WARNING: Game requires firmware version %i.%i", (fw_ver/10000), (fw_ver-10000*(fw_ver/10000))/100); show_msg((char*)text);}

				mem[pos+1]='4'; mem[pos+3]='2'; mem[pos+4]='0'; ret=true;
			}
			fcount++; if(fcount>=2) break;
		}
#endif

		READ_NEXT_SFO_FIELD()
	}

	return ret;
}

#ifdef FIX_GAME
static bool fix_ps3_extra(unsigned char *mem)
{
	READ_SFO_HEADER()

	FOR_EACH_SFO_FIELD()
	{
		if(!memcmp((char *) &mem[str], "ATTRIBUTE", 9))
		{
			if(!(mem[pos+2] & 2)) {mem[pos+2]|=0x2; return true;}
			break;
		}

		READ_NEXT_SFO_FIELD()
	}

	return false;
}

static void fix_game(char *path)
{
	int fd;

	if(cellFsOpendir(path, &fd) == CELL_FS_SUCCEEDED)
	{
		char filename[MAX_PATH_LEN];
		CellFsDirent dir; u64 read = sizeof(CellFsDirent);

#ifdef COPY_PS3
		sprintf(current_file, "%s", path);
#endif

		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read || fix_aborted) break;
			if(dir.d_name[0]=='.') continue;

			sprintf(filename, "%s/%s", path, dir.d_name);

			if(!extcasecmp(dir.d_name, ".sprx", 5) || !extcasecmp(dir.d_name, ".self", 5) || !strcmp(dir.d_name, "EBOOT.BIN"))
			{
				int fdw, offset; uint64_t msiz = 0; char ps3_sys_version[8];

				cellFsChmod(filename, MODE); //fix file read-write permission

				if(cellFsOpen(filename, CELL_FS_O_RDWR, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
				{
					cellFsLseek(fdw, 0xC, CELL_FS_SEEK_SET, &msiz);
					cellFsRead(fdw, (void *)&ps3_sys_version, 4, &msiz);

					offset=(ps3_sys_version[0]<<24) + (ps3_sys_version[1]<<16) + (ps3_sys_version[2]<<8) + ps3_sys_version[3]; offset-=0x78;

					retry_offset:
					if(offset < 0x90 || offset > 0x800) offset=!extcasecmp(dir.d_name, ".sprx", 5)?0x258:0x428;
					cellFsLseek(fdw, offset, CELL_FS_SEEK_SET, &msiz);
					cellFsRead(fdw, (void *)&ps3_sys_version, 8, &msiz);

					if(offset!=0x258 && offset!=0x428 && (ps3_sys_version[0] | ps3_sys_version[1] | ps3_sys_version[2] | ps3_sys_version[3] | ps3_sys_version[4] | ps3_sys_version[5])!=0)
					{
						offset=0; goto retry_offset;
					}

					if((ps3_sys_version[0]+ps3_sys_version[1]+ps3_sys_version[2]+ps3_sys_version[3]+ps3_sys_version[4]+ps3_sys_version[5])==0 && (ps3_sys_version[6] & 0xFF)>0xA4)
					{
						ps3_sys_version[6]=0XA4; ps3_sys_version[7]=0X10;

						cellFsLseek(fdw, offset, CELL_FS_SEEK_SET, &msiz);
						cellFsWrite(fdw, (char*)ps3_sys_version, 8, NULL);
					}
					cellFsClose(fdw);
				}
				cellFsClose(fdw);
			}
			else if(isDir(filename) && (webman_config->fixgame!=FIX_GAME_QUICK)) fix_game(filename);

			sys_timer_usleep(1);
		}
		cellFsClosedir(fd);
	}
}

#ifdef COBRA_ONLY
uint64_t getlba(const char *s1, u16 n1, const char *s2, u16 n2, u16 start)
{
    u16 c=0; u32 lba=0;
    for(u16 n=start+0x1F; n<n1-n2; n++)
    {
        c=0; while(s1[n+c]==s2[c] && c<n2) c++;
        if(c==n2)
        {
            while(n>0x1D && s1[n--]!=0x01); n-=0x1C;
            lba=(s1[n+0]&0xFF)+(s1[n+1]&0xFF)*0x100UL+(s1[n+2]&0xFF)*0x10000UL+(s1[n+3]&0xFF)*0x1000000UL;
            start=n+0x1C+n2; return lba;
        }
    }
    return 0;
}


void fix_iso(char *iso_file, uint64_t maxbytes, bool patch_update)
{
	struct CellFsStat buf;

	if(fix_aborted || cellFsStat(iso_file, &buf)!=CELL_FS_SUCCEEDED) return;

	int fd;

#ifdef COPY_PS3
	sprintf(current_file, "%s", iso_file);
#endif

	cellFsChmod(iso_file, MODE); //fix file read-write permission

	if(cellFsOpen((char*)iso_file, CELL_FS_O_RDWR, &fd, NULL, 0)==CELL_FS_SUCCEEDED)
	{
		uint64_t chunk_size=_4KB_; char chunk[_4KB_], ps3_sys_version[8];
		uint64_t msiz1 = 0, msiz2 = 0, lba = 0, pos=0xA000ULL;

		bool fix_sfo=true, fix_eboot=true, fix_ver=false, fix_update=false; char update_path[64];

		uint64_t size = buf.st_size; char titleID[10];
		if(maxbytes>0 && size>maxbytes) size=maxbytes;
		if(size>pos) size-=pos; else size=0;

		while(size>0ULL)
    	{
			if(fix_aborted) break;

			if(fix_sfo)
			{
				cellFsLseek(fd, pos, CELL_FS_SEEK_SET, &msiz2);
				cellFsRead(fd, chunk, chunk_size, &msiz1); if(msiz1<=0) break;

				lba=getlba(chunk, msiz1, "PARAM.SFO;1", 11, 0);

				if(lba)
				{
					lba*=0x800ULL; fix_sfo=false;
					cellFsLseek(fd, lba, CELL_FS_SEEK_SET, &msiz2);
					cellFsRead(fd, (void *)&chunk, chunk_size, &msiz1); if(msiz1<=0) break;

					fix_ver = fix_param_sfo((unsigned char *)chunk, titleID, 0);

					if(patch_update)
					{
						sprintf(update_path, "/dev_hdd0/game/%s/USRDIR/EBOOT.BIN", titleID); // has update on hdd0?
						if(cellFsStat(iso_file, &buf)==CELL_FS_SUCCEEDED) {fix_update=true; fix_ver=false;}
					}

					if(fix_ver)
					{
						cellFsLseek(fd, lba, CELL_FS_SEEK_SET, &msiz2);
						cellFsWrite(fd, chunk, msiz1, &msiz2);
					}
					else goto exit_fix; //do not fix if sfo version is ok

					if(size>lba) size=lba;

					sprintf(chunk, "%s %s", STR_FIXING, iso_file);
					show_msg(chunk);

					lba=getlba(chunk, msiz1, "PS3_DISC.SFB;1", 14, 0); lba*=0x800ULL; chunk_size=0x800; //1 sector
					if(lba>0 && size>lba) size=lba;
				}
			}

			u16 start, offset;

			for(u8 t=(fix_eboot?0:1);t<5;t++)
			{
				cellFsLseek(fd, pos, CELL_FS_SEEK_SET, &msiz2);
				cellFsRead(fd, chunk, chunk_size, &msiz1); if(msiz1<=0) break;

				start=0;

				while(true)
				{
					sys_timer_usleep(1);
					if(fix_aborted) goto exit_fix;

					if(t==0) lba=getlba(chunk, msiz1, "EBOOT.BIN;1", 11, start);
					if(t==1) lba=getlba(chunk, msiz1, ".SELF;1", 7, start);
					if(t==2) lba=getlba(chunk, msiz1, ".self;1", 7, start);
					if(t==3) lba=getlba(chunk, msiz1, ".SPRX;1", 7, start);
					if(t==4) lba=getlba(chunk, msiz1, ".sprx;1", 7, start);

					if(lba)
					{
						if(t==0) fix_eboot=false;

						lba*=0x800ULL;
						cellFsLseek(fd, lba, CELL_FS_SEEK_SET, &msiz2);
						cellFsRead(fd, (void *)&chunk, chunk_size, &msiz1); if(msiz1<=0) break;

    					offset=(chunk[0xC]<<24) + (chunk[0xD]<<16) + (chunk[0xE]<<8) + chunk[0xF]; offset-=0x78;
	    				if(offset < 0x90 || offset > 0x800 || (chunk[offset] | chunk[offset+1] | chunk[offset+2] | chunk[offset+3] | chunk[offset+4] | chunk[offset+5])) offset=(t>2)?0x258:0x428;

						for(u8 i=0;i<8;i++) ps3_sys_version[i]=chunk[offset+i];

						if((ps3_sys_version[0]+ps3_sys_version[1]+ps3_sys_version[2]+ps3_sys_version[3]+ps3_sys_version[4]+ps3_sys_version[5])==0 && (ps3_sys_version[6] & 0xFF)>0xA4)
						{
							ps3_sys_version[6]=0XA4; ps3_sys_version[7]=0X10;
							cellFsLseek(fd, lba+offset, CELL_FS_SEEK_SET, &msiz2);
							cellFsWrite(fd, ps3_sys_version, 8, &msiz2);
						}
						else goto exit_fix;

						if(t==0) break;

					} else break;
				}
			}

			if(msiz1<=0) break;

			pos+=msiz1;
			size-=msiz1;
			if(chunk_size>size) chunk_size=(int) size;

			sys_timer_usleep(1);
		}
exit_fix:
		cellFsClose(fd);

		if(fix_update) {sprintf(update_path, "/dev_hdd0/game/%s/USRDIR", titleID); fix_game(update_path);}
	}
}
#endif //#ifdef COBRA_ONLY

#endif //#ifdef FIX_GAME

static void get_name(char *name, char *filename, u8 cache)
{
	int pos = 0;
	if(cache) {pos=strlen(filename); while(pos>0 && filename[pos-1]!='/') pos--;}
	if(cache==2) cache=0;

	if(cache)
		sprintf(name, "%s/%s", WMTMP, filename+pos);
	else
		sprintf(name, "%s", filename+pos);

	int flen=strlen(name);
	if(flen>2 && name[flen-2]=='.' ) {name[flen-2]=0; flen-=2;}
	if(flen>4 && name[flen-4]=='.' )  name[flen-4]=0;
	else
	if(strstr(filename+pos, ".ntfs["))
	{
		while(name[flen]!='.') flen--; name[flen]=0;
		if(flen>4 && name[flen-4]=='.' && (strcasestr(ISO_EXTENSIONS, &name[flen-4]))) name[flen-4]=0; else
		if(!extcmp(name, ".BIN.ENC", 8)) name[flen-8]=0;
	}
	if(cache) return;

	if(name[4] == '_' && name[8] == '.' && (name[0] == 'B' || name[0] == 'N' || name[0] == 'S' || name[0] == 'U') && (name[9] >= '0' && name[9] <= '9') && (name[10] >= '0' && name[10] <= '9')) strcpy(&name[0], &name[12]);
	if(name[9]== '-' && name[10]=='[') {strcpy(&name[0], &name[11]); name[strlen(name)-1]='\0';}
	if(name[10]=='-' && name[11]=='[') {strcpy(&name[0], &name[12]); name[strlen(name)-1]='\0';}
	if(!webman_config->tid && strstr(name, " [")) *strstr(name, " [")='\0';
}

static bool get_cover(char *icon, char *titleid)
{
	if(!titleid[0]) return false;

	struct CellFsStat s;

#ifndef ENGLISH_ONLY
	if(covers_exist[0])
	{
		sprintf(icon, "%s/%s.JPG", COVERS_PATH, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, "%s/%s.PNG", COVERS_PATH, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}
#endif

	if(covers_exist[1])
	{
		if(titleid[0]=='S')
		{
			sprintf(icon, MM_ROOT_STD "/covers_retro/psx/%c%c%c%c_%c%c%c.%c%c_COV.JPG",
					titleid[0], titleid[1], titleid[2], titleid[3],
					titleid[4], titleid[5], titleid[6], titleid[7], titleid[8]); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		}

		sprintf(icon, "%s/covers/%s.JPG", MM_ROOT_STD, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, "%s/covers/%s.PNG", MM_ROOT_STD, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}

	if(covers_exist[2])
	{
		sprintf(icon, "%s/covers/%s.JPG", MM_ROOT_STL, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, "%s/covers/%s.PNG", MM_ROOT_STL, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}

	if(covers_exist[3])
	{
		sprintf(icon, "%s/covers/%s.JPG", MM_ROOT_SSTL, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, "%s/covers/%s.PNG", MM_ROOT_SSTL, titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}

	if(covers_exist[4])
	{
		sprintf(icon, "%s/covers/%s.JPG", "/dev_hdd0/GAMES", titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, "%s/covers/%s.PNG", "/dev_hdd0/GAMES", titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}

	if(covers_exist[5])
	{
		sprintf(icon, "%s/covers/%s.JPG", "/dev_hdd0/GAMEZ", titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, "%s/covers/%s.PNG", "/dev_hdd0/GAMEZ", titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}

	if(covers_exist[6])
	{
		sprintf(icon, WMTMP "/%s.JPG", titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
		sprintf(icon, WMTMP "/%s.PNG", titleid); if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;
	}

	icon[0]=0;
    return false;
}

static void get_iso_icon(char *icon, char *param, char *file, int isdir, int ns, int abort_connection)
{
	//this function is called only from get_default_icon

	struct CellFsStat s;
	int flen;

	if(!extcmp(file, ".BIN.ENC", 8))
	{
		sprintf(icon, "%s/%s.png", param, file);
		if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;
		sprintf(icon, "%s/%s.PNG", param, file);
		if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

		flen=strlen(icon)-12;
		if(flen>0 && icon[flen]=='.')
		{
			icon[flen]=0; strcat(icon, ".png");
			if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;
			icon[flen]=0; strcat(icon, ".PNG");
			if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;
		}
	}

	if(isdir || ns>=0)
		{get_name(icon, file, 1); strcat(icon, ".PNG");} //wmtmp
	else
	{
		sprintf(icon, "%s/%s", param, file);
		flen=strlen(icon);

		if(strstr(file, ".ntfs["))
		{
			while(icon[flen]!='.') flen--; icon[flen]=0;
		}

		if(flen>2 && icon[flen-2]=='.' ) {icon[flen-2]=0; flen-=2;} // remove file extension (split iso)
		if(flen>4 && icon[flen-4]=='.' )  icon[flen-4]=0;           // remove file extension

		//file name + .jpg
		strcat(icon, ".jpg");
		if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

		flen=strlen(icon);

		//file name + .png
		icon[flen-4]=0; // remove file extension
		strcat(icon, ".png");
		if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

		//file name + .JPG
		icon[flen-4]=0; // remove file extension
		strcat(icon, ".JPG");
		if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

		//file name + .PNG
		icon[flen-4]=0; // remove file extension
		strcat(icon, ".PNG");
	}

	//copy remote file
	if(cellFsStat(icon, &s)!=CELL_FS_SUCCEEDED)
	{
#ifdef COBRA_ONLY
		if(ns<0) {icon[0]=0; return;}

		char tempstr[_4KB_];

		if(isdir)
			sprintf(tempstr, "%s/%s/PS3_GAME/ICON0.PNG", param, file);
        else
		{
			get_name(icon, file, 0);
			sprintf(tempstr, "%s/%s.jpg", param, icon);

			get_name(icon, file, 1); strcat(icon, ".jpg"); //wmtmp
			if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;
		}

#ifndef LITE_EDITION
		int64_t file_size;
		int is_directory=0;
		u64 mtime, ctime, atime;

		abort_connection=0;
		if(remote_stat(ns, tempstr, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)!=0)
		{
			icon[strlen(icon)-4]=0; strcat(icon, ".png");
            if(cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

			tempstr[strlen(tempstr)-4]=0; strcat(tempstr, ".png");

			if(remote_stat(ns, tempstr, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)!=0)
			{
				icon[0]=0;
				return;
			}
		}

		//Copy remote icon locally
		if(file_size>0 && open_remote_file_2(ns, (char*)tempstr, &abort_connection)>0 && !abort_connection)
		{
			int fdw;
			if(cellFsOpen(icon, CELL_FS_O_CREAT|CELL_FS_O_RDWR|CELL_FS_O_TRUNC, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
			{
				int bytes_read, boff=0;

				while(boff<file_size)
				{
					bytes_read = read_remote_file(ns, (char*)tempstr, boff, _4KB_, &abort_connection);
					if(bytes_read)
						cellFsWrite(fdw, (char*)tempstr, bytes_read, NULL);
					boff+=bytes_read;
					if(bytes_read<(int)_4KB_ || abort_connection) break;
				}
				cellFsClose(fdw);
				if(boff<1 || abort_connection) cellFsUnlink(icon);
				open_remote_file_2(ns, (char*)"/CLOSEFILE", &abort_connection);
				return;
			}
		}
#endif //#ifndef LITE_EDITION

#endif //#ifdef COBRA_ONLY
		icon[0]=0;
	}
}

static bool get_cover_from_name(char *icon, char *name, char *titleid)
{
	struct CellFsStat s;
	if(icon[0]!=0 && cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return true;

	if(get_cover(icon, titleid)) return true;

	if(titleid[0]==0 && (strstr(name, "-[") || strstr(name, " [B") || strstr(name, " [N") || strstr(name, " [S")))
	{
		if(strstr(name, "-["))
			{char *p=name; while(*p && p[9]!='-' && p[10]!='[' && p[9]!=0) p++; strncpy(titleid, p, 10);}
		else if(strstr(name, " [B"))
			strncpy(titleid, strstr(name, " [B") + 2, 10); //BCES/BLES/BCUS/BLUS/etc.
		else if(strstr(name, " [N"))
			strncpy(titleid, strstr(name, " [N") + 2, 10); //NP*
		else
			strncpy(titleid, strstr(name, " [S") + 2, 10); //SLES/SCES/SCUS/SLUS/etc.
	}
	else if(titleid[0]==0 && name[4] == '_' && name[8] == '.' && (name[0] == 'B' || name[0] == 'N' || name[0] == 'S' || name[0] == 'U') && (name[9] >= '0' && name[9] <= '9') && (name[10] >= '0' && name[10] <= '9'))
	{
		sprintf(titleid, "%c%c%c%c%c%c%c%c%c", name[0], name[1], name[2], name[3], name[5], name[6], name[7], name[9], name[10]); //SCUS_999.99.filename.iso
	}

	if(titleid[4]=='-') strncpy(&titleid[4], &titleid[5], 5); titleid[9]='\0';

	if(get_cover(icon, titleid)) return true;

	return false;
}

static void get_default_icon(char *icon, char *param, char *file, int isdir, char *titleid, int ns, int abort_connection)
{
	struct CellFsStat s;

	// continue using cover or default icon0.png
	if(icon[0]!=0 && cellFsStat(icon, &s)==CELL_FS_SUCCEEDED)
	{
		if(!extcasecmp(icon, ".png", 4) || !extcasecmp(icon, ".jpg", 4)) return;
		icon[0]=0;
	}

	if(!webman_config->nocov && get_cover_from_name(icon, file, titleid)) return; // show mm cover

	// get icon from folder && copy remote icon
	get_iso_icon(icon, param, file, isdir, ns, abort_connection);

	if(icon[0]!=0 && cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

	//use the cached PNG from wmtmp if available
	get_name(icon, file, 1);
	strcat(icon, ".PNG");

	if(icon[0] && cellFsStat(icon, &s)==CELL_FS_SUCCEEDED) return;

	if(webman_config->nocov && get_cover_from_name(icon, file, titleid)) return; // show mm cover as last option (if it's disabled)

    //show the default icon by type
	if(strstr(param, "/PS2ISO") || !extcmp(param, ".BIN.ENC", 8))
		strcpy(icon, wm_icons[7]);
	else if(strstr(param, "/PSX") || !extcmp(file, ".ntfs[PSXISO]", 13))
		strcpy(icon, wm_icons[6]);
	else if(strstr(param, "/PSPISO") || strstr(param, "/ISO/"))
		strcpy(icon, wm_icons[8]);
	else if(strstr(param, "/DVDISO") || !extcmp(file, ".ntfs[DVDISO]", 13))
		strcpy(icon, wm_icons[9]);
	else //if(strstr(param, "/BDISO") || !extcmp(file, ".ntfs[BDISO]", 12) || || !extcmp(file, ".ntfs[BDFILE]", 13))
		strcpy(icon, wm_icons[5]);
}

static int get_title_and_id_from_sfo(char *templn, char *tempID, char *entry_name, char *icon, char *data)
{
	int fdw;

	if(cellFsOpen(templn, CELL_FS_O_RDONLY, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
	{
		uint64_t msiz = 0;
		cellFsLseek(fdw, 0, CELL_FS_SEEK_SET, &msiz);
		cellFsRead(fdw, (void *)data, _4KB_, &msiz);
		cellFsClose(fdw);

		templn[0]=0;
		if(msiz>256)
		{
			unsigned char *mem = (u8*)data;
			parse_param_sfo(mem, tempID, templn);
			if(!webman_config->nocov) get_cover(icon, tempID);
		}
        if(templn[0]==0) get_name(templn, entry_name, 2); //use file name as title
		return 0;
	}
	else
		{get_name(templn, entry_name, 2); utf8enc(data, templn);} //use file name as title

		return 1;
}

static void make_fb_xml(char *myxml)
{
	cellFsUnlink(FB_XML);
	sprintf(myxml,  "%s"
					"<View id=\"seg_fb\">"
					"<Attributes><Table key=\"mgames\">"
					XML_PAIR("icon_rsc","item_tex_ps3util")
					XML_PAIR("icon_notation","WNT_XmbItemSavePS3")
					XML_PAIR("title","%s%s")
					XML_PAIR("info","%s")
					"</Table>"
					"</Attributes>"
					"<Items>"
					QUERY_XMB("mgames", "xmb://localhost%s#seg_mygames")
					"%s</XMBML>", XML_HEADER, STR_MYGAMES, SUFIX2(profile), STR_LOADGAMES, MY_GAMES_XML, "</Items></View>");
	savefile((char*)FB_XML, (char*)myxml, strlen(myxml));
}

static void waitfor(char *path, uint8_t timeout)
{
	struct CellFsStat s;
	for(uint8_t n=0; n < (timeout*2); n++)
	{
		if(path[0]!=NULL && cellFsStat(path, &s)==CELL_FS_SUCCEEDED) break;
		sys_timer_usleep(500000); if(!working) break;
	}
}

static void enable_classic_ps2_mode(void)
{
	int fd;
	if(cellFsOpen((char*)PS2_CLASSIC_TOGGLER, CELL_FS_O_CREAT| CELL_FS_O_TRUNC |CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsClose(fd);
		cellFsChmod((char*)PS2_CLASSIC_TOGGLER, MODE);
	}
}

static void disable_classic_ps2_mode(void)
{
	cellFsUnlink((char*)PS2_CLASSIC_TOGGLER);
/*
	CellFsStat st;
	if(cellFsStat((char*)PS2_CLASSIC_TOGGLER, &st) != CELL_FS_SUCCEEDED)
	{   // create dummy ISO.BIN.ENC (not longer necessary)
		char iso_bin_enc[0x4000];
		char iso_header[0x87] = {0x50, 0x53, 0x32, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x32, 0x50, 0x30, 0x30, 0x30, 0x31, 0x2D, 0x50, 0x53, 0x32, 0x55, 0x31, 0x30, 0x30, 0x30, 0x30, 0x5F, 0x30, 0x30, 0x2D, 0x30, 0x30, 0x30, 0x30, 0x31, 0x31, 0x31, 0x31, 0x32, 0x32, 0x32, 0x32, 0x33, 0x33, 0x33, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0xA8, 0xF7, 0xAE, 0xD9, 0xA9, 0x85, 0x74, 0x94, 0xA9, 0xB5, 0xE3, 0xAE, 0xF8, 0x7D, 0x61, 0xA6, 0x2F, 0x90, 0xB9, 0x0A, 0x8E, 0xDD, 0x2E, 0x2D, 0x74, 0x29, 0xF2, 0x99, 0x05, 0x9A, 0x84, 0x49, 0xB7, 0x63, 0x05, 0xD8, 0xFC, 0xA7, 0x53, 0x79, 0xD8, 0x96, 0x99, 0xB4, 0x70, 0x49, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40};
		memset(iso_bin_enc, 0, 0x4000);
		for(u8 i=0; i<0x87; i++) iso_bin_enc[i]=iso_header[i];

		int fd;
		cellFsUnlink((char*)PS2_CLASSIC_ISO_PATH);
		if(cellFsOpen((char*)PS2_CLASSIC_ISO_PATH, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			uint64_t msiz = 0;
			cellFsWrite(fd, (void *)&iso_bin_enc, 0x4000, &msiz);
			cellFsClose(fd);
			cellFsChmod((char*)PS2_CLASSIC_ISO_PATH, MODE);
		}
	}
*/
}

#ifdef COBRA_ONLY
#ifndef LITE_EDITION
int connect_to_remote_server(u8 server_id)
{
	int ns=FAILED;
	if( (server_id==0 && webman_config->netp0 && webman_config->neth0[0]) ||
		(server_id==1 && webman_config->netp1 && webman_config->neth1[0]) ||
		(server_id==2 && webman_config->netp2 && webman_config->neth2[0]) )
	{
		if(server_id==1 && strcmp(webman_config->neth0, webman_config->neth1)==0 && webman_config->netp0==webman_config->netp1 && webman_config->netd0==webman_config->netd1) return FAILED;
		if(server_id==2 && strcmp(webman_config->neth0, webman_config->neth2)==0 && webman_config->netp0==webman_config->netp2 && webman_config->netd0==webman_config->netd2) return FAILED;
		if(server_id==2 && strcmp(webman_config->neth1, webman_config->neth2)==0 && webman_config->netp1==webman_config->netp2 && webman_config->netd1==webman_config->netd2) return FAILED;

		u8 retries=0;
	reconnect:
		if(server_id==0) ns=connect_to_server(webman_config->neth0, webman_config->netp0); else
		if(server_id==1) ns=connect_to_server(webman_config->neth1, webman_config->netp1); else
		if(server_id==2) ns=connect_to_server(webman_config->neth2, webman_config->netp2);

		if(ns<0)
		{
			if(retries<3)
			{
				retries++;
				sys_timer_sleep(1);
				goto reconnect;
			}
		}
	}
	return ns;
}
#endif //#ifndef LITE_EDITION
#endif //#ifdef COBRA_ONLY

static void mount_autoboot(void)
{
	struct CellFsStat s;
	char path[MAX_PATH_LEN];

	// get autoboot path
	if(webman_config->autob &&
      ((cobra_mode && strstr(webman_config->autoboot_path, "/net")!=NULL) || cellFsStat((char *)webman_config->autoboot_path, &s)==CELL_FS_SUCCEEDED)) // autoboot
		strcpy(path, (char *) webman_config->autoboot_path);
	else
	{   // get last game path
		sprintf(path, WMTMP "/last_game.txt");
		if(webman_config->lastp && cellFsStat(path, &s)==CELL_FS_SUCCEEDED)
		{
			int fd=0;
			if(cellFsOpen(path, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				u64 read_e = 0;
				if(cellFsRead(fd, (void *)path, MAX_PATH_LEN, &read_e) == CELL_FS_SUCCEEDED) path[read_e]=0;
				cellFsClose(fd);
			}
			else
				path[0]=0;
		}
		else
			path[0]=0;
	}

	bool do_mount=false;

	if(from_reboot && !path[0] && strstr(path, "/PS2")) return; //avoid re-launch PS2 returning to XMB

    // wait few seconds until path becomes ready
	if(strlen(path)>10 || (cobra_mode && strstr(path, "/net")!=NULL))
	{
		waitfor((char*)path, 2*(webman_config->boots+webman_config->bootd));
		do_mount=((cobra_mode && strstr(path, "/net")!=NULL) || cellFsStat(path, &s)==CELL_FS_SUCCEEDED);
	}

	if(do_mount)
	{   // add some delay
		if(webman_config->delay)      {sys_timer_sleep(10); waitfor((char*)path, 2*(webman_config->boots+webman_config->bootd));}
		else if(strstr(path, "/net"))  sys_timer_sleep(10);
#ifndef COBRA_ONLY
		if(strstr(path, "/net")==NULL && strstr(path, ".ntfs[")==NULL)
#endif
		mount_with_mm(path, 1); // mount path
	}
}

#ifdef COBRA_ONLY
#ifndef LITE_EDITION
static int add_net_game(int ns, netiso_read_dir_result_data *data, int v3_entry, char *neth, char *param, char *templn, char *tempstr, char *enc_dir_name, char *icon, char *tempID, u8 f1, u8 is_html)
{
	int abort_connection=0, is_directory=0, fdw = 0; int64_t file_size; u64 mtime, ctime, atime;

	if(!data[v3_entry].is_directory)
	{
        int flen = strlen(data[v3_entry].name)-4;
		if(flen<0 || data[v3_entry].name[flen]!='.') return FAILED;
		if(!strcasestr(".iso.bin.mdf.img", data[v3_entry].name + flen)) return FAILED;
	}
	else
	{
		if(data[v3_entry].name[0]=='.') return FAILED;
		//if(!strstr(param, "/GAME")) return FAILED;
	}

	icon[0]=tempID[0]=0;


	if(IS_PS3_FOLDER) //PS3 games only
	{
		if(data[v3_entry].is_directory)
			sprintf(templn, WMTMP "/%s.SFO", data[v3_entry].name);
		else
			{get_name(templn, data[v3_entry].name, 1); strcat(templn, ".SFO\0");}

		struct CellFsStat buf;

		if(data[v3_entry].is_directory && cellFsStat(templn, &buf)!=CELL_FS_SUCCEEDED)
		{
			sprintf(tempstr, "%s/%s/PS3_GAME/PARAM.SFO", param, data[v3_entry].name);

			if(remote_stat(ns, tempstr, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)!=0) {v3_entry++; return FAILED;}

			if(cellFsOpen(templn, CELL_FS_O_CREAT|CELL_FS_O_RDWR|CELL_FS_O_TRUNC, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
			{
				open_remote_file_2(ns, tempstr, &abort_connection);

				int bytes_read, boff=0;
				while(boff<file_size)
				{
					bytes_read = read_remote_file(ns, (char*)tempstr, boff, 4000, &abort_connection);
					if(bytes_read)
						cellFsWrite(fdw, (char*)tempstr, bytes_read, NULL);
					boff+=bytes_read;
					if(bytes_read<4000 || abort_connection) break;
				}
				open_remote_file_2(ns, (char*)"/CLOSEFILE", &abort_connection);
				cellFsClose(fdw);
			}
			cellFsChmod(templn, MODE);
		}

		get_title_and_id_from_sfo(templn, tempID, data[v3_entry].name, icon, tempstr);
	}
	else
		{get_name(enc_dir_name, data[v3_entry].name, 0); utf8enc(templn, enc_dir_name);}

	struct CellFsStat buf;

	{get_name(enc_dir_name, data[v3_entry].name, 1); strcat(enc_dir_name, ".PNG"); if((icon[0]==0 || webman_config->nocov) && cellFsStat((char*)enc_dir_name, &buf)==CELL_FS_SUCCEEDED) strcpy(icon, enc_dir_name);}

	if(data[v3_entry].is_directory && IS_ISO_FOLDER)
	{
		char iso_ext[4][4] = {"iso", "bin", "mdf", "img"}; u8 e;
		for(e=0; e<5; e++)
		{
			if(e>=4) return FAILED;
			sprintf(tempstr, "%s/%s.%s", data[v3_entry].name, data[v3_entry].name, iso_ext[e]);

			sprintf(enc_dir_name, "%s/%s", param, tempstr);
			if(remote_stat(ns, enc_dir_name, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)==0) break;
		}

		// cover: folder/filename.jpg
		sprintf(enc_dir_name, "%s/%s/%s.jpg", param, data[v3_entry].name, data[v3_entry].name);
		if(remote_stat(ns, enc_dir_name, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)==0)
			sprintf(icon, "%s%s", neth, enc_dir_name);
		else
			get_default_icon(icon, param, tempstr, data[v3_entry].is_directory, tempID, ns, abort_connection);

		urlenc(enc_dir_name, tempstr);
	}
	else
	{
		urlenc(enc_dir_name, data[v3_entry].name);
		get_default_icon(icon, param, data[v3_entry].name, data[v3_entry].is_directory, tempID, ns, abort_connection);
	}

	if(webman_config->tid && tempID[0]>'@' && strlen(templn) < 50 && strstr(templn, " [")==NULL) {strcat(templn, " ["); strcat(templn, tempID); strcat(templn, "]");}

	return 0;
}
#endif //#ifndef LITE_EDITION
#endif //#ifdef COBRA_ONLY

static void add_list_entry(char *tempstr, bool is_dir, char *ename, char *templn, char *name, char *fsize, CellRtcDateTime rDate, u16 flen, unsigned long long sz, char *sf, u8 is_net)
{
	if(sz<10240) sprintf(sf, "%s", STR_BYTE);
	else if(sz<2097152) {sprintf(sf, "%s", STR_KILOBYTE); sz>>=10;}
	else if(sz<2147483648U) {sprintf(sf, "%s", STR_MEGABYTE); sz>>=20;}
	else {sprintf(sf, "%s", STR_GIGABYTE); sz>>=30;}

	urlenc(tempstr, templn); strncpy(templn, tempstr, MAX_LINE_LEN);
	strcpy(tempstr, name);

	htmlenc(name, tempstr, 0);

	flen=strlen(name);

	if(is_dir)
	{
		if(name[0]=='.')
			sprintf(fsize, "<a href=\"%s\">%s</a>", templn, HTML_DIR);
		else if(flen == 9 && !strcmp(name, "dev_blind"))
			sprintf(fsize, "<a href=\"%s?0\">%s</a>", templn, HTML_DIR);
#ifdef FIX_GAME
		else if(flen == 9 && strstr(templn, "/dev_hdd0/game/"))
			sprintf(fsize, "<a href=\"/fixgame.ps3%s\">%s</a>", templn, HTML_DIR);
#endif //#ifdef FIX_GAME
		else
#ifdef PS2_DISC
			sprintf(fsize, "<a href=\"/mount%s%s\">%s</a>", strstr(name, "[PS2")?".ps2":".ps3", templn, HTML_DIR);
#else
			sprintf(fsize, "<a href=\"/mount.ps3%s\">%s</a>", templn, HTML_DIR);
#endif //#ifdef PS2_DISC
	}


#ifdef COBRA_ONLY
	else if( (flen > 4 && name[flen-4]=='.' && strcasestr(ISO_EXTENSIONS, name+flen-4)) || (!is_net && ( strstr(name, ".ntfs[") || !extcmp(name, ".BIN.ENC", 8) )) )
	{
		if( strcasestr(name, ".iso.") && extcasecmp(name, ".iso.0", 6) )
			sprintf(fsize, "%llu %s", sz, sf);
		else
			sprintf(fsize, "<a href=\"/mount.ps3%s\">%llu %s</a>", templn, sz, sf);
	}
#endif //#ifdef COBRA_ONLY


#ifdef COPY_PS3
 #ifdef SWAP_KERNEL
	else if(!is_net && ( !extcmp(name, ".pkg", 4) || !extcmp(name, ".edat", 5) || !extcmp(name, ".p3t", 4) || !memcmp(name, "webftp_server", 13) || !memcmp(name, "boot_plugins_", 13) || !memcmp(name, "lv2_kernel", 10) ))
 #else
	else if(!is_net && ( !extcmp(name, ".pkg", 4) || !extcmp(name, ".edat", 5) || !extcmp(name, ".p3t", 4) || !memcmp(name, "webftp_server", 13) || !memcmp(name, "boot_plugins_", 13) ))
 #endif
		sprintf(fsize, "<a href=\"/copy.ps3%s\">%llu %s</a>", templn, sz, sf);
#endif //#ifdef COPY_PS3


#ifdef LOAD_PRX
	else if(!is_net && ( !extcmp(name, ".sprx", 5)))
		sprintf(fsize, "<a href=\"/loadprx.ps3?slot=6&prx=%s\">%llu %s</a>", templn, sz, sf);
#endif
	else
		sprintf(fsize, "%llu %s", sz, sf);

	snprintf(ename, 6, "%s    ", name); urlenc(templn, tempstr);

	sprintf(tempstr, "%c%c%c%c%c%c<tr>"
                     "<td><a %shref=\"%s\">%s</a></td>",
	is_dir ? '0' : '1', ename[0], ename[1], ename[2], ename[3], ename[4],
	is_dir ? "class=\"d\" " : "class=\"w\" ", templn, name);

	flen=strlen(tempstr);
	if(flen>=LINELEN)
	{
		if(is_dir) sprintf(fsize, HTML_DIR); else sprintf(fsize, "%llu %s", sz, sf);

		sprintf(tempstr, "%c%c%c%c%c%c<tr>"
                         "<td><a %shref=\"%s\">%s</a></td>",
		is_dir ? '0' : '1', ename[0], ename[1], ename[2], ename[3], ename[4],
		is_dir ? "class=\"d\" " : "class=\"w\" ", templn, name);

		flen=strlen(tempstr);
		if(flen>=LINELEN)
		{
			if(is_dir) sprintf(fsize, HTML_DIR); else sprintf(fsize, "%llu %s", sz, sf);

			sprintf(tempstr, "%c%c%c%c%c%c<tr>"
                             "<td>%s</td>",
			is_dir ? '0' : '1', ename[0], ename[1], ename[2], ename[3], ename[4],
			name);
		}
	}

	sprintf(templn, "<td> %s &nbsp; </td>"
					"<td>%02i-%s-%04i %02i:%02i</td></tr>",
					fsize,
					rDate.day, smonth[rDate.month-1], rDate.year, rDate.hour, rDate.minute);
	strcat(tempstr, templn);

	flen=strlen(tempstr);
	if(flen>=LINELEN) {flen=0; tempstr[0]=0;} //ignore file if it is still too long
}

static void add_query_html(char *buffer, char *param, char *label)
{
    char templn[64];
    sprintf(templn, "[<a href=\"/index.ps3?%s\">%s</a>] ", param, label); strcat(buffer, templn);
}

static void add_xmb_entry(char *param, char *tempstr, char *templn, char *skey, u32 key, char *myxml_ps3, char *myxml_ps2, char *myxml_psx, char *myxml_psp, char *myxml_dvd, char *entry_name, u16 *item_count)
{
	if(strlen(templn)<5) strcat(templn, "     ");
	sprintf(skey, "3%c%c%c%c%04i", templn[0], templn[1], templn[2], templn[3], key);

	if( !(webman_config->nogrp) )
	{
		if(strstr(param, "/PS2ISO") && strlen(myxml_ps2)<(BUFFER_SIZE_PS2-1024))
		{strcat(myxml_ps2, tempstr); skey[0]=PS2; item_count[2]++;}
#ifdef COBRA_ONLY
		else
		if((strstr(param, "/PSPISO") || strstr(param, "/ISO")) && strlen(myxml_psp)<(BUFFER_SIZE_PSP-1024))
		{strcat(myxml_psp, tempstr); skey[0]=PSP; item_count[4]++;}
		else
		if((strstr(param, "/PSX") || !extcmp(entry_name, ".ntfs[PSXISO]", 13)) && strlen(myxml_psx)<(BUFFER_SIZE_PSX-1024))
		{strcat(myxml_psx, tempstr); skey[0]=PS1; item_count[1]++;}
		else
		if((strstr(param, "/BDISO") || strstr(param, "/DVDISO") || !extcmp(entry_name, ".ntfs[DVDISO]", 13) || !extcmp(entry_name, ".ntfs[BDISO]", 12) || !extcmp(entry_name, ".ntfs[BDFILE]", 13)) && strlen(myxml_dvd)<(BUFFER_SIZE_DVD-1024))
		{strcat(myxml_dvd, tempstr); skey[0]=BLU; item_count[0]++;}
#endif
		else
		if(strlen(myxml_ps3)<(BUFFER_SIZE-5000))
        {strcat(myxml_ps3, tempstr); item_count[3]++;}
	}
	else
	{
		if(strlen(myxml_ps3)<(BUFFER_SIZE-5000))
			strcat(myxml_ps3, tempstr);
	}
}

static void prepare_header(char *header, char *param, u8 is_binary)
{
	bool set_base_path = false;

	strcpy(header, "HTTP/1.1 200 OK\r\nContent-Type: \0");
	if(is_binary==1)
	{
		if(!extcasecmp(param, ".htm", 4) || !extcasecmp(param, ".html", 5) || strcasestr(param, ".shtm"))
			{strcat(header, "text/html"); set_base_path = true;}
		else
		if(!extcasecmp(param, ".jpg", 4) || !extcasecmp(param, ".jpeg", 5) || !extcmp(param, ".STH", 4))
			strcat(header, "image/jpeg");
		else
		if(!extcasecmp(param, ".png", 4))
			strcat(header, "image/png");
		else
		if(!extcasecmp(param, ".txt", 4) || !extcasecmp(param, ".log", 4) || !extcasecmp(param, ".ini", 4) || !extcmp(param, ".HIP", 4) || !extcmp(param, ".HIS", 4))
			strcat(header, "text/plain");
		else
		if(!extcasecmp(param, ".css", 4))
			strcat(header, "text/css");
		else
		if(!extcasecmp(param, ".js", 3))
			strcat(header, "text/javascript");
		else
		if(!extcasecmp(param, ".svg", 4))
			strcat(header, "image/svg+xml");
#ifndef LITE_EDITION
		else
		if(!extcasecmp(param, ".gif", 4))
			strcat(header, "image/gif");
		else
		if(!extcasecmp(param, ".avi", 4))
			strcat(header, "video/x-msvideo");
		else
		if(!extcasecmp(param, ".mkv", 4))
			strcat(header, "video/x-matroska");
		else
		if(!extcasecmp(param, ".mp4", 4))
			strcat(header, "video/mp4");
		else
		if(!extcasecmp(param, ".mpg", 4) || !extcasecmp(param, ".mp2", 4) || strcasestr(param, ".mpe"))
			strcat(header, "video/mpeg");
		else
		if(!extcasecmp(param, ".vob", 4))
			strcat(header, "video/vob");
		else
		if(!extcasecmp(param, ".wmv", 4))
			strcat(header, "video/x-ms-wmv");
		else
		if(!extcasecmp(param, ".mov", 4))
			strcat(header, "video/quicktime");
		else
		if(!extcasecmp(param, ".mp3", 4))
			strcat(header, "audio/mpeg");
		else
		if(!extcasecmp(param, ".wav", 4))
			strcat(header, "audio/x-wav");
		else
		if(!extcasecmp(param, ".bmp", 4))
			strcat(header, "image/bmp");
		else
		if(!extcasecmp(param, ".tif", 4))
			strcat(header, "image/tiff");
		else
		if(!extcasecmp(param, ".zip", 4))
			strcat(header, "application/zip");
		else
		if(!extcasecmp(param, ".pdf", 4))
			strcat(header, "application/pdf");
		else
		if(!extcasecmp(param, ".swf", 4))
			strcat(header, "application/x-shockwave-flash");
#endif
		else
			strcat(header, "application/octet-stream");
	}
	else
		{strcat(header, "text/html"); set_base_path = true;}

	if(set_base_path && param[0]=='/') {strncpy(html_base_path, param, MAX_PATH_LEN); html_base_path[MAX_PATH_LEN]=0; html_base_path[strrchr(html_base_path, '/')-html_base_path]=0;}

	strcat(header, "\r\n");
}

static void prepare_html(char *buffer, char *templn, char *param, u8 is_ps3_http, u8 is_cpursx, bool mount_ps3)
{
	strcpy(buffer,  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
					"<html xmlns=\"http://www.w3.org/1999/xhtml\">"
					"<meta http-equiv=\"Content-type\" content=\"text/html;charset=UTF-8\">"
					"<meta http-equiv=\"cache-control\" content=\"no-cache\">"
					"<meta content='target-densitydpi=device-dpi; width=device-width; initial-scale=0.6; maximum-scale=1.0;' name='viewport'>");

	if(is_cpursx) strcat(buffer, "<meta http-equiv=\"refresh\" content=\"6;URL=/cpursx.ps3\">");
	if(mount_ps3) {	strcat(buffer, "<body bgcolor=\"#101010\">"); return;}

	strcat(buffer,	"<head><title>webMAN MOD</title>"
					"<style type=\"text/css\"><!--\r\n"

					"a.s:active{color:#F0F0F0;}"
					"a:link{color:#909090;text-decoration:none;}"

					"a.f:active{color:#F8F8F8;}"
					"a,a.f:link,a:visited{color:#D0D0D0;}");

	if(!is_cpursx)
	strcat(buffer,	"a.d:link{color:#D0D0D0;background-position:0px 2px;background-image:url('data:image/gif;base64,R0lGODlhEAAMAIMAAOenIumzLbmOWOuxN++9Me+1Pe+9QvDAUtWxaffKXvPOcfTWc/fWe/fWhPfckgAAACH5BAMAAA8ALAAAAAAQAAwAAARQMI1Agzk4n5Sa+84CVNUwHAz4KWzLMo3SzDStOkrHMO8O2zmXsAXD5DjIJEdxyRie0KfzYChYr1jpYVAweb/cwrMbAJjP54AXwRa433A2IgIAOw==');padding:0 0 0 20px;background-repeat:no-repeat;margin-left:auto;margin-right: auto;}"
					"a.w:link{color:#D0D0D0;background-image:url('data:image/gif;base64,R0lGODlhDgAQAIMAAAAAAOfn5+/v7/f39////////////////////////////////////////////wAAACH5BAMAAA8ALAAAAAAOABAAAAQx8D0xqh0iSHl70FxnfaDohWYloOk6papEwa5g37gt5/zO475fJvgDCW8gknIpWToDEQA7');padding:0 0 0 20px;background-repeat:no-repeat; margin-left:auto; margin-right:auto;}");

	strcat(buffer,	"a:active,a:active:hover,a:visited:hover,a:link:hover{color:#FFFFFF;}"
					".list{display:inline;}"
					"input:focus{border:2px solid #0099FF;}"
					".gc{float:left;overflow:hidden;position:relative;text-align:center;width:280px;height:260px;margin:3px;border:1px dashed grey;}"
					".ic{position:absolute;top:5px;right:5px;left:5px;bottom:40px;}"
					".propfont{font-family:\"Courier New\",Courier,monospace;}"
					"body,a.s,td,th{color:#F0F0F0;white-space:nowrap;");

	if(is_ps3_http==2)
		strcat(buffer, "width:800px;}");
	else
		strcat(buffer, "}");

	if(!strstr(param, "/setup.ps3")) strcat(buffer, "td+td{text-align:right;white-space:nowrap}");

	if(is_ps3_http==1)
		strcat(buffer, ".gi{height:210px;width:267px;");
	else
		strcat(buffer, ".gi{position:absolute;max-height:210px;max-width:260px;");

	strcat(buffer, "position:absolute;bottom:0px;top:0px;left:0px;right:0px;margin:auto;}"
				   ".gn{position:absolute;height:38px;bottom:0px;right:7px;left:7px;text-align:center;}--></style>");

	if(param[1]!=NULL) {sprintf(templn, "<base href=\"%s/\">", param); strcat(buffer, templn);}

	strcat(buffer,	"</head>"
					"<body bgcolor=\"#101010\">"
					"<font face=\"Courier New\"><b>");

#ifndef ENGLISH_ONLY
    if(strlen(STR_TRADBY)==0) language("STR_TRADBY", STR_TRADBY); //strcpy(STR_TRADBY, "<br>");
    if(strlen(STR_HOME  )==0) language("STR_HOME"  , STR_HOME  ); //strcpy(STR_HOME  , "<br>");
#else
    strcpy(STR_HOME, "Home");
#endif

#ifdef PS3MAPI
	#ifdef WEB_CHAT
		sprintf(templn, "webMAN " WM_VERSION " %s <font style=\"font-size:18px\">[<a href=\"/\">%s</a>] [<a href=\"/index.ps3\">%s</a>] [<a href=\"/games.ps3\">Slider</a>] [<a href=\"/chat.ps3\">Chat</a>] [<a href=\"/home.ps3mapi\">PS3MAPI</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_TRADBY, STR_FILES, STR_GAMES, STR_SETUP); strcat(buffer, templn);
	#else
		sprintf(templn, "webMAN " WM_VERSION " %s <font style=\"font-size:18px\">[<a href=\"/\">%s</a>] [<a href=\"/index.ps3\">%s</a>] [<a href=\"/games.ps3\">Slider</a>] [<a href=\"/home.ps3mapi\">PS3MAPI</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_TRADBY, STR_FILES, STR_GAMES, STR_SETUP ); strcat(buffer, templn);
	#endif
#else
	#ifdef WEB_CHAT
		sprintf(templn, "webMAN " WM_VERSION " %s <font style=\"font-size:18px\">[<a href=\"/\">%s</a>] [<a href=\"/index.ps3\">%s</a>] [<a href=\"/games.ps3\">Slider</a>] [<a href=\"/chat.ps3\">Chat</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_TRADBY, STR_FILES, STR_GAMES, STR_SETUP); strcat(buffer, templn);
	#else
		sprintf(templn, "webMAN " WM_VERSION " %s <font style=\"font-size:18px\">[<a href=\"/\">%s</a>] [<a href=\"/index.ps3\">%s</a>] [<a href=\"/games.ps3\">Slider</a>] [<a href=\"/setup.ps3\">%s</a>]</b>", STR_TRADBY, STR_FILES, STR_GAMES, STR_SETUP ); strcat(buffer, templn);
	#endif
#endif

}

static bool update_mygames_xml(u64 conn_s_p)
{
	struct CellFsStat buf;

	char xml[48]; sprintf(xml, MY_GAMES_XML);

	if(conn_s_p==START_DAEMON && ((webman_config->refr==1) || from_reboot))
	{
		cellFsUnlink((char*)WMNOSCAN);

		mount_autoboot();

		if(cellFsStat((char*)xml, &buf)==CELL_FS_SUCCEEDED)
		{
			if(cellFsStat(FB_XML, &buf)==CELL_FS_SUCCEEDED) return false;
		}
	}

	set_buffer_sizes(webman_config->foot);

	_meminfo meminfo;
	{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}
	if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) set_buffer_sizes(3); //MIN+
	if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) set_buffer_sizes(1); //MIN
	if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) //leave if less than min memory
	{
		return false;
	}

	sys_addr_t sysmem=0;

#ifdef USE_VM
	if(sys_vm_memory_map(_32MB_, _1MB_, SYS_MEMORY_CONTAINER_ID_INVALID, SYS_MEMORY_PAGE_SIZE_64K, SYS_VM_POLICY_AUTO_RECOMMENDED, &sysmem)!=CELL_OK)
	{
		return false;
	}
#else
	if(sys_memory_allocate((BUFFER_SIZE_ALL), SYS_MEMORY_PAGE_SIZE_64K, &sysmem)!=0)
	{
		return false;
	}
#endif

	sys_addr_t sysmem1=sysmem +(BUFFER_SIZE);
	sys_addr_t sysmem2=sysmem1+(BUFFER_SIZE_PSX)+(BUFFER_SIZE_PSP);
	sys_addr_t sysmem3=sysmem2+(BUFFER_SIZE_PS2);

	char *myxml_ps3   = (char*)sysmem;
	char *myxml_psx   = NULL;
	char *myxml_psp   = NULL;
	char *myxml_ps2   = NULL;
	char *myxml_dvd   = NULL;
	char *myxml       = NULL;
	char *myxml_items = NULL;

	myxml_psx = (char*)sysmem1;
	myxml_psp = (char*)sysmem1+(BUFFER_SIZE_PSX);
	myxml_ps2 = (char*)sysmem2;

	myxml_dvd	= (char*)sysmem3;
	myxml		= (char*)sysmem+(BUFFER_SIZE)-4300;
	myxml_items = (char*)sysmem3;

	cellFsMkdir((char*)"/dev_hdd0/xmlhost", MODE);
	cellFsMkdir((char*)"/dev_hdd0/xmlhost/game_plugin", MODE);
	u32 key=0;

	make_fb_xml(myxml);

	if(conn_s_p==START_DAEMON)
	{
#ifdef USE_VM
		sys_vm_unmap(sysmem);
#else
		sys_memory_free(sysmem);
#endif
		// start a new thread for refresh content at start up
		if(!webman_config->refr || cellFsStat((char*)xml, &buf)!=CELL_FS_SUCCEEDED)
		{
			sys_ppu_thread_t id3;
			sys_ppu_thread_create(&id3, handleclient, (u64)REFRESH_CONTENT, -0x1d8, 0x20000, 0, "wwwd2");
		}
		return false;
	}

	// --- build group headers ---
	char templn[1024]; char *tempstr, *folder_name; tempstr=myxml; memset(tempstr, 0, _4KB_); folder_name=myxml+(3*KB);

	u16 item_count[5];
	for(u8 i=0;i<5;i++) item_count[i]=0;

	cellFsUnlink(xml);

	key=0;

	if(!(webman_config->nogrp))
	{
		if(!(webman_config->cmask & PS3)) strcpy(myxml_ps3, "<View id=\"seg_wm_ps3_items\"><Attributes>");
		if(!(webman_config->cmask & PS2))
		{
			strcpy(myxml_ps2, "<View id=\"seg_wm_ps2_items\"><Attributes>");
			if(webman_config->ps2l && cellFsStat((char*)"/dev_hdd0/game/PS2U10000", &buf)==CELL_FS_SUCCEEDED)
			{
				sprintf(templn, "<Table key=\"ps2_classic_launcher\">"
								XML_PAIR("icon","/dev_hdd0/game/PS2U10000/ICON0.PNG")
								XML_PAIR("title","PS2 Classic Launcher")
								XML_PAIR("info","%s") "%s",
								STR_LAUNCHPS2, "</Table>"); strcat(myxml_ps2, templn);
			}
		}
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) strcpy(myxml_psx, "<View id=\"seg_wm_psx_items\"><Attributes>");
		if(!(webman_config->cmask & PSP))
		{
			strcpy(myxml_psp, "<View id=\"seg_wm_psp_items\"><Attributes>");
			if(webman_config->pspl && cellFsStat((char*)"/dev_hdd0/game/PSPC66820", &buf)==CELL_FS_SUCCEEDED)
			{
				sprintf(templn, "<Table key=\"cobra_psp_launcher\">"
								XML_PAIR("icon","/dev_hdd0/game/PSPC66820/ICON0.PNG")
								XML_PAIR("title","PSP Launcher")
								XML_PAIR("info","%s") "%s",
								STR_LAUNCHPSP, "</Table>"); strcat(myxml_psp, templn);
			}
		}
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			strcpy(myxml_dvd, "<View id=\"seg_wm_dvd_items\"><Attributes>");
			if(webman_config->rxvid)
			{
				sprintf(templn, "<Table key=\"rx_video\">"
								XML_PAIR("icon","%s")
								XML_PAIR("title","%s")
								XML_PAIR("child","segment") "%s",
								wm_icons[4], STR_VIDLG, STR_NOITEM_PAIR); strcat(myxml_dvd, templn);
			}
		}
#endif
	}

	CellRtcTick pTick;
	cellRtcGetCurrentTick(&pTick);

	int fd;
	char skey[1024][10];
	u8 is_net=0;

	// --- scan xml content ---

	led(YELLOW, BLINK_FAST);

	for(u8 f0=0; f0<14; f0++)  // drives: 0="/dev_hdd0", 1="/dev_usb000", 2="/dev_usb001", 3="/dev_usb002", 4="/dev_usb003", 5="/dev_usb006", 6="/dev_usb007", 7="/net0", 8="/net1", 9="/net2", 10="/ext", 11="/dev_sd", 12="/dev_ms", 13="/dev_cf"
	{
		if(!webman_config->usb0 && (f0==1)) continue;
		if(!webman_config->usb1 && (f0==2)) continue;
		if(!webman_config->usb2 && (f0==3)) continue;
		if(!webman_config->usb3 && (f0==4)) continue;
		if(!webman_config->usb6 && (f0==5)) continue;
		if(!webman_config->usb7 && (f0==6)) continue;

		if(!webman_config->dev_sd && (f0==11)) continue;
		if(!webman_config->dev_ms && (f0==12)) continue;
		if(!webman_config->dev_cf && (f0==13)) continue;

		if( f0==NTFS && (!webman_config->usb0 && !webman_config->usb1 && !webman_config->usb2 &&
						 !webman_config->usb3 && !webman_config->usb6 && !webman_config->usb7)) continue;

		int ns=-2; u8 uprofile=profile;
		for(u8 f1=0; f1<11; f1++) // paths: 0="GAMES", 1="GAMEZ", 2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO", 7="PSXGAMES", 8="PSPISO", 9="ISO", 10="video"
		{
#ifndef COBRA_ONLY
			if(IS_ISO_FOLDER && !(IS_PS2_FOLDER)) continue; // 0="GAMES", 1="GAMEZ", 5="PS2ISO", 10="video"
#endif
			if(key>1600) break;

			cellRtcGetCurrentTick(&pTick);

			if(IS_PS2_FOLDER && f0>0)  continue; // PS2ISO is supported only from /dev_hdd0
            if(f1>=10) {if(f0<7 || f0>NTFS) strcpy(paths[10], f0==0 ? "video" : "GAMES_DUP"); else continue;}
			if(f0==NTFS) {if(f1>6 || !cobra_mode) break; else if(f1<2 || f1==5) continue;}
			if(f0==7 && (!webman_config->netd0 || f1>6 || !cobra_mode)) break; //net0
			if(f0==8 && (!webman_config->netd1 || f1>6 || !cobra_mode)) break; //net1
			if(f0==9 && (!webman_config->netd2 || f1>6 || !cobra_mode)) break; //net2

			if( (webman_config->cmask & PS3) && (IS_PS3_FOLDER)) continue; // 0="GAMES", 1="GAMEZ", 2="PS3ISO", 10="video"
			if( (webman_config->cmask & BLU) && (IS_BLU_FOLDER)) continue;
			if( (webman_config->cmask & DVD) && (IS_DVD_FOLDER)) continue;
			if( (webman_config->cmask & PS2) && (IS_PS2_FOLDER)) continue;
			if( (webman_config->cmask & PS1) && (IS_PSX_FOLDER)) continue;
			if( (webman_config->cmask & PSP) && (IS_PSP_FOLDER)) continue;

			is_net=(f0>=7 && f0<=9);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
			if(ns==-2 && is_net) ns=connect_to_remote_server(f0-7);
 #endif
#endif

			if(is_net && (ns<0)) break;

//
			char param[MAX_PATH_LEN];

			bool ls=false; u8 li=0, subfolder=0;

		subfolder_letter_xml:
			subfolder = 0; uprofile = profile;
read_folder_xml:
//
#ifndef LITE_EDITION
			if(is_net)
			{
				char ll[4]; if(li) sprintf(ll, "/%c", '@'+li); else ll[0]=0;
				sprintf(param, "/%s%s%s",    paths[f1], SUFIX(uprofile), ll);
			}
			else
#endif
			{
				if(f0==NTFS) //ntfs
					sprintf(param, "%s", WMTMP);
				else
					sprintf(param, "%s/%s%s", drives[f0], paths[f1], SUFIX(uprofile));
			}

			if(conn_s_p==START_DAEMON && f1==0)
			{
				if(webman_config->bootd && (f0==1))
				{
					waitfor((char*)"/dev_usb", webman_config->bootd); // wait for any usb
				}

				if(webman_config->boots && (f0>=1 && f0<=6)) // usb000->007
				{
					if( (webman_config->usb0 && (f0==1)) ||
						(webman_config->usb1 && (f0==2)) ||
						(webman_config->usb2 && (f0==3)) ||
						(webman_config->usb3 && (f0==4)) ||
						(webman_config->usb6 && (f0==5)) ||
						(webman_config->usb7 && (f0==6)) )
					{
						waitfor((char*)drives[f0], webman_config->boots);
					}
				}
			}

			if(!is_net && cellFsOpendir( param, &fd) != CELL_FS_SUCCEEDED) goto continue_reading_folder_xml; //continue;

			int abort_connection=0;

#ifndef LITE_EDITION
 #ifdef COBRA_ONLY
			if(is_net && open_remote_dir(ns, param, &abort_connection) < 0) goto continue_reading_folder_xml; //continue;
 #endif
#endif

			//led(YELLOW, ON);
			{
				CellFsDirent entry;
				u64 read_e;
				int fs;
				uint64_t msiz = 0;
				u8 is_iso=0;
				char icon[MAX_PATH_LEN], enc_dir_name[1024], subpath[MAX_PATH_LEN]; int fd2;
				char tempID[12];

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				sys_addr_t data2=0;
				int v3_entries, v3_entry; v3_entries=v3_entry=0;
				netiso_read_dir_result_data *data=NULL; char neth[8];

				if(is_net)
				{
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(data2==NULL) goto continue_reading_folder_xml; //continue;
					data=(netiso_read_dir_result_data*)data2; sprintf(neth, "/net%i", (f0-7));
				}
 #endif
#endif

				while((!is_net && cellFsReaddir(fd, &entry, &read_e) == 0 && read_e > 0)
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					|| (is_net && v3_entry<v3_entries)
 #endif
#endif
					)
				{
					if(key>1600) break;
					cellRtcGetCurrentTick(&pTick);
					icon[0]=tempID[0]=0;

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					if(is_net)
					{
						if(!ls && li==0 && f1>1 && data[v3_entry].is_directory && strlen(data[v3_entry].name)==1) ls=true;

						if(add_net_game(ns, data, v3_entry, neth, param, templn, tempstr, enc_dir_name, icon, tempID, f1, 0)==FAILED) {v3_entry++; continue;}

						sprintf(tempstr, "<Table key=\"%04i\">"
										 XML_PAIR("icon","%s")
										 XML_PAIR("title","%s") "%s"
										 XML_PAIR("module_action","http://%s/mount_ps3%s%s/%s?random=%x")
										 XML_PAIR("info","%s%s%s") "</Table>",
								key, icon,
								templn, WEB_LINK_PAIR, local_ip, neth, param, enc_dir_name, (u16)pTick.tick, neth, param, "");

						add_xmb_entry(param, tempstr, templn, skey[key], key, myxml_ps3, myxml_ps2, myxml_psx, myxml_psp, myxml_dvd, data[v3_entry].name, item_count); key++;

						v3_entry++;
					}
					else
 #endif
#endif

					{
						if(entry.d_name[0]=='.') continue;

/////////////////////////////////////////
						subfolder=0;
						if(IS_ISO_FOLDER && (f0<7 || f0>NTFS))
						{
							sprintf(subpath, "%s/%s", param, entry.d_name);
							if(isDir(subpath) && cellFsOpendir(subpath, &fd2) == CELL_FS_SUCCEEDED)
							{
								strcpy(subpath, entry.d_name); subfolder = 1;
next_xml_entry:
								cellFsReaddir(fd2, &entry, &read_e);
								if(read_e<1) continue;
								if(entry.d_name[0]=='.') goto next_xml_entry;
								sprintf(templn, "%s/%s", subpath, entry.d_name); strcpy(entry.d_name, templn);
							}
						}
						int flen = strlen(entry.d_name);
/////////////////////////////////////////

#ifdef COBRA_ONLY
						is_iso = (f0==NTFS && flen>13 && strstr(entry.d_name + flen - 13, ".ntfs[")!=NULL) ||
								 (IS_ISO_FOLDER && flen > 4 && (
								 (            !strncasecmp(entry.d_name + flen - 4, ".iso", 4)) ||
								 (flen > 6 && !strncasecmp(entry.d_name + flen - 6, ".iso.0", 6)) ||
								 ((IS_PS2_FOLDER) && strcasestr(".bin.img.mdf.enc", entry.d_name + flen - 4)) ||
								 ((IS_PSX_FOLDER) && strcasestr(".cue.img.mdf", entry.d_name + flen - 4))
								 ));
#else
						is_iso = (IS_PS2_FOLDER && flen > 8 && !strncmp(entry.d_name + flen - 8, ".BIN.ENC", 8));
#endif //#ifdef COBRA_ONLY
						if(!is_iso)
						{
							sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", param, entry.d_name);
						}

						if(is_iso || (f1<2 && cellFsStat(templn, &buf)==CELL_FS_SUCCEEDED))
						{
							msiz=0;
							if(!is_iso)
							{
                                get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr);
							}
							else
							{
#ifdef COBRA_ONLY
								if(f0==NTFS)
								{   // ntfs
									if(f1< 2 || f1>6) continue; //2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO"
									if((uprofile >0) && !strstr(entry.d_name, SUFIX3(uprofile))) continue;
									if((uprofile==0 && flen>17)) {for(u8 u=1;u<5;u++) if(strstr(entry.d_name + flen - 17, SUFIX3(u))) continue;}
								}

								if((strstr(param, "/PS3ISO") && f0!=NTFS) || (f0==NTFS && f1==2 && !extcmp(entry.d_name, ".ntfs[PS3ISO]", 13)))
								{
									get_name(templn, entry.d_name, 1); strcat(templn, ".SFO\0");
									if(f0!=NTFS && cellFsStat(templn, &buf)!=CELL_FS_SUCCEEDED)
									{
										get_name(tempstr, entry.d_name, 0);
										sprintf(templn, "%s/%s.SFO", param, tempstr);
									}

									get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr);
								}
								else
#endif
								{
#ifdef COBRA_ONLY
									if(f0==NTFS)
									{   // ntfs
										int flen=strlen(entry.d_name)-13; if(flen<0) continue;

										if(f1==2 && !strstr(entry.d_name+flen, ".ntfs[PS3ISO]")) continue;
										if(f1==3 && !strstr(entry.d_name+flen, ".ntfs[BD"))      continue;
										if(f1==4 && !strstr(entry.d_name+flen, ".ntfs[DVDISO]")) continue;
										if(f1==6 && !strstr(entry.d_name+flen, ".ntfs[PSXISO]")) continue;
									}
#endif
									get_name(templn, entry.d_name, 0);
								}

								if(f0!=NTFS && tempID[0]==0 && strstr(param, "/PS3ISO"))
								{   // get title ID from ISO
									sprintf(icon, "%s/%s", param, entry.d_name);
									if(cellFsOpen(icon, CELL_FS_O_RDONLY, &fs, NULL, 0) == CELL_FS_SUCCEEDED)
									{
										if(cellFsLseek(fs, 0x810, CELL_FS_SEEK_SET, &msiz) == CELL_FS_SUCCEEDED)
										{
											if(cellFsRead(fs, (void *)&tempID, 11, &msiz) == CELL_FS_SUCCEEDED)
											{
												strncpy(&tempID[4], &tempID[5], 5); tempID[9]='\0';
											}
										}
										cellFsClose(fs);
									}
									icon[0]=0;
								}
							}
		//title_foundx:
							if(!is_iso && f1<2 && (icon[0]==0 || webman_config->nocov)) sprintf(icon, "%s/%s/PS3_GAME/ICON0.PNG", param, entry.d_name);

							get_cover_from_name(icon, entry.d_name, tempID);

							if(is_iso)
							{
								if(icon[0]==0 || cellFsStat(icon, &buf)!=CELL_FS_SUCCEEDED)
								{
									sprintf(icon, "%s/%s", param, entry.d_name);

									flen = strlen(icon);
#ifdef COBRA_ONLY
									if(flen > 13 && (!extcmp(icon, ".ntfs[PS3ISO]", 13) || !extcmp(icon, ".ntfs[DVDISO]", 13) || !extcmp(icon, ".ntfs[PSXISO]", 13) || !extcmp(icon, ".ntfs[BDFILE]", 13))) {flen -= 13; icon[flen]=0;} else
									if(flen > 12 &&  !extcmp(icon, ".ntfs[BDISO]" , 12)) {flen -= 12; icon[flen]=0;}
#endif
									if(flen > 4 && icon[flen-4]=='.')
									{
										icon[flen-3]='p'; icon[flen-2]='n'; icon[flen-1]='g';
										if(cellFsStat(icon, &buf)!=CELL_FS_SUCCEEDED)
										{
											icon[flen-3]='P'; icon[flen-2]='N'; icon[flen-1]='G';
										}
									}
									else
									if(flen > 5 && icon[flen-2]=='.')
									{
										icon[flen-5]='p'; icon[flen-4]='n'; icon[flen-3]='g'; flen -= 2; icon[flen]=0;
									}

									if(cellFsStat(icon, &buf)!=CELL_FS_SUCCEEDED)
										{icon[flen-3]='j'; icon[flen-2]='p'; icon[flen-1]='g';}
								}
							}
							else if(icon[0]==0 || cellFsStat(icon, &buf)!=CELL_FS_SUCCEEDED)
								sprintf(icon, "%s/%s/PS3_GAME/ICON0.PNG", param, entry.d_name);

							get_default_icon(icon, param, entry.d_name, 0, tempID, ns, abort_connection);

							if(webman_config->tid && tempID[0]>'@' && strlen(templn) < 50 && strstr(templn, " [")==NULL) {strcat(templn, " ["); strcat(templn, tempID); strcat(templn, "]");}

							urlenc(enc_dir_name, entry.d_name);

							// subfolder name
							if(f0==NTFS && entry.d_name[0]=='[')
							{
								strcpy(folder_name, entry.d_name); folder_name[0]='/'; char *p=strchr(folder_name, ']'); if(p) p[0]=0;
							}
							else
							{
								char *p=strchr(entry.d_name, '/');
								folder_name[0]=0; if(p) {p[0]=0; sprintf(folder_name, "/%s", entry.d_name); p[0]='/';}
							}

							sprintf(tempstr, "<Table key=\"%04i\">"
											 XML_PAIR("icon","%s")
											 XML_PAIR("title","%s") "%s"
											 XML_PAIR("module_action","http://%s/mount_ps3%s%s/%s?random=%x")
											 XML_PAIR("info","%s%s%s") "</Table>",
								key, icon,
								templn, WEB_LINK_PAIR, local_ip, "", param, enc_dir_name, (u16)pTick.tick, (f0==NTFS?(char*)"/ntfs/":param), (f0==NTFS?paths[f1]:""), folder_name);

							add_xmb_entry(param, tempstr, templn, skey[key], key, myxml_ps3, myxml_ps2, myxml_psx, myxml_psp, myxml_dvd, entry.d_name, item_count); key++;
						}
//////////////////////////////
						if(subfolder) goto next_xml_entry;
//////////////////////////////
					}
				}
				if(!is_net) cellFsClosedir(fd);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(data2) sys_memory_free(data2);
 #endif
#endif
			}

//
continue_reading_folder_xml:

			if((uprofile>0) && (f1<9)) {subfolder=uprofile=0; goto read_folder_xml;}
			if(is_net && ls && li<27) {li++; goto subfolder_letter_xml;}
//
		}
		if(is_net && ns>=0) {shutdown(ns, SHUT_RDWR); socketclose(ns); ns=-2;}
	}

	if( !(webman_config->nogrp))
	{
		if(!(webman_config->cmask & PS3)) {strcat(myxml_ps3, "</Attributes><Items>");}
		if(!(webman_config->cmask & PS2)) {strcat(myxml_ps2, "</Attributes><Items>"); if(webman_config->ps2l && cellFsStat((char*)PS2_CLASSIC_PLACEHOLDER, &buf)==CELL_FS_SUCCEEDED) strcat(myxml_ps2, QUERY_XMB("ps2_classic_launcher", "xcb://127.0.0.1/query?limit=1&cond=Ae+Game:Game.titleId PS2U10000"));}

#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) {strcat(myxml_psx, "</Attributes><Items>");}
		if(!(webman_config->cmask & PSP)) {strcat(myxml_psp, "</Attributes><Items>"); if(webman_config->pspl && cellFsStat((char*)"/dev_hdd0/game/PSPC66820", &buf)==CELL_FS_SUCCEEDED) strcat(myxml_psp, QUERY_XMB("cobra_psp_launcher", "xcb://127.0.0.1/query?limit=1&cond=Ae+Game:Game.titleId PSPC66820"));}
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU)) {strcat(myxml_dvd, "</Attributes><Items>"); if(webman_config->rxvid) strcat(myxml_dvd, QUERY_XMB("rx_video", "#seg_wm_bdvd"));}
#endif
	}
	else
		myxml_dvd[0]=0;

	// --- sort scanned content

	led(YELLOW, OFF);
	led(GREEN, ON);

	if(key)
	{   // sort xmb items
		char swap[16]; u16 m, n;

		if((webman_config->nogrp))
		{
			for(n=0; n<(key-1); n++)
				for(m=(n+1); m<key; m++)
					if(strcasecmp(skey[n]+1, skey[m]+1)>0)
					{
						strcpy(swap, skey[n]);
						strcpy(skey[n], skey[m]);
						strcpy(skey[m], swap);
					}
		}
		else
			for(n=0; n<(key-1); n++)
				for(m=(n+1); m<key; m++)
					if(strcasecmp(skey[n], skey[m])>0)
					{
						strcpy(swap, skey[n]);
						strcpy(skey[n], skey[m]);
						strcpy(skey[m], swap);
					}
	}

	// --- add eject & setup/xmbm+ menu
#ifdef ENGLISH_ONLY
	bool add_xmbm_plus = cellFsStat("/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN.xml", &buf)==CELL_FS_SUCCEEDED;
#else
	bool add_xmbm_plus = false;

	while(true)
	{
		sprintf(templn, "/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN%s.xml", lang_code);
		add_xmbm_plus = cellFsStat(templn, &buf)==CELL_FS_SUCCEEDED;
		if(add_xmbm_plus || lang_code[0]==0) break; lang_code[0]=0;
	}
#endif

	if( (webman_config->nogrp))
	{
		strcat(myxml_items, ADD_XMB_ITEM("eject"));
		if(!webman_config->noset)
        {
			if(add_xmbm_plus)
#ifdef ENGLISH_ONLY
				strcat(myxml_items, QUERY_XMB("setup", "xmb://localhost/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN.xml#seg_webman_links_items"));
#else
			{
				sprintf(tempstr, QUERY_XMB("setup", "xmb://localhost%s#seg_webman_links_items"), templn);
				strcat(myxml_items, tempstr);
			}
#endif
			else
				strcat(myxml_items, ADD_XMB_ITEM("setup"));
		}
	}

	// --- add sorted items to xml

	for(u16 a=0; a<key; a++)
	{
		sprintf(templn, ADD_XMB_ITEM("%s"), skey[(a)]+5, skey[(a)]+5);
		if( !(webman_config->nogrp))
		{
#ifdef COBRA_ONLY
			if(skey[(a)][0]==PSP && strlen(myxml_psp)<(BUFFER_SIZE_PSP-128))
				strcat(myxml_psp, templn);
			else
			if(skey[(a)][0]==PS1 && strlen(myxml_psx)<(BUFFER_SIZE_PSX-128))
				strcat(myxml_psx, templn);
			else
			if(skey[(a)][0]==BLU && strlen(myxml_dvd)<(BUFFER_SIZE_DVD-1200))
				strcat(myxml_dvd, templn);
			else
#endif
			if(skey[(a)][0]==PS2 && strlen(myxml_ps2)<(BUFFER_SIZE_PS2-128))
				strcat(myxml_ps2, templn);
			else
			if(strlen(myxml_ps3)<(BUFFER_SIZE-5000))
				strcat(myxml_ps3, templn);

		}
		else
			if(strlen(myxml_dvd)<(BUFFER_SIZE-1000))
				strcat(myxml_items, templn);
	}

	// --- close xml items

	if( !(webman_config->nogrp))
	{
		if(!(webman_config->cmask & PS3)) strcat(myxml_ps3, "</Items></View>");
		if(!(webman_config->cmask & PS2)) strcat(myxml_ps2, "</Items></View>");
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) strcat(myxml_psx, "</Items></View>");
		if(!(webman_config->cmask & PSP)) strcat(myxml_psp, "</Items></View>");
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU))
		{
			strcat(myxml_dvd, "</Items></View>");
			if(webman_config->rxvid)
			{
				strcat(myxml_dvd,	"<View id=\"seg_wm_bdvd\">"
									"<Items>"
									QUERY_XMB("rx_video1", "xcb://localhost/query?table=MMS_MEDIA_TYPE_SYSTEM&genre=Video&sort=+StorageMedia:StorageMedia.sortOrder+StorageMedia:StorageMedia.timeInserted&cond=Ae+StorageMedia:StorageMedia.stat.mediaStatus %xCB_MEDIA_INSERTED+Ae+StorageMedia:StorageMedia.mediaFormat %xCB_MEDIA_FORMAT_DATA+AGL+StorageMedia:StorageMedia.type %xCB_MEDIA_TYPE_BDROM %xCB_MEDIA_TYPE_WM")
									QUERY_XMB("rx_video2", "xcb://localhost/query?sort=+Game:Common.titleForSort&cond=AGL+Game:Game.titleId RXMOV0000 RXMOVZZZZ+An+Game:Game.category 2D+An+Game:Game.category BV+An+Game:Game.category HG")
									"</Items>"
									"</View>");
			}
		}
#endif
	}

	// --- build xml headers
	memset(myxml, 0, 4300);
	sprintf(templn, "%s"
					"<View id=\"seg_mygames\">"
					"<Attributes><Table key=\"eject\">"
					XML_PAIR("icon","%s")
					XML_PAIR("title","%s")
					XML_PAIR("info","%s") "%s"
					XML_PAIR("module_action","http://%s/mount_ps3/unmount") "%s",
					XML_HEADER, wm_icons[11], STR_EJECTDISC, STR_UNMOUNTGAME, WEB_LINK_PAIR, local_ip, "</Table>"); strcpy(myxml, templn);

	if( !(webman_config->nogrp))
	{
		if( !(webman_config->cmask & PS3)) {sprintf(templn, "<Table key=\"wm_ps3\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAE\x33")
															XML_PAIR("info","%i %s") "%s",
															wm_icons[0], item_count[3], STR_PS3FORMAT, STR_NOITEM_PAIR); strcat(myxml, templn);}
		if( !(webman_config->cmask & PS2)) {sprintf(templn, "<Table key=\"wm_ps2\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAE\x32")
															XML_PAIR("info","%i %s") "%s",
															wm_icons[2], item_count[2], STR_PS2FORMAT, STR_NOITEM_PAIR); strcat(myxml, templn);}
#ifdef COBRA_ONLY
		if( !(webman_config->cmask & PS1)) {sprintf(templn, "<Table key=\"wm_psx\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAE")
															XML_PAIR("info","%i %s") "%s",
															wm_icons[1], item_count[1], STR_PS1FORMAT, STR_NOITEM_PAIR);strcat(myxml, templn);}
		if( !(webman_config->cmask & PSP)) {sprintf(templn, "<Table key=\"wm_psp\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","PLAYSTATION\xC2\xAEPORTABLE")
															XML_PAIR("info","%i %s") "%s",
															wm_icons[3], item_count[4], STR_PSPFORMAT, STR_NOITEM_PAIR);strcat(myxml, templn);}
		if( !(webman_config->cmask & DVD) ||
            !(webman_config->cmask & BLU)) {sprintf(templn, "<Table key=\"wm_dvd\">"
															XML_PAIR("icon","%s")
															XML_PAIR("title","%s")
															XML_PAIR("info","%i %s") "%s",
															wm_icons[4], STR_VIDFORMAT, item_count[0], STR_VIDEO, STR_NOITEM_PAIR); strcat(myxml, templn);}
#endif
	}

	if(!webman_config->noset)
	{
		sprintf(templn, "<Table key=\"setup\">"
						 XML_PAIR("icon","%s")
						 XML_PAIR("title","%s")
						 XML_PAIR("info","%s") "%s",
						 add_xmbm_plus ? "/dev_hdd0/game/XMBMANPLS/USRDIR/IMAGES/multiman.png" : wm_icons[10],
						 STR_WMSETUP, STR_WMSETUP2, WEB_LINK_PAIR); strcat(myxml, templn);

		if(add_xmbm_plus)
			strcat(myxml, XML_PAIR("child","segment"));
		else
			{sprintf(templn, XML_PAIR("module_action","http://%s/setup.ps3"), local_ip); strcat(myxml, templn);}

		strcat(myxml, "</Table>");
	}

	if(!(webman_config->nogrp))
	{
        strcat(myxml, "</Attributes><Items>");
		if( !(webman_config->noset) )
		{
			if(add_xmbm_plus)
#ifdef ENGLISH_ONLY
				strcat(myxml, QUERY_XMB("setup", "xmb://localhost/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN.xml#seg_webman_links_items"));
#else
			{
				sprintf(templn, QUERY_XMB("setup", "xmb://localhost/dev_hdd0/game/XMBMANPLS/USRDIR/FEATURES/webMAN%s.xml#seg_webman_links_items"), lang_code);
				strcat(myxml, templn);
			}
#endif
			else
				strcat(myxml, ADD_XMB_ITEM("setup"));
		}

		if(!add_xmbm_plus) strcat(myxml, ADD_XMB_ITEM("eject"));

		if(!(webman_config->cmask & PS3)) strcat(myxml, QUERY_XMB("wm_ps3", "#seg_wm_ps3_items"));
		if(!(webman_config->cmask & PS2)) strcat(myxml, QUERY_XMB("wm_ps2", "#seg_wm_ps2_items"));
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) strcat(myxml, QUERY_XMB("wm_psx", "#seg_wm_psx_items"));
		if(!(webman_config->cmask & PSP)) strcat(myxml, QUERY_XMB("wm_psp", "#seg_wm_psp_items"));
		if(!(webman_config->cmask & DVD) ||
		   !(webman_config->cmask & BLU)) strcat(myxml, QUERY_XMB("wm_dvd", "#seg_wm_dvd_items"));
#endif

		strcat(myxml, "</Items></View>");
	}

	// --- save xml file
	int fdxml=0;
	cellFsOpen(xml, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdxml, NULL, 0);
	cellFsWrite(fdxml, (char*)myxml, strlen(myxml), NULL);

	if( (webman_config->nogrp))
	{
		cellFsWrite(fdxml, (char*)myxml_ps3, strlen(myxml_ps3), NULL);
		cellFsWrite(fdxml, (char*)"</Attributes><Items>", 20, NULL);
		cellFsWrite(fdxml, (char*)myxml_items, strlen(myxml_items), NULL);
		sprintf(myxml, "</Items></View></XMBML>\r\n");
	}
	else
	{
		if(!(webman_config->cmask & PS3)) cellFsWrite(fdxml, (char*)myxml_ps3, strlen(myxml_ps3), NULL);
		if(!(webman_config->cmask & PS2)) cellFsWrite(fdxml, (char*)myxml_ps2, strlen(myxml_ps2), NULL);
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS1)) cellFsWrite(fdxml, (char*)myxml_psx, strlen(myxml_psx), NULL);
		if(!(webman_config->cmask & PSP)) cellFsWrite(fdxml, (char*)myxml_psp, strlen(myxml_psp), NULL);
		if(!(webman_config->cmask & DVD) || !(webman_config->cmask & BLU)) cellFsWrite(fdxml, (char*)myxml_dvd, strlen(myxml_dvd), NULL);
#endif
		sprintf(myxml, "</XMBML>\r\n");
	}

	cellFsWrite(fdxml, (char*)myxml, strlen(myxml), NULL);
	cellFsClose(fdxml);
	cellFsChmod(xml, MODE);

	// --- replace & with ^ for droidMAN

	if(cellFsOpen(xml, CELL_FS_O_RDONLY, &fdxml, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		u64 read_e = 0;
		u32 xmlsize=BUFFER_SIZE;
		cellFsRead(fdxml, (void *)myxml_ps3, xmlsize, &read_e);
		cellFsClose(fdxml);
		for(u32 n=0;n<read_e;n++) if(myxml_ps3[n]=='&') myxml_ps3[n]='^';

		strcpy(xml+37, ".droid\0"); // .xml -> .droid
		cellFsOpen((char*)xml, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fdxml, NULL, 0);
		cellFsWrite(fdxml, (char*)myxml_ps3, strlen(myxml_ps3), NULL);
		cellFsClose(fdxml);
	}

	// --- release allocated memory

	led(GREEN, ON);

#ifdef USE_VM
	sys_vm_unmap(sysmem);
#else
	sys_memory_free(sysmem);
#endif

	return true;
}

static void get_value(char *text, char *url, u16 size)
{
	u16 n;
	for(n=0;n<size;n++)
	{
		if(url[n]=='&' || url[n]==0) break;
		if(url[n]=='+') url[n]=' ';
		text[n]=url[n];
	}
	text[n]=0;
}

#ifndef LITE_EDITION
#ifdef WEB_CHAT
static void webchat(char *buffer, char *templn, char *param, char *tempstr, sys_net_sockinfo_t conn_info_main)
{
	struct CellFsStat buf;

	int fd; uint64_t msiz = 0; int size = 0;

	// truncate msg log
	if(cellFsStat((char*)WMCHATFILE, &buf)!=CELL_FS_SUCCEEDED || buf.st_size>0x8000UL || buf.st_size==0)
	{
		memset(tempstr, 0, _4KB_);

		if(buf.st_size>0x8000UL)
		{
			if(cellFsOpen((char*)WMCHATFILE, CELL_FS_O_RDONLY, &fd, NULL, 0)==CELL_FS_SUCCEEDED)
			{
				cellFsLseek(fd, (buf.st_size-4080), CELL_FS_SEEK_SET, &msiz);
				cellFsRead(fd, (void *)&tempstr, 4080, &msiz);
				cellFsClose(fd);
			}
		}

		cellFsUnlink((char*)WMCHATFILE);

		if(cellFsOpen((char*)WMCHATFILE, CELL_FS_O_RDWR|CELL_FS_O_CREAT|CELL_FS_O_APPEND, &fd, NULL, 0) == CELL_OK)
		{
			strcpy(templn,	"<meta http-equiv=\"refresh\" content=\"10\">"
							"<body bgcolor=\"#101010\" text=\"#c0c0c0\">"
							"<script>window.onload=toBottom;function toBottom(){window.scrollTo(0, document.body.scrollHeight);}</script>\0");
			if(tempstr[0]) strcat(templn, "<!--");
			size = strlen(templn);
			cellFsWrite(fd, templn, size, &msiz);
			size = strlen(templn);
            if(size) cellFsWrite(fd, tempstr, size, &msiz);
		}
		cellFsClose(fd);
    }

	// append msg
	char msg[200]="", user[20]="guest\0"; char *pos;
	if(conn_info_main.remote_adr.s_addr==0x7F000001) strcpy(user,"console\0");
	if(strstr(param, "/chat.ps3?"))
	{
		pos=strstr(param, "u="); if(pos) get_value(user, pos+2, 20);
		pos=strstr(param, "m="); if(pos) get_value(msg , pos+2, 200);

		sprintf(templn, "<font color=\"red%s\"><b>%s</b></font><br>%s<br><!---->", user, user, msg);

		if(cellFsOpen((char*)WMCHATFILE, CELL_FS_O_RDWR|CELL_FS_O_CREAT|CELL_FS_O_APPEND, &fd, NULL, 0) == CELL_OK)
		{
		    size = strlen(templn);
		    cellFsWrite(fd, templn, size, &msiz);
		}
		cellFsClose(fd);

		if(conn_info_main.remote_adr.s_addr!=0x7F000001) show_msg((char*)(msg));
	}

	// show msg log
	sprintf(templn, "<iframe src=\"%s\" width=\"99%%\" height=\"300\"></iframe>", WMCHATFILE); strcat(buffer, templn);

	// prompt msg
	sprintf(templn, "<hr>"
					"<form name=\"f\" action=\"\">"
					HTML_INPUT("u", "%s", "10", "5") ":" HTML_INPUT("m", "", "500", "80")
					"<input type=submit value=\"send\">"
					"</form><script>f.m.focus();</script>", user); strcat(buffer, templn);
}
#endif
#endif

static void cpu_rsx_stats(char *buffer, char *templn, char *param)
{
	u32 t1=0, t2=0, t1f=0, t2f=0;
	get_temperature(0, &t1); // 3E030000 -> 3E.03°C -> 62.(03/256)°C
	get_temperature(1, &t2);
	t1=t1>>24;
	t2=t2>>24;
	t1f=(1.8f*(float)t1+32.f);
	t2f=(1.8f*(float)t2+32.f);

	_meminfo meminfo;
	{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}

	if(!webman_config->fanc && (get_fan_policy_offset>0))
	{
		u8 st, mode, unknown;
		backup[5]=peekq(get_fan_policy_offset);
		lv2poke32(get_fan_policy_offset, 0x38600001); // sys 409 get_fan_policy
		sys_sm_get_fan_policy(0, &st, &mode, &fan_speed, &unknown);
		pokeq(get_fan_policy_offset, backup[5]);
	}

	uint64_t eid0_idps[2], buffr[0x40], start_sector;
	uint32_t read;
	sys_device_handle_t source;
	if(sys_storage_open(0x100000000000004ULL, 0, &source, 0)!=0)
	{
		start_sector = 0x204;
		sys_storage_close(source);
		sys_storage_open(0x100000000000001ULL, 0, &source, 0);
	}
	else start_sector = 0x178;
	sys_storage_read(source, 0, start_sector, 1, buffr, &read, 0);
	sys_storage_close(source);

	eid0_idps[0]=buffr[0x0E];
	eid0_idps[1]=buffr[0x0F];

	get_idps_psid();

	uint32_t blockSize;
	uint64_t freeSize;
	cellFsGetFreeSize((char*)"/dev_hdd0", &blockSize, &freeSize);

	sprintf(templn, " [<a href=\"/shutdown.ps3\">%s</a>] [<a href=\"/restart.ps3\">%s</a>]", STR_SHUTDOWN, STR_RESTART ); strcat(buffer, templn);

	if(View_Find("game_plugin"))
	{
#ifdef VIDEO_REC
		sprintf(templn, " [<a href=\"/videorec.ps3\">REC</a>]<hr><H2><a href=\"%s", search_url); strcat(buffer, templn);
#else
		sprintf(templn, "<hr><H2><a href=\"%s", search_url); strcat(buffer, templn);
#endif
		get_game_info(); sprintf(templn, "%s\">%s %s</a></H2>", _game_Title, _game_TitleID, _game_Title); strcat(buffer, templn);
	}

	if(strstr(param, "?"))
	{

		if(strstr(param, "?m")) {if(max_temp) max_temp=0; else max_temp=webman_config->temp1;}

		if(max_temp) //auto mode
		{
			if(strstr(param, "?u")) max_temp++;
			if(strstr(param, "?d")) max_temp--;
			webman_config->temp1=RANGE(max_temp, 40, 85); //°C
		}
		else
		{
			if(strstr(param, "?u")) webman_config->manu++;
			if(strstr(param, "?d")) webman_config->manu--;
			webman_config->manu=RANGE(webman_config->manu, 20, 99); //%

			webman_config->temp0= (u8)(((float)webman_config->manu * 255.f)/100.f);
			webman_config->temp0=RANGE(webman_config->temp0, 0x33, MAX_FANSPEED);
			fan_control(webman_config->temp0, 0);
		}
		save_settings();
	}

	char max_temp1[50], max_temp2[50]; max_temp2[0]=0;

	if(!webman_config->fanc || (!webman_config->temp0 && !max_temp))
		sprintf(max_temp1, " <small>[%s %s]</small>", STR_FANCTRL3, STR_DISABLED);
	else if(max_temp)
	{
		sprintf(max_temp1, " (MAX: %i°C)", max_temp);
		sprintf(max_temp2, " (MAX: %i°F)", (int)(1.8f*(float)max_temp+32.f));
	}
	else
		sprintf(max_temp1, " <small>[FAN: %i%% %s]</small>", webman_config->manu, STR_MANUAL);

	sprintf( templn, "<hr><font size=\"42px\"><b><a class=\"s\" href=\"/cpursx.ps3?up\">"
											"CPU: %i°C%s<br>"
											"RSX: %i°C</a><hr>"
											"<a class=\"s\" href=\"/cpursx.ps3?dn\">"
											"CPU: %i°F%s<br>"
											"RSX: %i°F</a><hr>"
											"<a class=\"s\" href=\"/games.ps3\">"
											"MEM: %iKB<br>"
											"HDD: %i %s</a><hr>"
											"<a class=\"s\" href=\"/cpursx.ps3?mode\">"
											"FAN SPEED: %i%% (0x%X)</a><hr>",
					t1, max_temp1, t2,
					t1f, max_temp2, t2f,
					(meminfo.avail>>10), (int)((blockSize*freeSize)>>20), STR_MBFREE,
					(int)((int)fan_speed*100)/255, fan_speed); strcat(buffer, templn);

	CellRtcTick pTick; cellRtcGetCurrentTick(&pTick); u32 dd, hh, mm, ss;

	// detect aprox. time when a game is launched
	if(View_Find("game_plugin")==0) gTick=rTick; else if(gTick.tick==rTick.tick) cellRtcGetCurrentTick(&gTick);

	////// play time //////
	if(gTick.tick>rTick.tick)
	{
		ss = (u32)((pTick.tick-gTick.tick)/1000000);
		dd = (u32)(ss / 86400); ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;
		if(dd<100) {sprintf( templn, "<label title=\"Play\">&#9737;</label> %id %02d:%02d:%02d<br>", dd, hh, mm, ss); strcat(buffer, templn);}
	}
	///////////////////////

	//// startup time /////
	ss = (u32)((pTick.tick-rTick.tick)/1000000);
	dd = (u32)(ss / 86400); ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;
	sprintf( templn, "<label title=\"Startup\">&#8986;</label> %id %02d:%02d:%02d", dd, hh, mm, ss); strcat(buffer, templn);
	///////////////////////

	// Get mac address [0xD-0x12]
	u8 mac_address[0x13];
	{system_call_3(SYS_NET_EURUS_POST_COMMAND, CMD_GET_MAC_ADDRESS, (u64)(u32)mac_address, 0x13);}

#ifdef COBRA_ONLY
    #define SYSCALL8_OPCODE_GET_MAMBA  0x7FFFULL
    bool is_mamba; {system_call_1(8, SYSCALL8_OPCODE_GET_MAMBA); is_mamba = ((int)p1 ==0x666);}

    uint16_t cobra_version; sys_get_version2(&cobra_version);
    sprintf(param, "%s %s: %X.%X", dex_mode ? "DEX" : "CEX", is_mamba ? "Mamba" : "Cobra", cobra_version>>8, (cobra_version & 0xF) ? (cobra_version & 0xFF) : ((cobra_version>>4) & 0xF));
#else
    sprintf(param, "%s", dex_mode ? "DEX" : "CEX");
#endif

	sprintf( templn, "<hr></font><h2><a class=\"s\" href=\"/setup.ps3\">"
						"Firmware : %i.%02i %s<br>"
						"PSID LV2 : %016llX%016llX<hr>"
						"IDPS EID0: %016llX%016llX<br>"
						"IDPS LV2 : %016llX%016llX<br>"
						"MAC Addr : %02X:%02X:%02X:%02X:%02X:%02X</h2></a></b>",
					(int)c_firmware, ((u32)(c_firmware * 1000.0f) % 1000) / 10, param,
					PSID[0], PSID[1],
					eid0_idps[0], eid0_idps[1],
					IDPS[0], IDPS[1],
					mac_address[13], mac_address[14], mac_address[15], mac_address[16], mac_address[17], mac_address[18]); strcat(buffer, templn);

	/////////////////////////////
#ifdef COPY_PS3
	if(copy_in_progress)
	{
		sprintf( templn, "<hr>%s %s (%i %s)", STR_COPYING, current_file, copied_count, STR_FILES); strcat(buffer, templn);
	}
	else
	if(fix_in_progress)
	{
		strcat(buffer, "<hr>"); sprintf( templn, "%s %s", STR_FIXING, current_file); strcat(buffer, templn);
	}
#endif
	/////////////////////////////
}

static void setup_parse_settings(char *param)
{
	memset(webman_config, 0, sizeof(WebmanCfg));

	if(strstr(param, "u0=1")) webman_config->usb0=1;
	if(strstr(param, "u1=1")) webman_config->usb1=1;
	if(strstr(param, "u2=1")) webman_config->usb2=1;
	if(strstr(param, "u3=1")) webman_config->usb3=1;
	if(strstr(param, "u6=1")) webman_config->usb6=1;
	if(strstr(param, "u7=1")) webman_config->usb7=1;

	if(strstr(param, "x0=1")) webman_config->dev_sd=1;
	if(strstr(param, "x1=1")) webman_config->dev_ms=1;
	if(strstr(param, "x2=1")) webman_config->dev_cf=1;

	if(strstr(param, "lp=1")) webman_config->lastp=1;
	if(strstr(param, "ab=1")) webman_config->autob=1;
	if(strstr(param, "dy=1")) webman_config->delay=1;

	//Wait for any USB device to be ready
	if(strstr(param, "b=5"))  webman_config->bootd=5;
	if(strstr(param, "b=9"))  webman_config->bootd=9;

	//Wait additionally for each selected USB device to be ready
	if(strstr(param, "s=3"))  webman_config->boots=3;
	if(strstr(param, "s=5"))  webman_config->boots=5;
	if(strstr(param, "s=10")) webman_config->boots=10;

	if(strstr(param, "bl=1")) webman_config->blind=1;
	if(webman_config->blind)
		enable_dev_blind(NULL);
	else
		{system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}

	if(strstr(param, "ns=1")) webman_config->noset=1;
	if(strstr(param, "ng=1")) webman_config->nogrp=1;
#ifdef NOSINGSTAR
	if(strstr(param, "ss=1")) {webman_config->noss=1; no_singstar_icon();}
#endif

#ifdef COBRA_ONLY
	webman_config->cmask=0;
	if(!strstr(param, "ps1=1")) webman_config->cmask|=PS1;
	if(!strstr(param, "psp=1")) webman_config->cmask|=PSP;
	if(!strstr(param, "blu=1")) webman_config->cmask|=BLU;
	if(!strstr(param, "dvd=1")) webman_config->cmask|=DVD;
#else
	webman_config->cmask=(PSP | PS1 | BLU | DVD);
#endif
	if(!strstr(param, "ps3=1")) webman_config->cmask|=PS3;
	if(!strstr(param, "ps2=1")) webman_config->cmask|=PS2;

	if(strstr(param, "psl=1")) webman_config->pspl=1;
	if(strstr(param, "p2l=1")) webman_config->ps2l=1;
	if(strstr(param, "rxv=1")) webman_config->rxvid=1;
	if(strstr(param, "pse=1")) webman_config->ps1emu=1;

	webman_config->combo=webman_config->combo2=0;
	if(!strstr(param, "pfs=1")) webman_config->combo|=FAIL_SAFE;
	if(!strstr(param, "pss=1")) webman_config->combo|=SHOW_TEMP;
	if(!strstr(param, "ppv=1")) webman_config->combo|=PREV_GAME;
	if(!strstr(param, "pnx=1")) webman_config->combo|=NEXT_GAME;
	if(!strstr(param, "psd=1")) webman_config->combo|=SHUT_DOWN;
	if(!strstr(param, "pid=1")) webman_config->combo|=SHOW_IDPS;
	if(!strstr(param, "prs=1")) webman_config->combo|=RESTARTPS;
	if(!strstr(param, "puw=1")) webman_config->combo|=UNLOAD_WM;
	if(!strstr(param, "pf1=1")) webman_config->combo|=MANUALFAN;
	if(!strstr(param, "pf2=1")) webman_config->combo|=MINDYNFAN;
	if(!strstr(param, "pdf=1")) webman_config->combo|=DISABLEFC;
	if(!strstr(param, "psc=1")) webman_config->combo|=DISABLESH;
#ifdef COBRA_ONLY
	if(!strstr(param, "pdc=1")) webman_config->combo|=DISACOBRA;

	if(strstr(param, "bus=1")) webman_config->bus=1;
#endif
#ifdef REX_ONLY
	if(!strstr(param, "pr0=1")) webman_config->combo2|=REBUGMODE;
	if(!strstr(param, "pr1=1")) webman_config->combo2|=NORMAMODE;
	if(!strstr(param, "pr2=1")) webman_config->combo2|=DEBUGMENU;

	if(!strstr(param, "p2c=1")) webman_config->combo2|=PS2TOGGLE;
#endif
	if(!strstr(param, "p2s=1")) webman_config->combo2|=PS2SWITCH;
	if(!strstr(param, "pgd=1")) webman_config->combo2|=EXTGAMDAT;
#ifndef LITE_EDITION
	if(!strstr(param, "pn0=1")) webman_config->combo2|=MOUNTNET0;
	if(!strstr(param, "pn1=1")) webman_config->combo2|=MOUNTNET1;
#endif
	if(!strstr(param, "psv=1")) webman_config->combo2|=BLOCKSVRS;
	if(!strstr(param, "pxr=1")) webman_config->combo2|=XMLREFRSH;
	if(!strstr(param, "umt=1")) webman_config->combo2|=UMNT_GAME;
#ifdef VIDEO_REC
	if(!strstr(param, "vrc=1")) webman_config->combo2|=VIDRECORD;
#endif
	if(strstr(param, "wmdn")) webman_config->wmdn=1;
	if(strstr(param, "tid=1")) webman_config->tid=1;
	if(strstr(param, "pl=1")) webman_config->poll=1;
	if(strstr(param, "ft=1")) webman_config->ftpd=1;
	if(strstr(param, "np=1")) webman_config->nopad=1;
	if(strstr(param, "nc=1")) webman_config->nocov=1;

#ifdef FIX_GAME
	if(strstr(param, "fm=0")) webman_config->fixgame=FIX_GAME_AUTO;
	if(strstr(param, "fm=1")) webman_config->fixgame=FIX_GAME_QUICK;
	if(strstr(param, "fm=2")) webman_config->fixgame=FIX_GAME_FORCED;
	if(strstr(param, "nf=3")) webman_config->fixgame=FIX_GAME_DISABLED;
#endif

	if(strstr(param, "nsp=1")) webman_config->nospoof=1; //don't spoof fw version
    if(c_firmware==4.53f || c_firmware>=4.65f) webman_config->nospoof=1;

	if(strstr(param, "fc=1")) webman_config->fanc=1;

	webman_config->temp1=MY_TEMP;

	char *pos; char mytemp[8];

	webman_config->minfan=DEFAULT_MIN_FANSPEED;
	pos=strstr(param, "mfan=");
	if(pos)
	{
		get_value(mytemp, pos + 5, 2);

		webman_config->minfan=val(mytemp);
	}
	webman_config->minfan=RANGE(webman_config->minfan, MIN_FANSPEED, 99); //%

	webman_config->bind=0;
	if(strstr(param, "bind")) webman_config->bind=1;

	webman_config->refr=0;
	if(strstr(param, "refr")) webman_config->refr=1;

	webman_config->manu=35;
	webman_config->temp0=0;

	pos=strstr(param, "step=");
	if(pos)
	{

		get_value(mytemp, pos + 5, 2);

		webman_config->temp1=val(mytemp);
	}
	webman_config->temp1=RANGE(webman_config->temp1, 40, 83); //°C

	pos=strstr(param, "fsp0=");
	if(pos)
	{
		get_value(mytemp, pos + 5, 2);

		webman_config->ps2temp=val(mytemp);
	}
	webman_config->ps2temp=RANGE(webman_config->ps2temp, 20, 99); //%

	pos=strstr(param, "manu=");
	if(pos)
	{
		get_value(mytemp, pos + 5, 2);

		webman_config->manu=val(mytemp);
	}
	webman_config->manu=RANGE(webman_config->manu, 20, 99); //%

	if(strstr(param, "temp=1"))
		webman_config->temp0= (u8)(((float)webman_config->manu * 255.f)/100.f);
	else
		webman_config->temp0=0;

	max_temp=0;
	if(webman_config->fanc)
	{
		if(webman_config->temp0==0) max_temp=webman_config->temp1;
		fan_control(webman_config->temp0, 0);
	}
	else
		restore_fan(0); //restore syscon fan control mode

	webman_config->warn=0;
	if(strstr(param, "warn=1")) webman_config->warn=1;

	webman_config->foot=0;                           //STANDARD
	if(strstr(param, "fp=1")) webman_config->foot=1; //MIN
	if(strstr(param, "fp=2")) webman_config->foot=2; //MAX
	if(strstr(param, "fp=3")) webman_config->foot=3; //MIN+

	webman_config->spp=0;
#ifdef COBRA_ONLY
	#ifdef REMOVE_SYSCALLS
	if(strstr(param, "spp=1"))  webman_config->spp|=1;  //remove syscalls & history
	#endif
	if(strstr(param, "shh=1"))  webman_config->spp|=2;  //remove history only
#endif

#ifdef SPOOF_CONSOLEID
	if(strstr(param, "id1=1"))  webman_config->sidps=1; //spoof IDPS
	if(strstr(param, "id2=1"))  webman_config->spsid=1; //spoof PSID

	pos=strstr(param, "vID1=");
	if(pos) get_value(webman_config->vIDPS1, pos + 5, 16);

	pos=strstr(param, "vID2=");
	if(pos) get_value(webman_config->vIDPS2, pos + 5, 16);

	pos=strstr(param, "vPS1=");
	if(pos) get_value(webman_config->vPSID1, pos + 5, 16);

	pos=strstr(param, "vPS2=");
	if(pos) get_value(webman_config->vPSID2, pos + 5, 16);

	spoof_idps_psid();
#endif

	webman_config->lang=0; //English

#ifndef ENGLISH_ONLY
	if(strstr(param, "&l=99")) webman_config->lang=99; // Unknown LANG_XX.TXT
	else
	for(uint8_t i=22; i>0; i--)
	{
		sprintf(mytemp, "&l=%i", i); if(strstr(param, mytemp)) {webman_config->lang=i; break;}
	}

	update_language();
#endif
	webman_config->neth1[0] = webman_config->neth2[0] = webman_config->neth0[0]=0;
	webman_config->netp1    = webman_config->netp2    = webman_config->netp0=38008;
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
	{
		if(strstr(param, "nd0="))  webman_config->netd0=1;
		if(strstr(param, "nd1="))  webman_config->netd1=1;
		if(strstr(param, "nd2="))  webman_config->netd2=1;

		char netp[7];
		pos=strstr(param, "neth0=");
		if(pos)
		{
			get_value(webman_config->neth0, pos + 6, 16);

			pos=strstr(param, "netp0=");
			if(pos)
			{
				get_value(netp, pos + 6, 6);
				webman_config->netp0=val(netp);
			}
		}

		pos=strstr(param, "neth1=");
		if(pos)
		{
			get_value(webman_config->neth1, pos + 6, 16);

			pos=strstr(param, "netp1=");
			if(pos)
			{
				get_value(netp, pos + 6, 6);
				webman_config->netp1=val(netp);
			}
		}

		pos=strstr(param, "neth2=");
		if(pos)
		{
			get_value(webman_config->neth2, pos + 6, 16);

			pos=strstr(param, "netp2=");
			if(pos)
			{
				get_value(netp, pos + 6, 6);
				webman_config->netp2=val(netp);
			}
		}

		pos=strstr(param, "aip=");
		if(pos) get_value(webman_config->allow_ip, pos + 4, 16);
	}
 #endif
#endif
	pos=strstr(param, "autop=");
	if(pos) get_value(webman_config->autoboot_path, pos + 6, 255);
	if(strlen(webman_config->autoboot_path)==0) strcpy(webman_config->autoboot_path, DEFAULT_AUTOBOOT_PATH);

	pos=strstr(param, "uacc=");
	if(pos) get_value(webman_config->uaccount, pos + 5, 8);

	if(strstr(param, "hm=")) webman_config->homeb=1;

	pos=strstr(param, "hurl=");
	if(pos) get_value(webman_config->home_url, pos + 5, 255);

#ifdef COBRA_ONLY
#ifdef BDVD_REGION
		//if(cobra_mode)
		{
			u8 cconfig[15]; char region[2];
			CobraConfig *cobra_config = (CobraConfig*) cconfig;
			memset(cobra_config, 0, 15);
			cobra_read_config(cobra_config);

			pos=strstr(param, "bdr="); //BD Region
			if(pos)
			{
				get_value(region, pos + 4, 1);
				cobra_config->bd_video_region=val(region);
			}

			pos=strstr(param, "dvr="); //DVD Region
			if(pos)
			{
				get_value(region, pos + 4, 1);
				cobra_config->dvd_video_region=val(region);
			}

			cobra_write_config(cobra_config);

		}
#endif
#endif
}

static void setup_form(char *buffer, char *templn)
{
	struct CellFsStat buf;

	sprintf(templn, "<form action=\"/setup.ps3\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\">"
					"<table width=\"820\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
					"<tr><td width=\"250\"><u>%s:</u><br>", STR_SCAN1); strcat(buffer, templn);

	//Scan these devices
	add_check_box("u0", "1", drives[1], NULL, (webman_config->usb0), buffer);
	add_check_box("u1", "1", drives[2], NULL, (webman_config->usb1), buffer);
	add_check_box("u2", "1", drives[3], NULL, (webman_config->usb2), buffer);
	add_check_box("u3", "1", drives[4], NULL, (webman_config->usb3), buffer);
	add_check_box("u6", "1", drives[5], NULL, (webman_config->usb6), buffer);
	add_check_box("u7", "1", drives[6], NULL, (webman_config->usb7), buffer);

	if(cellFsStat(drives[11], &buf)==CELL_FS_SUCCEEDED) add_check_box("x0", "1", drives[11], NULL, (webman_config->dev_sd), buffer);
	if(cellFsStat(drives[12], &buf)==CELL_FS_SUCCEEDED) add_check_box("x1", "1", drives[12], NULL, (webman_config->dev_ms), buffer);
	if(cellFsStat(drives[13], &buf)==CELL_FS_SUCCEEDED) add_check_box("x2", "1", drives[13], NULL, (webman_config->dev_cf), buffer);

	//Scan for content
	sprintf(templn, "<td nowrap valign=top><u>%s:</u><br>", STR_SCAN2); strcat(buffer, templn);

	add_check_box("ps3", "1", "PLAYSTATION\xC2\xAE\x33"    , NULL     , !(webman_config->cmask & PS3), buffer);
	add_check_box("ps2", "1", "PLAYSTATION\xC2\xAE\x32"    , " ("     , !(webman_config->cmask & PS2), buffer);
	add_check_box("p2l", "1", STR_PS2L                     , ")<br>"  ,  (webman_config->ps2l)       , buffer);
#ifdef COBRA_ONLY
	add_check_box("ps1", "1", "PLAYSTATION\xC2\xAE&nbsp;"  , " ("     , !(webman_config->cmask & PS1), buffer);
	add_check_box("pse", "1", "ps1_netemu"                 , ")<br>"  ,  (webman_config->ps1emu)     , buffer);

    add_check_box("psp", "1", "PLAYSTATION\xC2\xAEPORTABLE", " ("     , !(webman_config->cmask & PSP), buffer);
    add_check_box("psl", "1", STR_PSPL                     , ")<br>"  ,  (webman_config->pspl)       , buffer);
	add_check_box("blu", "1", "Blu-ray\xE2\x84\xA2"        , " ("     , !(webman_config->cmask & BLU), buffer);
	add_check_box("rxv", "1", STR_RXVID                    , ")<br>"  ,  (webman_config->rxvid)      , buffer);

	add_check_box("dvd", "1", "DVD "                       , STR_VIDLG, !(webman_config->cmask & DVD), buffer);
#endif

	//general settings
	strcat(buffer, "</td></tr></table><hr color=\"#0099FF\"/>");

	add_check_box("lp", "1", STR_LPG    , NULL, (webman_config->lastp), buffer);
	add_check_box("ab", "1", STR_AUTOB  , NULL, (webman_config->autob), buffer);
	add_check_box("dy", "1", STR_DELAYAB, NULL, (webman_config->delay), buffer);

	add_check_box("bl", "1"   ,  STR_DEVBL,    NULL, (webman_config->blind), buffer);
	add_check_box("wn", "wmdn",  STR_NOWMDN,   NULL, (webman_config->wmdn) , buffer);
	add_check_box("rf", "refr",  STR_CONTSCAN, NULL, (webman_config->refr) , buffer);

	add_check_box("pl", "1", STR_USBPOLL,  NULL, (webman_config->poll) , buffer);
	add_check_box("ft", "1", STR_FTPSVC,   NULL, (webman_config->ftpd) , buffer);
	add_check_box("np", "1", STR_COMBOS,   NULL, (webman_config->nopad), buffer);

#ifdef LITE_EDITION
	add_check_box("ip", "bind",  STR_ACCESS,   NULL, (webman_config->bind) , buffer);
#else
	add_check_box("ip", "bind",  STR_ACCESS,  " : ", (webman_config->bind) , buffer);
	sprintf(templn, HTML_INPUT("aip", "%s", "15", "16") "<br>", webman_config->allow_ip); strcat(buffer, templn);
#endif

#ifdef COBRA_ONLY
	if((c_firmware!=4.53f && c_firmware<4.65f))
		add_check_box("nsp", "1", STR_NOSPOOF, NULL, (webman_config->nospoof), buffer);
#endif

#ifdef NOSINGSTAR
	add_check_box("ss", "1", STR_NOSINGSTAR,   NULL, (webman_config->noss), buffer);
#endif

	//game listing
	strcat(buffer, "<hr color=\"#0099FF\"/>");
	add_check_box("ng" , "1", STR_NOGRP, NULL, (webman_config->nogrp), buffer);
	add_check_box("ns" , "1", STR_NOSETUP,  NULL, (webman_config->noset), buffer);
	add_check_box("nc" , "1", STR_MMCOVERS, NULL, (webman_config->nocov), buffer);
	add_check_box("tid", "1", STR_TITLEID, NULL, (webman_config->tid), buffer);

	//game mounting
#ifdef COBRA_ONLY
	add_check_box("bus", "1", STR_RESET_USB  , NULL, (webman_config->bus), buffer);
#endif

#ifdef FIX_GAME
	if(c_firmware>=4.20f && c_firmware<4.70f)
	{
		add_check_box("nf", "3", STR_FIXGAME,  " : <select name=\"fm\">", (webman_config->fixgame==FIX_GAME_DISABLED), buffer);
		add_option_item("0", "Auto"  , (webman_config->fixgame==FIX_GAME_AUTO) , buffer);
		add_option_item("1", "Quick" , (webman_config->fixgame==FIX_GAME_QUICK) , buffer);
		add_option_item("2", "Forced", (webman_config->fixgame==FIX_GAME_FORCED) , buffer);
		strcat(buffer, "</select><br>");
	}
#endif

	//fan control settings
	strcat(buffer, "<hr color=\"#0099FF\"/><table width=\"900\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr class=\"propfont\"><td>");

	add_check_box("fc"  , "1", STR_FANCTRL, " </td><td>", (webman_config->fanc), buffer);
	add_check_box("warn", "1" , STR_NOWARN, " </td></tr>", (webman_config->warn), buffer);

	strcat(buffer, "<tr class=\"propfont\"><td>");
	add_radio_button("temp", "0", "t_0", STR_AUTOAT , " : ", (webman_config->temp0==0), buffer);
	sprintf(templn, HTML_INPUT("step", "%i", "2", "3") " °C</td><td><label><input type=\"checkbox\"%s/> %s</label> : " HTML_INPUT("mfan", "%i", "2", "3") " %% %s </td></tr>", webman_config->temp1, (webman_config->fanc && webman_config->temp0==0)?ITEM_CHECKED:"", STR_LOWEST, webman_config->minfan, STR_FANSPEED); strcat(buffer, templn);

	strcat(buffer, "<tr class=\"propfont\"><td>");
	add_radio_button("temp", "1", "t_1", STR_MANUAL , " : ", (webman_config->temp0!=0), buffer);
	sprintf(templn, HTML_INPUT("manu", "%i", "2", "3") " %% %s </td><td> %s : " HTML_INPUT("fsp0", "%i", "2", "3") " %% %s </td></tr></table>", (webman_config->manu), STR_FANSPEED, STR_PS2EMU, webman_config->ps2temp, STR_FANSPEED); strcat(buffer, templn);

#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
	//ps3netsvr settings
	strcat(buffer, "<hr color=\"#0099FF\"/>");
	add_check_box("nd0", "1", STR_LANGAMES,  " &nbsp; PS3NETSRV#1 IP:", (webman_config->netd0), buffer);
	sprintf(templn, HTML_INPUT("neth0", "%s", "15", "16") ":" HTML_INPUT("netp0", "%i", "5", "6") "<br>", webman_config->neth0, webman_config->netp0); strcat(buffer, templn);
	add_check_box("nd1", "1", STR_LANGAMES,  " &nbsp; PS3NETSRV#2 IP:", (webman_config->netd1), buffer);
	sprintf(templn, HTML_INPUT("neth1", "%s", "15", "16") ":" HTML_INPUT("netp1", "%i", "5", "6") "<br>", webman_config->neth1, webman_config->netp1); strcat(buffer, templn);
	add_check_box("nd2", "1", STR_LANGAMES,  " &nbsp; PS3NETSRV#3 IP:", (webman_config->netd2), buffer);
	sprintf(templn, HTML_INPUT("neth2", "%s", "15", "16") ":" HTML_INPUT("netp2", "%i", "5", "6")       , webman_config->neth2, webman_config->netp2); strcat(buffer, templn);
 #endif
#endif

	//Wait for any USB device to be ready
	sprintf(templn, "<hr color=\"#0099FF\"/><u> %s:</u><br>", STR_ANYUSB); strcat(buffer, templn);

	add_radio_button("b", "0", "b_0", "0 sec" , NULL, (webman_config->bootd==0), buffer);
	add_radio_button("b", "5", "b_1", "5 sec" , NULL, (webman_config->bootd==5), buffer);
	add_radio_button("b", "9", "b_2", "10 sec", NULL, (webman_config->bootd==9), buffer);

	//Wait additionally for each selected USB device to be ready
	sprintf(templn, "<hr color=\"#0099FF\"/><u> %s:</u><br>", STR_ADDUSB); strcat(buffer, templn);

	add_radio_button("s", "0",  "s_0", "0 sec" , NULL, (webman_config->boots==0), buffer);
	add_radio_button("s", "3",  "s_1", "3 sec" , NULL, (webman_config->boots==3), buffer);
	add_radio_button("s", "5",  "s_2", "5 sec",  NULL, (webman_config->boots==5), buffer);
	add_radio_button("s", "10", "s_3", "10 sec", NULL, (webman_config->boots==10), buffer);

#ifdef SPOOF_CONSOLEID
	//Change idps and psid in lv2 memory at system startup
	sprintf(templn, "<hr color=\"#0099FF\"/><u> %s:</u><br>", STR_SPOOFID); strcat(buffer, templn);

    if(!webman_config->vIDPS1[0] && !webman_config->vIDPS1[1]) {get_idps_psid(); sprintf(webman_config->vIDPS1, "%016llX", IDPS[0]); sprintf(webman_config->vIDPS2, "%016llX", IDPS[1]);}
    if(!webman_config->vPSID1[0] && !webman_config->vPSID1[1]) {get_idps_psid(); sprintf(webman_config->vPSID1, "%016llX", PSID[0]); sprintf(webman_config->vPSID2, "%016llX", PSID[1]);}

	add_check_box("id1", "1", "IDPS", " : ", (webman_config->sidps), buffer);
	sprintf(templn, HTML_INPUT("vID1", "%s", "16", "22")       , webman_config->vIDPS1); strcat(buffer, templn);
	sprintf(templn, HTML_INPUT("vID2", "%s", "16", "22"), webman_config->vIDPS2); strcat(buffer, templn);
    sprintf(templn, HTML_BUTTON_FMT "<br>", HTML_BUTTON, " ", "onclick=\"vID2.value=", "1000000000000000"); strcat(buffer, templn);

	add_check_box("id2", "1", "PSID", " : ", (webman_config->spsid), buffer);
	sprintf(templn, HTML_INPUT("vPS1", "%s", "16", "22")       , webman_config->vPSID1); strcat(buffer, templn);
	sprintf(templn, HTML_INPUT("vPS2", "%s", "16", "22"), webman_config->vPSID2); strcat(buffer, templn);
    sprintf(templn, HTML_BUTTON_FMT "<br><br>", HTML_BUTTON, " ", "onclick=\"vPS1.value=vPS2.value=", "0000000000000000"); strcat(buffer, templn);
#else
	strcat(buffer, "<hr color=\"#0099FF\"/>");
#endif

	//Home
	sprintf(templn, " : " HTML_INPUT("hurl", "%s", "255", "50") "<br>", webman_config->home_url);
	add_check_box("hm", "hom", STR_HOME, templn, webman_config->homeb, buffer);

	//Disable lv1&lv2 peek&poke syscalls (6,7,9,10,36) and delete history files at system startup
#ifdef COBRA_ONLY
	#ifdef REMOVE_SYSCALLS
	add_check_box("spp", "1", STR_DELCFWSYS, " ", (webman_config->spp & 1), buffer);
	#endif
	add_check_box("shh", "1", "Offline", NULL, (webman_config->spp & 2), buffer);
#endif
	strcat(buffer, "<hr color=\"#0099FF\"/>");

	//default content profile
	sprintf(templn, "%s : <select name=\"usr\">", STR_PROFILE); strcat(buffer, templn);
	add_option_item("0" , STR_DEFAULT, (profile==0) , buffer);
	add_option_item("1", "1", (profile==1) , buffer);
	add_option_item("2", "2", (profile==2) , buffer);
	add_option_item("3", "3", (profile==3) , buffer);
	add_option_item("4", "4", (profile==4) , buffer);

	//default user account
	strcat(buffer, "</select> : hdd0/home/<select name=\"uacc\">");
	{
		int fd;

		if(cellFsOpendir("/dev_hdd0/home", &fd) == CELL_FS_SUCCEEDED)
		{
			CellFsDirent dir; u64 read = sizeof(CellFsDirent);

			while(!cellFsReaddir(fd, &dir, &read))
			{
				if(!read) break;
				if(strlen(dir.d_name)==8)
					add_option_item(dir.d_name, dir.d_name, (strcmp(dir.d_name, webman_config->uaccount)==0), buffer);
			}
			cellFsClosedir(fd);
		}

	}

	//memory usage
#ifndef LITE_EDITION
	sprintf(templn, "</select> &nbsp; %s : [<a href=\"/delete.ps3?wmconfig\">wmconfig</a>] [<a href=\"/delete.ps3?wmtmp\">wmtmp</a>] [<a href=\"/delete.ps3?history\">history</a>] • [<a href=\"/rebuild.ps3\">rebuild</a>] [<a href=\"/recovery.ps3\">recovery</a>]<p>", STR_DELETE); strcat(buffer, templn);
	sprintf(templn, " %s [%iKB]: ", STR_MEMUSAGE, (int)(BUFFER_SIZE_ALL / KB)); strcat(buffer, templn);
#else
	sprintf(templn, "</select><p> %s [%iKB]: ", STR_MEMUSAGE, (int)(BUFFER_SIZE_ALL / KB)); strcat(buffer, templn);
#endif
	add_radio_button("fp", "0", "fo_0", "Standard (896KB)", ",  ", (webman_config->foot==0), buffer);
	add_radio_button("fp", "1", "fo_1", "Min (320KB)"     , ",  ", (webman_config->foot==1), buffer);
	add_radio_button("fp", "3", "fo_3", "Min+ (512KB)"    , ",  ", (webman_config->foot==3), buffer);
	add_radio_button("fp", "2", "fo_2", "Max (1280KB)"    , NULL , (webman_config->foot==2), buffer);

#ifndef ENGLISH_ONLY
	//language
	sprintf(templn, "<br> %s: <select name=\"l\">", STR_PLANG); strcat(buffer, templn);

	add_option_item("0" , "English"                                                 , (webman_config->lang==0) , buffer);
	add_option_item("1" , "Fran\xC3\xA7\x61is"                                      , (webman_config->lang==1) , buffer);
	add_option_item("2" , "Italiano"                                                , (webman_config->lang==2) , buffer);
	add_option_item("3" , "Espa\xC3\xB1ol"                                          , (webman_config->lang==3) , buffer);
	add_option_item("4" , "Deutsch"                                                 , (webman_config->lang==4) , buffer);
	add_option_item("5" , "Nederlands"                                              , (webman_config->lang==5) , buffer);
	add_option_item("6" , "Portugu\xC3\xAAs"                                        , (webman_config->lang==6) , buffer);
	add_option_item("7" , "\xD0\xA0\xD1\x83\xD1\x81\xD1\x81\xD0\xBA\xD0\xB8\xD0\xB9", (webman_config->lang==7) , buffer);
	add_option_item("8" , "Magyar"                                                  , (webman_config->lang==8) , buffer);
	add_option_item("9" , "Polski"                                                  , (webman_config->lang==9) , buffer);
	add_option_item("10", "\xCE\x95\xCE\xBB\xCE\xBB\xCE\xB7\xCE\xBD\xCE\xB9\xCE\xBA\xCF\x8E\xCE\xBD", (webman_config->lang==10), buffer);
	add_option_item("11", "Hrvatski"                                                , (webman_config->lang==11), buffer);
	add_option_item("12", "\xD0\xB1\xD1\x8A\xD0\xBB\xD0\xB3\xD0\xB0\xD1\x80\xD1\x81\xD0\xBA\xD0\xB8", (webman_config->lang==12), buffer);
	add_option_item("20", "Dansk"                                                   , (webman_config->lang==20), buffer);
	add_option_item("21", "&#268;e&scaron;tina"                                     , (webman_config->lang==21), buffer);
	add_option_item("22", "Sloven&#269;ina"                                         , (webman_config->lang==22), buffer);

	add_option_item("13", "Indonesian"												, (webman_config->lang==13), buffer);
	add_option_item("14", "T\xC3\xBCrk\xC3\xA7\x65"									, (webman_config->lang==14), buffer);
	add_option_item("15", "\xD8\xA7\xD9\x84\xD8\xB9\xD8\xB1\xD8\xA8\xD9\x8A\xD8\xA9", (webman_config->lang==15), buffer);
	add_option_item("16", "\xE4\xB8\xAD\xE6\x96\x87"								, (webman_config->lang==16), buffer);
	add_option_item("19", "&#32321;&#39636;&#20013;&#25991;"						, (webman_config->lang==19), buffer);
	add_option_item("17", "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4"					, (webman_config->lang==17), buffer);
	add_option_item("18", "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E"					, (webman_config->lang==18), buffer);
	add_option_item("99", "Unknown"													, (webman_config->lang==99), buffer);

	strcat(buffer, "</select> ");
#endif

#ifdef COBRA_ONLY
#ifdef BDVD_REGION
	u8 cconfig[15];
	CobraConfig *cobra_config = (CobraConfig*) cconfig;
	memset(cobra_config, 0, 15);
	cobra_read_config(cobra_config);

	//BD Region
	strcat(buffer, " • BD Region: <select name=\"bdr\">");
	add_option_item("0" , STR_DEFAULT, (cobra_config->bd_video_region==0) , buffer);
	add_option_item("1" , "A"        , (cobra_config->bd_video_region==1) , buffer);
	add_option_item("2" , "B"        , (cobra_config->bd_video_region==2) , buffer);
	add_option_item("4" , "C"        , (cobra_config->bd_video_region==4) , buffer);

	//DVD Region
	strcat(buffer, "</select> • DVD Region: <select name=\"dvr\">");
	add_option_item("0"  , STR_DEFAULT, (cobra_config->dvd_video_region==0)  , buffer);
	add_option_item("1"  , "1"        , (cobra_config->dvd_video_region==1)  , buffer);
	add_option_item("2"  , "2"        , (cobra_config->dvd_video_region==2)  , buffer);
	add_option_item("4"  , "3"        , (cobra_config->dvd_video_region==4)  , buffer);
	add_option_item("8"  , "4"        , (cobra_config->dvd_video_region==8)  , buffer);
	add_option_item("16" , "5"        , (cobra_config->dvd_video_region==16) , buffer);
	add_option_item("32" , "6"        , (cobra_config->dvd_video_region==32) , buffer);
	strcat(buffer, "</select>");
#endif
#endif

	//combos
	sprintf(templn, "<hr color=\"#0099FF\"/><b><u> %s :</u></b><br><table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr><td nowrap valign=top>", STR_COMBOS2); strcat(buffer, templn);

	add_check_box("pfs", "1", STR_FAILSAFE,   " : <b>SELECT+L3+L2+R2</b><br>"  , !(webman_config->combo & FAIL_SAFE), buffer);
	add_check_box("pss", "1", STR_SHOWTEMP,   " : <b>SELECT+START</b><br>"     , !(webman_config->combo & SHOW_TEMP), buffer);
	add_check_box("ppv", "1", STR_PREVGAME,   " : <b>SELECT+L1</b><br>"        , !(webman_config->combo & PREV_GAME), buffer);
	add_check_box("pnx", "1", STR_NEXTGAME,   " : <b>SELECT+R1</b><br>"        , !(webman_config->combo & NEXT_GAME), buffer);
	add_check_box("pdf", "1", STR_FANCTRL4,   " : <b>L3+R2+START</b><br>"      , !(webman_config->combo & DISABLEFC), buffer);

	add_check_box("umt", "1", STR_UNMOUNT,    " : <b>SELECT+O</b><br>"         , !(webman_config->combo2 & UMNT_GAME), buffer);
	add_check_box("psv", "1", "OFFLINE",      " : <b>R2+口</b><br>"             , !(webman_config->combo2 & BLOCKSVRS), buffer);
	add_check_box("pgd", "1", "gameDATA",     " : <b>SELECT+口</b><br>"         , !(webman_config->combo2 & EXTGAMDAT), buffer);

	sprintf(templn, "%s XML", STR_REFRESH);
	add_check_box("pxr", "1", templn,         " : <b>SELECT+L3</b><br>"        , !(webman_config->combo2 & XMLREFRSH), buffer);

#ifdef VIDEO_REC
	add_check_box("vrc", "1", "VIDEO REC (in-game)", " : <b>SELECT+R3</b><br>" , !(webman_config->combo2 & VIDRECORD), buffer);
#endif

#ifdef REX_ONLY
	add_check_box("pid", "1", STR_SHOWIDPS,   " : <b>R2+O</b><br>"             , !(webman_config->combo & SHOW_IDPS), buffer);
	add_check_box("psd", "1", STR_SHUTDOWN2,  " : <b>L3+R2+X</b><br>"          , !(webman_config->combo & SHUT_DOWN), buffer);
	add_check_box("prs", "1", STR_RESTART2,   " : <b>L3+R2+O</b></td><td>"     , !(webman_config->combo & RESTARTPS), buffer);
#else
	add_check_box("pid", "1", STR_SHOWIDPS,   " : <b>R2+O</b></td><td>"        , !(webman_config->combo & SHOW_IDPS), buffer);
	add_check_box("psd", "1", STR_SHUTDOWN2,  " : <b>L3+R2+X</b><br>"          , !(webman_config->combo & SHUT_DOWN), buffer);
	add_check_box("prs", "1", STR_RESTART2,   " : <b>L3+R2+O</b><br>"          , !(webman_config->combo & RESTARTPS), buffer);
#endif
	add_check_box("puw", "1", STR_UNLOADWM,   " : <b>L3+R2+R3</b><br>"         , !(webman_config->combo & UNLOAD_WM), buffer);
	add_check_box("pf1", "1", STR_FANCTRL2,   " : <b>SELECT+"                  , !(webman_config->combo & MANUALFAN), buffer); sprintf(templn, "%s</b><br>", STR_UPDN); strcat(buffer, templn);
	add_check_box("pf2", "1", STR_FANCTRL5,   " : <b>SELECT+"                  , !(webman_config->combo & MINDYNFAN), buffer); sprintf(templn, "%s</b><br>", STR_LFRG); strcat(buffer, templn);
#ifdef REMOVE_SYSCALLS
	add_check_box("psc", "1", STR_DELCFWSYS2, " : <b>R2+&#8710;</b><br>"       , !(webman_config->combo & DISABLESH), buffer);
#endif
#ifdef COBRA_ONLY
	add_check_box("pdc", "1", STR_DISCOBRA,   " : <b>L3+L2+&#8710;</b><br>"    , !(webman_config->combo & DISACOBRA), buffer);
#endif

#ifndef LITE_EDITION
	add_check_box("pn0", "1", "NET0",       " : <b>SELECT+R2+口</b><br>"        , !(webman_config->combo2 & MOUNTNET0), buffer);
	add_check_box("pn1", "1", "NET1",       " : <b>SELECT+L2+口</b><br>"        , !(webman_config->combo2 & MOUNTNET1), buffer);
#endif

#ifdef REX_ONLY
	add_check_box("pr0", "1", STR_RBGMODE, 	" : <b>L3+L2+口</b><br>"            , !(webman_config->combo2 & REBUGMODE), buffer);
	add_check_box("pr1", "1", STR_RBGNORM, 	" : <b>L3+L2+O</b><br>"            , !(webman_config->combo2 & NORMAMODE), buffer);
	add_check_box("pr2", "1", STR_RBGMENU, 	" : <b>L3+L2+X</b><br>"            , !(webman_config->combo2 & DEBUGMENU), buffer);

	if(c_firmware>=4.65f)
	add_check_box("p2c", "1", "PS2 CLASSIC",  " : <b>SELECT+L2+&#8710;</b><br>", !(webman_config->combo2 & PS2TOGGLE), buffer);
#endif

	add_check_box("p2s", "1", "PS2 SWITCH",   " : <b>SELECT+L2+R2</b><br>"     , !(webman_config->combo2 & PS2SWITCH), buffer);

	sprintf(templn, "</td></tr></table>"
					"<hr color=\"#FF0000\"/><input type=\"submit\" value=\" %s \"/>"
					"</form>", STR_SAVE); strcat(buffer, templn);

	strcat(buffer,  "<hr color=\"#FF0000\"/>"
					"<a href=\"http://store.brewology.com/ahomebrew.php?brewid=255\">prepNTFS - Prepare NTFS drives for webMAN access</a><br>"
					"<a href=\"http://store.brewology.com/ahomebrew.php?brewid=257\">webMAN-MOD - Latest version of webMAN-MOD on Brewology</a><br>");

/*
	#define VSH_GCM_OBJ			0x70A8A8 // 4.53cex
	//#define VSH_GCM_OBJ		0x71A5F8 // 4.46dex

	uint32_t *gcm_obj0 = VSH_GCM_OBJ + ((uint32_t) 0 << 4);
	uint32_t *gcm_obj1 = VSH_GCM_OBJ + ((uint32_t) 1 << 4); // offset, pitch, width, height


	_cellGcmIoOffsetToAddress = (void*)((int)getNIDfunc("sdk", 0x2a6fba9c, 0));

	void *buf_adr[2];

	if(_cellGcmIoOffsetToAddress)
	{
		_cellGcmIoOffsetToAddress(gcm_obj0[0], &buf_adr[0]);
		_cellGcmIoOffsetToAddress(gcm_obj1[0], &buf_adr[1]); //0x37ee5ac
	}

	sprintf(templn, "OFFSET#0: %x, P: %i, W: %i, H: %i, E: %x <br>",
		gcm_obj0[0], gcm_obj0[1], gcm_obj0[2], gcm_obj0[3], buf_adr[0]); strcat(buffer, templn);

	sprintf(templn, "OFFSET#1: %x, P: %i, W: %i, H: %i, E: %x <br>",
		gcm_obj1[0], gcm_obj1[1], gcm_obj1[2], gcm_obj1[3], buf_adr[1]); strcat(buffer, templn);
*/

}

static void game_mount(char *buffer, char *templn, char *param, char *tempstr, u8 is_binary, bool mount_ps3, bool forced_mount)
{
	struct CellFsStat buf;

	//unmount game
	if(strstr(param, "ps3/unmount"))
	{
		do_umount(true);

		strcat(buffer, STR_GAMEUM);
	}
#ifdef PS2_DISC
	else if(strstr(param, "ps2/unmount"))
	{
		do_umount_ps2disc(false);

		strcat(buffer, STR_GAMEUM);
	}
#endif
	else
	{
		if(strstr(param, "?random="))
			param[strrchr(param, '?')-param]=0;

		int plen=10;
#ifdef COPY_PS3
		if(strstr(param, "copy.ps3")) plen=IS_COPY;
		char target[MAX_PATH_LEN];
#endif
		char enc_dir_name[1024];
		bool mounted=false; max_mapped=0;
		is_binary=1;

		if(!(plen==IS_COPY && !copy_in_progress))
		{
			for(int n=0; n<(int)(strlen(param)-9); n++)
				if(memcmp(param + n, "/PS3_GAME", 9)==0) {param[n]=0; break;}
#ifdef PS2_DISC
			if(!memcmp(param, "/mount.ps2", 10))
			{
				mounted=mount_ps2disc(param+plen);
			}
			else
			if(!memcmp(param, "/mount_ps2", 10))
			{
				do_umount(true);
				mounted=mount_ps2disc(param+plen);
			}
			else
#endif
			if(!forced_mount && get_game_info())
			{
				sprintf(templn, "<H3>%s : <a href=\"/mount.ps3/unmount\">%s %s</a></H3><hr><a href=\"/mount_ps3%s\">", STR_UNMOUNTGAME, _game_TitleID, _game_Title, param+plen); strcat(buffer, templn);
			}
			else
				mounted=mount_with_mm(param+plen, 1);
		}

		if(mount_ps3)
		{
			is_busy=false;
			return;
		}
		else
		{
			htmlenc(templn, param+plen, 0);
			sprintf(tempstr, "%s/PS3_GAME/ICON0.PNG", param+plen);

			if(cellFsStat(tempstr, &buf)!=CELL_FS_SUCCEEDED)
			{
				char fpath[MAX_PATH_LEN], fname[MAX_PATH_LEN], tempID[10]; tempstr[0]=0;

				// get iso name
				strcpy(fname, strrchr(param+plen, '/')+1);
				strcpy(fpath, param+plen); fpath[strlen(fpath)-strlen(fname)-1]=0;

				get_default_icon(tempstr, fpath, fname, 0, tempID, -1, 0);
			}

			urlenc(enc_dir_name, tempstr);

#ifdef COPY_PS3
			if(plen==IS_COPY)
			{
				bool is_copying_from_hdd = (strstr(param+plen, "/dev_hdd0")!=NULL);
#ifdef SWAP_KERNEL
				if(strstr(param+plen, "/lv2_kernel"))
				{
					if(cellFsStat(param+plen, &buf)!=CELL_FS_SUCCEEDED)
						sprintf(target, STR_ERROR);
					else
					{
						uint64_t size = buf.st_size;

						enable_dev_blind(param+plen);

						// for	cobra req: /dev_flash/sys/stage2.bin & /dev_flash/sys/lv2_self
						sprintf(tempstr, "%s", param+plen);
						strcpy(strrchr(tempstr, '/'), "/stage2.bin");

						sprintf(target, "/dev_blind/sys/stage2.bin");
						if(cellFsStat(target, &buf)!=CELL_FS_SUCCEEDED)
							filecopy(tempstr, target, COPY_WHOLE_FILE);

						// copy: /dev_flash/sys/lv2_self
						sprintf(target, "/dev_blind/sys/lv2_self");
						if(cellFsStat(target, &buf)!=CELL_FS_SUCCEEDED || buf.st_size != size)
							filecopy(param+plen, target, COPY_WHOLE_FILE);

						if(cellFsStat(target, &buf)==CELL_FS_SUCCEEDED)
						{
							u64 lv2_offset=0x15DE78; // 4.xx CFW LV1 memory location for: /flh/os/lv2_kernel.self
							/*
							if(peek_lv1(lv2_offset)!=0x2F666C682F6F732FULL)
								for(u64	addr=0;	addr<0xFFFFF8ULL; addr+=4)         // Find in 16MB
									if(peek_lv1(addr)	== 0x2F6F732F6C76325FULL)  // /os/lv2_
									{
										lv2_offset=addr-4; break; // 0x12A2C0 on 3.55
									}
							*/
							if(peek_lv1(lv2_offset)==0x2F666C682F6F732FULL)  // Original: /flh/os/lv2_kernel.self
							{
								poke_lv1(lv2_offset + 0x00, 0x2F6C6F63616C5F73ULL); // replace:	/flh/os/lv2_kernel.self -> /local_sys0/sys/lv2_self
								poke_lv1(lv2_offset + 0x08, 0x7973302F7379732FULL);
								poke_lv1(lv2_offset + 0x10, 0x6C76325F73656C66ULL);

								working = 0;
								{ DELETE_TURNOFF }
								savefile((char*)WMNOSCAN, NULL, 0);
								{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);}
								sys_ppu_thread_exit(0);
							}
						}
					}
					plen=0; //do not copy
				}
				else
#endif // #ifdef SWAP_KERNEL
				if(!extcmp(param+plen, ".p3t", 4))
				{
					if(is_copying_from_hdd)
						sprintf(target, "/dev_usb000/PS3/THEME");
					else
						sprintf(target, "/dev_hdd0/theme");

					strcat(target, strrchr(param, '/'));
				}
				else if(!extcmp(param+plen, ".pkg", 4))
				{
					if(is_copying_from_hdd)
						sprintf(target, "/dev_usb000/Packages");
					else
						sprintf(target, "/dev_hdd0/packages");

					strcat(target, strrchr(param, '/'));
				}
				else if(!extcmp(param+plen, ".edat", 5))
				{
					if(is_copying_from_hdd)
						sprintf(target, "/dev_usb000/exdata");
					else
						sprintf(target, "%s/%s/exdata", "/dev_hdd0/home", webman_config->uaccount);

					strcat(target, strrchr(param, '/'));
				}
				else if(strstr(param+plen, "/exdata"))
				{
					if(is_copying_from_hdd)
						sprintf(target, "/dev_usb000/exdata");
					else
						sprintf(target, "%s/%s/exdata", "/dev_hdd0/home", webman_config->uaccount);
				}
				else if(strstr(param+plen, "/PS3/THEME"))
					sprintf(target, "/dev_hdd0/theme");
				else if(strcasestr(param+plen, "/savedata/"))
				{
					if(is_copying_from_hdd)
						sprintf(target, "/dev_usb000/PS3/SAVEDATA");
					else
						sprintf(target, "%s/%s/savedata", "/dev_hdd0/home", webman_config->uaccount);

					strcat(target, strrchr(param, '/'));
				}
				else if(strcasestr(param+plen, "/trophy/"))
				{
					if(is_copying_from_hdd)
						sprintf(target, "/dev_usb000/PS3/TROPHY");
					else
						sprintf(target, "%s/%s/trophy", "/dev_hdd0/home", webman_config->uaccount);

					strcat(target, strrchr(param, '/'));
				}
				else if(strstr(param+plen, "/webftp_server"))
				{
					sprintf(target, "/dev_hdd0/plugins/webftp_server.sprx");
					if(cellFsStat((char*)target, &buf)!=CELL_FS_SUCCEEDED) sprintf(target, "/dev_hdd0/webftp_server.sprx");
				}
				else if(strstr(param+plen, "/boot_plugins_"))
					sprintf(target, "/dev_hdd0/boot_plugins.txt");
				else if(strstr(param+plen, "/dev_usb"))
					sprintf(target, "/dev_hdd0%s", param+plen+11);
				else if(is_copying_from_hdd)
					sprintf(target, "/dev_usb000%s", param+plen+9);
				else
				{
					sprintf(target, "%s/%s", "/dev_hdd0/GAMES", "My Disc Backup");
					if(strstr(param, "/dev_bdvd"))
					{
						char titleid[10], title[60];
						unsigned char *mem = (u8*)tempstr;

						int fdw;
						if(cellFsOpen((char*)"/dev_bdvd/PS3_GAME/PARAM.SFO", CELL_FS_O_RDONLY, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
						{   memset(mem, 0, _4KB_); uint64_t msiz = 0;
							cellFsLseek(fdw, 0, CELL_FS_SEEK_SET, &msiz);
							cellFsRead(fdw, (void *)mem, _4KB_, &msiz);
							cellFsClose(fdw);

							parse_param_sfo(mem, titleid, title);
							if(titleid[0] && titleid[8]>='0')
							{
								if(strstr(title, " ["))
									sprintf(target, "%s/%s", "/dev_hdd0/GAMES", title);
								else
									sprintf(target, "%s/%s [%s]", "/dev_hdd0/GAMES", title, titleid);
							}
						}
					}
				}

				sprintf(tempstr, "%s <a href=\"%s\">%s</a><hr>"
								 "<a href=\"%s\"><img src=\"%s\" border=0></a><hr>"
								 "%s: <a href=\"%s\">%s</a>",
								 STR_COPYING, param+plen, templn,
								 target, enc_dir_name,
								 STR_CPYDEST, target, target);

				if(strstr(target, "/webftp_server")) {strcat(buffer, tempstr); sprintf(tempstr, "<HR>%s", STR_SETTINGSUPD);}
			}
			else
#endif // #ifdef COPY_PS3
			if(!extcmp(param, ".BIN.ENC", 8))
				sprintf(tempstr, "%s: %s<hr><img src=\"%s\"><hr>%s", STR_GAMETOM, templn, enc_dir_name, mounted?STR_PS2LOADED:STR_ERROR);
			else if(strstr(param, "/PSPISO") || strstr(param, "/ISO/"))
				sprintf(tempstr, "%s: %s<hr><img src=\"%s\" height=%i><hr>%s", STR_GAMETOM, templn, enc_dir_name, strcasestr(enc_dir_name,".png")?200:300, mounted?STR_PSPLOADED:STR_ERROR);
			else if(strstr(param, "/BDISO") || strstr(param, "/DVDISO") || !extcmp(param, ".ntfs[BDISO]", 12) || !extcmp(param, ".ntfs[DVDISO]", 13))
				sprintf(tempstr, "%s: %s<hr><a href=\"/dev_bdvd\"><img src=\"%s\" border=0></a><hr>%s", STR_MOVIETOM, templn, enc_dir_name, mounted?STR_MOVIELOADED:STR_ERROR);
			else
				sprintf(tempstr, "%s: %s<hr><a href=\"/dev_bdvd\"><img src=\"%s\" border=0></a><hr>%s", STR_GAMETOM, templn, enc_dir_name, mounted?STR_GAMELOADED:STR_ERROR);

			strcat(buffer, tempstr);

#ifdef PS2_DISC
			if(mounted && (strstr(param+plen, "/GAME") || strstr(param+plen, "/PS3ISO") || strstr(param+plen, ".ntfs[PS3ISO]")))
			{
				CellFsDirent entry; u64 read_e; int fd2; u16 pcount=0; u32 tlen=strlen(buffer)+8;

				sprintf(target, "%s", param+plen); u8 is_iso=0;
				if(strstr(target, "Sing"))
				{
					if(strstr(target, "/PS3ISO")) {strcpy(strstr(target, "/PS3ISO"), "/PS2DISC\0"); is_iso=1;}
					if(strstr(target, ".ntfs[PS3ISO]")) {strcpy(target, "/dev_hdd0/PS2DISC\0"); is_iso=1;}
				}

				// check for [PS2] extracted folders
				if(cellFsOpendir(target, &fd2) == CELL_FS_SUCCEEDED)
				{
					while((cellFsReaddir(fd2, &entry, &read_e) == 0 && read_e > 0))
					{
						if((entry.d_name[0]=='.')) continue;

						if(is_iso || strstr(entry.d_name, "[PS2")!=NULL)
						{
							if(pcount==0) strcat(buffer, "<br><HR>");
							urlenc(enc_dir_name, entry.d_name);
							sprintf(templn, "<a href=\"/mount.ps2%s/%s\">%s</a><br>", target, enc_dir_name, entry.d_name);

							tlen+=strlen(tempstr);
							if(tlen>(BUFFER_SIZE-1024)) break;
							strcat(buffer, templn); pcount++;
						}
					}
				}
			}
#endif // #ifdef PS2_DISC
		}
#ifdef COPY_PS3
		if(plen==IS_COPY && !copy_in_progress)
		{
			if(cellFsStat((char*)param+plen, &buf)!=CELL_FS_SUCCEEDED)
				sprintf(templn, "%s", STR_ERROR);
			else
			{
				// show msg begin
				sprintf(templn, "%s %s\n%s %s", STR_COPYING, param+plen, STR_CPYDEST, target);
				show_msg((char*)templn);
				copy_in_progress=true; copied_count = 0;

				// make target dir tree
				for(u16 p=12; p<strlen(target); p++)
					if(target[p]=='/') {target[p]=0; cellFsMkdir((char*)target, MODE); target[p]='/';}

				// copy folder to target
				if(strstr(param+plen,"/exdata"))
					import_edats(param+plen, target);
				else if(isDir(param+plen))
					folder_copy(param+plen, target);
				else
					filecopy(param+plen, target, COPY_WHOLE_FILE);

				copy_in_progress=false;

				// show msg end
				if(copy_aborted)
					show_msg((char*)STR_CPYABORT);
				else
					show_msg((char*)STR_CPYFINISH);
			}
		}
#endif //#ifdef COPY_PS3
	}
}

static bool game_listing(char *buffer, char *templn, char *param, int conn_s, char *tempstr, bool mobile_mode)
{
    u64 c_len = 0;
	CellRtcTick pTick;

	struct CellFsStat buf;
	int fd;

	gmobile_mode = mobile_mode;

	//if(cobra_mode) strcat(buffer, "[Cobra] ");
	//strcat(buffer, "PS3 Game List:<br>");
	if(!mobile_mode && strstr(param, "/index.ps3"))
	{
		strcat(buffer, "<font style=\"font-size:18px\">");
#ifdef COBRA_ONLY
		if(!(webman_config->cmask & PS3)) { add_query_html(buffer, (char*)"ps3", (char*)"PS3");
											add_query_html(buffer, (char*)"games", (char*)"GAMES");
											add_query_html(buffer, (char*)"PS3ISO", (char*)"PS3ISO");}

		if(!(webman_config->cmask & PS2))   add_query_html(buffer, (char*)"PS2ISO", (char*)"PS2ISO");
		if(!(webman_config->cmask & PSP))   add_query_html(buffer, (char*)"PSPISO", (char*)"PSPISO");
		if(!(webman_config->cmask & PS1))   add_query_html(buffer, (char*)"PSXISO", (char*)"PSXISO");
		if(!(webman_config->cmask & BLU))   add_query_html(buffer, (char*)"BDISO" , (char*)"BDISO" );
		if(!(webman_config->cmask & DVD))   add_query_html(buffer, (char*)"DVDISO", (char*)"DVDISO");
 #ifndef LITE_EDITION
		if(webman_config->netd0 || webman_config->netd1 || webman_config->netd2) add_query_html(buffer, (char*)"net", (char*)"NET");
 #endif
		add_query_html(buffer, (char*)"hdd", (char*)"HDD");
		add_query_html(buffer, (char*)"usb", (char*)"USB");
		add_query_html(buffer, (char*)"ntfs", (char*)"NTFS");
#else
		if(!(webman_config->cmask & PS3)) add_query_html(buffer, (char*)"games", (char*)"GAMES");
		if(!(webman_config->cmask & PS2)) add_query_html(buffer, (char*)"PS2ISO", (char*)"PS2ISO");

		add_query_html(buffer, (char*)"hdd", (char*)"HDD");
		add_query_html(buffer, (char*)"usb", (char*)"USB");
#endif //#ifdef COBRA_ONLY
		strcat(buffer, "</font><span style=\"white-space:normal;\">");
	}
	else
		strcat(buffer, " <br>");

	c_len = 0; while(loading_games && working && c_len < 500) {sys_timer_usleep(200000); c_len++;}

	if(c_len >= 500 || !working) {strcat(buffer, "503 Server is busy"); return true;}

	u32 buf_len=strlen(buffer);

/*
	CellRtcTick pTick, pTick2;
	cellRtcGetCurrentTick(&pTick);
	int upd_time=0;

	if(cellFsStat((char*)WMTMP "/games.html", &buf)==CELL_FS_SUCCEEDED)
		upd_time=buf.st_mtime;

	CellRtcDateTime rDate;
	cellRtcSetTime_t(&rDate, upd_time);
	cellRtcGetTick(&rDate, &pTick2);

	sprintf(templn, "[%ull %ull %i ]<br>", pTick2, pTick, (pTick.tick-pTick2.tick)/1000000);
	strcat(buffer, templn);

	if(strstr(param, "/index.ps3?") || ((pTick.tick-pTick2.tick)/1000000)>43200) {DELETE_CACHED_GAMES}
*/
	loading_games = 1;
	if(mobile_mode) {cellFsUnlink((char*)GAMELIST_JS); buffer[0]=0; buf_len=0;}
	else
	{
		if(strstr(param, "/index.ps3?")) cellFsUnlink((char*)WMTMP "/games.html");

		if(cellFsStat((char*)WMTMP "/games.html", &buf)==CELL_FS_SUCCEEDED && buf.st_size > 10)
		{
			int fdu;
			if(cellFsOpen((char*)WMTMP "/games.html", CELL_FS_O_RDONLY, &fdu, NULL, 0)==CELL_FS_SUCCEEDED)
			{
				cellFsRead(fdu, (char*)(buffer+buf_len), buf.st_size, NULL);
				cellFsClose(fdu);
				loading_games = 0;
			}
		}
	}

	if(loading_games)
	{
		int abort_connection=0;
		u8 is_net=0;

		char ename[8];
		char swap[1024];
		u16 idx=0;
		u32 tlen=strlen(buffer); buffer[tlen]=0;
		char *sysmem_html=buffer+_8KB_;

		typedef struct
		{
			char 	path[MAX_LINE_LEN];
		}
		t_line_entries;
		t_line_entries *line_entry	= (t_line_entries *)sysmem_html;
		u16 max_entries=(((BUFFER_SIZE_ALL-_8KB_))/MAX_LINE_LEN)-1; tlen=0;

		// filter html content
		u8 filter0, filter1, b0, b1; char filter_name[MAX_PATH_LEN]; filter_name[0]=0; filter0=filter1=b0=b1=0;
#ifdef COBRA_ONLY
		if(strstr(param, "ntfs")) {filter0=NTFS; b0=1;} else
#endif
		for(u8 f0=0; f0<14; f0++) if(strstr(param, drives[f0])) {filter0=f0; b0=1; break;}
		for(u8 f1=0; f1<11; f1++) if(strstr(param, paths [f1])) {filter1=f1; b1=1; break;}
		if(!b0 && strstr(param, "hdd" ))  {filter0=0; b0=1;}
		if(!b0 && strstr(param, "usb" ))  {filter0=1; b0=2;}
		if(!b1 && strstr(param, "games")) {filter1=0; b1=2;}
		if(!b1 && strstr(param, "?ps3"))  {filter1=0; b1=3;}
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
		if(!b0 && strstr(param, "net" )) {filter0=7; b0=3;}
 #endif
#endif
		if(b0==0 && b1==0 && strstr(param, "?")!=NULL && strstr(param, "?html")==NULL && strstr(param, "mobile")==NULL) strcpy(filter_name, strstr(param, "?")+1);

		for(u8 f0=filter0; f0<14; f0++)  // drives: 0="/dev_hdd0", 1="/dev_usb000", 2="/dev_usb001", 3="/dev_usb002", 4="/dev_usb003", 5="/dev_usb006", 6="/dev_usb007", 7="/net0", 8="/net1", 9="/net2", 10="/ext", 11="/dev_sd", 12="/dev_ms", 13="/dev_cf"
		{

			if(!webman_config->usb0 && (f0==1)) continue;
			if(!webman_config->usb1 && (f0==2)) continue;
			if(!webman_config->usb2 && (f0==3)) continue;
			if(!webman_config->usb3 && (f0==4)) continue;
			if(!webman_config->usb6 && (f0==5)) continue;
			if(!webman_config->usb7 && (f0==6)) continue;

			if(!webman_config->dev_sd && (f0==11)) continue;
			if(!webman_config->dev_ms && (f0==12)) continue;
			if(!webman_config->dev_cf && (f0==13)) continue;

			if( f0==NTFS && (!webman_config->usb0 && !webman_config->usb1 && !webman_config->usb2 &&
							 !webman_config->usb3 && !webman_config->usb6 && !webman_config->usb7)) continue;

//
			int ns=-2; u8 uprofile=profile;
			for(u8 f1=filter1; f1<11; f1++) // paths: 0="GAMES", 1="GAMEZ", 2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO", 7="PSXGAMES", 8="PSPISO", 9="ISO", 10="video"
			{
#ifndef COBRA_ONLY
				if(IS_ISO_FOLDER && !(IS_PS2_FOLDER)) continue; // 0="GAMES", 1="GAMEZ", 5="PS2ISO", 10="video"
#endif
				if(tlen>(BUFFER_SIZE-1024)) break;
				if(idx>=(max_entries-1)) break;

				cellRtcGetCurrentTick(&pTick);

				if(IS_PS2_FOLDER && f0>0)  continue; // PS2ISO is supported only from /dev_hdd0
				if(f1>=10) {if(f0<7 || f0>NTFS) strcpy(paths[10], f0==0 ? "video" : "GAMES_DUP"); else continue;}
				if(f0==NTFS) {if(f1>6 || !cobra_mode) break; else if(f1<2 || f1==5) continue;}
				if(f0==7 && (!webman_config->netd0 || f1>6 || !cobra_mode)) break;
				if(f0==8 && (!webman_config->netd1 || f1>6 || !cobra_mode)) break;
				if(f0==9 && (!webman_config->netd2 || f1>6 || !cobra_mode)) break;

				if(b0) {if(b0==2 && f0<7); else if(b0==3 && f0!=NTFS); else if(filter0!=f0) continue;}
				if(b1) {if(b1>=2 && (f1<b1 || f1>=10) && filter1<3); else if(filter1!=f1) continue;}
				else
				{
					if( (webman_config->cmask & PS3) && (IS_PS3_FOLDER)) continue;
					if( (webman_config->cmask & BLU) && (IS_BLU_FOLDER)) continue;
					if( (webman_config->cmask & DVD) && (IS_DVD_FOLDER)) continue;
					if( (webman_config->cmask & PS2) && (IS_PS2_FOLDER)) continue;
					if( (webman_config->cmask & PS1) && (IS_PSX_FOLDER)) continue;
					if( (webman_config->cmask & PSP) && (IS_PSP_FOLDER)) continue;
				}

				is_net=(f0>=7 && f0<=9);
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(ns==-2 && is_net) ns=connect_to_remote_server(f0-7);
 #endif
#endif
				if(is_net && (ns<0)) break;

				bool ls=false; u8 li=0, subfolder=0;

		subfolder_letter_html:
				subfolder = 0; uprofile = profile;
		read_folder_html:
//
#ifndef LITE_EDITION
				if(is_net)
				{
					char ll[4]; if(li) sprintf(ll, "/%c", '@'+li); else ll[0]=0;
					sprintf(param, "/%s%s%s",    paths[f1], SUFIX(uprofile), ll);
				}
				else
#endif
				{
					if(f0==NTFS) //ntfs
						sprintf(param, "%s", WMTMP);
					else
						sprintf(param, "%s/%s%s", drives[f0], paths[f1], SUFIX(uprofile));
				}
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
				if(is_net && open_remote_dir(ns, param, &abort_connection) < 0) goto continue_reading_folder_html; //continue;
 #endif
#endif
				CellFsDirent entry;
				u64 read_e;
				u8 is_iso=0;
				char icon[MAX_PATH_LEN], enc_dir_name[1024], subpath[MAX_PATH_LEN]; int fd2;
				char tempID[12];
				sys_addr_t data2=0;
				int v3_entries, v3_entry; v3_entries=v3_entry=0;
#ifdef COBRA_ONLY
				uint64_t msiz = 0;
 #ifndef LITE_EDITION
				netiso_read_dir_result_data *data=NULL; char neth[8];
				if(is_net)
				{
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(data2==NULL) goto continue_reading_folder_html; //continue;
					data=(netiso_read_dir_result_data*)data2;
					sprintf(neth, "/net%i", (f0-7));
				}
 #endif
#endif
				if(!is_net && cellFsOpendir( param, &fd) != CELL_FS_SUCCEEDED) goto continue_reading_folder_html; //continue;

				while((!is_net && cellFsReaddir(fd, &entry, &read_e) == 0 && read_e > 0)
					|| (is_net && v3_entry<v3_entries)
					)
				{
#ifdef COBRA_ONLY
 #ifndef LITE_EDITION
					if(is_net)
					{
						if(!ls && li==0 && f1>1 && data[v3_entry].is_directory && strlen(data[v3_entry].name)==1) ls=true;

						if(filter_name[0]>=' ' && strcasestr(param, filter_name)==NULL && strcasestr(data[v3_entry].name, filter_name)==NULL) {v3_entry++; continue;}

						if(add_net_game(ns, data, v3_entry, neth, param, templn, tempstr, enc_dir_name, icon, tempID, f1, 1)==FAILED) {v3_entry++; continue;}

						snprintf(ename, 6, "%s    ", templn);

						strcpy(tempstr, icon); urlenc(icon, tempstr);

						if(mobile_mode)
						{
							if(strstr(enc_dir_name, "'")) continue; // ignore: cause syntax error in javascript
							for(size_t c=0; templn[c]!=0; c++) {if(templn[c]==39) templn[c]=96; else if(templn[c]<=31) templn[c]=32;} // replace invalid chars

							int w=260, h=300; if(strstr(icon, "ICON0.PNG")) {w=320; h=176;} else if(strstr(icon, "icon_wm_")) {w=280; h=280;}

							sprintf(tempstr, "%c%c%c%c{img:'%s',width:%i,height:%i,desc:'%s',url:'%s%s/%s'},",
								ename[0], ename[1], ename[2], ename[3],
								icon, w, h, templn, neth, param, enc_dir_name);
						}
						else
							sprintf(tempstr, "%c%c%c%c<div class=\"gc\"><div class=\"ic\"><a href=\"/mount.ps3%s%s/%s?random=%x\"><img src=\"%s\" class=\"gi\"></a></div><div class=\"gn\"><a href=\"%s%s/%s\">%s</a></div></div>",
								ename[0], ename[1], ename[2], ename[3],
								neth, param, enc_dir_name, (u16)pTick.tick,
								icon,
								neth, param, enc_dir_name,
								templn);

						v3_entry++;
						if(strlen(tempstr)>MAX_LINE_LEN) continue; //ignore lines too long
						strncpy(line_entry[idx].path, tempstr, MAX_LINE_LEN); idx++;
						tlen+=strlen(tempstr);
						if(tlen>(BUFFER_SIZE-1024)) break;
					}
					else
 #endif
#endif
					{
						if(entry.d_name[0]=='.') continue;

						char tmp_param[8];
						strncpy(tmp_param, param+strlen(drives[f0]), 8);
//////////////////////////////
						subfolder = 0;
						sprintf(subpath, "%s/%s", param, entry.d_name);
						if(IS_ISO_FOLDER && isDir(subpath) && cellFsOpendir(subpath, &fd2) == CELL_FS_SUCCEEDED)
						{
							strcpy(subpath, entry.d_name); subfolder = 1;
next_html_entry:
							cellFsReaddir(fd2, &entry, &read_e);
							if(read_e<1) continue;
							if(entry.d_name[0]=='.') goto next_html_entry;
							sprintf(templn, "%s/%s", subpath, entry.d_name); strcpy(entry.d_name, templn);
						}
						int flen = strlen(entry.d_name);
//////////////////////////////

						if(filter_name[0]>=' ' && strcasestr(param, filter_name)==NULL && strcasestr(entry.d_name, filter_name)==NULL)
						{if(subfolder) goto next_html_entry;}

#ifdef COBRA_ONLY
						is_iso = (f0==NTFS && flen>13 && strstr(entry.d_name + flen - 13, ".ntfs[")!=NULL) ||
								 (IS_ISO_FOLDER && flen > 4 && (
								 (            !strncasecmp(entry.d_name + flen - 4, ".iso", 4)) ||
								 (flen > 6 && !strncasecmp(entry.d_name + flen - 6, ".iso.0", 6)) ||
								 ((IS_PS2_FOLDER) && strcasestr(".bin.img.mdf.enc", entry.d_name + flen - 4)) ||
								 ((IS_PSX_FOLDER) && strcasestr(".cue.img.mdf", entry.d_name + flen - 4))
								 ));
#else
						is_iso = (IS_PS2_FOLDER && flen > 8 && !strncmp(entry.d_name + flen - 8, ".BIN.ENC", 8));
#endif
						if(!is_iso)
						{
							sprintf(templn, "%s/%s/PS3_GAME/PARAM.SFO", param, entry.d_name);
						}

						if(is_iso || (f1<2 && cellFsStat(templn, &buf)==CELL_FS_SUCCEEDED))
						{
							icon[0]=tempID[0]=0;

							if(is_iso)
							{
								get_name(templn, entry.d_name, 0);
#ifdef COBRA_ONLY
								if(f0==NTFS)
								{   // ntfs
									if(f1< 2 || f1>6) continue; //2="PS3ISO", 3="BDISO", 4="DVDISO", 5="PS2ISO", 6="PSXISO"
									if((uprofile >0) && !strstr(entry.d_name, SUFIX3(uprofile))) continue;
									if((uprofile==0 && flen>17)) {for(u8 u=1;u<5;u++) if(strstr(entry.d_name + flen - 17, SUFIX3(u))) continue;}
								}

								if((strstr(tmp_param, "/PS3ISO") && f0!=NTFS) || (f0==NTFS && f1==2 && !extcmp(entry.d_name, ".ntfs[PS3ISO]", 13)))
								{
									int fs=0;
									get_name(templn, entry.d_name, 1); strcat(templn, ".SFO\0");
									if(f0!=NTFS && cellFsStat(templn, &buf)!=CELL_FS_SUCCEEDED)
									{
										get_name(tempstr, entry.d_name, 0);
										sprintf(templn, "%s/%s.SFO", param, tempstr);
									}

									if(get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr)==1)
									{
										if( f0!=NTFS ) //get title id from ISO
										{
											sprintf(icon, "%s/%s", param, entry.d_name);
											if(cellFsOpen(icon, CELL_FS_O_RDONLY, &fs, NULL, 0) == CELL_FS_SUCCEEDED)
											{
												if(cellFsLseek(fs, 0x810, CELL_FS_SEEK_SET, &msiz) == CELL_FS_SUCCEEDED)
												{
													if(cellFsRead(fs, (void *)&tempID, 11, &msiz) == CELL_FS_SUCCEEDED)
													{
														strncpy(&tempID[4], &tempID[5], 5); tempID[9]='\0';
													}
												}
												cellFsClose(fs);
											}
											icon[0]=0;
										}
									}
								}
								else
								{
									if(f0==NTFS)
									{   // ntfs
										int flen=strlen(entry.d_name)-13; if(flen<0) continue;

										if(f1==2 && !strstr(entry.d_name+flen, ".ntfs[PS3ISO]")) continue;
										if(f1==3 && !strstr(entry.d_name+flen, ".ntfs[BD" ))     continue;
										if(f1==4 && !strstr(entry.d_name+flen, ".ntfs[DVDISO]")) continue;
										if(f1==6 && !strstr(entry.d_name+flen, ".ntfs[PSXISO]")) continue;
									}

									get_name(templn, entry.d_name, 0);
								}
#endif
							}
							else
							{
								get_title_and_id_from_sfo(templn, tempID, entry.d_name, icon, tempstr);
							}

							if(!is_iso && f1<2 && (icon[0]==0 || webman_config->nocov)) sprintf(icon, "%s/%s/PS3_GAME/ICON0.PNG", param, entry.d_name);

							get_cover_from_name(icon, entry.d_name, tempID);

							if(icon[0]==0)
							{
								sprintf(icon, "%s/%s", param, entry.d_name);
								flen = strlen(icon);
#ifdef COBRA_ONLY
								if(flen > 13 && (!extcmp(icon, ".ntfs[PS3ISO]", 13) || !extcmp(icon, ".ntfs[DVDISO]", 13) || !extcmp(icon, ".ntfs[PSXISO]", 13) || !extcmp(icon, ".ntfs[BDFILE]", 13))) {flen -= 13; icon[flen]=0;} else
								if(flen > 12 &&  !extcmp(icon, ".ntfs[BDISO]" , 12)) {flen -= 12; icon[flen]=0;}
#endif
								if(flen > 4 && icon[flen-4]=='.')
								{
									icon[flen-3]='p'; icon[flen-2]='n'; icon[flen-1]='g';
									if(cellFsStat(icon, &buf)!=CELL_FS_SUCCEEDED)
									{
										icon[flen-3]='P'; icon[flen-2]='N'; icon[flen-1]='G';
									}
								}
								else
								if(flen > 5 && icon[flen-2]=='.')
								{
									icon[flen-5]='p'; icon[flen-4]='n'; icon[flen-3]='g'; flen -= 2; icon[flen]=0;
								}

								if(cellFsStat(icon, &buf)!=CELL_FS_SUCCEEDED)
									{icon[flen-3]='j'; icon[flen-2]='p'; icon[flen-1]='g';}
							}

							get_default_icon(icon, param, entry.d_name, 0, tempID, ns, abort_connection);

							if(webman_config->tid && tempID[0]>'@' && strlen(templn) < 50 && strstr(templn, " [")==NULL) {strcat(templn, " ["); strcat(templn, tempID); strcat(templn, "]");}

							urlenc(enc_dir_name, entry.d_name);
							templn[64]=0; flen=strlen(templn);

							snprintf(ename, 6, "%s    ", templn);

							strcpy(tempstr, icon); urlenc(icon, tempstr);

							if(mobile_mode)
							{
								if(strstr(enc_dir_name, "'")) continue; // ignore: cause syntax error in javascript
								for(size_t c=0; templn[c]!=0; c++) {if(templn[c]==39) templn[c]=96; else if(templn[c]<=31) templn[c]=32;} // replace invalid chars

								int w=260, h=300; if(strstr(icon, "ICON0.PNG")) {w=320; h=176;} else if(strstr(icon, "icon_wm_")) {w=280; h=280;}

								sprintf(tempstr, "%c%c%c%c{img:'%s',width:%i,height:%i,desc:'%s',url:'%s/%s'},",
									ename[0], ename[1], ename[2], ename[3],
									icon, w, h, templn, param, enc_dir_name);
							}
							else
							{
								do
								{
									{
										sprintf(tempstr, "%c%c%c%c<div class=\"gc\"><div class=\"ic\"><a href=\"/mount.ps3%s%s/%s?random=%x\"><img src=\"%s\" class=\"gi\"></a></div><div class=\"gn\"><a href=\"%s%s/%s\">%s</a></div></div>",
											ename[0], ename[1], ename[2], ename[3],
											param, "", enc_dir_name, (u16)pTick.tick, icon, param, "", enc_dir_name, templn);
									}

									flen-=4; if(flen<32) break;
									templn[flen]=0;
								}
								while(strlen(templn)>MAX_LINE_LEN);
							}

							if(strlen(tempstr)>MAX_LINE_LEN) continue; //ignore lines too long

							strncpy(line_entry[idx].path, tempstr, MAX_LINE_LEN); idx++;
							tlen+=strlen(tempstr);
							if(tlen>(BUFFER_SIZE-1024)) break;

							cellRtcGetCurrentTick(&pTick);
						}
//////////////////////////////
						if(subfolder) goto next_html_entry;
//////////////////////////////
					}
				}

				if(!is_net) cellFsClosedir(fd);
				if(data2) sys_memory_free(data2);

//
	continue_reading_folder_html:
				if((uprofile>0) && (f1<9)) {subfolder=uprofile=0; goto read_folder_html;}
				if(is_net && ls && li<27) {li++; goto subfolder_letter_html;}
//
			}
			if(is_net && ns>=0) {shutdown(ns, SHUT_RDWR); socketclose(ns); ns=-2;}
		}


		if(idx)
		{   // sort html game items
			u16 n, m;
			for(n=0; n<(idx-1); n++)
				for(m=(n+1); m<idx; m++)
					if(strcasecmp(line_entry[n].path, line_entry[m].path)>0)
					{
						strcpy(swap, line_entry[n].path);
						strcpy(line_entry[n].path, line_entry[m].path);
						strcpy(line_entry[m].path, swap);
					}
		}

		if(mobile_mode)
			sprintf(buffer, "slides = [");
		else
			{sprintf(templn, "<HR title=\"%i %s\">", idx, (strstr(param, "DI")!=NULL) ? STR_FILES : STR_GAMES); strcat(buffer, templn);}

		for(u16 m=0;m<idx;m++)
		{
			strcat(buffer, (line_entry[m].path)+4);
			if(strlen(buffer)>(BUFFER_SIZE-1024)) break;
		}

		//if(sysmem_html) sys_memory_free(sysmem_html);
		loading_games = 0;

		if(mobile_mode)
		{
			strcat(buffer, "];");
			savefile((char*)GAMELIST_JS, (char*)(buffer), strlen(buffer));
		}
		else
			savefile((char*)WMTMP "/games.html", (char*)(buffer+buf_len), (strlen(buffer)-buf_len));
	}
	return true;
}

static bool folder_listing(char *buffer, char *templn, char *param, int conn_s, char *tempstr, char *header)
{
	struct CellFsStat buf;
	int fd;

	CellRtcDateTime rDate;

	if(strstr(param, "/dev_blind?"))
	{
		if(strstr(param, "?1")) enable_dev_blind(NULL);
		if(strstr(param, "?0")) {system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}

		sprintf(templn, "/dev_blind: %s", isDir("/dev_blind")?STR_ENABLED:STR_DISABLED); strcat(buffer, templn); return true; //goto send_response;
	}

	absPath(templn, param, "/"); // auto mount /dev_blind

	u8 is_net = (param[1]=='n');

	if(is_net || cellFsOpendir(param, &fd) == CELL_FS_SUCCEEDED)
	{
		CellFsDirent entry;
		u64 read_e;
		unsigned long long sz=0, dir_size=0;
		char sf[8];
		char fsize[LINELEN];
		char ename[8];
		char swap[MAX_PATH_LEN];
		u16 idx=0, dirs=0, flen; bool is_dir;
		u32 tlen=strlen(buffer); buffer[tlen]=0;
		char *sysmem_html=buffer+_8KB_;

		typedef struct
		{
			char 	path[LINELEN];
		}
		t_line_entries;
		t_line_entries *line_entry = (t_line_entries *)sysmem_html;
		u16 max_entries=(((BUFFER_SIZE_ALL-_8KB_))/MAX_LINE_LEN)-1; tlen=0;

		strcat(buffer, "<table class=\"propfont\"><tr><td>");

		// breadcrumb trail
		strcpy(templn, param);
		while(strchr(templn+1, '/'))
        {
			templn[strchr(templn+1, '/')-templn]=0;
			tlen+=strlen(templn)+1;

			strcpy(swap, param);
			swap[tlen]=0;

			strcat(buffer, "<a class=\"f\" href=\"");
			strcat(buffer, swap);

			sprintf(swap, "\">%s</a>/", templn);
			strcat(buffer, swap);
			strcpy(templn, param+tlen);
		}
		sprintf(swap, "<a href=\"/mount.ps3%s\">%s</a>", param, templn); strcat(buffer, swap);

        strcat(buffer, ":</td><td width=90>&nbsp;</td><td></td></tr>");
		tlen=0;

 #ifdef COBRA_ONLY
  #ifndef LITE_EDITION
		if(is_net)
		{
			int ns=FAILED;
			int abort_connection=0;
			if(param[4]=='0') ns=connect_to_server(webman_config->neth0, webman_config->netp0); else
			if(param[4]=='1') ns=connect_to_server(webman_config->neth1, webman_config->netp1); else
			if(param[4]=='2') ns=connect_to_server(webman_config->neth2, webman_config->netp2);
			if(ns>=0)
			{
				strcat(param, "/");
				if(open_remote_dir(ns, param+5, &abort_connection)>=0)
				{
					strcpy(templn, param); if(templn[strlen(templn)-1]=='/') templn[strlen(templn)-1]=0;
					if(strrchr(templn, '/')) templn[strrchr(templn, '/')-templn]=0; if(strlen(templn)<6 && strlen(param)<8) {templn[0]='/'; templn[1]=0;}
					sprintf(tempstr, "!00000<tr><td><a class=\"f\" href=\"%s\">..</a></td><td> <a href=\"%s\">%s</a> &nbsp; </td><td>11-Nov-2006 11:11</td></tr>", templn, templn, HTML_DIR);

					if(strlen(tempstr)>MAX_LINE_LEN) return false; //ignore lines too long
					strncpy(line_entry[idx].path, tempstr, LINELEN); idx++; dirs++;
					tlen+=strlen(tempstr);

					sys_addr_t data2=0;
					netiso_read_dir_result_data *data=NULL;
					int v3_entries=0;
					v3_entries = read_remote_dir(ns, &data2, &abort_connection);
					if(data2!=NULL)
					{
						data=(netiso_read_dir_result_data*)data2;

						for(int n=0;n<v3_entries;n++)
						{
							if(data[n].name[0]=='.' && data[n].name[1]==0) continue;
							if(tlen>(BUFFER_SIZE-1024)) break;
							if(idx>=(max_entries-3)) break;

							if(param[1]==0)
								sprintf(templn, "/%s", data[n].name);
							else
							{
								sprintf(templn, "%s%s", param, data[n].name);
							}
							flen=strlen(templn)-1; if(templn[flen]=='/') templn[flen]=0;

							cellRtcSetTime_t(&rDate, data[n].mtime);

							sz=(unsigned long long)data[n].file_size; dir_size+=sz;

							is_dir=data[n].is_directory; if(is_dir) dirs++;

							add_list_entry(tempstr, is_dir, ename, templn, data[n].name, fsize, rDate, flen, sz, sf, true);

							if(strlen(tempstr)>MAX_LINE_LEN) continue; //ignore lines too long
							strncpy(line_entry[idx].path, tempstr, LINELEN); idx++;
							tlen+=strlen(tempstr);

							if(!working) break;
						}
						sys_memory_free(data2);
					}
				}
				else //may be a file
				{
					flen=strlen(param)-1; if(param[flen]=='/') param[flen]=0;

					int is_directory=0;
					int64_t file_size;
					u64 mtime, ctime, atime;
					if(remote_stat(ns, param+5, &is_directory, &file_size, &mtime, &ctime, &atime, &abort_connection)==0)
					{
						if(file_size && !is_directory)
						{
							if(open_remote_file_2(ns, param+5, &abort_connection)>0)
							{
								prepare_header(header, param, 1);
								sprintf(templn, "Content-Length: %llu\r\n\r\n", (unsigned long long)file_size); strcat(header, templn);

								ssend(conn_s, header);
								int bytes_read, boff=0;
								while(boff<file_size)
								{
									bytes_read = read_remote_file(ns, (char*)tempstr, boff, 4000, &abort_connection);
									if(bytes_read)
									{
										if(send(conn_s, tempstr, bytes_read, 0)<0) break;
									}
									boff+=bytes_read;
									if(bytes_read<4000 || boff>=file_size) break;
								}
								open_remote_file_2(ns, (char*)"/CLOSEFILE", &abort_connection);
								shutdown(ns, SHUT_RDWR); socketclose(ns);
								sclose(&conn_s);
								return false;
							}
						}
					}
				}
				shutdown(ns, SHUT_RDWR); socketclose(ns);
			}
		}
		else
  #endif
 #endif
		{
			while(cellFsReaddir(fd, &entry, &read_e) == 0 && read_e > 0)
			{
				if(entry.d_name[0]=='.' && entry.d_name[1]==0) continue;
				if(tlen>(BUFFER_SIZE-1024)) break;
				if(idx>=(max_entries-3)) break;

				if(param[1]==0)
					sprintf(templn, "/%s", entry.d_name);
				else
				{
					sprintf(templn, "%s/%s", param, entry.d_name);
				}
				flen = strlen(templn)-1; if(templn[flen]=='/') templn[flen]=0;

				cellFsStat(templn, &buf);
				cellRtcSetTime_t(&rDate, buf.st_mtime);

				sz=(unsigned long long)buf.st_size; dir_size+=sz;

				is_dir=(buf.st_mode & S_IFDIR); if(is_dir) dirs++;

				add_list_entry(tempstr, is_dir, ename, templn, entry.d_name, fsize, rDate, flen, sz, sf, false);

				if(strlen(tempstr)>MAX_LINE_LEN) continue; //ignore lines too long
				strncpy(line_entry[idx].path, tempstr, LINELEN); idx++;
				tlen+=flen;

				if(!working) break;
			}
			cellFsClosedir(fd);
		}

		if(strlen(param)<4)
		{
			for(u8 n=0; n<3; n++)
			if( (n==0 && (webman_config->netd0 && webman_config->neth0[0] && webman_config->netp0)) ||
				(n==1 && (webman_config->netd1 && webman_config->neth1[0] && webman_config->netp1)) ||
				(n==2 && (webman_config->netd2 && webman_config->neth2[0] && webman_config->netp2)) )
			{
				sprintf(tempstr, "0net%i <tr>"
										"<td><a class=\"d\" href=\"/net%i\">net%i (%s:%i)</a></td>"
										"<td> <a href=\"/mount.ps3/net%i\">%s</a> &nbsp; </td><td>11-Nov-2006 11:11</td>"
										"</tr>", n, n, n, n==1 ? webman_config->neth1 : n==2 ? webman_config->neth2 : webman_config->neth0,
														  n==1 ? webman_config->netp1 : n==2 ? webman_config->netp2 : webman_config->netp0, n, HTML_DIR);
				strncpy(line_entry[idx].path, tempstr, LINELEN); idx++;
				tlen+=strlen(tempstr);
			}
		}

		//sclose(&data_s);
		if(idx)
		{   // sort html file entries
			u16 n, m;
			for(n=0; n<(idx-1); n++)
				for(m=(n+1); m<idx; m++)
					if(strcasecmp(line_entry[n].path, line_entry[m].path)>0)
					{
						strcpy(swap, line_entry[n].path);
						strcpy(line_entry[n].path, line_entry[m].path);
						strcpy(line_entry[m].path, swap);
					}
		}

		for(u16 m=0;m<idx;m++)
		{
			strcat(buffer, (line_entry[m].path)+6);
			if(strlen(buffer)>(BUFFER_SIZE-1024)) break;
		}

		//if(sysmem_html) sys_memory_free(sysmem_html);
		strcat(buffer, "</table>");

		if(strlen(param)>4)
		{
			uint32_t blockSize;
			uint64_t freeSize;
			if(strchr(param+1, '/'))
				param[strchr(param+1, '/')-param]=0;

			if(param[1]=='n')
				sprintf(templn, "<hr>"
								"<b><a href=\"%s\">%s</a>:", param, param);
			else
			{
				cellFsGetFreeSize(param, &blockSize, &freeSize);
				sprintf(templn, "<hr>"
								"<b><a href=\"%s\">%s</a>: %i %s",
								param, param, (int)((blockSize*freeSize)>>20), STR_MBFREE);
			}
			strcat(buffer, templn);

			sprintf(templn, "</b> &nbsp; <font color=\"#707070\">%i Dir(s) %i %s %i %s</font><br>",
							(dirs-1), (idx-dirs), STR_FILES,
							dir_size<(_1MB_)?(int)(dir_size>>10):(int)(dir_size>>20),
							dir_size<(_1MB_)?STR_KILOBYTE:STR_MEGABYTE);
			strcat(buffer, templn);
		}
		else
			strcat(buffer, "<hr>webMAN - Simple Web Server" EDITION "<br>");
	}
	return true;
}

#ifdef PS3MAPI
static void ps3mapi_buzzer(char *buffer, char *templn, char *param);
static void ps3mapi_led(char *buffer, char *templn, char *param);
static void ps3mapi_notify(char *buffer, char *templn, char *param);
static void ps3mapi_syscall(char *buffer, char *templn, char *param);
static void ps3mapi_syscall8(char *buffer, char *templn, char *param);
static void ps3mapi_setidps(char *buffer, char *templn, char *param);
static void ps3mapi_getmem(char *buffer, char *templn, char *param);
static void ps3mapi_setmem(char *buffer, char *templn, char *param);
static void ps3mapi_vshplugin(char *buffer, char *templn, char *param);

static void ps3mapi_home(char *buffer, char *templn)
{
	int syscall8_state = -1;
	{system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); syscall8_state = (int)p1;}
	int version = 0;
	if (syscall8_state>=0) {system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); version = (int)(p1);}
	int versionfw = 0;
	if (syscall8_state>=0) {system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION); versionfw = (int)(p1);}
	char fwtype[32];
	if (syscall8_state>=0) {system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)fwtype);}

	//---------------------------------------------
	//PS3 Commands---------------------------------
	//---------------------------------------------
	sprintf(templn, "<b>%s</b>"
					"<hr color=\"#0099FF\"/>"
					"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr>", "PS3 Commands");
	strcat(buffer, templn);

	//RingBuzzer
	ps3mapi_buzzer(buffer, templn, (char*)" ");

	//LedRed
	ps3mapi_led(buffer, templn, (char*)" ");

	sprintf(templn, "%s", "</td>");
	strcat(buffer, templn);

	//Notify
    ps3mapi_notify(buffer, templn, (char*)" ");

	if (syscall8_state>=0 && syscall8_state<3 )
	{
        ps3mapi_syscall(buffer, templn, (char*)" ");
	}
	if (syscall8_state>=0)
	{
		//Syscall8
		ps3mapi_syscall8(buffer, templn, (char*)" ");
	}
	if (syscall8_state>=0 && syscall8_state<3 )
	{
		//IDPS/PSID
		if (version >= 0x0120 )
		{
            ps3mapi_setidps(buffer, templn, (char*)" ");
		}
		else
		{
			sprintf(templn, "%s", "</table><br>");
			strcat(buffer, templn);
		}
		//---------------------------------------------
		//Process Commands-----------------------------
		//---------------------------------------------
		//GetMem
		ps3mapi_getmem(buffer, templn, (char*)" ");

		//SetMem
		ps3mapi_setmem(buffer, templn, (char*)" ");

		//---------------------------------------------
		//VSH Plugin-----------------------------------
		//---------------------------------------------
		ps3mapi_vshplugin(buffer, templn, (char*)" ");

		sprintf(templn, "<hr color=\"#FF0000\"/>"
						"Firmware: %X %s | PS3MAPI: webUI v%X, Server v%X, Core v%X | By NzV", versionfw, fwtype, PS3MAPI_WEBUI_VERSION, PS3MAPI_SERVER_VERSION, version);
		strcat(buffer, templn);
	}
	else
	{
		sprintf(templn, "</table><br><hr color=\"#FF0000\"/>[SYSCALL8 %sDISABLED] | PS3MAPI: webUI v%X, Server v%X | By NzV", (syscall8_state==3)?"PARTIALY ":"", PS3MAPI_WEBUI_VERSION, PS3MAPI_SERVER_VERSION);
		strcat(buffer, templn);
	}
}

static void ps3mapi_buzzer(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	if(strstr(param, "buzzer.ps3mapi?"))
	{
		if(strstr(param, "mode=1")) { BEEP1 } else
		if(strstr(param, "mode=2")) { BEEP2 } else
		if(strstr(param, "mode=3")) { BEEP3 }
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						"<hr color=\"#0099FF\"/>", "PS3MAPI", "PS3 Commands", "Buzzer");
    else
		sprintf(templn, "<td width=\"260\" style=\"text-align:left; float:left;\"><u>%s:</u><br>", "Buzzer");
	strcat(buffer, templn);

	sprintf(templn, "<form id=\"buzzer\" action=\"/buzzer.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\"><br>"
					"<b>%s:</b>  <select name=\"mode\">", "Mode");
	strcat(buffer, templn);
	add_option_item("1" , "Simple", strstr(param, "mode=1"), buffer);
	add_option_item("2" , "Double", strstr(param, "mode=2"), buffer);
	add_option_item("3" , "Triple", strstr(param, "mode=3"), buffer);
	sprintf(templn, "</select>   <input type=\"submit\" value=\" %s \"/></td></form><br>", "Ring");
	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>"); else strcat(templn, "</td>");
	strcat(buffer, templn);
}

static void ps3mapi_led(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	if(strstr(param, "led.ps3mapi?"))
	{
		int color=0, mode=OFF;
		if(strstr(param, "color=0")) color = RED; else
		if(strstr(param, "color=1")) color = GREEN; else
		if(strstr(param, "color=2")) color = RED+GREEN; //YELLOW

		if(strstr(param, "mode=1")) mode = ON; else
		if(strstr(param, "mode=2")) mode = BLINK_FAST; else
		if(strstr(param, "mode=3")) mode = BLINK_SLOW;

		if(color & RED)   { system_call_2(SC_SYS_CONTROL_LED, RED  , mode); }
		if(color & GREEN) { system_call_2(SC_SYS_CONTROL_LED, GREEN, mode); }
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						"<hr color=\"#0099FF\"/>", "PS3MAPI", "PS3 Commands", "Led");
    else
		sprintf(templn, "<td width=\"260\" style=\"text-align:left; float:left;\"><u>%s:</u><br>", "Led");

	strcat(buffer, templn);

	sprintf(templn, "<form id=\"led\" action=\"/led.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\"><br>"
					"<b>%s:</b>  <select name=\"color\">",
					"Color");
	strcat(buffer, templn);

	add_option_item("0" , "Red", strstr(param, "color=0"), buffer);
	add_option_item("1" , "Green", strstr(param, "color=1"), buffer);
	add_option_item("2" , "Yellow (Red+Green)", strstr(param, "color=2"), buffer);
	sprintf(templn, "</select>   <b>%s:</b>  <select name=\"mode\">", "Mode");
	strcat(buffer, templn);
	add_option_item("0" , "Off", strstr(param, "mode=0"), buffer);
	add_option_item("1" , "On", strstr(param, "mode=1"), buffer);
	add_option_item("2" , "Blink fast", strstr(param, "mode=2"), buffer);
	add_option_item("3" , "Blink slow", strstr(param, "mode=3"), buffer);
	sprintf(templn, "</select>   <input type=\"submit\" value=\" %s \"/></form><br>", "Set");
	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>"); else strcat(templn, "</table></td>");
	strcat(buffer, templn);
}

static void ps3mapi_notify(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	char msg[200] = "Hello :)";
	if(strstr(param, "notify.ps3mapi?"))
	{
		char *pos;
		pos=strstr(param, "msg=");
		if(pos)
		{
			get_value(msg, pos + 4, 199);
			show_msg(msg);
		}
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						"<hr color=\"#0099FF\"/>", "PS3MAPI", "PS3 Commands", "Notify");
	else
		sprintf(templn, "<tr><td style=\"text-align:left; float:left;\"><br><u>%s:</u><br><br>", "Notify");

	strcat(buffer, templn);

	sprintf(templn, "<form action=\"/notify.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\">"
					"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr><td style=\"text-align:left; float:left;\">"
					"<textarea name=\"msg\" cols=\"111\" rows=\"2\" maxlength=\"199\">%s</textarea></td></tr>"
					"<tr><td style=\"text-align:right; float:right;\"><br><input type=\"submit\" value=\" %s \"/></td></tr></table></form>", msg, "Send");

	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>"); else strcat(templn, "</td>");
	strcat(buffer, templn);
}

static void ps3mapi_syscall(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	if(strstr(param, "syscall.ps3mapi?"))
	{
		if(strstr(param, "sc9=1"))  { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 9);  }
		if(strstr(param, "sc10=1")) { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 10); }
		if(strstr(param, "sc11=1")) { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 11); }
		if(strstr(param, "sc35=1")) { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 35); }
		if(strstr(param, "sc36=1")) { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 36); }
		if(strstr(param, "sc6=1"))  { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 6);  }
		if(strstr(param, "sc7=1"))  { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 7);  }
	}

	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						"<hr color=\"#0099FF\"/>"
						"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">", "PS3MAPI", "PS3 Commands", "CFW syscall");
	else
		sprintf(templn, "<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
						"<tr><td style=\"text-align:left; float:left;\"><u>%s:</u><br><br></td></tr>", "CFW syscall");

	strcat(buffer, templn);

	sprintf(templn, "<form id=\"syscall\" action=\"/syscall.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\">"
					"<br><tr><td width=\"260\" style=\"text-align:left; float:left;\">");
	strcat(buffer, templn);

	int ret_val = -1;

    { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 6); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc6", "1\" disabled=\"disabled", "[6]LV2 Peek", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc6", "1", "[6]LV2 Peek", NULL, false, buffer);}

    { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 7); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc7", "1\" disabled=\"disabled", "[7]LV2 Poke", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc7", "1", "[7]LV2 Poke", NULL, false, buffer);}

    strcat(buffer, "</td><td  width=\"260\"  valign=\"top\" style=\"text-align:left; float:left;\">");

    { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 9); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc9", "1\" disabled=\"disabled", "[9]LV1 Poke", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc9", "1", "[9]LV1 Poke", NULL, false, buffer);}

    { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 10); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc10", "1\" disabled=\"disabled", "[10]LV1 Call", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc10", "1", "[10]LV1 Call", NULL, false, buffer);}

    { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 11); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc11", "1\" disabled=\"disabled", "[11]LV1 Peek", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc11", "1", "[11]LV1 Peek", NULL, false, buffer);}

    strcat(buffer, "</td><td  width=\"260\"  valign=\"top\" style=\"text-align:left; float:left;\">");

    { system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 35); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc35", "1\" disabled=\"disabled", "[35]Map Path", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc35", "1", "[35]Map Path", NULL, false, buffer);}

	{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, 36); ret_val = (int)p1;}
	if( ret_val != 0 )  add_check_box("sc36", "1\" disabled=\"disabled", "[36]Map Game", NULL, true, buffer);
	else {ret_val = -1; add_check_box("sc36", "1", "[36]Map Game", NULL, false, buffer);}

	sprintf(templn, "</td></tr><tr><td style=\"text-align:right; float:right;\"><br><input type=\"submit\" value=\" %s \"/></td></tr></form></table><br>", "Disable");
	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>");
	strcat(buffer, templn);
}

static void ps3mapi_syscall8(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	if(strstr(param, "syscall8.ps3mapi?"))
	{
		if(strstr(param, "mode=0")) {{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 0); }}
		if(strstr(param, "mode=1")) {{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 1); }}
		if(strstr(param, "mode=2")) {{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 2); }}
		if(strstr(param, "mode=3")) {{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, 3); }}
		if(strstr(param, "mode=4")) {{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, 8); }}
	}

	sprintf(templn, "<b>%s%s --> %s</b>"
					"<hr color=\"#0099FF\"/>"
					"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
					"<form id=\"syscall8\" action=\"/syscall8.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\">"
					"<br><tr><td style=\"text-align:left; float:left;\">",
					is_ps3mapi_home ? "" : "PS3MAPI --> ", "PS3 Commands", "CFW syscall 8");
	strcat(buffer, templn);
	int ret_val = -1;
	{ system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); ret_val = (int)p1;}
	if(ret_val < 0)
	{
		add_radio_button("mode\" disabled=\"disabled", "0", "sc8_0", "Fully enabled", NULL, false, buffer);
		add_radio_button("mode\" disabled=\"disabled", "1", "sc8_1", "Partially disabled : Keep only COBRA/MAMBA/PS3MAPI features", NULL, false, buffer);
		add_radio_button("mode\" disabled=\"disabled", "2", "sc8_2", "Partially disabled : Keep only PS3MAPI features", NULL, false, buffer);
		add_radio_button("mode\" disabled=\"disabled", "3", "sc8_3", "Fake disabled (can be re-enabled)", NULL, false, buffer);
		add_radio_button("mode\" disabled=\"disabled", "4", "sc8_4", "Fully disabled (cant be re-enabled)", NULL, true, buffer);
	}
	else
	{
		add_radio_button("mode", "0", "sc8_0", "Fully enabled", NULL, (ret_val == 0), buffer);
		add_radio_button("mode", "1", "sc8_1", "Partially disabled : Keep only COBRA/MAMBA/PS3MAPI features", NULL, (ret_val == 1), buffer);
		add_radio_button("mode", "2", "sc8_2", "Partially disabled : Keep only PS3MAPI features", NULL, (ret_val == 2), buffer);
		add_radio_button("mode", "3", "sc8_3", "Fake disabled (can be re-enabled)", NULL, (ret_val == 3), buffer);
		add_radio_button("mode", "4", "sc8_4", "Fully disabled (cant be re-enabled)", NULL, false, buffer);
	}
	sprintf(templn, "</td></tr><tr><td style=\"text-align:right; float:right;\"><br><input type=\"submit\" value=\" %s \"/></td></tr></form></table><br>", "Set");
	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>");
	strcat(buffer, templn);
}

static void ps3mapi_getmem(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	u32 pid = 0;
	u64 address = 0;
	int length = 0;
	if(strstr(param, "getmem.ps3mapi?"))
	{
		char *pos;
		pos=strstr(param, "addr=");
		if(pos)
		{
			char addr_tmp[40];
			get_value(addr_tmp, pos + 5, 39);
			address = convertH(addr_tmp);
		}
		else goto getmem_err_arg;
		pos=strstr(param, "len=");
		if(pos)
		{
			char len_tmp[20];
			get_value(len_tmp, pos + 4, 19);
			length = val(len_tmp);
			length = RANGE(length, 0, 2048);
		}
		else goto getmem_err_arg;
		pos=strstr(param, "proc=");
		if(pos)
		{
			char pid_tmp[20];
			get_value(pid_tmp, pos + 5, 19);
			pid = val(pid_tmp);
		}
		else goto getmem_err_arg;
	}

 getmem_err_arg:
	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						"<hr color=\"#0099FF\"/>",
						"PS3MAPI", "Processes Commands", "Get process memory");
	else
		sprintf(templn, "<b>%s</b><hr color=\"#0099FF\"/>", "Processes Commands");

	strcat(buffer, templn);

	sprintf(templn, "<form action=\"/getmem.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\"><br>"
					"<b><u>%s:</u></b>  ", "Process");
	strcat(buffer, templn); memset(templn, 0, MAX_LINE_LEN);
	if(pid == 0 )
	{
		strcat(buffer, "<select name=\"proc\">");
		char pid_str[32];
		u32 pid_list[16];
		{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
		for(int i = 0; i < 16; i++)
		{
			if(1 < pid_list[i])
			{
				memset(templn, 0, MAX_LINE_LEN);
				memset(pid_str, 0, sizeof(pid_str));
				{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)(u32)pid_list[i], (u64)(u32)templn); }
				sprintf(pid_str, "%i", pid_list[i]);
				if(1 < strlen(templn))add_option_item(pid_str , templn, true, buffer);
			}
		}
		strcat(buffer, "</select>   ");
	}
	else
	{
		memset(templn, 0, MAX_LINE_LEN);
		{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)templn); }
		strcat(buffer, templn);
		sprintf(templn, "<input name=\"proc\" type=\"hidden\" value=\"%u\"><br><br>", pid);
		strcat(buffer, templn);
	}
	sprintf(templn, "<b><u>%s:</u></b> " HTML_INPUT("addr", "%llX", "16", "18")
					"   <b><u>%s:</u></b> <input name=\"len\" type=\"number\" value=\"%i\" min=\"1\" max=\"2048\">"
					"   <input type=\"submit\" value=\" %s \"/></form>", "Address", address, "Length", length, "Get");
	strcat(buffer, templn);

	if(pid != 0  && length != 0)
	{
		sprintf(templn, "<br><br><b><u>%s:</u></b><br><br><textarea name=\"output\" cols=\"111\" rows=\"10\" readonly=\"true\">", "Output");
	    strcat(buffer, templn);
		char buffer_tmp[length + 1];
		memset(buffer_tmp, 0, sizeof(buffer_tmp));
		int retval = -1;
		{system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)buffer_tmp, (u64)length); retval = (int)p1;}
		if(0 <= retval)
		{
			for(int i = 0; i < length; i++)
			{
				sprintf(templn, "%02X", (uint8_t)buffer_tmp[i]);
				strcat(buffer, templn);
			}
		}
		else {sprintf(templn, "%s: %i", "Error", retval); strcat(buffer, templn);}
		strcat(buffer, "</textarea><br>");
	}

	if(!is_ps3mapi_home) strcat(buffer, "<br><hr color=\"#FF0000\"/>"); else strcat(buffer, "<br>");
	strcat(buffer, "Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]");
	if(!is_ps3mapi_home) {sprintf(templn, " [<a href=\"/dump.ps3?%llx\">LV1 Dump 0x%llx</a>] [<a href=\"/peek.lv1?%llx\">LV1 Peek 0x%llx</a>]", address, address, address, address); strcat(buffer, templn);}
	strcat(buffer, "<p>");
}

static void ps3mapi_setmem(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	u32 pid = 0;
	u64 address = 0;
	int length = 0;
	char value[130];
	char val_tmp[260];
	if(strstr(param, "setmem.ps3mapi?"))
	{
		char *pos;
		pos=strstr(param, "addr=");
		if(pos)
		{
			char addr_tmp[40];
			get_value(addr_tmp, pos + 5, 39);
			address = convertH(addr_tmp);
		}
		else goto setmem_err_arg;
		pos=strstr(param, "val=");
		if(pos)
		{
			get_value(val_tmp, pos + 4, 259);
			length = (strlen(val_tmp) / 2);
			Hex2Bin(val_tmp, value);
		}
		else goto setmem_err_arg;
		pos=strstr(param, "proc=");
		if(pos)
		{
			char pid_tmp[20];
			get_value(pid_tmp, pos + 5, 19);
			pid = val(pid_tmp);
		}
		else goto setmem_err_arg;
	}

 setmem_err_arg:
	if(!is_ps3mapi_home)
		sprintf(templn, "<b>%s --> %s --> %s</b>"
						"<hr color=\"#0099FF\"/>",
						"PS3MAPI", "Processes Commands", "Set process memory");
	else
		sprintf(templn, "<u>%s:</u>", "Set process memory");

	strcat(buffer, templn);

	sprintf(templn, "<form action=\"/setmem.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\">"
					"<b><u>%s:</u></b>  ", "Process");
	strcat(buffer, templn); memset(templn, 0, MAX_LINE_LEN);
	if(pid == 0 )
	{
		strcat(buffer, "<select name=\"proc\">");
		char pid_str[32];
		u32 pid_list[16];
		{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
		for(int i = 0; i < 16; i++)
		{
			if(1 < pid_list[i])
			{
				memset(templn, 0, MAX_LINE_LEN);
				memset(pid_str, 0, sizeof(pid_str));
				{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid_list[i], (u64)(u32)templn); }
				sprintf(pid_str, "%i", pid_list[i]);
				if(1 < strlen(templn))add_option_item(pid_str , templn, true, buffer);
			}
		}
		strcat(buffer, "</select>   ");
	}
	else
	{
		memset(templn, 0, MAX_LINE_LEN);
		{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)templn); }
		strcat(buffer, templn);
		sprintf(templn, "<input name=\"proc\" type=\"hidden\" value=\"%u\"><br><br>", pid);
		strcat(buffer, templn);
	}
	if(strlen(val_tmp) < 1) sprintf(val_tmp, "%02X", 0);
	sprintf(templn, "<b><u>%s:</u></b> "  HTML_INPUT("addr", "%llX", "16", "18")
					"<br><br><b><u>%s:</u></b><br><br>"
					"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
					"<tr><td style=\"text-align:left; float:left;\">"
					"<textarea name=\"val\" cols=\"111\" rows=\"3\" maxlength=\"199\">%s</textarea></td></tr>"
					"<tr><td style=\"text-align:right; float:right;\"><br>"
					"<input type=\"submit\" value=\" %s \"/></td></tr></table></form>", "Address", address, "Value", val_tmp, "Set");
	strcat(buffer, templn);
	if(pid != 0 && length != 0)
	{
		int retval = -1;
		{system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)pid, (u64)address, (u64)(u32)value, (u64)length); retval = (int)p1;}
		if(0 <= retval) sprintf(templn, "<br><b><u>%s!</u></b>", "Done");
		else sprintf(templn, "<br><b><u>%s: %i</u></b>", "Error", retval);
		strcat(buffer, templn);
	}

	if(!is_ps3mapi_home) strcat(buffer, "<br><hr color=\"#FF0000\"/>"); else strcat(buffer, "<br>");
}

static void ps3mapi_setidps(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)IDPS);}
	{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)PSID);}
	u64 _new_IDPS[2] = { IDPS[0], IDPS[1]};
	u64 _new_PSID[2] = { PSID[0], PSID[1]};

	if(strstr(param, "setidps.ps3mapi?"))
	{
		char *pos;
		pos=strstr(param, "idps1=");
		if(pos)
		{
			char idps1_tmp[17];
			get_value(idps1_tmp, pos + 6, 16);
			_new_IDPS[0] = convertH(idps1_tmp);
		}
		else goto setidps_err_arg1;
		pos=strstr(param, "idps2=");
		if(pos)
		{
			char idps2_tmp[17];
			get_value(idps2_tmp, pos + 6, 16);
			_new_IDPS[1] = convertH(idps2_tmp);
		}
		else goto setidps_err_arg1;
		{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, (u64)_new_IDPS[0], (u64)_new_IDPS[1]);}

	 setidps_err_arg1:

		pos=strstr(param, "psid1=");
		if(pos)
		{
			char psid1_tmp[17];
			get_value(psid1_tmp, pos + 6, 16);
			_new_PSID[0] = convertH(psid1_tmp);
		}
		else goto setidps_err_arg2;
		pos=strstr(param, "psid2=");
		if(pos)
		{
			char psid2_tmp[17];
			get_value(psid2_tmp, pos + 6, 16);
			_new_PSID[1] = convertH(psid2_tmp);
		}
		else goto setidps_err_arg2;
		{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)_new_PSID[0], (u64)_new_PSID[1]);}
	}

 setidps_err_arg2:

	sprintf(templn, "<b>%s%s --> %s</b>"
					"<hr color=\"#0099FF\"/>"
					"<form action=\"/setidps.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\"><br>"
					"<table width=\"800\" border=\"0\" cellspacing=\"2\" cellpadding=\"0\"><tr><td width=\"400\" style=\"text-align:left; float:left;\">"
	                "<br><b><u>%s:</u></b><br>" HTML_INPUT("idps1", "%016llX", "16", "18") HTML_INPUT("idps2", "%016llX", "16", "18") "</td>"
					"<td style=\"text-align:left; float:left;\">"
	                "<br><b><u>%s:</u></b><br>" HTML_INPUT("psid1", "%016llX", "16", "18") HTML_INPUT("psid2", "%016llX", "16", "18") "</td></tr>"
	                "<tr><td style=\"text-align:right; float:right;\"><br><input type=\"submit\" value=\" %s \"/></td></tr></table></form><br>",
					is_ps3mapi_home ? "" : "PS3MAPI --> ", "PS3 Commands", "Set IDPS/PSID", "IDPS", _new_IDPS[0], _new_IDPS[1], "PSID", _new_PSID[0], _new_PSID[1], "Set");
	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>");
	strcat(buffer, templn);
}

static void ps3mapi_vshplugin(char *buffer, char *templn, char *param)
{
	bool is_ps3mapi_home = (param[0] == ' ');

	if(strstr(param, "vshplugin.ps3mapi?"))
	{
		char *pos;
		unsigned int uslot = 99;
		pos=strstr(param, "unload_slot=");
		if(pos)
		{
			char uslot_str[3];
			get_value(uslot_str, pos + 12, 2);
			uslot = val(uslot_str);
			if ( uslot ){{system_call_2(8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, (u64)uslot);}}
		}
		else
		{
			pos=strstr(param, "load_slot=");
			if(pos)
			{
				char uslot_str[3];
				get_value(uslot_str, pos + 10, 2);
				uslot = val(uslot_str);
			}
			else goto loadvshplug_err_arg;
			pos=strstr(param, "prx=");
			if(pos)
			{
				char prx_path[256];
				get_value(prx_path, pos + 4, 256);
				if ( uslot ){{system_call_5(8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, (u64)uslot, (u64)(u32)prx_path, NULL, 0);}}
			}
		}
	}

loadvshplug_err_arg:

	sprintf(templn, "<b>%s%s </b>"
					"<hr color=\"#0099FF\"/><br>"
					"<table border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
					"<tr><td width=\"75\" style=\"text-align:left; float:left;\">%s</td>"
					"<td width=\"100\" style=\"text-align:left; float:left;\">%s</td>"
					"<td width=\"500\" style=\"text-align:left; float:left;\">%s</td>"
					"<td width=\"125\" style=\"text-align:right; float:right;\"> </td></tr>",
					is_ps3mapi_home ? "" : "PS3MAPI --> ", "VSH Plugins", "Slot", "Name", "File name");

	strcat(buffer, templn);
	char tmp_name[30];
	char tmp_filename[256];
	for (unsigned int slot = 0; slot < 7; slot++)
	{
		memset(tmp_name, 0, sizeof(tmp_name));
		memset(tmp_filename, 0, sizeof(tmp_filename));
		{system_call_5(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)tmp_name, (u64)(u32)tmp_filename); }
		if (strlen(tmp_filename) > 0)
		{
			sprintf(templn, "<tr><td width=\"75\" style=\"text-align:left; float:left;\">%i</td>"
							"<td width=\"100\" style=\"text-align:left; float:left;\">%s</td>"
							"<td width=\"500\" style=\"text-align:left; float:left;\">%s</td>"
							"<td width=\"100\" style=\"text-align:right; float:right;\">"
							"<form action=\"/vshplugin.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\">"
							"<input name=\"unload_slot\" type=\"hidden\" value=\"%i\"><input type=\"submit\" %s/></form></td></tr>",
							slot, tmp_name, tmp_filename, slot, (slot) ? "value=\" Unload \"" : "value=\" Reserved \" disabled=\"disabled\"" );
		}
		else
 		{
			sprintf(templn, "<tr><td width=\"75\" style=\"text-align:left; float:left;\">%i</td>"
							"<td width=\"100\" style=\"text-align:left; float:left;\">%s</td>"
							"<form action=\"/vshplugin.ps3mapi\" method=\"get\" enctype=\"application/x-www-form-urlencoded\" target=\"_self\"><td width=\"500\" style=\"text-align:left; float:left;\">"
							HTML_INPUT("prx\" list=\"plugins", "/dev_hdd0/tmp/my_plugin_%i.sprx", "128", "75") "<input name=\"load_slot\" type=\"hidden\" value=\"%i\"></td>"
							"<td width=\"100\" style=\"text-align:right; float:right;\"><input type=\"submit\" %s/></td></form></tr>",
							slot, "NULL", slot, slot, (slot) ? "value=\" Load \"" : "value=\" Reserved \" disabled=\"disabled\"" );
		}
			strcat(buffer, templn);
	}


	//add plugins list
	{
		strcat(buffer, "<datalist id=\"plugins\">");
		int fd, cnt = 0; char paths[6][32] = {"/dev_hdd0", "/dev_hdd0/plugins", "/dev_usb000", "/dev_usb001", "/dev_hdd0/game/UPDWEBMOD/USRDIR", "/dev_hdd0/tmp"};

		for(u8 i = 0; i < 6; i++)
		if(cellFsOpendir(paths[i], &fd) == CELL_FS_SUCCEEDED)
		{
			CellFsDirent dir; u64 read = sizeof(CellFsDirent);

			while(!cellFsReaddir(fd, &dir, &read))
			{
				if(!read || cnt>50) break;
				if(strstr(dir.d_name, ".sprx"))
				{
					sprintf(templn, "<option>%s/%s</option>", paths[i], dir.d_name); strcat(buffer, templn); cnt++;
				}
			}
			cellFsClosedir(fd);
		}

		strcat(buffer, "</datalist>");
	}

	sprintf(templn, "%s", "</table><br>");
	if(!is_ps3mapi_home) strcat(templn, "<hr color=\"#FF0000\"/>");
	strcat(buffer, templn);
}

#endif

#ifdef DEBUG_MEM
static void ps3mapi_mem_dump(char *buffer, char *templn, char *param)
{
	char dump_file[MAX_PATH_LEN]; uint64_t start=0; uint32_t size=8;
	strcat(buffer, "Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]<hr>");

	if(strlen(param+10))
	{
		if(strstr(param,"?lv1")        ) {size=16;} else
		if(strstr(param,"?lv2")        ) {start=0x1000000;} else
		//if(strstr(param,"?v") /*vsh  */) {start=0x910000;}  else
		if(strstr(param,"?r") /*rsx  */) {start=0x0000028080000000ULL; size=256;}  else
		if(strstr(param,"?f") /*full */) {size=256;} else
		if(strstr(param,"?m") /*mem  */) {size=256;} else
		{
			start = convertH(param+10);
			if(start>=LV1_UPPER_MEMORY-((uint64_t)(size*_1MB_))) start=LV1_UPPER_MEMORY-((uint64_t)(size*_1MB_));
		}

		char *pos=strstr(param, "&size=");
		if(pos) size = convertH(pos+6);

		sprintf(dump_file, "/dev_hdd0/dump_%s.bin", param+10);
		dump_mem(dump_file, start, size);
		sprintf(templn, "<p>Dumped: <a href=\"%s\">%s</a> [<a href=\"/delete.ps3%s\">%s</a>]", dump_file, dump_file+10, dump_file, STR_DELETE); strcat(buffer, templn);
	}
}

static void ps3mapi_find_peek_poke(char *buffer, char *templn, char *param)
{
	uint64_t address, addr, fvalue, value=0, upper_memory=LV2_UPPER_MEMORY, found_address=0;
	u8 byte=0, p=0, lv1=0;
	bool bits8=false, bits16=false, bits32=false, found=false;
	u8 flen=0;
	char *v;

	v=strstr(param+10,"&");
	if(v) v=NULL;

	address = convertH(param+10);

	v=strstr(param+10, "=");
	if(v)
	{
		flen=strlen(v+1);
		for(p=1; p<=flen;p++) if(!memcmp(v+p," ",1)) byte++; //ignore spaces
		flen-=byte; byte=p=0;
	}

	bits32=(flen>4) && (flen<=8);
	bits16=(flen>2) && (flen<=4);
	bits8 =(flen<=2);

	strcat(buffer, "<pre>");

	address|=0x8000000000000000ULL;

	lv1=strstr(param,".lv1?")?1:0;
	upper_memory=(lv1?LV1_UPPER_MEMORY:LV2_UPPER_MEMORY)-8;

	if(v!=NULL && strstr(param, "find.lv") && (address<upper_memory))
	{
		uint64_t j;
		fvalue = convertH(v+1);

		if(bits8)  fvalue=(fvalue<<56);
		if(bits16) fvalue=(fvalue<<48);
		if(bits32) fvalue=(fvalue<<32);

		for(j = address; j < upper_memory-8; j++) {
			value = (lv1?peek_lv1(j):peekq(j));

			if(bits8 ) value&=0xff00000000000000ULL;
			if(bits16) value&=0xffff000000000000ULL;
			if(bits32) value&=0xffffffff00000000ULL;

			if(value==fvalue) {found=true; break;}
		}

		if(!found)
		{
			sprintf(templn, "<b><font color=red>%s</font></b><br>", STR_NOTFOUND); strcat(buffer, templn);
		}
		else
		{
			found_address=address=j;
			sprintf(templn, "Offset: 0x%X<br><br>", (u32)address); strcat(buffer, templn);
		}
	}
	else
	if(v!=NULL && strstr(param, "poke.lv2") && (address<upper_memory))
    {
		value  = convertH(v+1);
		fvalue = peekq(address);

		if(bits32) value = ((uint64_t)(value<<32) | (uint64_t)(fvalue & 0xffffffffULL));
		if(bits16) value = ((uint64_t)(value<<48) | (uint64_t)(fvalue & 0xffffffffffffULL));
		if(bits8)  value = ((uint64_t)(value<<56) | (uint64_t)(fvalue & 0xffffffffffffffULL));

		pokeq(address, value);
		found_address=address; found=true;
	}
	else
	if(v!=NULL && strstr(param, "poke.lv1") && (address<upper_memory))
    {
		value = convertH(v+1);
		fvalue = peek_lv1(address);

		if(bits32) value = ((uint64_t)(value<<32) | (uint64_t)(fvalue & 0xffffffffULL));
		if(bits16) value = ((uint64_t)(value<<48) | (uint64_t)(fvalue & 0xffffffffffffULL));
		if(bits8)  value = ((uint64_t)(value<<56) | (uint64_t)(fvalue & 0xffffffffffffffULL));

		poke_lv1(address, value);
		found_address=address; found=true;
	}

	if(address+0x200>(upper_memory+8)) address=0;

	flen=(bits8)?1:(bits16)?2:(bits32)?4:8;
	address&=0xFFFFFFFFFFFFFFF0ULL;
	addr=address;

	for(u16 i=0; i<0x200; i++)
	{
		if(!p)
		{
			sprintf(templn, "%X  ", (int)((address & 0xFFFFFFFFULL) +i));
			for(u8 c=10-strlen(templn);c>0;c--) strcat(buffer, "0");
			strcat(buffer, templn);
		}

		byte=(u8)((lv1?peek_lv1(address+i):peekq(address+i))>>56);

		if(found && address+i>=found_address && address+i<found_address+flen) strcat(buffer, "<font color=yellow><b>");
		sprintf(templn, byte<16?"0%X ":"%X ", byte); strcat(buffer, templn);
		if(found && address+i>=found_address && address+i<found_address+flen) strcat(buffer, "</b></font>");

		if(p==0x7) strcat(buffer, " ");

		if(p==0xF)
		{
			strcat(buffer, " ");
			for(u8 c=0;c<0x10;c++, addr++)
			{
				byte=(u8)((lv1?peek_lv1(addr):peekq(addr))>>56);
				if(byte<32 || byte>=127) byte='.';

				if(found && addr>=found_address && addr<found_address+flen) strcat(buffer, "<font color=yellow><b>");
				if(byte==0x3C)
					strcat(buffer, "&lt;");
				else if(byte==0x3E)
					strcat(buffer, "&gt;");
				else
					{sprintf(templn,"%c", byte); strcat(buffer, templn);}
				if(found && addr>=found_address && addr<found_address+flen) strcat(buffer, "</b></font>");
			}
			strcat(buffer, "<br>");
		}

		p++; if(p>=0x10) p=0;
	}
	strcat(buffer, "<hr>Dump: [<a href=\"/dump.ps3?mem\">Full Memory</a>] [<a href=\"/dump.ps3?lv1\">LV1</a>] [<a href=\"/dump.ps3?lv2\">LV2</a>]");
	sprintf(templn, " [<a href=\"/dump.ps3?%llx\">Dump 0x%llx</a>]", address, address); strcat(buffer, templn);
	sprintf(templn, " <a href=\"/peek.lv%i?%llx\">&lt;&lt;</a> <a href=\"/peek.lv%i?%llx\">&lt;Back</a>", lv1?1:2, ((int)(address-0x1000)>=0)?(address-0x1000):0, lv1?1:2, ((int)(address-0x200)>=0)?(address-0x200):0); strcat(buffer, templn);
	sprintf(templn, " <a href=\"/peek.lv%i?%llx\">Next&gt;</a> <a href=\"/peek.lv%i?%llx\">&gt;&gt;</a></pre>", lv1?1:2, ((int)(address+0x400)<(int)upper_memory)?(address+0x200):(upper_memory-0x200), lv1?1:2, ((int)(lv1+0x1200)<(int)upper_memory)?(address+0x1000):(upper_memory-0x200)); strcat(buffer, templn);
}
#endif

static void http_response(int conn_s, char *header, char *param, int code, char *msg)
{
	char text[MAX_LINE_LEN]; if(msg[0]=='/') sprintf(text, "%s : OK", msg+1); else sprintf(text, "%s", msg);

	sprintf(header, "HTTP/1.1 %i OK\r\n"
					"X-PS3-Info: [%s]\r\n"
					"Content-Type: text/html\r\n"
					"Content-Length: %i\r\n\r\n"
					"<body bgcolor=\"#101010\" text=\"#c0c0c0\">"
					"<font face=\"Courier New\">"
					"webMAN MOD " WM_VERSION "<hr><h2>%s</h2>"
					"</font></body>",
					 code, param, 113+strlen(text), text);

	ssend(conn_s, header);
	sclose(&conn_s);

	if(msg[0]=='/') {show_msg((char*)text); sys_timer_sleep(1); }
}

static void handleclient(u64 conn_s_p)
{
	int conn_s = (int)conn_s_p; // main communications socket


	char param[HTML_RECV_SIZE];
	struct CellFsStat buf;
	int fd;

	if(conn_s_p==START_DAEMON || conn_s_p==REFRESH_CONTENT)
	{

#ifndef ENGLISH_ONLY
		update_language();
#endif

		if(conn_s_p==START_DAEMON)
		{
			if(profile || (!(webman_config->wmdn) && strlen(STR_WMSTART)>0))
			{
				sys_timer_sleep(10);
				sprintf(param, "%s%s", STR_WMSTART, SUFIX2(profile));
				show_msg((char*)param);
			}
		}
		else //if(conn_s_p==REFRESH_CONTENT)
		{
			{DELETE_CACHED_GAMES} // refresh XML will force "refresh HTML" to rebuild the cache file
		}

		init_running = 1;

		cellFsMkdir((char*)WMTMP, MODE);

		//identify covers folders to be scanned
#ifndef ENGLISH_ONLY
													covers_exist[0]=isDir(COVERS_PATH);
#endif
		sprintf(param, "%s/covers", MM_ROOT_STD) ;	covers_exist[1]=isDir(param);
		sprintf(param, "%s/covers", MM_ROOT_STL) ;	covers_exist[2]=isDir(param);
		sprintf(param, "%s/covers", MM_ROOT_SSTL);	covers_exist[3]=isDir(param);
													covers_exist[4]=isDir("/dev_hdd0/GAMES/covers");
													covers_exist[5]=isDir("/dev_hdd0/GAMEZ/covers");
													covers_exist[6]=isDir(WMTMP);

		for(u8 i=0; i<12; i++)
		{
			if(cellFsStat(wm_icons[i], &buf)!=CELL_FS_SUCCEEDED)
			{
				sprintf(param, "/dev_flash/vsh/resource/explore/icon/%s", wm_icons[i] + 23); strcpy(wm_icons[i], param);
				if(cellFsStat(param, &buf)==CELL_FS_SUCCEEDED) continue;
				else
				if(i==0 || i==5) strcpy(wm_icons[i] + 32, "user/024.png\0"); else //ps3
				if(i==1 || i==6) strcpy(wm_icons[i] + 32, "user/026.png\0"); else //psx
				if(i==2 || i==7) strcpy(wm_icons[i] + 32, "user/025.png\0"); else //ps2
				if(i==3 || i==8) strcpy(wm_icons[i] + 32, "user/022.png\0"); else //psp
				if(i==4 || i==9) strcpy(wm_icons[i] + 32, "user/023.png\0"); else //dvd
								 strcpy(wm_icons[i] + 37, "icon_home.png\0"); //setup / eject
			}
		}

#ifdef NOSINGSTAR
		if(webman_config->noss) no_singstar_icon();
#endif

#ifdef COBRA_ONLY
		//if(cobra_mode)
		{
			u8 cconfig[15];
			CobraConfig *cobra_config = (CobraConfig*) cconfig;
			memset(cobra_config, 0, 15);
			cobra_read_config(cobra_config);

			if(webman_config->nospoof)
			{
				cobra_config->spoof_version=0;
				cobra_config->spoof_revision=0;
			}
			else
			{   // cobra spoofer not working on 4.53 & 4.65
    			if((c_firmware!=4.53f && c_firmware<4.65f))
				{
					cobra_config->spoof_version=0x0470;
					cobra_config->spoof_revision=64978;
				}
			}

			if( cobra_config->ps2softemu == 0 && cobra_get_ps2_emu_type()==PS2_EMU_SW )
				cobra_config->ps2softemu =  1;

			cobra_write_config(cobra_config);

		}
#endif

#ifdef SPOOF_CONSOLEID
		spoof_idps_psid();
#endif

#ifdef COBRA_ONLY
	#ifdef REMOVE_SYSCALLS
		if(webman_config->spp & 1) //remove syscalls & history
        {
			sys_timer_sleep(5);

			remove_cfw_syscalls();
			delete_history(true);
		}
		else
	#endif
		if(webman_config->spp & 2) //remove history only
		{
			delete_history(false);
			block_online_servers();
		}
#endif

		if(update_mygames_xml(conn_s_p)) mount_autoboot();

		init_running = 0;
		sys_ppu_thread_exit(0);
	}

#ifdef USE_DEBUG
	ssend(debug_s, "waiting...");
#endif
	if(loading_html>10) loading_html=0;
	//while((init_running/* || loading_html>3*/) && working) sys_timer_usleep(10000);

	sys_net_sockinfo_t conn_info_main;
	sys_net_get_sockinfo(conn_s, &conn_info_main, 1);

	char ip_address[16];
	sprintf(ip_address, "%s", inet_ntoa(conn_info_main.remote_adr));
	if( webman_config->bind && ((conn_info_main.local_adr.s_addr!=conn_info_main.remote_adr.s_addr) && strncmp(ip_address, webman_config->allow_ip, strlen(webman_config->allow_ip))!=0))
	{
		sclose(&conn_s);
		loading_html--;
		sys_ppu_thread_exit(0);
	}

	if(!webman_config->netd0 && !webman_config->neth0[0]) strcpy(webman_config->neth0, ip_address);
	if(!webman_config->bind) strcpy(webman_config->allow_ip, ip_address);

	set_buffer_sizes(webman_config->foot);

	_meminfo meminfo;
	u8 retries=0;
again3:
	{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}
	if((meminfo.avail)<( (_64KB_) + MIN_MEM)) //leave if less than min memory
	{
#ifdef USE_DEBUG
	ssend(debug_s, "!!! NOT ENOUGH MEMORY!\r\n");
#endif
		retries++;
		sys_timer_sleep(1);
		if(retries<5) goto again3;
		init_running = 0;
		sclose(&conn_s);
		loading_html--;
		sys_ppu_thread_exit(0);
	}

	sys_addr_t sysmem=0;

	u8 is_binary = 0, served=0;	// served http requests
	u64 c_len = 0;
	char cmd[16], header[HTML_RECV_SIZE];

	u8 is_ps3_http=0;
	u8 is_cpursx=0;
	u8 is_popup=0;

	struct timeval tv;
	tv.tv_usec = 0;
	tv.tv_sec = 3;
	setsockopt(conn_s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	while(!served && working)
	{
		served++;
		header[0]=0;

#ifdef USE_DEBUG
	ssend(debug_s, "ACC - ");
#endif
		if(recv(conn_s, header, HTML_RECV_SIZE, 0) > 0 && header[0]=='G' && header[4]=='/') // serve only GET /xxx requests
		{
			if(strstr(header, "Mozilla/5.0 (PLAYSTATION 3;")) is_ps3_http=1; else
			if(strstr(header, "Gecko/36")) is_ps3_http=2; else is_ps3_http=0;

			header[strcspn(header, "\n")] = '\0';
			header[strcspn(header, "\r")] = '\0';

			ssplit(header, cmd, 15, header, (HTML_RECV_SIZE-1));
			ssplit(header, param, (HTML_RECV_SIZE-1), cmd, 15);

			bool allow_retry_response=true, small_alloc = true, mobile_mode = false;

#ifdef USE_DEBUG
	ssend(debug_s, param);
	ssend(debug_s, "\r\n");
#endif
			//url decode (unescape)
			if(strstr(param, "%"))
			{
				strcpy(header, param);

				u16 pos=0, len=strlen(param);
				for(u16 i=0;i<len;i++, pos++)
				{
					if(header[i]!='%')
						param[pos]=header[i];
					else
					{
						i++;
						if(header[i]>='0' && header[i]<='9') param[pos]=(header[i]-0x30)*0x10; else
						if(header[i]>='A' && header[i]<='F') param[pos]=(header[i]-0x37)*0x10; else
						if(header[i]>='a' && header[i]<='f') param[pos]=(header[i]-0x57)*0x10;

						i++;
						if(header[i]>='0' && header[i]<='9') param[pos]+=header[i]-0x30; else
						if(header[i]>='A' && header[i]<='F') param[pos]+=header[i]-0x37; else
						if(header[i]>='a' && header[i]<='f') param[pos]+=header[i]-0x57;
					}
				}
				param[pos]=0;
			}

#ifndef LITE_EDITION
 #ifdef WEB_CHAT
			if(strstr(param, "/chat.ps3"))
			{
				is_popup=1; is_binary=0;
				goto html_response;
			}
 #endif
			if(strstr(param, "/popup.ps3"))
			{
				is_popup=1; is_binary=0;
				goto html_response;
			}
			if(strstr(param, "/dev_blind"))
			{
				is_binary=2;
				goto html_response;
			}
#endif
			if(strstr(param, "quit.ps3"))
			{
				http_response(conn_s, header, param, 200, param);
quit:
				if(!webman_config->fanc || strstr(param, "?0") || webman_config->ps2temp<33)
					restore_fan(0); //restore syscon fan control mode
				else
					restore_fan(1); //set ps2 fan control mode

				working = 0;
				sclose(&conn_s);
				if(sysmem) sys_memory_free(sysmem);
				loading_html=0;

				stop_prx_module();
				sys_ppu_thread_exit(0);
				break;
			}
			if(strstr(param, "shutdown.ps3"))
			{
				http_response(conn_s, header, param, 200, param);
				working = 0;
				{ DELETE_TURNOFF } { BEEP1 }
				{system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0);}
				sys_ppu_thread_exit(0);
				break;
			}
			if(strstr(param, "rebuild.ps3"))
			{
				http_response(conn_s, header, param, 200, param);

				cmd[0] = cmd[1] = 0; cmd[2] = 0x03; cmd[3] = 0xE9; // 00 00 03 E9
				savefile((char*)"/dev_hdd0/mms/db.err", cmd, 4);
				goto restart;
			}
			if(strstr(param, "recovery.ps3"))
			{
				#define SC_UPDATE_MANAGER_IF				863
				#define UPDATE_MGR_PACKET_ID_READ_EPROM		0x600B
				#define UPDATE_MGR_PACKET_ID_WRITE_EPROM	0x600C
				#define RECOVER_MODE_FLAG_OFFSET			0x48C61

				http_response(conn_s, header, param, 200, param);

				{system_call_7(SC_UPDATE_MANAGER_IF, UPDATE_MGR_PACKET_ID_WRITE_EPROM, RECOVER_MODE_FLAG_OFFSET, 0x00, 0, 0, 0, 0);} // set recovery mode
				goto reboot;
			}
			if(strstr(param, "restart.ps3"))
			{
				http_response(conn_s, header, param, 200, param);
restart:
				working = 0;
				{ DELETE_TURNOFF } { BEEP2 }
				if(strstr(param,"?0")==NULL) savefile((char*)WMNOSCAN, NULL, 0);
				{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);}
				sys_ppu_thread_exit(0);
				break;
			}
			if(strstr(param, "reboot.ps3"))
			{
				http_response(conn_s, header, param, 200, param);
reboot:
				working = 0;
				{ DELETE_TURNOFF } { BEEP2 }
				{system_call_3(SC_SYS_POWER, SYS_HARD_REBOOT, NULL, 0);}
				sys_ppu_thread_exit(0);
				break;
			}

#ifdef FIX_GAME
			if(strstr(param, "/fixgame.ps3"))
			{
				// fix game folder
                char *game_path = param + 12;
				if(cellFsStat((char*)game_path, &buf)==CELL_FS_SUCCEEDED)
				{
					fix_in_progress=true; fix_aborted=false;
#ifdef COBRA_ONLY
					if(strcasestr(game_path, ".iso"))
						fix_iso(game_path, 0x100000UL, false);
					else
#endif //#ifdef COBRA_ONLY
						fix_game(game_path);

					fix_in_progress=false;

					is_popup=1; is_binary=0;
					goto html_response;
				}
			}
#endif

			if(strstr(param, "/games.ps3"))
			{
mobile_response:
				mobile_mode = false;

				if(cellFsStat((char*)MOBILE_HTML, &buf)!=CELL_FS_SUCCEEDED)
					sprintf(param, "/index.ps3%s", param+10);
				else if(strstr(param, "?g="))
					sprintf(param, "%s", MOBILE_HTML);
				else if(strstr(param, "?"))
					{sprintf(param, "/index.ps3%s", param+10); mobile_mode = true;}
				else if(cellFsStat((char*)GAMELIST_JS, &buf)!=CELL_FS_SUCCEEDED)
					sprintf(param, "%s", "index.ps3?mobile");
				else
					sprintf(param, "%s", MOBILE_HTML);
			}
			else mobile_mode = false;

			if(strstr(param, "index.ps3")!=NULL) small_alloc=false;

			if(!is_busy && (strstr(param, "index.ps3?")  ||
#ifdef DEBUG_MEM
							strstr(param, "peek.lv2?")   ||
							strstr(param, "poke.lv2?")   ||
							strstr(param, "find.lv2?")   ||
							strstr(param, "peek.lv1?")   ||
							strstr(param, "poke.lv1?")   ||
							strstr(param, "find.lv1?")   ||
							strstr(param, "dump.ps3")    ||
#endif

#ifndef LITE_EDITION
							strstr(param, "delete.ps3")  ||
							strstr(param, "delete_ps3")  ||
#endif

#ifdef PS3MAPI
							strstr(param, "home.ps3mapi")     ||
							strstr(param, "setmem.ps3mapi")   ||
							strstr(param, "getmem.ps3mapi")   ||
							strstr(param, "led.ps3mapi")   	  ||
							strstr(param, "buzzer.ps3mapi")   ||
							strstr(param, "notify.ps3mapi")   ||
							strstr(param, "syscall.ps3mapi")  ||
							strstr(param, "syscall8.ps3mapi") ||
							strstr(param, "setidps.ps3mapi")   ||
							strstr(param, "vshplugin.ps3mapi")   ||
#endif

#ifdef COPY_PS3
							strstr(param, "copy.ps3/") ||
#endif
							strstr(param, "refresh.ps3")
			))
				is_binary=0;
			else if(strstr(param, "cpursx.ps3")  ||
					strstr(param, "index.ps3")   ||
					strstr(param, "setup.ps3")   ||
					strstr(param, "mount_ps3/")  ||
					strstr(param, "mount.ps3/")  ||
#ifdef PS2_DISC
					strstr(param, "mount.ps2/")  ||
					strstr(param, "mount_ps2/")  ||
#endif

#ifdef VIDEO_REC
					strstr(param, "videorec.ps3") ||
#endif

#ifdef EXT_GDATA
					strstr(param, "extgd.ps3")   ||
#endif

#ifdef SYS_BGM
					strstr(param, "sysbgm.ps3")  ||
#endif

#ifdef LOAD_PRX
					strstr(param, "loadprx.ps3") ||
#endif

					strstr(param, "eject.ps3")   ||
					strstr(param, "insert.ps3"))
				is_binary=0;
			else if(param[1]=='n' && param[2]=='e' && param[3]=='t' && (param[3]>='0' && param[4]<='2')) //net0/net1/net2
			{
				is_binary=2; small_alloc = false;
			}
			else
			{
				is_binary=(cellFsStat(param, &buf)==CELL_FS_SUCCEEDED);

				if(!is_binary) {strcpy(header, param);  sprintf(param, "%s/%s", html_base_path, header); is_binary=(cellFsStat(param, &buf)==CELL_FS_SUCCEEDED);} // use html path (if path is omitted)

				if(is_binary)
				{
					c_len=buf.st_size;
					if((buf.st_mode & S_IFDIR) != 0) {is_binary=2; small_alloc = false;} // folder listing
				}
				else
				{
					c_len=0;
					is_binary=0;
					http_response(conn_s, header, param, is_busy ? 503:400, is_busy ? (char*)"503 Server is Busy":(char*)"400 Bad Request");
					loading_html--;
					sys_ppu_thread_exit(0);
				}
			}

html_response:
			prepare_header(header, param, is_binary);
			char templn[1024];
			{u16 ulen=strlen(param); if(ulen>1 && param[ulen-1]=='/') param[ulen-1]=0;}
			//sprintf(templn, "X-PS3-Info: %llu [%s]\r\n", (unsigned long long)c_len, param); strcat(header, templn);

			//-- select content profile
			if(strstr(param, ".ps3?"))
			{
				u8 uprofile=profile; char url[10]; bool is_index_ps3 = (strstr(param, "index.ps3?")!=NULL);

				for(u8 i=0;i<5;i++)
				{
					sprintf(url, "?%i", i); if(strstr(param, url)) {profile=i; break;}
					sprintf(url, "usr=%i", i); if(strstr(param, url)) {profile=i; break;}
					if(is_index_ps3) {sprintf(url, "_%i", i); if(strstr(param, url)) {profile=i; break;}}
				}

				if(uprofile!=profile) {webman_config->profile=profile; save_settings();}
				if((uprofile!=profile) || is_index_ps3) {DELETE_CACHED_GAMES}
			}
			//--

			if(is_binary==1) //file
			{
				sprintf(templn, "Content-Length: %llu\r\n\r\n", (unsigned long long)c_len); strcat(header, templn);
				ssend(conn_s, header);
				if(!sysmem && sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)!=0)
				{
					sclose(&conn_s);
					loading_html--;
					sys_ppu_thread_exit(0);
				}
				char *buffer= (char*)sysmem;
				if(cellFsOpen(param, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
				{
					u64 read_e = 0, pos;
					cellFsLseek(fd, 0, CELL_FS_SEEK_SET, &pos);

					while(working)
					{
						//sys_timer_usleep(500);
						if(cellFsRead(fd, (void *)buffer, _64KB_, &read_e)==CELL_FS_SUCCEEDED)
						{
							if(read_e>0)
							{
								if(send(conn_s, buffer, (size_t)read_e, 0)<0) break;
							}
							else
								break;
						}
						else
							break;
					}
					cellFsClose(fd);
				}
				sys_memory_free(sysmem);
				sclose(&conn_s);
				loading_html--;
				sys_ppu_thread_exit(0);

			}
			if(strstr(param, "cpursx.ps3"))
			{
				if(!sysmem && sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)!=0)
				{
					sclose(&conn_s);
					loading_html--;
					sys_ppu_thread_exit(0);
				}
				is_cpursx=1;
			}
			else
			{
				if(!small_alloc)
				{
					set_buffer_sizes(webman_config->foot);

					_meminfo meminfo;
					{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}

					if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) set_buffer_sizes(3); //MIN+
					if((meminfo.avail)<( (BUFFER_SIZE_ALL) + MIN_MEM)) set_buffer_sizes(1); //MIN
				}

				if(!sysmem && sys_memory_allocate(small_alloc?_64KB_:BUFFER_SIZE_ALL, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)!=0)
				{
					sclose(&conn_s);
					loading_html--;
					sys_ppu_thread_exit(0);
				}
			}

			char *buffer= (char*)sysmem;
			//else	// text page
			{
				if(is_binary!=2 && strstr(param, "setup.ps3?"))
				{
					setup_parse_settings(param);
				}

				bool mount_ps3 = !is_popup && (strstr(param, "mount_ps3")!=NULL), forced_mount = false;

				if(mount_ps3 && View_Find("game_plugin")) {mount_ps3=false; forced_mount=true;}

				prepare_html(buffer, templn, param, is_ps3_http, is_cpursx, mount_ps3);

				if(is_cpursx)
				{
					cpu_rsx_stats(buffer, templn, param);

					is_cpursx = 0; goto send_response;

					//CellGcmConfig config; cellGcmGetConfiguration(&config);
					//sprintf(templn, "localAddr: %x", (u32) config.localAddress); strcat(buffer, templn);
				}
				else if(!mount_ps3)
				{
					u32 t1=0, t2=0;
					get_temperature(0, &t1); // 3E030000 -> 3E.03°C -> 62.(03/256)°C
					get_temperature(1, &t2);
					t1=t1>>24;
					t2=t2>>24;

					sprintf(templn, " [<a href=\"/cpursx.ps3\">CPU: %i°C | RSX: %i°C</a>]<hr>"
									"<form action=\"\">", t1, t2); strcat(buffer, templn);

					if((webman_config->homeb) && (strlen(webman_config->home_url)>0))
					{sprintf(templn, HTML_BUTTON_FMT, HTML_BUTTON, STR_HOME, HTML_ONCLICK, webman_config->home_url); strcat(buffer, templn);}

					sprintf(templn, HTML_BUTTON_FMT
									HTML_BUTTON_FMT
									HTML_BUTTON_FMT
#ifdef EXT_GDATA
									HTML_BUTTON_FMT
#endif
									, HTML_BUTTON, STR_EJECT, HTML_ONCLICK, "/eject.ps3"
									, HTML_BUTTON, STR_INSERT, HTML_ONCLICK, "/insert.ps3"
									, HTML_BUTTON, STR_UNMOUNT, HTML_ONCLICK, "/mount.ps3/unmount"
#ifdef EXT_GDATA
									, HTML_BUTTON, "gameDATA", HTML_ONCLICK, "/extgd.ps3"
#endif
                    ); strcat(buffer, templn);
#ifdef COPY_PS3
					if(((strstr(param, "/dev_") && strlen(param)>12 && !strstr(param,"?")) || strstr(param, "/dev_bdvd")) && !strstr(param,".ps3/") && !strstr(param,".ps3?"))
					{sprintf(templn, "%s%s\" onclick='window.location.href=\"/copy.ps3%s\";'\">", HTML_BUTTON, STR_COPY, param); strcat(buffer, templn);}
#endif
					sprintf(templn,  "%s%s XML%s\" %s'%s';\"> "
									 "%s%s HTML%s\" %s'%s';\">"
									 HTML_BUTTON_FMT
									 HTML_BUTTON_FMT
									 "</form><hr>",
									 HTML_BUTTON, STR_REFRESH, SUFIX2(profile), HTML_ONCLICK, "/refresh.ps3",
									 HTML_BUTTON, STR_REFRESH, SUFIX2(profile), HTML_ONCLICK, "/index.ps3?html",
									 HTML_BUTTON, STR_SHUTDOWN, HTML_ONCLICK, "/shutdown.ps3",
									 HTML_BUTTON, STR_RESTART, HTML_ONCLICK, "/restart.ps3"); strcat(buffer, templn);
				}

				char tempstr[_4KB_];

#ifndef LITE_EDITION
				if(is_popup)
				{
#ifdef WEB_CHAT
					if(strstr(param, "/chat.ps3"))
					{
						webchat(buffer, templn, param, tempstr, conn_info_main);
					}
					else
#endif
#ifdef FIX_GAME
					if(strstr(param, "/fixgame.ps3"))
					{
						char *game_path = param + 12;
						sprintf(templn, "Fixed: %s", game_path);
						templn[211]=0; //limit message to 200 characters
						show_msg((char*)templn);
						sprintf(templn, "Fixed: <a href=\"%s\">%s</a>", game_path, game_path); strcat(buffer, templn);
					}
					else
#endif
					{
						param[211]=0; //limit message to 200 characters
						show_msg((char*)(param+11));
						sprintf(templn, "Message sent: %s", param+11); strcat(buffer, templn);
					}

					is_popup=0; goto send_response;
				}
#endif
				if(is_binary==2) // folder listing
				{
					if (folder_listing(buffer, templn, param, conn_s, tempstr, header) == false)
					{
						if(sysmem) sys_memory_free(sysmem);
						loading_html--;
						sys_ppu_thread_exit(0);
					}
				}
				else
				{
					is_busy=true;

					if(strstr(param, "refresh.ps3") && init_running==0)
					{
						init_running = 1;
						refresh_xml(templn);
						sprintf(templn,  "<br>%s", STR_XMLRF); strcat(buffer, templn);
					}
					else
					if(strstr(param, "eject.ps3"))
					{
						eject_insert(1, 0);
						strcat(buffer, STR_EJECTED);
					}
					else
					if(strstr(param, "insert.ps3"))
					{
						eject_insert(0, 1);
						strcat(buffer, STR_LOADED);
					}
#ifdef LOAD_PRX
					else
					if(strstr(param, "loadprx.ps3"))
					{
						char *pos; unsigned int slot=6; bool prx_found;

						pos=strstr(param, "slot=");
						if(pos)
						{
							get_value(templn, pos + 5, 2);
							slot=RANGE((unsigned int)val(templn), 1, 6);
						}

						if(param[12]=='/') sprintf(templn, "%s", param+12);
						else
						{
							sprintf(templn, "/dev_hdd0/plugins/webftp_server.sprx");
							if(cellFsStat(templn, &buf)!=CELL_FS_SUCCEEDED) sprintf(templn, "/dev_hdd0/webftp_server.sprx");

							pos=strstr(param, "prx=");
							if(pos) get_value(templn, pos + 4, MAX_PATH_LEN);
						}

						prx_found = (cellFsStat(templn, &buf)==CELL_FS_SUCCEEDED);

						if(prx_found)
							sprintf(param, "slot: %i<br>load prx: %s", slot, templn);
						else
							sprintf(param, "unload slot: %i", slot);

						strcat(buffer, param); strcat(buffer, "</font></body></html>");

						cobra_unload_vsh_plugin(slot);

						if(prx_found)
							{cobra_load_vsh_plugin(slot, templn, NULL, 0); if(strstr(templn, "/webftp_server")) goto quit;}
					}
#endif

#ifdef VIDEO_REC
					else
					if(strstr(param, "videorec.ps3"))
					{
						toggle_video_rec();
						strcat(buffer,	"<a class=\"f\" href=\"/dev_hdd0\">/dev_hdd0/</a><a href=\"/dev_hdd0/VIDEO\">VIDEO</a>:<p>"
										"Video recording: <a href=\"/videorec.ps3\">");
						strcat(buffer, recording?STR_ENABLED:STR_DISABLED);
						strcat(buffer, "</a>");
					}
#endif

#ifdef EXT_GDATA
					else
					if(strstr(param, "extgd.ps3"))
					{
						if(strstr(param,"?s" /*status */ )); else
						if(strstr(param,"?e" /*enable */ ) || strstr(param, "?1"))  extgd=1; else
						if(strstr(param,"?d" /*disable*/ ) || strstr(param, "?0"))  extgd=0; else
																					extgd=extgd^1;

						strcat(buffer, "External Game DATA: ");
						if(set_gamedata_status(extgd, true))
							strcat(buffer, STR_ERROR);
						else
							strcat(buffer, extgd?STR_ENABLED:STR_DISABLED);
					}
#endif

#ifdef SYS_BGM
					else
					if(strstr(param, "sysbgm.ps3"))
					{
						static int system_bgm=0;

						if(system_bgm==0)
						{
							system_bgm=-1;
							BgmPlaybackEnable  = (void*)((int)getNIDfunc("vshmain", 0xEDAB5E5E, 16*2));
							BgmPlaybackDisable = (void*)((int)getNIDfunc("vshmain", 0xEDAB5E5E, 17*2));
						}

						if(strstr(param, "?1") || strstr(param, "?e")) system_bgm=-1; //enable
						if(strstr(param, "?0") || strstr(param, "?d")) system_bgm=1;  //disable

						if(strstr(param, "?s")) goto bgm_status;

						int * arg2;
						if(system_bgm<0)
						{
							BgmPlaybackEnable(0, &arg2); system_bgm=1;
						}
						else
						{
							BgmPlaybackDisable(0, &arg2); system_bgm=-1;
						}

bgm_status:
						sprintf(templn, "System BGM: %s", (system_bgm>0)?STR_ENABLED:STR_DISABLED);
						strcat(buffer, templn);
						show_msg(templn);
					}
#endif

#ifdef DEBUG_MEM
					else
					if(strstr(param, "dump.ps3"))
					{
						ps3mapi_mem_dump(buffer, templn, param);
					}
					else
					if(strstr(param, "peek.lv") || strstr(param, "poke.lv") || strstr(param, "find.lv"))
					{
						ps3mapi_find_peek_poke(buffer, templn, param);
					}
#endif
					else
					if(strstr(param, "setup.ps3?"))
					{
						if(strstr(param, "&")==NULL)
						{
							cellFsUnlink(WMCONFIG);
							reset_settings();
						}
						if(save_settings() == CELL_FS_SUCCEEDED)
						{
							sprintf(templn, "<br> %s", STR_SETTINGSUPD); strcat(buffer, templn);
						}
						else
							strcat(buffer, STR_ERROR);
					}
					else
					if(strstr(param, "setup.ps3"))
					{
                        setup_form(buffer, templn);
					}
#ifndef LITE_EDITION
					else
					if(strstr(param, "/delete.ps3") || strstr(param, "/delete_ps3"))
					{
						bool is_reset = false;
						if(strstr(param, "?wmreset")) is_reset=true;
						if(is_reset || strstr(param, "?wmconfig")) {cellFsUnlink(WMCONFIG); reset_settings(); sprintf(param, "/delete_ps3%s", WMCONFIG);}
						if(is_reset || strstr(param, "?wmtmp")) strcpy(param, "/delete_ps3/dev_hdd0/tmp/wmtmp\0");

						if(strstr(param, "?history"))
							{delete_history(true); sprintf(templn, "%s : history", STR_DELETE);}
						else if(strstr(param, "?uninstall"))
						{
							if(cellFsStat((char*)"/dev_hdd0/boot_plugins.txt", &buf)==CELL_FS_SUCCEEDED && buf.st_size<40) cellFsUnlink((char*)"/dev_hdd0/boot_plugins.txt");
							cellFsUnlink((char*)"/dev_hdd0/webftp_server.sprx");
							cellFsUnlink((char*)"/dev_hdd0/plugins/webftp_server.sprx");
							cellFsUnlink((char*)WMCONFIG);
							del((char*)WMTMP, true);
							del((char*)"/dev_hdd0/xmlhost", true);
							del((char*)"/dev_hdd0/tmp/wm_lang", true);
							del((char*)"/dev_hdd0/tmp/wm_icons", true);
							http_response(conn_s, header, param, 200, param);
							goto restart;
						}
						else if(del(param+11, (strstr(param, "/delete.ps3")!=NULL)))
							sprintf(templn, "%s %s : <a href=\"%s\">%s</a><br>", STR_DELETE, STR_ERROR, param+11, param+11);
						else
							sprintf(templn, "%s : <a href=\"%s\">%s</a><br>", STR_DELETE, param+11, param+11);

						strcat(buffer, templn);
					}
#endif

#ifdef PS3MAPI
					else
					if(strstr(param, "home.ps3mapi"))
					{
						ps3mapi_home(buffer, templn);
					}
					else
					if(strstr(param, "buzzer.ps3mapi"))
					{
						ps3mapi_buzzer(buffer, templn, param);
					}
					else
					if(strstr(param, "led.ps3mapi"))
					{
						ps3mapi_led(buffer, templn, param);
					}
					else
					if(strstr(param, "notify.ps3mapi"))
					{
						ps3mapi_notify(buffer, templn, param);
					}
					else
					if(strstr(param, "syscall.ps3mapi"))
					{
						ps3mapi_syscall(buffer, templn, param);
					}
					else
					if(strstr(param, "syscall8.ps3mapi"))
					{
						ps3mapi_syscall8(buffer, templn, param);
					}
					else
					if(strstr(param, "getmem.ps3mapi"))
					{
						ps3mapi_getmem(buffer, templn, param);
					}
					else
					if(strstr(param, "setmem.ps3mapi"))
					{
						ps3mapi_setmem(buffer, templn, param);
					}
					else
					if(strstr(param, "setidps.ps3mapi"))
					{
						ps3mapi_setidps(buffer, templn, param);
					}
					else
					if(strstr(param, "vshplugin.ps3mapi"))
					{
						ps3mapi_vshplugin(buffer, templn, param);
					}
#endif

					else
#ifdef PS2_DISC
					if(mount_ps3 || forced_mount || !memcmp(param, "/mount.ps3", 10) || !memcmp(param, "/mount.ps2", 10) || !memcmp(param, "/mount_ps2", 10) || !memcmp(param, "/copy.ps3", 9))
#else
					if(mount_ps3 || forced_mount || !memcmp(param, "/mount.ps3", 10) || !memcmp(param, "/copy.ps3", 9))
#endif
					{
                        game_mount(buffer, templn, param, tempstr, is_binary, mount_ps3, forced_mount);
					}
					else
					{
						mobile_mode|=(strstr(param, "?mob")!=NULL || strstr(param, "&mob")!=NULL);

						if(game_listing(buffer, templn, param, conn_s, tempstr, mobile_mode) == false)
						{
							is_busy=false;
							if(sysmem) sys_memory_free(sysmem);
							loading_html--;
							sys_ppu_thread_exit(0);
							break;
                        }
					}

					is_busy=false;
				}

send_response:
				if(mobile_mode && allow_retry_response) {allow_retry_response=false; goto mobile_response;}

				if(mount_ps3)
					strcat(buffer, "<script type=\"text/javascript\">window.close(this);</script>"); //auto-close
				else
					strcat(buffer, "</font></body></html>"); //end-html

				sprintf(templn, "Content-Length: %llu\r\n\r\n", (unsigned long long)strlen(buffer)); strcat(header, templn);
				ssend(conn_s, header);
				ssend(conn_s, buffer);
				buffer[0]=0;
			}
		}

		break;
	}

#ifdef USE_DEBUG
	ssend(debug_s, "Request served.\r\n");
#endif

	sclose(&conn_s);
	if(sysmem) sys_memory_free(sysmem);
	loading_html--;
	sys_ppu_thread_exit(0);
}

static void handleclient_ftp(u64 conn_s_ftp_p)
{
	int conn_s_ftp = (int)conn_s_ftp_p; // main communications socket
	int data_s = -1;			// data socket
	int data_ls = -1;

	int connactive = 1;			// whether the ftp connection is active or not
	int dataactive = 0;			// prevent the data connection from being closed at the end of the loop
	u8 loggedin = 0;			// whether the user is logged in or not

	char cwd[MAX_PATH_LEN], tempcwd[MAX_PATH_LEN];	// Current Working Directory
	int rest = 0;									// for resuming file transfers

	char buffer[FTP_RECV_SIZE];
	char cmd[16], param[MAX_PATH_LEN], filename[MAX_PATH_LEN], source[MAX_PATH_LEN]; // used as source parameter in RNFR and COPY commands
	struct CellFsStat buf;
	int fd;

	int p1x = 0;
	int p2x = 0;

	#define FTP_OK_150    "150 OK\r\n"    // File status okay; about to open data connection.
	#define FTP_OK_200    "200 OK\r\n"    // The requested action has been successfully completed.
	#define FTP_OK_221    "221 OK\r\n"    // Service closing control connection.
	#define FTP_OK_226    "226 OK\r\n"    // Closing data connection. Requested file action successful (for example, file transfer or file abort).
	#define FTP_OK_250    "250 OK\r\n"    // Requested file action okay, completed.

	#define FTP_ERROR_425 "425 Error\r\n" // Can't open data connection.
	#define FTP_ERROR_451 "451 Error\r\n" // Requested action aborted. Local error in processing.
	#define FTP_ERROR_500 "500 Error\r\n" // Syntax error, command unrecognized and the requested action did not take place.
	#define FTP_ERROR_501 "501 Error\r\n" // Syntax error in parameters or arguments.
	#define FTP_ERROR_550 "550 Error\r\n" // Requested action not taken. File unavailable (e.g., file not found, no access).

	CellRtcDateTime rDate;
	CellRtcTick pTick;

	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(conn_s_ftp, &conn_info, 1);

	char ip_address[16];
	char pasv_output[56];
	sprintf(ip_address, "%s", inet_ntoa(conn_info.remote_adr));

	ssend(conn_s_ftp, "220-VSH ftpd\r\n");
	if(webman_config->bind && ((conn_info.local_adr.s_addr!=conn_info.remote_adr.s_addr)  && strncmp(ip_address, webman_config->allow_ip, strlen(webman_config->allow_ip))!=0))
	{
		sprintf(buffer, "%i Access Denied. Use SETUP to allow remote connections.\r\n", 500); ssend(conn_s_ftp, buffer);
		sclose(&conn_s_ftp);
		sys_ppu_thread_exit(0);
	}

	sprintf(ip_address, "%s", inet_ntoa(conn_info.local_adr));
	for(u8 n=0;n<strlen(ip_address);n++) if(ip_address[n]=='.') ip_address[n]=',';

	sprintf(buffer, "%i webMAN ftpd " WM_VERSION "\r\n", 220); ssend(conn_s_ftp, buffer);

	strcpy(cwd, "/");

	while((connactive == 1) && working)
	{

		if(working && (recv(conn_s_ftp, buffer, FTP_RECV_SIZE, 0) > 0))
		{
			buffer[strcspn(buffer, "\n")] = '\0';
			buffer[strcspn(buffer, "\r")] = '\0';

			int split = ssplit(buffer, cmd, 15, param, MAX_PATH_LEN-1);

			if(working && loggedin == 1)
			{
				if(strcasecmp(cmd, "CWD") == 0)
				{

					strcpy(tempcwd, cwd);

					if(split == 1)
					{
						absPath(tempcwd, param, cwd);
					}

					if(isDir(tempcwd))
					{
						strcpy(cwd, tempcwd);
						ssend(conn_s_ftp, FTP_OK_250);
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_550);
					}
				}
				else
				if(strcasecmp(cmd, "CDUP") == 0)
				{
					u16 pos = strlen(cwd) - 2;

					for(u16 i = pos; i > 0; i--)
					{
						if(i < pos && cwd[i] == '/')
						{
							break;
						}
						else
						{
							cwd[i] = '\0';
						}
					}
					ssend(conn_s_ftp, FTP_OK_250);
				}
				else
				if(strcasecmp(cmd, "PWD") == 0)
				{
					sprintf(buffer, "257 \"%s\"\r\n", cwd);
					ssend(conn_s_ftp, buffer);
				}
				else
				if(strcasecmp(cmd, "TYPE") == 0)
				{
					ssend(conn_s_ftp, "200 TYPE OK\r\n");
					dataactive = 1;
				}
				else
				if(strcasecmp(cmd, "REST") == 0)
				{
					if(split == 1)
					{
						ssend(conn_s_ftp, "350 REST command successful\r\n");
						rest = val(param);
						dataactive = 1;
					}
					else
					{
						ssend(conn_s_ftp, "501 No restart point\r\n");
					}
				}
				else
				if(strcasecmp(cmd, "QUIT") == 0 || strcasecmp(cmd, "BYE") == 0)
				{
					ssend(conn_s_ftp, "221 BYE\r\n");
					connactive = 0;
				}
				else
				if(strcasecmp(cmd, "FEAT") == 0)
				{
					ssend(conn_s_ftp,	"211-Ext:\r\n"
										" SIZE\r\n"
										" MDTM\r\n"
										" PORT\r\n"
										" CDUP\r\n"
										" ABOR\r\n"
										" REST STREAM\r\n"
										" PASV\r\n"
										" LIST\r\n"
										" MLSD\r\n"
										" MLST type*;size*;modify*;UNIX.mode*;UNIX.uid*;UNIX.gid*;\r\n"
										"211 End\r\n");
				}
				else
				if(strcasecmp(cmd, "PORT") == 0)
				{
					rest = 0;

					if(split == 1)
					{
						char data[6][4];
						int i = 0;
						u8 k=0;

						for(u8 j=0;j<=strlen(param);j++)
						{
							if(param[j]!=',' && param[j]!=0) { data[i][k]=param[j]; k++; }
							else {data[i][k]=0; i++; k=0;}
							if(i>=6) break;
						}

						if(i == 6)
						{
							char ipaddr[16];
							sprintf(ipaddr, "%s.%s.%s.%s", data[0], data[1], data[2], data[3]);

							data_s=connect_to_server(ipaddr, getPort(val(data[4]), val(data[5])));

							if(data_s>=0)
							{
								ssend(conn_s_ftp, FTP_OK_200);
								dataactive = 1;
							}
							else
							{
								ssend(conn_s_ftp, FTP_ERROR_451);
							}
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_501);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "SITE") == 0)
				{
					if(split == 1)
					{
						split = ssplit(param, cmd, 10, filename, MAX_PATH_LEN-1);

						if(strcasecmp(cmd, "HELP") == 0)
						{
							ssend(conn_s_ftp, "214-CMDs:\r\n"
#ifndef LITE_EDITION
											  " SITE FLASH\r\n"
 #ifdef EXT_GDATA
											  " SITE EXTGD <ON/OFF>\r\n"
 #endif
											  " SITE MAPTO <path>\r\n"
 #ifdef FIX_GAME
											  " SITE FIX <path>\r\n"
 #endif
											  " SITE UMOUNT\r\n"
											  " SITE COPY <file>\r\n"
											  " SITE PASTE <file>\r\n"
											  " SITE CHMOD 777 <file>\r\n"
#endif
											  " SITE SHUTDOWN\r\n"
											  " SITE RESTART\r\n"
											  "214 End\r\n");
						}
						else
						if(strcasecmp(cmd, "SHUTDOWN") == 0)
						{
							ssend(conn_s_ftp, FTP_OK_221);

							working = 0;
							{ DELETE_TURNOFF } { BEEP1 }
							{system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0);}
							sys_ppu_thread_exit(0);
						}
						else
						if(strcasecmp(cmd, "RESTART") == 0 || strcasecmp(cmd, "REBOOT") == 0)
						{
							ssend(conn_s_ftp, FTP_OK_221);

							working = 0;
							{ DELETE_TURNOFF } { BEEP2 }
							if(strcasecmp(cmd, "REBOOT")) savefile((char*)WMNOSCAN, NULL, 0);
							{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);}
							sys_ppu_thread_exit(0);
						}
						else
						if(strcasecmp(cmd, "FLASH") == 0)
						{
							ssend(conn_s_ftp, FTP_OK_250);

							bool rw_flash=isDir("/dev_blind");

							if(filename[0] == 0) ; else
							if(strcasecmp(filename, "ON" ) == 0) {if( rw_flash) continue;} else
							if(strcasecmp(filename, "OFF") == 0) {if(!rw_flash) continue;}

							if(rw_flash)
								{system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}
							else
								enable_dev_blind(NULL);
						}
#ifndef LITE_EDITION
 #ifdef EXT_GDATA
						else
						if(strcasecmp(cmd, "EXTGD") == 0)
						{
							ssend(conn_s_ftp, FTP_OK_250);

							if(filename[0] == 0)					set_gamedata_status(extgd^1, true); else
							if(strcasecmp(filename, "ON" ) == 0)	set_gamedata_status(0, true);		else
							if(strcasecmp(filename, "OFF") == 0)	set_gamedata_status(1, true);

						}
 #endif
						else
						if(strcasecmp(cmd, "UMOUNT") == 0)
						{
							ssend(conn_s_ftp, FTP_OK_250);
							do_umount(true);
						}
 #ifdef COBRA_ONLY
						else
						if(strcasecmp(cmd, "MAPTO") == 0)
						{
							ssend(conn_s_ftp, FTP_OK_250);
							if(filename[0]=='/')
							{
								sys_map_path((char*)filename, (char*)cwd);
							}
							else
							{
								mount_with_mm(cwd, 1);
							}
						}
 #endif //#ifdef COBRA_ONLY
 #ifdef FIX_GAME
						else
						if(strcasecmp(cmd, "FIX") == 0)
						{
							if(fix_in_progress)
								ssend(conn_s_ftp, FTP_ERROR_451);
							else
							{
								ssend(conn_s_ftp, FTP_OK_250);
								absPath(param, filename, cwd);

								fix_in_progress=true; fix_aborted = false;

  #ifdef COBRA_ONLY
								if(strcasestr(filename, ".iso"))
									fix_iso(param, 0x100000UL, false);
								else
  #endif //#ifdef COBRA_ONLY
									fix_game(param);

								fix_in_progress=false;
							}
						}
 #endif //#ifdef FIX_GAME
						else
						if(strcasecmp(cmd, "CHMOD") == 0)
						{
							split = ssplit(param, cmd, 10, filename, MAX_PATH_LEN-1);

							strcpy(param, filename); absPath(filename, param, cwd);

							ssend(conn_s_ftp, FTP_OK_250);
							int attributes = val(cmd);
							if(attributes == 0)
								cellFsChmod(filename, MODE);
							else
								cellFsChmod(filename, attributes);
						}
						else
						if(strcasecmp(cmd, "COPY") == 0)
						{
							sprintf(buffer, "%s %s", STR_COPYING, filename);
							show_msg(buffer);

							absPath(source, filename, cwd);
							ssend(conn_s_ftp, FTP_OK_200);
						}
						else
						if(strcasecmp(cmd, "PASTE") == 0)
						{
							struct CellFsStat s;

							absPath(param, filename, cwd);
							if((!copy_in_progress) && (strlen(source) > 0) && (strcmp(source, param) != 0) && cellFsStat(source, &s)==CELL_FS_SUCCEEDED)
							{
								copy_in_progress=true; copied_count = 0;
								ssend(conn_s_ftp, FTP_OK_250);

								sprintf(buffer, "%s %s\n%s %s", STR_COPYING, source, STR_CPYDEST, param);
								show_msg(buffer);

								if(isDir(source))
									folder_copy(source, param);
								else
									filecopy(source, param, COPY_WHOLE_FILE);

								show_msg((char*)STR_CPYFINISH);
								//memset(source, 0, 512);
								copy_in_progress=false;
							}
							else
							{
								ssend(conn_s_ftp, FTP_ERROR_500);
							}
						}
#endif //#ifndef LITE_EDITION
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_500);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "NOOP") == 0)
				{
					ssend(conn_s_ftp, "200 NOOP\r\n");
				}
				else
				if(strcasecmp(cmd, "MLSD") == 0 || strcasecmp(cmd, "LIST") == 0 || strcasecmp(cmd, "MLST") == 0)
				{
					if(data_s > 0)
					{
						int nolist = (strcasecmp(cmd, "MLSD") == 0 || strcasecmp(cmd, "MLST") == 0);

						strcpy(tempcwd, cwd);

						if(split == 1)
						{
							absPath(tempcwd, param, cwd);
						}

						if(cellFsOpendir( (isDir(tempcwd) ? tempcwd : cwd), &fd) == CELL_FS_SUCCEEDED)
						{
							ssend(conn_s_ftp, FTP_OK_150);

							CellFsDirent entry;
							u64 read_e;

							while(cellFsReaddir(fd, &entry, &read_e) == 0 && read_e > 0)
							{
								if(!strcmp(entry.d_name, "app_home") || !strcmp(entry.d_name, "host_root")) continue;

								absPath(filename, entry.d_name, cwd);

								cellFsStat(filename, &buf);
								cellRtcSetTime_t(&rDate, buf.st_mtime);
								if(nolist)
								{

									char dirtype[2];
									if(strcmp(entry.d_name, ".") == 0)
									{
										dirtype[0] = 'c';
									}
									else
									if(strcmp(entry.d_name, "..") == 0)
									{
										dirtype[0] = 'p';
									}
									else
									{
										dirtype[0] = '\0';
									}

									dirtype[1] = '\0';

									if(strcasecmp(cmd, "MLSD") == 0)
									sprintf(buffer, "type=%s%s;siz%s=%llu;modify=%04i%02i%02i%02i%02i%02i;UNIX.mode=0%i%i%i;UNIX.uid=root;UNIX.gid=root; %s\r\n",
										dirtype,
										((buf.st_mode & S_IFDIR) != 0) ? "dir" : "file",
										((buf.st_mode & S_IFDIR) != 0) ? "d" : "e", (unsigned long long)buf.st_size, rDate.year, rDate.month, rDate.day, rDate.hour, rDate.minute, rDate.second,
										(((buf.st_mode & S_IRUSR) != 0) * 4 + ((buf.st_mode & S_IWUSR) != 0) * 2 + ((buf.st_mode & S_IXUSR) != 0) * 1),
										(((buf.st_mode & S_IRGRP) != 0) * 4 + ((buf.st_mode & S_IWGRP) != 0) * 2 + ((buf.st_mode & S_IXGRP) != 0) * 1),
										(((buf.st_mode & S_IROTH) != 0) * 4 + ((buf.st_mode & S_IWOTH) != 0) * 2 + ((buf.st_mode & S_IXOTH) != 0) * 1),
										entry.d_name);
									else
										sprintf(buffer, " type=%s%s;siz%s=%llu;modify=%04i%02i%02i%02i%02i%02i;UNIX.mode=0%i%i%i;UNIX.uid=root;UNIX.gid=root; %s\r\n",
											dirtype,
											((buf.st_mode & S_IFDIR) != 0) ? "dir" : "file",
											((buf.st_mode & S_IFDIR) != 0) ? "d" : "e", (unsigned long long)buf.st_size, rDate.year, rDate.month, rDate.day, rDate.hour, rDate.minute, rDate.second,
											(((buf.st_mode & S_IRUSR) != 0) * 4 + ((buf.st_mode & S_IWUSR) != 0) * 2 + ((buf.st_mode & S_IXUSR) != 0) * 1),
											(((buf.st_mode & S_IRGRP) != 0) * 4 + ((buf.st_mode & S_IWGRP) != 0) * 2 + ((buf.st_mode & S_IXGRP) != 0) * 1),
											(((buf.st_mode & S_IROTH) != 0) * 4 + ((buf.st_mode & S_IWOTH) != 0) * 2 + ((buf.st_mode & S_IXOTH) != 0) * 1),
											entry.d_name);
								}
								else
									sprintf(buffer, "%s%s%s%s%s%s%s%s%s%s   1 root  root        %llu %s %02i %02i:%02i %s\r\n",
									(buf.st_mode & S_IFDIR) ? "d" : "-",
									(buf.st_mode & S_IRUSR) ? "r" : "-",
									(buf.st_mode & S_IWUSR) ? "w" : "-",
									(buf.st_mode & S_IXUSR) ? "x" : "-",
									(buf.st_mode & S_IRGRP) ? "r" : "-",
									(buf.st_mode & S_IWGRP) ? "w" : "-",
									(buf.st_mode & S_IXGRP) ? "x" : "-",
									(buf.st_mode & S_IROTH) ? "r" : "-",
									(buf.st_mode & S_IWOTH) ? "w" : "-",
									(buf.st_mode & S_IXOTH) ? "x" : "-",
									(unsigned long long)buf.st_size, smonth[rDate.month-1], rDate.day,
									rDate.hour, rDate.minute, entry.d_name);

								if(ssend(data_s, buffer)<0) break;
								sys_timer_usleep(1000);
							}

							cellFsClosedir(fd);
							if(strlen(tempcwd)>6)
							{
								uint32_t blockSize;
								uint64_t freeSize;
								char tempstr[128];
								if(strchr(tempcwd+1, '/'))
									tempcwd[strchr(tempcwd+1, '/')-tempcwd]=0;
								cellFsGetFreeSize(tempcwd, &blockSize, &freeSize);
								sprintf(tempstr, "226 [%s] [ %i %s ]\r\n", tempcwd, (int)((blockSize*freeSize)>>20), STR_MBFREE);
								ssend(conn_s_ftp, tempstr);
							}
							else
							{
								ssend(conn_s_ftp, FTP_OK_226);
							}
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_425);
					}
				}
				else
				if(strcasecmp(cmd, "PASV") == 0)
				{
					u8 pasv_retry=0;
					rest = 0;
pasv_again:
					if(!p1x)
					{
						cellRtcGetCurrentTick(&pTick);
						p1x = ( ( (pTick.tick & 0xfe0000) >> 16) & 0xff) | 0x80; // use ports 32768 -> 65279 (0x8000 -> 0xFEFF)
						p2x = ( ( (pTick.tick & 0x00ff00) >>  8) & 0xff);
					}
					data_ls = slisten(getPort(p1x, p2x), 1);

					if(data_ls >= 0)
					{
						sprintf(pasv_output, "227 Entering Passive Mode (%s,%i,%i)\r\n", ip_address, p1x, p2x);
						ssend(conn_s_ftp, pasv_output);

						if((data_s = accept(data_ls, NULL, NULL)) > 0)
						{
							dataactive = 1;
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_451);
						}

					}
					else
					{
						p1x=0;
						if(pasv_retry<10)
						{
							pasv_retry++;
							goto pasv_again;
						}
						ssend(conn_s_ftp, FTP_ERROR_451);
					}
				}
				else
				if(strcasecmp(cmd, "RETR") == 0)
				{
					if(data_s > 0)
					{
						if(split == 1)
						{
							absPath(filename, param, cwd);

							//if(cellFsStat(filename, &buf)==CELL_FS_SUCCEEDED)
							{
								int rr=-4;

								if(cellFsOpen(filename, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
								{
									sys_addr_t sysmem=0;
									if(sys_memory_allocate(BUFFER_SIZE_FTP, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)==0)
									{
										char *buffer2= (char*)sysmem;

										u64 read_e = 0, pos; //, write_e

										cellFsLseek(fd, rest, CELL_FS_SEEK_SET, &pos);
										rest = 0;
										//int optval = BUFFER_SIZE_FTP;
										//setsockopt(data_s, SOL_SOCKET, SO_SNDBUF, &optval, sizeof(optval));

										ssend(conn_s_ftp, FTP_OK_150);
										rr=0;

										while(working)
										{
											//sys_timer_usleep(1668);
											if(cellFsRead(fd, (void *)buffer2, BUFFER_SIZE_FTP, &read_e)==CELL_FS_SUCCEEDED)
											{
												if(read_e>0)
												{
													if(send(data_s, buffer2, (size_t)read_e, 0)<0) {rr=-3; break;}
												}
												else
													break;
											}
											else
												{rr=-2;break;}
										}
										sys_memory_free(sysmem);
									}
									cellFsClose(fd);
								}

								if( rr == 0)
									ssend(conn_s_ftp, FTP_OK_226);

								else if( rr == -4)
									ssend(conn_s_ftp, FTP_ERROR_550);
								else
									ssend(conn_s_ftp, FTP_ERROR_451);

							}
							//else ssend(conn_s_ftp, FTP_ERROR_550);
							}
							else
							ssend(conn_s_ftp, FTP_ERROR_501);
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_425);
					}
				}
				else
				if(strcasecmp(cmd, "DELE") == 0)
				{
					if(split == 1)
					{

						absPath(filename, param, cwd);

						if(cellFsUnlink(filename) == 0)
						{
							ssend(conn_s_ftp, FTP_OK_250);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "MKD") == 0)
				{
					if(split == 1)
					{

						absPath(filename, param, cwd);

						if(cellFsMkdir((char*)filename, MODE) == 0)
						{
							sprintf(buffer, "257 \"%s\" OK\r\n", param);
							ssend(conn_s_ftp, buffer);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "RMD") == 0)
				{
					if(split == 1)
					{

						absPath(filename, param, cwd);

						if(cellFsRmdir(filename) == 0)
						{
							ssend(conn_s_ftp, FTP_OK_250);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "STOR") == 0)
				{
					if(data_s > 0)
					{
						if(split == 1)
						{
							absPath(filename, param, cwd);

							int rr=FAILED;
							u64 pos=0;

							if(cellFsOpen(filename, CELL_FS_O_CREAT|CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
							{

								sys_addr_t sysmem=0;
								if(sys_memory_allocate(BUFFER_SIZE_FTP, SYS_MEMORY_PAGE_SIZE_64K, &sysmem)==0)
								{
									char *buffer2= (char*)sysmem;
									u64 read_e = 0;

									if(rest)
										cellFsLseek(fd, rest, CELL_FS_SEEK_SET, &pos);
									else
										cellFsFtruncate(fd, 0);

									rest = 0;
									rr = 0;

									ssend(conn_s_ftp, FTP_OK_150);
									//int optval = BUFFER_SIZE_FTP;
									//setsockopt(data_s, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
									while(working)
									{
										//sys_timer_usleep(1668);
										if((read_e = (u64)recv(data_s, buffer2, BUFFER_SIZE_FTP, MSG_WAITALL)) > 0)
										{
											if(cellFsWrite(fd, buffer2, read_e, NULL)!=CELL_FS_SUCCEEDED) {rr=FAILED;break;}
										}
										else
											break;
									}
									sys_memory_free(sysmem);
								}
								cellFsClose(fd);
								cellFsChmod(filename, MODE);
								if(!working || rr!=0) cellFsUnlink(filename);
							}

							if(rr == 0)
							{
								ssend(conn_s_ftp, FTP_OK_226);
							}
							else
							{
								ssend(conn_s_ftp, FTP_ERROR_451);
							}
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_501);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_425);
					}
				}
				else
				if(strcasecmp(cmd, "SIZE") == 0)
				{
					if(split == 1)
					{
						absPath(filename, param, cwd);
						if(cellFsStat(filename, &buf)==CELL_FS_SUCCEEDED)
						{
							sprintf(buffer, "213 %llu\r\n", (unsigned long long)buf.st_size);
							ssend(conn_s_ftp, buffer);
							dataactive = 1;
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "SYST") == 0)
				{
					ssend(conn_s_ftp, "215 UNIX Type: L8\r\n");
				}
				else
				if(strcasecmp(cmd, "MDTM") == 0)
				{
					if(split == 1)
					{
						absPath(filename, param, cwd);
						if(cellFsStat(filename, &buf)==CELL_FS_SUCCEEDED)
						{
							cellRtcSetTime_t(&rDate, buf.st_mtime);
							sprintf(buffer, "213 %04i%02i%02i%02i%02i%02i\r\n", rDate.year, rDate.month, rDate.day, rDate.hour, rDate.minute, rDate.second);
							ssend(conn_s_ftp, buffer);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "ABOR") == 0)
				{
					sclose(&data_s);
					ssend(conn_s_ftp, "226 ABOR OK\r\n"); // Closing data connection. Requested file action successful
				}

				else
				if(strcasecmp(cmd, "RNFR") == 0)
				{
					if(split == 1)
					{
						absPath(source, param, cwd);

						if(cellFsStat(source, &buf)==CELL_FS_SUCCEEDED)
						{
							ssend(conn_s_ftp, "350 RNFR OK\r\n"); // Requested file action pending further information
						}
						else
						{
							source[0]=0;
							ssend(conn_s_ftp, "550 RNFR Error\r\n");  // Requested action not taken. File unavailable
						}
					}
					else
					{
						source[0]=0;
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}

				else
				if(strcasecmp(cmd, "RNTO") == 0)
				{
					if(split == 1 && source[0]=='/')
					{
						absPath(filename, param, cwd);

						if(cellFsRename(source, filename) == CELL_FS_SUCCEEDED)
						{
							ssend(conn_s_ftp, FTP_OK_250);
						}
						else
						{
							ssend(conn_s_ftp, FTP_ERROR_550);
						}
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
					source[0]=0;
				}

				else
				if(strcasecmp(cmd, "USER") == 0 || strcasecmp(cmd, "PASS") == 0)
				{
					ssend(conn_s_ftp, "230 Already in\r\n");
				}
				else
				if(strcasecmp(cmd, "OPTS") == 0
				|| strcasecmp(cmd, "REIN") == 0 || strcasecmp(cmd, "ADAT") == 0
				|| strcasecmp(cmd, "AUTH") == 0 || strcasecmp(cmd, "CCC" ) == 0
				|| strcasecmp(cmd, "CONF") == 0 || strcasecmp(cmd, "ENC" ) == 0
				|| strcasecmp(cmd, "EPRT") == 0 || strcasecmp(cmd, "EPSV") == 0
				|| strcasecmp(cmd, "LANG") == 0 || strcasecmp(cmd, "LPRT") == 0
				|| strcasecmp(cmd, "LPSV") == 0 || strcasecmp(cmd, "MIC" ) == 0
				|| strcasecmp(cmd, "PBSZ") == 0 || strcasecmp(cmd, "PROT") == 0
				|| strcasecmp(cmd, "SMNT") == 0 || strcasecmp(cmd, "STOU") == 0
				|| strcasecmp(cmd, "XRCP") == 0 || strcasecmp(cmd, "XSEN") == 0
				|| strcasecmp(cmd, "XSEM") == 0 || strcasecmp(cmd, "XRSQ") == 0
				|| strcasecmp(cmd, "STAT") == 0)
				{
					ssend(conn_s_ftp, "502 Not implemented\r\n"); // Command not implemented.
				}
				else
				{
					ssend(conn_s_ftp, "502 Error\r\n"); // Command not implemented.
				}

				if(dataactive == 1)
				{
					dataactive = 0;
				}
				else
				{
					sclose(&data_s);
					if(data_ls>0) {sclose(&data_ls); data_ls=FAILED;}
					rest = 0;
				}
			}
			else if (working)
			{
				// available commands when not logged in
				if(strcasecmp(cmd, "USER") == 0)
				{
					if(split == 1)
					{
						ssend(conn_s_ftp, "331 OK\r\n"); // User name okay, need password.
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "PASS") == 0)
				{
					if(split == 1)
					{
#ifndef LITE_EDITION
						int fd=0;
						if(cellFsOpen(WMTMP "/password.txt", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
						{
							u64 read_e = 0;
							if(cellFsRead(fd, (void *)&ftp_password, 20, &read_e) == CELL_FS_SUCCEEDED) ftp_password[read_e]=0;
							cellFsClose(fd);
							ftp_password[strcspn(ftp_password, "\n")] = '\0';
							ftp_password[strcspn(ftp_password, "\r")] = '\0';
						}
						else
							ftp_password[0]=0;
#endif
						if(ftp_password[0]==0 || strcmp(ftp_password, param) == 0)
						{
							ssend(conn_s_ftp, "230 OK\r\n"); // User logged in, proceed. Logged out if appropriate.
							loggedin = 1;
						}
						else ssend(conn_s_ftp, "430 Error\r\n");
					}
					else
					{
						ssend(conn_s_ftp, FTP_ERROR_501);
					}
				}
				else
				if(strcasecmp(cmd, "QUIT") == 0 || strcasecmp(cmd, "BYE") == 0)
				{
					ssend(conn_s_ftp, FTP_OK_221);
					connactive = 0;
				}
				else
				{
					ssend(conn_s_ftp, "530 Error\r\n"); // Not logged in.
				}
			}
			else
			{
				connactive = 0;
				loggedin = 0;
				break;
			}

		}
		else
		{
			connactive = 0;
			loggedin = 0;
			break;
		}

		sys_timer_usleep(1668);
	}

	sclose(&conn_s_ftp);
	sclose(&data_s);

	sys_ppu_thread_exit(0);
}



static void ftpd_thread(uint64_t arg)
{
	int list_s=FAILED;
relisten:
	if(working) list_s = slisten(FTPPORT, 4);
	else goto end;

	if(working && (list_s<0))
	{
		sys_timer_sleep(3);
		if(working) goto relisten;
		else goto end;
	}

	//if(list_s >= 0)
	{
		while(working)
		{
			sys_timer_usleep(1668);
			int conn_s_ftp;
			if(!working) break;
			else
			if(working &&(conn_s_ftp = accept(list_s, NULL, NULL)) > 0)
			{
				sys_ppu_thread_t id;
				if(working) sys_ppu_thread_create(&id, handleclient_ftp, (u64)conn_s_ftp, -0x1d8, 0x10000, 0, "ftpd");
				else {sclose(&conn_s_ftp); break;}
			}
			else
			if((sys_net_errno==SYS_NET_EBADF) || (sys_net_errno==SYS_NET_ENETDOWN))
			{
				sclose(&list_s);
				list_s=FAILED;
				if(working) goto relisten;
				else break;
			}
		}
	}
end:
	sclose(&list_s);
	sys_ppu_thread_exit(0);
}

/*
	u64 sa=0x03300000ULL;
	u64 offset=0;
	for(u64 i=0;i<0x7700000;i+=8)
	{
		if( peek(0x8000000000000000ULL+i+sa	 )==0xFFFFFF00FFFFFF00ULL &&
			peek(0x8000000000000000ULL+i+sa+8)==0xFFFFFF00FFFFFF00ULL )
		{
			show_msg((char*)"Found match!");
			offset=i+sa+16;
			for(u64 i2=offset;i2<(offset+320*4*90);i2+=8)
				poke(0x8000000000000000ULL+i2, 0x30405060708090A0ULL);
		}
	}
*/

#ifdef DEBUG_MEM
static void peek_chunk(uint64_t start, uint64_t size, uint8_t* buf) // read from lv1
{
	uint64_t i = 0, t = 0;
	for(i = 0; i < size; i += 8)
	{
		t = peek_lv1(start + i); memcpy(buf + i, &t, 8);
	}
}

static void dump_mem(char *file, uint64_t start, uint32_t size_mb)
{
	int fp;
	uint64_t sw;
	uint32_t mem_size = (_128KB_), i;
	sys_addr_t sys_mem = 0;

    if(start < 0x0000028080000000ULL) start |= 0x8000000000000000ULL;

	if(sys_memory_allocate(mem_size, SYS_MEMORY_PAGE_SIZE_64K, &sys_mem)==0)
	{
		uint8_t *mem_buf	= (uint8_t*)sys_mem;

		if(cellFsOpen((char*)file, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fp, NULL, 0)==CELL_FS_SUCCEEDED)
		{
			for(i = 0; i < size_mb*8UL; i++)
			{
				peek_chunk(start + (i * mem_size), mem_size, mem_buf);
				cellFsWrite(fp, mem_buf, mem_size, &sw);
			}
			cellFsClose(fp);
		}
		sys_memory_free((sys_addr_t)sys_mem);
		show_msg((char*)"Memory dump completed!");
		{ BEEP2 }
	}
}
#endif

#ifdef NOSINGSTAR
static void no_singstar_icon(void)
{
	int fd;

	if(cellFsOpendir("/dev_hdd0/tmp/explore/xil2/game", &fd) == CELL_FS_SUCCEEDED)
	{
		u64 read; CellFsDirent dir; char xmlpath[64];
		read = sizeof(CellFsDirent);
		while(!cellFsReaddir(fd, &dir, &read))
		{
			if(!read) break;
			if(dir.d_name[0]=='.') continue;
			if(strlen(dir.d_name)==2)
			{
				sprintf(xmlpath, "/dev_hdd0/tmp/explore/xil2/game/%s/c/db.xml", dir.d_name);
				cellFsUnlink(xmlpath);
			}
		}
		cellFsClosedir(fd);
	}
}
#endif

static void refresh_xml(char *msg)
{
	webman_config->profile=profile; save_settings();

	sprintf(msg, "%s XML%s: %s", STR_REFRESH, SUFIX2(profile), STR_SCAN2);
	show_msg((char*) msg);
	init_running = 1;
	sys_ppu_thread_t id3;
	sys_ppu_thread_create(&id3, handleclient, (u64)REFRESH_CONTENT, -0x1d8, 0x20000, 0, "wwwd2");
	while(init_running && working) sys_timer_usleep(300000);
	sprintf(msg, "%s XML%s: OK", STR_REFRESH, SUFIX2(profile));
	show_msg((char*) msg);
}

static void enable_dev_blind(char *msg)
{
	if(!isDir("/dev_blind"))
		{system_call_8(SC_FS_MOUNT, (u64)(char*)"CELL_FS_IOS:BUILTIN_FLSH1", (u64)(char*)"CELL_FS_FAT", (u64)(char*)"/dev_blind", 0, 0, 0, 0, 0);}

	if(!msg) return;

	show_msg((char*) msg);
	sys_timer_sleep(2);
}

static void poll_thread(uint64_t poll)
{
	/*u8 d0[157];
	u8 d1[157];
	u8 d0t[157];
	u8 d1t[157];
	int u0=0, u1=0;
	char un[128];

	while(working)
	{
		if(u0<128)
		{
			int fd;
			u64 read_e;
			bool toupd0, toupd1;
			cellFsOpen((char*)"/dev_hdd0/vsh/task/00000001/d0.pdb", CELL_FS_O_RDONLY, &fd, NULL, 0);
			cellFsRead(fd, (void *)&d0, 157, &read_e); cellFsClose(fd);
			cellFsOpen((char*)"/dev_hdd0/vsh/task/00000001/d1.pdb", CELL_FS_O_RDONLY, &fd, NULL, 0);
			cellFsRead(fd, (void *)&d1, 157, &read_e); cellFsClose(fd);
			toupd0=0;
			toupd1=0;
			for(u8 b=0;b<157;b++)
			{
				if(d0[b]!=d0t[b]) toupd0=1;
				if(d1[b]!=d1t[b]) toupd1=1;
				d0t[b]=d0[b];
				d1t[b]=d1[b];
			}
			if(toupd0)
			{
				u0++;
				sprintf(un, "/dev_hdd0/tmp/d0-%03i.bin", u0);
				cellFsOpen(un, CELL_FS_O_CREAT|CELL_FS_O_WRONLY, &fd, NULL, 0);
				cellFsWrite(fd, (void *)d0, 157, &read_e);
				cellFsClose(fd);
			}
			if(toupd1)
			{
				u1++;
				sprintf(un, "/dev_hdd0/tmp/d1-%03i.bin", u1);
				cellFsOpen(un, CELL_FS_O_CREAT|CELL_FS_O_WRONLY, &fd, NULL, 0);
				cellFsWrite(fd, (void *)d1, 157, &read_e);
				cellFsClose(fd);
			}
		}
	}
	*/

	u8 to=0;
	u8 sec=0;
	u8 step=3;
	u32 t1=0, t2=0;
	u8 lasttemp=0;
	old_fan=0;
	u8 stall=0;
	u8 step_up=5;
	//u8 step_down=2;
	u8 smoothstep=0;
	int delta=0;
	uint32_t usb_handle = -1;
	uint8_t msg[200];

	uint32_t r;

	CellPadData data;

	old_fan=0;
	while(working)
	{
		if(max_temp)
		{
			t1=0;
			get_temperature(0, &t1); // 3E030000 -> 3E.03°C -> 62.(03/256)°C
			t2=t1;
			sys_timer_usleep(300000);

			get_temperature(1, &t2); // 3E030000 -> 3E.03°C -> 62.(03/256)°C
			sys_timer_usleep(200000);

			t1=t1>>24;
			t2=t2>>24;

			if(t2>t1) t1=t2;

			if(!lasttemp) lasttemp=t1;

			delta=(lasttemp-t1);

			lasttemp=t1;

			if(t1>=max_temp || t1>84)
			{
				if(delta< 0) fan_speed+=2;
				if(delta==0 && t1!=(max_temp-1)) fan_speed++;
				if(delta==0 && t1>=(max_temp+1)) fan_speed+=(2+(t1-max_temp));
				if(delta> 0)
				{
					smoothstep++;
					if(smoothstep>1)
					{
						fan_speed--;
						smoothstep=0;
					}
				}
				if(t1>84)	 fan_speed+=step_up;
				if(delta< 0 && (t1-max_temp)>=2) fan_speed+=step_up;
			}
			else
			{
				if(delta< 0 && t1>=(max_temp-1)) fan_speed+=2;
				if(delta==0 && t1<=(max_temp-2))
				{
					smoothstep++;
					if(smoothstep>1)
					{
						fan_speed--;
						if(t1<=(max_temp-5)) {fan_speed--; if(fan_speed>0xC0) fan_speed--;} // 75%
						if(t1<=(max_temp-8)) {fan_speed--; if(fan_speed>0x99) fan_speed--;} // 60%
						smoothstep=0;
					}
				}
				//if(delta==0 && t1>=(max_temp-1)) fan_speed++;
				if(delta> 0)
				{
					//smoothstep++;
					//if(smoothstep)
					{
						fan_speed--;
						smoothstep=0;
					}
				}
			}

			if(t1>76 && old_fan<0x43) fan_speed++;
			if(t1>84 && fan_speed<0xB0) {old_fan=0; fan_speed=0xB0;}

			if(fan_speed<((webman_config->minfan*255)/100)) fan_speed=(webman_config->minfan*255)/100;
			if(fan_speed>MAX_FANSPEED) fan_speed=MAX_FANSPEED;

			//sprintf(debug, "OFAN: %x | CFAN: %x | TEMP: %i | STALL: %i\r\n", old_fan, fan_speed, t1, stall);	ssend(data_s, mytxt);
			//if(abs(old_fan-fan_speed)>=0x0F || stall>35 || (abs(old_fan-fan_speed) /*&& webman_config->aggr*/))
			if(old_fan!=fan_speed || stall>35)
			{
				//if(t1>76 && fan_speed<0x50) fan_speed=0x50;
				//if(t1>77 && fan_speed<0x58) fan_speed=0x58;
				if(t1>78 && fan_speed<0x50) fan_speed+=2;
				if(old_fan!=fan_speed)
				{
				old_fan=fan_speed;
				fan_control(fan_speed, 1);
				//sprintf(debug, "OFAN: %x | CFAN: %x | TEMP: %i | SPEED APPLIED!\r\n", old_fan, fan_speed, t1); ssend(data_s, mytxt);
				stall=0;
			}
			}
			else
				if( old_fan>fan_speed && (old_fan-fan_speed)>8 && t1<(max_temp-3) )
					stall++;
		}

/*
 FAIL SAFE    : SELECT+L3+L2+R2
 RESET SAFE   : SELECT+R3+L2+R2

 REFRESH XML  : SELECT+L3 (+R2=profile1, +L2=profile2)
 UNLOAD WM    : L3+R2+R3

 PREV GAME    : SELECT+L1
 NEXT GAME    : SELECT+R1
 SHUTDOWN     : L3+R2+X
 RESTART      : L3+R2+O

 FAN CNTRL    : L3+R2+START
 SHOW TEMP    : SELECT+R3 / SELECT+START
 DYNAMIC TEMP : SELECT+LEFT/RIGHT
 MANUAL TEMP  : SELECT+UP/DOWN

 SYSCALLS     : R2+TRIANGLE
 SHOW IDPS    : R2+O
 OFFLINE MODE : R2+口

 EXT GAME DATA: SELECT+口
 MOUNT net0/  : SELECT+R2+口
 MOUNT net1/  : SELECT+L2+口

 TOGGLE PS2CLASSIC    : SELECT+L2+TRIANGLE
 SWITCH PS2EMU        : SELECT+L2+R2

 COBRA TOGGLE         : L3+L2+TRIANGLE
 REBUG  Mode Switcher : L3+L2+口
 Normal Mode Switcher : L3+L2+O
 DEBUG  Menu Switcher : L3+L2+X
*/
		bool reboot = false;

#ifdef COBRA_ONLY
		struct CellFsStat s;
#endif

		for(u8 n=0;n<10;n++)
		{
			if(!webman_config->nopad)
			{
				data.len=0;
				if(cellPadGetData(0, &data) != CELL_PAD_OK)
					if(cellPadGetData(1, &data) != CELL_PAD_OK)
						if(cellPadGetData(2, &data) != CELL_PAD_OK) {sys_timer_usleep(300000); continue;}

				if(data.len > 0)
				{
					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT))
					{
						if( !(webman_config->combo2 & (EXTGAMDAT | MOUNTNET0 | MOUNTNET1))         // Toggle External Game Data
                            && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE)) // SELECT+SQUARE
						{
#ifndef LITE_EDITION
							if(!(webman_config->combo2 & MOUNTNET0) &&
								(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2))
							{if(webman_config->netp0 && webman_config->neth0[0]) mount_with_mm((char*)"/net0", 1);}
							else
							if(!(webman_config->combo2 & MOUNTNET1) &&
								(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2))
							{if(webman_config->netp1 && webman_config->neth1[0]) mount_with_mm((char*)"/net1", 1);}
							else
#endif

#ifdef EXT_GDATA
							set_gamedata_status(extgd^1, true);
							sys_timer_sleep(2);
#endif
							break;
						}
						else
						if( !(webman_config->combo & FAIL_SAFE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) // fail-safe mode
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) // SELECT+L3+L2+R2
							)
						{
							cellFsUnlink((char*)"/dev_hdd0/boot_plugins.txt");
							goto reboot;
						}
						else
						if( (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) // reset-safe mode
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) // SELECT+R3+L2+R2
							)
						{
							cellFsUnlink(WMCONFIG);
							{ BEEP1 }
							show_msg((char*)STR_RMVWMCFG);
							sys_timer_sleep(2);
							show_msg((char*)STR_RMVWMCFGOK);
							sys_timer_sleep(3);
							goto reboot;
						}
#ifdef COBRA_ONLY
						else
						if( !(webman_config->combo2 & PS2TOGGLE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE) // SELECT+L2+TRIANGLE
							&& (c_firmware>=4.65f) )
						{
							bool classic_ps2_enabled = (cellFsStat((char*)PS2_CLASSIC_TOGGLER, &s)==CELL_FS_SUCCEEDED);

							if(classic_ps2_enabled)
							{
								disable_classic_ps2_mode();
							}
							else
							{
								enable_classic_ps2_mode();
							}

							sprintf((char*) msg, (char*)"PS2 Classic %s", classic_ps2_enabled ? STR_DISABLED : STR_ENABLED);
							show_msg((char*) msg);
							sys_timer_sleep(3);
						}
						else
						if( !(webman_config->combo2 & PS2SWITCH)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) // Clone ps2emu habib's switcher
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) // SELECT+L2+R2
							&& (c_firmware>=4.53f) )
						{
								enable_dev_blind((char*)"Swapping ps2emu activated!");
 #ifdef REX_ONLY
								if(cellFsStat((char*)REBUG_TOOLBOX "ps2_netemu.self", &s)==CELL_FS_SUCCEEDED)
								{
									uint64_t size1, size2;

									// ---- Backup PS2Emus to Rebug Toolbox folder ----
									if( cellFsStat( (char*)REBUG_TOOLBOX "ps2_netemu.self.cobra", &s)!=CELL_FS_SUCCEEDED )
										  filecopy( (char*)PS2_EMU_PATH  "ps2_netemu.self",
													(char*)REBUG_TOOLBOX "ps2_netemu.self.cobra", COPY_WHOLE_FILE);

									if( cellFsStat( (char*)REBUG_TOOLBOX "ps2_gxemu.self.cobra", &s)!=CELL_FS_SUCCEEDED )
										  filecopy( (char*)PS2_EMU_PATH  "ps2_gxemu.self",
													(char*)REBUG_TOOLBOX "ps2_gxemu.self.cobra", COPY_WHOLE_FILE);

									if( cellFsStat( (char*)REBUG_TOOLBOX "ps2_emu.self.cobra", &s)!=CELL_FS_SUCCEEDED )
										  filecopy( (char*)PS2_EMU_PATH  "ps2_emu.self",
													(char*)REBUG_TOOLBOX "ps2_emu.self.cobra", COPY_WHOLE_FILE);

									// ---- Swap ps2_netemu.self ----
									size1 = size2 = 0;
									if( cellFsStat((char*)PS2_EMU_PATH  "ps2_netemu.self", &s)==CELL_FS_SUCCEEDED) size1 = s.st_size;
									if( cellFsStat((char*)REBUG_TOOLBOX "ps2_netemu.self", &s)==CELL_FS_SUCCEEDED) size2 = s.st_size;

									show_msg((size1==size2) ?   (char*)"Restoring original Cobra ps2emu...":
																(char*)"Switching to custom ps2emu...");

									if(size1>0 && size2>0)
										filecopy((size1==size2) ?   (char*)REBUG_TOOLBOX "ps2_netemu.self.cobra":
																	(char*)REBUG_TOOLBOX "ps2_netemu.self",
																	(char*)PS2_EMU_PATH  "ps2_netemu.self", COPY_WHOLE_FILE);

									// ---- Swap ps2_gxemu.self ----
									size1 = size2 = 0;
									if( cellFsStat((char*)PS2_EMU_PATH  "ps2_gxemu.self", &s)==CELL_FS_SUCCEEDED) size1 = s.st_size;
									if( cellFsStat((char*)REBUG_TOOLBOX "ps2_gxemu.self", &s)==CELL_FS_SUCCEEDED) size2 = s.st_size;

									if(size1>0 && size2>0)
										filecopy((size1==size2) ?   (char*)REBUG_TOOLBOX "ps2_gxemu.self.cobra":
																	(char*)REBUG_TOOLBOX "ps2_gxemu.self",
																	(char*)PS2_EMU_PATH  "ps2_gxemu.self", COPY_WHOLE_FILE);

									// ---- Swap ps2_emu.self ----
									size1 = size2 = 0;
									if( cellFsStat((char*)PS2_EMU_PATH  "ps2_emu.self", &s)==CELL_FS_SUCCEEDED) size1 = s.st_size;
									if( cellFsStat((char*)REBUG_TOOLBOX "ps2_emu.self", &s)==CELL_FS_SUCCEEDED) size2 = s.st_size;

									if(size1>0 && size2>0)
										filecopy((size1==size2) ?   (char*)REBUG_TOOLBOX "ps2_emu.self.cobra":
																	(char*)REBUG_TOOLBOX "ps2_emu.self",
																	(char*)PS2_EMU_PATH  "ps2_emu.self", COPY_WHOLE_FILE);
								}
								else
 #endif //#ifdef REX_ONLY
								if(cellFsStat((char*)PS2_EMU_PATH "ps2_netemu.self.swap", &s)==CELL_FS_SUCCEEDED)
								{
									show_msg((char*)"Switch to custom ps2emu...");

									cellFsRename(PS2_EMU_PATH "ps2_netemu.self"     , PS2_EMU_PATH "ps2_netemu.tmp");
									cellFsRename(PS2_EMU_PATH "ps2_netemu.self.swap", PS2_EMU_PATH "ps2_netemu.self");

									cellFsRename(PS2_EMU_PATH "ps2_gxemu.self"      , PS2_EMU_PATH "ps2_gxemu.tmp");
									cellFsRename(PS2_EMU_PATH "ps2_gxemu.self.swap" , PS2_EMU_PATH "ps2_gxemu.self");

									cellFsRename(PS2_EMU_PATH "ps2_emu.self"        , PS2_EMU_PATH "ps2_emu.tmp");
									cellFsRename(PS2_EMU_PATH "ps2_emu.self.swap"   , PS2_EMU_PATH "ps2_emu.self");
								}
								else if(cellFsStat((char*)PS2_EMU_PATH "ps2_netemu.self.sp", &s)==CELL_FS_SUCCEEDED)
								{
									show_msg((char*)"Switching to custom ps2emu...");

									cellFsRename(PS2_EMU_PATH "ps2_netemu.self"   , PS2_EMU_PATH "ps2_netemu.tmp");
									cellFsRename(PS2_EMU_PATH "ps2_netemu.self.sp", PS2_EMU_PATH "ps2_netemu.self");

									cellFsRename(PS2_EMU_PATH "ps2_gxemu.self"    , PS2_EMU_PATH "ps2_gxemu.tmp");
									cellFsRename(PS2_EMU_PATH "ps2_gxemu.self.sp" , PS2_EMU_PATH "ps2_gxemu.self");

									cellFsRename(PS2_EMU_PATH "ps2_emu.self"      , PS2_EMU_PATH "ps2_emu.tmp");
									cellFsRename(PS2_EMU_PATH "ps2_emu.self.sp"   , PS2_EMU_PATH "ps2_emu.self");
								}
								else if(cellFsStat(PS2_EMU_PATH "ps2_netemu.tmp", &s)==CELL_FS_SUCCEEDED)
								{
									show_msg((char*)"Restoring original ps2emu...");

									if(c_firmware>=4.65f)
									{
										cellFsRename(PS2_EMU_PATH "ps2_netemu.self", PS2_EMU_PATH "ps2_netemu.self.swap");
										cellFsRename(PS2_EMU_PATH "ps2_netemu.tmp" , PS2_EMU_PATH "ps2_netemu.self");

										cellFsRename(PS2_EMU_PATH "ps2_gxemu.self" , PS2_EMU_PATH "ps2_gxemu.self.swap");
										cellFsRename(PS2_EMU_PATH "ps2_gxemu.tmp"  , PS2_EMU_PATH "ps2_gxemu.self");

										cellFsRename(PS2_EMU_PATH "ps2_emu.self"   , PS2_EMU_PATH "ps2_emu.self.swap");
										cellFsRename(PS2_EMU_PATH "ps2_emu.tmp"    , PS2_EMU_PATH "ps2_emu.self");
									}
									else
									{
										cellFsRename(PS2_EMU_PATH "ps2_netemu.self", PS2_EMU_PATH "ps2_netemu.self.sp");
										cellFsRename(PS2_EMU_PATH "ps2_netemu.tmp" , PS2_EMU_PATH "ps2_netemu.self");

										cellFsRename(PS2_EMU_PATH "ps2_gxemu.self" , PS2_EMU_PATH "ps2_gxemu.self.sp");
										cellFsRename(PS2_EMU_PATH "ps2_gxemu.tmp"  , PS2_EMU_PATH "ps2_gxemu.self");

										cellFsRename(PS2_EMU_PATH "ps2_emu.self"   , PS2_EMU_PATH "ps2_emu.self.sp");
										cellFsRename(PS2_EMU_PATH "ps2_emu.tmp"    , PS2_EMU_PATH "ps2_emu.self");
									}
								}
						}
#endif //#ifdef COBRA_ONLY

						else
						if(!(webman_config->combo2 & XMLREFRSH) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) ) // SELECT+L3 refresh XML
						{
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2) profile=1; else
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L2) profile=2; else
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R1) profile=3; else
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L1) profile=4; else profile=0;

							refresh_xml((char*)msg);
						}
                        else
						if(!(webman_config->combo & SHOW_TEMP) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & (CELL_PAD_CTRL_R3 | CELL_PAD_CTRL_START))) // SELECT+START show temperatures / hdd space
						{
#ifdef VIDEO_REC
							if(!(webman_config->combo2 & VIDRECORD) && data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) // SELECT + R3
							{
								toggle_video_rec();
								sys_timer_sleep(2);
								break;
							}
#endif

#ifdef EXTRA_FEAT
							if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == (CELL_PAD_CTRL_R2 | CELL_PAD_CTRL_L2) )
								saveBMP();
							else
#endif
							{
								CellRtcTick pTick; cellRtcGetCurrentTick(&pTick); u32 dd, hh, mm, ss; char tmp[200];

								get_temperature(0, &t1);
								get_temperature(1, &t2);

								uint32_t blockSize;
								uint64_t freeSize;
								cellFsGetFreeSize((char*)"/dev_hdd0", &blockSize, &freeSize);

								u8 st, mode, speed, unknown;
								if(get_fan_policy_offset)
								{
									if(!webman_config->fanc)
									{
										backup[5]=peekq(get_fan_policy_offset);
										lv2poke32(get_fan_policy_offset, 0x38600001); // sys 409 get_fan_policy  4.55/4.60/4.65
									}

									sys_sm_get_fan_policy(0, &st, &mode, &speed, &unknown);

									if(!webman_config->fanc)
									{
										pokeq(get_fan_policy_offset, backup[5]); // sys 409 get_fan_policy  4.55/4.60/4.65
									}
								}
								_meminfo meminfo;
								{system_call_1(SC_GET_FREE_MEM, (uint64_t)(u32) &meminfo);}

								// detect aprox. time when a game is launched
								if(View_Find("game_plugin")==0) gTick=rTick; else if(gTick.tick==rTick.tick) cellRtcGetCurrentTick(&gTick);

								bool R2 = (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R2), bb;

								///// startup/play time /////
								bb = (!R2 && gTick.tick>rTick.tick); // show play time
								ss = (u32)((pTick.tick-(bb?gTick.tick:rTick.tick))/1000000); dd = (u32)(ss / 86400);
								if(dd>100) {bb=false; ss = (u32)((pTick.tick-rTick.tick)/1000000); dd = (u32)(ss / 86400);}
								ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;
								/////////////////////////////

								char cfw_info[20];
#ifdef COBRA_ONLY
								#define SYSCALL8_OPCODE_GET_MAMBA           0x7FFFULL
								bool is_mamba; {system_call_1(8, SYSCALL8_OPCODE_GET_MAMBA); is_mamba = ((int)p1 ==0x666);}

								uint16_t cobra_version; sys_get_version2(&cobra_version);
								sprintf(cfw_info, "%s %s: %X.%X", dex_mode ? "DEX" : "CEX", is_mamba ? "Mamba" : "Cobra", cobra_version>>8, (cobra_version & 0xF) ? (cobra_version & 0xFF) : ((cobra_version>>4) & 0xF));
#else
								sprintf(cfw_info, "%s", dex_mode ? "DEX" : "CEX");
#endif
								sprintf((char*)tmp, "CPU: %i°C  RSX: %i°C  FAN: %i%%   \r\n"
													"%s: %id %02d:%02d:%02d\r\n"
													"Firmware : %i.%02i %s\r\n",
													t1>>24, t2>>24, (int)(((int)speed*100)/255),
													bb?"Play":"Startup", dd, hh, mm, ss,
													(int)c_firmware, ((u32)(c_firmware * 1000.0f) % 1000) / 10, cfw_info);

								sprintf((char*)msg, "%s\r\n%s: %i %s\r\n"
													"%s: %i %s", tmp,
													STR_STORAGE, (int)((blockSize*freeSize)>>20), STR_MBFREE,
													STR_MEMORY, meminfo.avail>>10, STR_KBFREE);

								if(R2 && gTick.tick>rTick.tick)
								{
									////// play time //////
									ss = (u32)((pTick.tick-gTick.tick)/1000000);
									dd = (u32)(ss / 86400); ss = ss % 86400; hh = (u32)(ss / 3600); ss = ss % 3600; mm = (u32)(ss / 60); ss = ss % 60;

									if(dd<100) {char gname[200]; get_game_info(); sprintf(gname, "%s %s\r\n\r\n", _game_TitleID, _game_Title); sprintf((char*) msg, "%sPlay: %id %02d:%02d:%02d\r\n%s", gname, dd, hh, mm, ss, tmp); }
								}

								show_msg((char*) msg);
								sys_timer_sleep(2);

								/////////////////////////////
#ifdef COPY_PS3
								if(copy_in_progress)
								{
									sprintf((char*) msg, "<hr>%s %s (%i %s)", STR_COPYING, current_file, copied_count, STR_FILES);
									show_msg((char*) msg);
									sys_timer_sleep(2);
								}
								else
								if(fix_in_progress)
								{
									sprintf((char*) msg, "%s %s", STR_FIXING, current_file);
									show_msg((char*) msg);
									sys_timer_sleep(2);
								}
#endif
								/////////////////////////////
							}
						}
						else
						if(webman_config->fanc && !(webman_config->combo & MANUALFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_UP) ) // SELECT+UP increase TEMP/FAN
						{
							if(max_temp) //auto mode
							{
								if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) max_temp+=5; else max_temp+=1;
								if(max_temp>85) max_temp=85;
								webman_config->temp1=max_temp;
								sprintf((char*) msg, "%s\r\n%s %i°C", STR_FANCH0, STR_FANCH1, max_temp);
							}
							else
							{
								if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) webman_config->manu+=5; else webman_config->manu+=1;
								webman_config->manu=RANGE(webman_config->manu, 20, 99); //%
								webman_config->temp0= (u8)(((float)webman_config->manu * 255.f)/100.f);
								webman_config->temp0=RANGE(webman_config->temp0, 0x33, MAX_FANSPEED);
								fan_control(webman_config->temp0, 0);
								sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH2, webman_config->manu);
							}
							save_settings();
							show_msg((char*) msg);
							sys_timer_sleep(2);
						}
						else
						if(webman_config->fanc && !(webman_config->combo & MANUALFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_DOWN) ) // SELECT+DOWN increase TEMP/FAN
						{
							if(max_temp) //auto mode
							{
								if(max_temp>30) {if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) max_temp-=5; else max_temp-=1;}
								webman_config->temp1=max_temp;
								sprintf((char*) msg, "%s\r\n%s %i°C", STR_FANCH0, STR_FANCH1, max_temp);
							}
							else
							{
								if(webman_config->manu>20) {if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) webman_config->manu-=5; else webman_config->manu-=1;}
								webman_config->temp0= (u8)(((float)webman_config->manu * 255.f)/100.f);
								if(webman_config->temp0<0x33) webman_config->temp0=0x33;
								if(webman_config->temp0>MAX_FANSPEED) webman_config->temp0=MAX_FANSPEED;
								fan_control(webman_config->temp0, 0);
								sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH2, webman_config->manu);
							}
							save_settings();
							show_msg((char*) msg);
							sys_timer_sleep(2);
						}
						else
						if(webman_config->minfan && !(webman_config->combo & MINDYNFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_LEFT) ) // SELECT+LEFT decrease Minfan
						{
							if(webman_config->minfan-5>=MIN_FANSPEED) webman_config->minfan-=5;
							sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH3, webman_config->minfan);

							save_settings();
							show_msg((char*) msg);
							sys_timer_sleep(2);
						}
						else
						if(webman_config->minfan && !(webman_config->combo & MINDYNFAN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_RIGHT) ) // SELECT+RIGHT increase Minfan
						{
							if(webman_config->minfan+5<100) webman_config->minfan+=5;
							sprintf((char*) msg, "%s\r\n%s %i%%", STR_FANCH0, STR_FANCH3, webman_config->minfan);

							save_settings();
							show_msg((char*) msg);
							sys_timer_sleep(2);
						}
						else
						if(!(webman_config->combo & PREV_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_L1) ) // SELECT+L1 (previous title)
						{
							led(GREEN, BLINK_FAST);
							mount_with_mm((char*)"_prev", 1);
							sys_timer_sleep(3);
							led(GREEN, ON);
						}
						else
						if(!(webman_config->combo & NEXT_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_R1) ) // SELECT+R1 (next title)
						{
							led(GREEN, BLINK_FAST);
							mount_with_mm((char*)"_next", 1);
							sys_timer_sleep(3);
							led(GREEN, ON);
						}
						else
						if(!(webman_config->combo & UMNT_GAME) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] == CELL_PAD_CTRL_CIRCLE) ) // SELECT+O (unmount)
						{
							do_umount(true);
						}
					}
					else
					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2))
					{
						if(!(webman_config->combo & SHUT_DOWN) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS) ) // L3+R2+X (shutdown)
						{
							// power off
							working = 0;
							{ DELETE_TURNOFF } { BEEP1 }
							{system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0);}
							sys_ppu_thread_exit(0);
						}
						else if(!(webman_config->combo & RESTARTPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) ) // L3+R2+O (restart)
						{
							// reboot
							{ BEEP2 }
							goto reboot;
						}
						else if(!(webman_config->combo & UNLOAD_WM) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) ) // L3+R3+R2 (quit webMAN)
						{
							if(!webman_config->fanc || webman_config->ps2temp<33)
								restore_fan(0); //restore syscon fan control mode
							else
								restore_fan(1); //set ps2 fan control mode

							working = 0;
							wm_unload_combo = 1;

							stop_prx_module();
							sys_ppu_thread_exit(0);
							break;
						}
						else if(!(webman_config->combo & DISABLEFC) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_START) ) // L3+R2+START (enable/disable fancontrol)
						{
							webman_config->fanc = (webman_config->fanc ? 0 : 1);

							max_temp=0;
							if(webman_config->fanc)
							{
								if(webman_config->temp0==0) max_temp=webman_config->temp1; else max_temp=0;
								fan_control(webman_config->temp0, 0);
								sprintf((char*) msg, "%s %s", STR_FANCTRL3, STR_ENABLED);
							}
							else
							{
								restore_fan(0); //syscon
								sprintf((char*) msg, "%s %s", STR_FANCTRL3, STR_DISABLED);
							}
							save_settings();
							show_msg((char*) msg);
							sys_timer_sleep(2);
							break;
						}
					}
					else
					if(!(webman_config->combo & RESTARTPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R1))
					{
						if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) // L3+R1+O (soft restart)
						{
							// soft reboot
							working = 0;
							{ DELETE_TURNOFF }
							{system_call_4(SC_SYS_POWER,SYS_SOFT_REBOOT, 0, 0, 0);}
							sys_ppu_thread_exit(0);
						}
					}
					else
					if(data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2)
					{
						if((copy_in_progress || fix_in_progress) && data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) // R2+O Abort copy process
						{
							fix_aborted=copy_aborted=true;
						}
#ifdef REMOVE_SYSCALLS
						else
						if(!(webman_config->combo & DISABLESH) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE) ) // R2+TRIANGLE Disable CFW Sycalls
						{
							if(peekq(0x8000000000003000ULL)==SYSCALLS_UNAVAILABLE) {
								{ BEEP2 }
								show_msg((char*)STR_CFWSYSALRD);
								sys_timer_sleep(2);
							} else {
								show_msg((char*)STR_CFWSYSRIP);
								remove_cfw_syscalls();
								delete_history(true);
								if(peekq(0x8000000000003000ULL)==SYSCALLS_UNAVAILABLE) {
									{ BEEP1 }
									show_msg((char*)STR_RMVCFWSYS);
									sys_timer_sleep(2);
								} else {
									{ BEEP2 }
									show_msg((char*)STR_RMVCFWSYSF);
									sys_timer_sleep(2);
								}
							}
						}
#endif
						else
						if(!(webman_config->combo2 & BLOCKSVRS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE) ) // R2+SQUARE
						{
							show_msg((char*)"Blocking servers");
							block_online_servers();
							show_msg((char*)"Servers blocked");
						}
						else
						if(!(webman_config->combo & SHOW_IDPS) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE) ) // R2+O Show IDPS EID0+LV2
						{
							vshmain_is_ss_enabled = (void*)((int)getNIDfunc("vshmain", 0x981D7E9F, 0)); //is screenshot enabled?


							if(vshmain_is_ss_enabled()==0)
							{
								set_SSHT_ = (uint32_t*)&opd;
								memcpy(set_SSHT_, vshmain_is_ss_enabled, 8);
								opd[0] -= 0x2C; // Sub before vshmain_981D7E9F sets Screenshot Flag
								set_SSHT_(1);	// enable screenshot

								show_msg((char*)"Screenshot enabled");
								sys_timer_sleep(2);
							}

							uint64_t eid0_idps[2], buffer[0x40], start_sector;
							uint32_t read;
							sys_device_handle_t source;
							if(sys_storage_open(0x100000000000004ULL, 0, &source, 0)!=0)
							{
								start_sector = 0x204;
								sys_storage_close(source);
								sys_storage_open(0x100000000000001ULL, 0, &source, 0);
							}
							else start_sector = 0x178;
							sys_storage_read(source, 0, start_sector, 1, buffer, &read, 0);
							sys_storage_close(source);

							eid0_idps[0]=buffer[0x0E];
							eid0_idps[1]=buffer[0x0F];

							get_idps_psid();

							#define SEP "\n                  "
							sprintf((char*) msg, "IDPS EID0 : %016llX" SEP
															 "%016llX\n"
												 "IDPS LV2  : %016llX" SEP
															 "%016llX\r\n"
												 "PSID LV2 : %016llX" SEP
															"%016llX", eid0_idps[0], eid0_idps[1], IDPS[0], IDPS[1], PSID[0], PSID[1]);
							show_msg((char*) msg);
							sys_timer_sleep(2);
						}
					}
					else
					if((data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_L3) && (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2))
					{
#ifdef COBRA_ONLY
						if(!(webman_config->combo & DISACOBRA)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_TRIANGLE))
						{ // L3+L2+TRIANGLE COBRA Toggle
							enable_dev_blind((char*)"COBRA Toggle activated!");
 #ifdef REX_ONLY
							if( (cellFsStat((char*) REBUG_COBRA_PATH "stage2.cex", &s)==CELL_FS_SUCCEEDED) &&
								(cellFsStat((char*) REBUG_COBRA_PATH "stage2.dex", &s)==CELL_FS_SUCCEEDED))
							{
								show_msg((char*)"REBUG COBRA is active!\r\nDeactivating COBRA...");

								cellFsRename(REBUG_COBRA_PATH "stage2.cex", REBUG_COBRA_PATH "stage2.cex.bak");
								cellFsRename(REBUG_COBRA_PATH "stage2.dex", REBUG_COBRA_PATH "stage2.dex.bak");
								reboot=true;
							}
							else if((cellFsStat((char*) REBUG_COBRA_PATH "stage2.cex.bak", &s)==CELL_FS_SUCCEEDED) &&
									(cellFsStat((char*) REBUG_COBRA_PATH "stage2.dex.bak", &s)==CELL_FS_SUCCEEDED))
							{
								show_msg((char*)"REBUG COBRA is inactive!\r\nActivating COBRA...");

								cellFsRename(REBUG_COBRA_PATH "stage2.cex.bak", REBUG_COBRA_PATH "stage2.cex");
								cellFsRename(REBUG_COBRA_PATH "stage2.dex.bak", REBUG_COBRA_PATH "stage2.dex");
								reboot=true;
							}
 #else

							if(cellFsStat((char*)HABIB_COBRA_PATH "stage2.cex", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"COBRA is active!\r\nDeactivating COBRA...");

								cellFsRename(HABIB_COBRA_PATH "stage2.cex", HABIB_COBRA_PATH "stage2_disabled.cex");

								reboot=true;
							}
							else if(cellFsStat((char*)HABIB_COBRA_PATH "stage2_disabled.cex", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"COBRA is inactive!\r\nActivating COBRA...");

								cellFsRename(HABIB_COBRA_PATH "stage2_disabled.cex", HABIB_COBRA_PATH "stage2.cex");

								reboot=true;
							}

							if(cellFsStat((char*)SYS_COBRA_PATH "stage2.bin", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"COBRA is active!\r\nDeactivating COBRA...");

								cellFsRename(SYS_COBRA_PATH "stage2.bin", SYS_COBRA_PATH "stage2_disabled.bin");

								if(cellFsStat((char*)COLDBOOT_PATH ".normal", &s)==CELL_FS_SUCCEEDED)
								{
									cellFsRename(COLDBOOT_PATH          , COLDBOOT_PATH ".cobra");
									cellFsRename(COLDBOOT_PATH ".normal", COLDBOOT_PATH);
								}

								reboot=true;
							}
							else if(cellFsStat((char*)SYS_COBRA_PATH "stage2_disabled.bin", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"COBRA is inactive!\r\nActivating COBRA...");

								cellFsRename(SYS_COBRA_PATH "stage2_disabled.bin", SYS_COBRA_PATH "stage2.bin");

								if(cellFsStat((char*)COLDBOOT_PATH ".cobra", &s)==CELL_FS_SUCCEEDED)
								{
									cellFsRename(COLDBOOT_PATH         , COLDBOOT_PATH ".normal");
									cellFsRename(COLDBOOT_PATH ".cobra", COLDBOOT_PATH);
								}

								reboot=true;
							}
 #endif //#ifdef REX_ONLY
						}
#endif //#ifdef COBRA_ONLY

#ifdef REX_ONLY
						if(!(webman_config->combo2 & REBUGMODE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_SQUARE))
						{ // L3+L2+口 REBUG Mode Switcher
							enable_dev_blind((char*)"REBUG Mode Switcher activated!");

							if(cellFsStat((char*) VSH_MODULE_PATH "vsh.self.swp", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"Normal Mode detected!\r\nSwitch to REBUG Mode Debug XMB...");
								sys_timer_sleep(3);

								cellFsRename(VSH_ETC_PATH "index.dat", VSH_ETC_PATH "index.dat.nrm");
								cellFsRename(VSH_ETC_PATH "index.dat.swp", VSH_ETC_PATH "index.dat");

								cellFsRename(VSH_ETC_PATH "version.txt", VSH_ETC_PATH "version.txt.nrm");
								cellFsRename(VSH_ETC_PATH "version.txt.swp", VSH_ETC_PATH "version.txt");

								cellFsRename(VSH_MODULE_PATH "vsh.self", VSH_MODULE_PATH "vsh.self.nrm");
								cellFsRename(VSH_MODULE_PATH "vsh.self.swp", VSH_MODULE_PATH "vsh.self");

								reboot=true;
							}
							else
							if((cellFsStat((char*) VSH_MODULE_PATH "vsh.self.nrm", &s)==CELL_FS_SUCCEEDED)
							&& (cellFsStat((char*) VSH_MODULE_PATH "vsh.self.cexsp", &s)==CELL_FS_SUCCEEDED))
							{
								show_msg((char*)"REBUG Mode Debug XMB detected!\r\nSwitch to Retail XMB...");
								sys_timer_sleep(3);

								cellFsRename(VSH_MODULE_PATH "vsh.self", VSH_MODULE_PATH "vsh.self.dexsp");
								cellFsRename(VSH_MODULE_PATH "vsh.self.cexsp", VSH_MODULE_PATH "vsh.self");

								reboot=true;
							}
							else
							if(cellFsStat((char*) VSH_MODULE_PATH "vsh.self.dexsp", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"REBUG Mode Retail XMB detected!\r\nSwitch to Debug XMB...");
								sys_timer_sleep(3);

								cellFsRename(VSH_MODULE_PATH "vsh.self", VSH_MODULE_PATH "vsh.self.cexsp");
								cellFsRename(VSH_MODULE_PATH "vsh.self.dexsp", VSH_MODULE_PATH "vsh.self");

								reboot=true;
							}
						}
						else
						if(!(webman_config->combo2 & NORMAMODE)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CIRCLE))
						{ // L3+L2+O Normal Mode Switcher
							enable_dev_blind((char*)"Normal Mode Switcher activated!");

							if((cellFsStat((char*) VSH_MODULE_PATH "vsh.self.nrm", &s)==CELL_FS_SUCCEEDED)
							&& (cellFsStat(VSH_MODULE_PATH "vsh.self.cexsp", &s)==CELL_FS_SUCCEEDED))
							{
								show_msg((char*)"REBUG Mode Debug XMB detected!\r\nSwitch to Normal Mode...");

								cellFsRename(VSH_ETC_PATH "index.dat", VSH_ETC_PATH "index.dat.swp");
								cellFsRename(VSH_ETC_PATH "index.dat.nrm", VSH_ETC_PATH "index.dat");

								cellFsRename(VSH_ETC_PATH "version.txt", VSH_ETC_PATH "version.txt.swp");
								cellFsRename(VSH_ETC_PATH "version.txt.nrm", VSH_ETC_PATH "version.txt");

								cellFsRename(VSH_MODULE_PATH "vsh.self", VSH_MODULE_PATH "vsh.self.swp");
								cellFsRename(VSH_MODULE_PATH "vsh.self.nrm", VSH_MODULE_PATH "vsh.self");

								reboot=true;
							}
							else
							if(cellFsStat((char*) VSH_MODULE_PATH "vsh.self.dexsp", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"REBUG Mode Retail XMB detected!\r\nSwitch to Normal Mode...");

								cellFsRename(VSH_ETC_PATH "index.dat", VSH_ETC_PATH "index.dat.swp");
								cellFsRename(VSH_ETC_PATH "index.dat.nrm", VSH_ETC_PATH "index.dat");

								cellFsRename(VSH_ETC_PATH "version.txt", VSH_ETC_PATH "version.txt.swp");
								cellFsRename(VSH_ETC_PATH "version.txt.nrm", VSH_ETC_PATH "version.txt");

								cellFsRename(VSH_MODULE_PATH "vsh.self.dexsp", VSH_MODULE_PATH "vsh.self.swp");
								cellFsRename(VSH_MODULE_PATH "vsh.self", VSH_MODULE_PATH "vsh.self.cexsp");
								cellFsRename(VSH_MODULE_PATH "vsh.self.nrm", VSH_MODULE_PATH "vsh.self");

								reboot=true;
							}
							else
							if(cellFsStat((char*) VSH_MODULE_PATH "vsh.self.swp", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"Normal Mode detected!\r\nNo need to switch!");
								sys_timer_sleep(3);
								{system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}
								break;
							}
						}
						else
						if(!(webman_config->combo2 & DEBUGMENU)
							&& (data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_CROSS))
						{ // L3+L2+X DEBUG Menu Switcher
							enable_dev_blind((char*)"Debug Menu Switcher activated!");

							if(cellFsStat((char*) VSH_MODULE_PATH "sysconf_plugin.sprx.dex", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"CEX QA Menu is active!\r\nSwitch to DEX Debug Menu...");

								cellFsRename(VSH_MODULE_PATH "sysconf_plugin.sprx", VSH_MODULE_PATH "sysconf_plugin.sprx.cex");
								cellFsRename(VSH_MODULE_PATH "sysconf_plugin.sprx.dex", VSH_MODULE_PATH "sysconf_plugin.sprx");
							}
							else
							if(cellFsStat((char*) VSH_MODULE_PATH "sysconf_plugin.sprx.cex", &s)==CELL_FS_SUCCEEDED)
							{
								show_msg((char*)"DEX Debug Menu is active!\r\nSwitch to CEX QA Menu...");

								cellFsRename(VSH_MODULE_PATH "sysconf_plugin.sprx", VSH_MODULE_PATH "sysconf_plugin.sprx.dex");
								cellFsRename(VSH_MODULE_PATH "sysconf_plugin.sprx.cex", VSH_MODULE_PATH "sysconf_plugin.sprx");
							}
							sys_timer_sleep(1);
							{system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}
						}
#endif //#ifdef REX_ONLY
					}
				}

				if(reboot)
				{
					sys_timer_sleep(1);
					// reboot
					show_msg((char*)"Switching successful! Reboot now...");
					sys_timer_sleep(3);
					{system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_blind", 0, 1);}
reboot:
					working = 0;
					{ DELETE_TURNOFF }
					savefile((char*)WMNOSCAN, NULL, 0);
					{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0);}
					sys_ppu_thread_exit(0);
				}

			}
			//sys_timer_sleep(step);
			sys_timer_usleep(300000);
		}

		to++;
		if(to==20)
		{
			get_temperature(0, &t1);
			get_temperature(1, &t2);
			t1>>=24; t2>>=24;
			if(t1>83 || t2>83)
			{
				if(!webman_config->warn)
				{
					sprintf((char*) msg, "%s\r\n CPU: %i°C   RSX: %i°C", STR_OVERHEAT, t1, t2);
					show_msg((char*) msg);
					sys_timer_sleep(2);
				}
				if(t1>85 || t2>85)
				{
					if(!max_temp) max_temp=82;
					if(fan_speed<0xB0) fan_speed=0xB0;
					else
						if(fan_speed<MAX_FANSPEED) fan_speed+=8;

					old_fan=fan_speed;
					fan_control(fan_speed, 0);
					show_msg((char*)STR_OVERHEAT2);
				}
			}
		}
		if(to>40) to=0;

		// detect aprox. time when a game is launched
		if((sec % 10)==0) {if(View_Find("game_plugin")==0) gTick=rTick; else if(gTick.tick==rTick.tick) cellRtcGetCurrentTick(&gTick);}

		if(poll==0 && sec>=120) // check USB drives each 120 seconds
		{
			uint8_t tmp[2048];
			for(u8 f0=0; f0<8; f0++)
			{
				if(sys_storage_open(((f0<6)?USB_MASS_STORAGE_1(f0):USB_MASS_STORAGE_2(f0)), 0, &usb_handle, 0)==0)
				{
					sys_storage_read(usb_handle, 0, to, 1, tmp, &r, 0);
					sys_storage_close(usb_handle);
					//sprintf(tmp, "/dev_usb00%i: Read %i sectors @ %i offset", f0, r, to); show_msg((char*)tmp);
				}
			}
			sec=0;
		}
		sec+=step;
	}

	sys_ppu_thread_exit(0);
}

static void restore_fan(u8 set_ps2_temp)
{
	if(backup[0]==1 && (get_fan_policy_offset>0))
	{
		//pokeq(backup[0] + (u64) (130 * 8), backup[1]);
		//pokeq(backup[0] + (u64) (138 * 8), backup[2]);
		//pokeq(backup[0] + (u64) (379 * 8), backup[3]);

		if(set_ps2_temp)
		{
			webman_config->ps2temp=RANGE(webman_config->ps2temp, 20, 99); //%
			sys_sm_set_fan_policy(0, 2, ((webman_config->ps2temp*255)/100));
			fan_ps2_mode=true;
		}
		else sys_sm_set_fan_policy(0, 1, 0x0); //syscon

		pokeq(set_fan_policy_offset, backup[4]);  // sys 389 set_fan_policy
		pokeq(get_fan_policy_offset, backup[5]);  // sys 409 get_fan_policy  4.55/4.60/4.65

		backup[0]=0;
	}
}

static void fan_control(u8 temp0, u8 initial)
{
	if(fan_ps2_mode) return; //do not change fan settings while PS2 game is mounted

	if(get_fan_policy_offset)
	{
		if(!initial)
		{
			if(backup[0]==0)
			{
				backup[0]=1;
				//backup[1]=peekq(syscall_base + (u64) (130 * 8));
				//backup[2]=peekq(syscall_base + (u64) (138 * 8));
				//backup[3]=peekq(syscall_base + (u64) (379 * 8));

				backup[4]=peekq(set_fan_policy_offset);
				backup[5]=peekq(get_fan_policy_offset);
				lv2poke32(get_fan_policy_offset, 0x38600001); // sys 409 get_fan_policy  4.55/4.60/4.65
				lv2poke32(set_fan_policy_offset, 0x38600001); // sys 389 set_fan_policy

			    sys_sm_set_fan_policy(0, 2, 0x33);
		    }
		}

		if(temp0<0x33)
		{
			u8 st, mode, unknown;
			u8 fan_speed8=0;
			sys_sm_get_fan_policy(0, &st, &mode, &fan_speed8, &unknown);
			if(fan_speed8<0x33) return;
			fan_speed=fan_speed8;
		}
		else
			fan_speed=temp0;

		if(fan_speed<0x33 || fan_speed>0xFC)
		{
			fan_speed=0x48;
			sys_sm_set_fan_policy(0, 2, fan_speed);
			sys_timer_sleep(2);
		}
		old_fan=fan_speed;
		sys_sm_set_fan_policy(0, 2, fan_speed);
	}
}

static int save_settings()
{
	u64 written; int fdwm=0;
	if(cellFsOpen(WMCONFIG, CELL_FS_O_CREAT|CELL_FS_O_WRONLY, &fdwm, NULL, 0) == CELL_FS_SUCCEEDED)
	{
		cellFsWrite(fdwm, (void *)wmconfig, sizeof(WebmanCfg), &written);
		cellFsClose(fdwm);
        return CELL_FS_SUCCEEDED;
	}
	else
		return FAILED;
}

static void reset_settings()
{
	memset(webman_config, 0, sizeof(WebmanCfg));

	webman_config->usb0=1;
	webman_config->usb1=1;
	//webman_config->usb2=0;
	//webman_config->usb3=0;
	webman_config->usb6=1;
	//webman_config->usb7=0;

	//webman_config->dev_sd=0;
	//webman_config->dev_ms=0;
	//webman_config->dev_cf=0;

	//webman_config->lastp=0;      //disable last play
	//webman_config->autob=0;      //disable check for AUTOBOOT.ISO
	//webman_config->delay=0;      //don't delay loading of AUTOBOOT.ISO/last-game (Disc Auto-start)

	//webman_config->bootd=0;      //don't wait for any USB device to be ready
	webman_config->boots=3;      //wait 3 additional seconds for each selected USB device to be ready

	//webman_config->nogrp=0;      //group content on XMB
	//webman_config->wmdn=0;       //enable start up message (webMAN Loaded!)
	//webman_config->tid=0;        //don't include the ID as part of the title of the game
	//webman_config->noset=0;      //enable webMAN Setup entry in "webMAN Games"

#ifdef COBRA_ONLY
	webman_config->cmask=0;
#else
	webman_config->cmask=(PSP | PS1 | BLU | DVD);
#endif

	webman_config->poll=1;       //disable USB polling
	//webman_config->nopad=0;      //enable all PAD shortcuts
	//webman_config->nocov=0;      //enable multiMAN covers

	webman_config->fanc=1;       //fan control enabled
	//webman_config->temp0=0;      //auto
	webman_config->temp1=MY_TEMP;
	webman_config->manu=35;      //manual temp
	webman_config->ps2temp=37;   //ps2 temp

	webman_config->minfan=DEFAULT_MIN_FANSPEED;

	//webman_config->bind=0;       //enable remote access to FTP/WWW services
	//webman_config->ftpd=0;       //enable ftp server
	//webman_config->refr=0;       //enable content scan on startup

	//webman_config->netd1    = webman_config->netd2    = webman_config->netd0=0;
	//webman_config->neth1[0] = webman_config->neth2[0] = webman_config->neth0[0]=0;
	webman_config->netp1    = webman_config->netp2    = webman_config->netp0=38008;

	webman_config->foot=1;       //MIN
	webman_config->nospoof=1;    //don't spoof fw version

	webman_config->pspl=1;       //Show PSP Launcher
	webman_config->ps2l=1;       //Show PS2 Classic Launcher

	//webman_config->spp=0;        //disable removal of syscalls
	webman_config->fixgame=FIX_GAME_AUTO;

	//webman_config->sidps=0;      //spoof IDPS
	//webman_config->spsid=0;      //spoof PSID

	//webman_config->vIDPS1[0]=webman_config->vIDPS2[0]=0;
	//webman_config->vPSID1[0]=webman_config->vPSID2[0]=0;

	//webman_config->bus=0;      //enable reset USB bus

	webman_config->combo=DISACOBRA; //disable combo for cobra toggle
	webman_config->combo2|=(REBUGMODE|NORMAMODE|DEBUGMENU|PS2SWITCH|VIDRECORD); //disable combos for rebug/ps2 switch/video record

	char upath[24];
	struct CellFsStat buf;
	sprintf(webman_config->uaccount, "%08i", 1);
	for(u8 acc=1; acc<100; acc++)
	{
		sprintf(upath, "%s/%08i", "/dev_hdd0/home", acc);
		if(cellFsStat(upath, &buf)==CELL_FS_SUCCEEDED) {sprintf(webman_config->uaccount, "%08i", acc); break;}
	}

#ifndef ENGLISH_ONLY
	get_system_language(&webman_config->lang);
#else
	webman_config->lang=0; // english
#endif

	strcpy(webman_config->autoboot_path, DEFAULT_AUTOBOOT_PATH);

    int fdwm=0; cellFsStat(WMCONFIG, &buf);

	for(u8 n=0;n<10;n++)
	{
		if(cellFsOpen(WMCONFIG, CELL_FS_O_RDONLY, &fdwm, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			cellFsRead(fdwm, (void *)wmconfig, MIN(buf.st_size, sizeof(WebmanCfg)), NULL);
			cellFsClose(fdwm);

#ifndef COBRA_ONLY
		webman_config->spp=0; //disable removal of syscalls on nonCobra
#endif
			break;
		}
	    sys_timer_usleep(500000);
		save_settings();
	}

	if(strlen(webman_config->autoboot_path)==0) strcpy(webman_config->autoboot_path, DEFAULT_AUTOBOOT_PATH);

	if(webman_config->warn>1) webman_config->warn=0;
	webman_config->minfan=RANGE(webman_config->minfan, MIN_FANSPEED, 99);

	profile=webman_config->profile;
}

static void set_buffer_sizes(int footprint)
{
	if(footprint==1) //MIN
	{
#ifndef LITE_EDITION
		BUFFER_SIZE_ALL = ( 320*KB);
#else
		BUFFER_SIZE_ALL = ( _256KB_);
#endif
		BUFFER_SIZE_FTP	= ( _128KB_);
		//BUFFER_SIZE	= ( _128KB_);
		BUFFER_SIZE_PSX	= (  _32KB_);
		BUFFER_SIZE_PSP	= (  _32KB_);
		BUFFER_SIZE_PS2	= (  _64KB_);
		BUFFER_SIZE_DVD	= (  _64KB_);
	}
	else
	if(footprint==2) //MAX
	{
		BUFFER_SIZE_ALL = ( 1280*KB);
		BUFFER_SIZE_FTP	= ( _256KB_);
		//BUFFER_SIZE	= ( 512*KB);
		BUFFER_SIZE_PSX	= ( _192KB_);
		BUFFER_SIZE_PSP	= (  _64KB_);
		BUFFER_SIZE_PS2	= ( _128KB_);
		BUFFER_SIZE_DVD	= ( _256KB_);

		if((webman_config->cmask & PS1)) BUFFER_SIZE_PSX	= (_64KB_);
		if((webman_config->cmask & PS2)) BUFFER_SIZE_PS2	= (_64KB_);
		if((webman_config->cmask & (BLU | DVD)) == (BLU | DVD)) BUFFER_SIZE_DVD = (_64KB_);
	}
	else
	if(footprint==3) //MIN+
	{
		BUFFER_SIZE_ALL = ( 512*KB);
		BUFFER_SIZE_FTP	= ( _128KB_);
		//BUFFER_SIZE	= ( 320*KB);
		BUFFER_SIZE_PSX	= (  _32KB_);
		BUFFER_SIZE_PSP	= (  _32KB_);
		BUFFER_SIZE_PS2	= (  _64KB_);
		BUFFER_SIZE_DVD	= (  _64KB_);
	}
	else	//STANDARD
	{
		BUFFER_SIZE_ALL = ( 896*KB);
		BUFFER_SIZE_FTP	= ( _128KB_);
		//BUFFER_SIZE	= ( 448*KB);
		BUFFER_SIZE_PSX	= ( 160*KB);
		BUFFER_SIZE_PSP	= (  _32KB_);
		BUFFER_SIZE_PS2	= (  _64KB_);
		BUFFER_SIZE_DVD	= ( _192KB_);

		if((webman_config->cmask & PS1)) BUFFER_SIZE_PSX	= (_32KB_);
		if((webman_config->cmask & (BLU | DVD)) == (BLU | DVD)) BUFFER_SIZE_DVD = (_64KB_);
	}

	BUFFER_SIZE = BUFFER_SIZE_ALL - (BUFFER_SIZE_PSX + BUFFER_SIZE_PSP + BUFFER_SIZE_PS2 + BUFFER_SIZE_DVD);
}


#ifdef PS3MAPI

///////////// PS3MAPI BEGIN //////////////


#define THREAD_NAME_PS3MAPI						"ps3m_api_server"
#define THREAD02_NAME_PS3MAPI					"ps3m_api_client"



#define PS3MAPIPORT			(7887)

#define PS3MAPI_RECV_SIZE  2048

static u32 BUFFER_SIZE_PS3MAPI = (_64KB_);

static sys_ppu_thread_t thread_id_ps3mapi = -1;

static void handleclient_ps3mapi(u64 conn_s_ps3mapi_p)
{
	int conn_s_ps3mapi = (int)conn_s_ps3mapi_p; // main communications socket
	int data_s = -1;							// data socket
	int data_ls = -1;

	int connactive = 1;							// whether the ps3mapi connection is active or not
	int dataactive = 0;							// prevent the data connection from being closed at the end of the loop

	char buffer[PS3MAPI_RECV_SIZE];
	char cmd[20], param1[384], param2[384];

	int p1x = 0;
	int p2x = 0;

	#define PS3MAPI_OK_150    "150 OK: Binary status okay; about to open data connection.\r\n"
	#define PS3MAPI_OK_200    "200 OK: The requested action has been successfully completed.\r\n"
	#define PS3MAPI_OK_220    "220 OK: PS3 Manager API Server v1.\r\n"
	#define PS3MAPI_OK_221    "221 OK: Service closing control connection.\r\n"
	#define PS3MAPI_OK_226    "226 OK: Closing data connection. Requested binary action successful.\r\n"
	#define PS3MAPI_OK_230    "230 OK: Connected to PS3 Manager API Server.\r\n"
	#define PS3MAPI_OK_250    "250 OK: Requested binary action okay, completed.\r\n"

	#define PS3MAPI_ERROR_425 "425 Error: Can't open data connection.\r\n"
	#define PS3MAPI_ERROR_451 "451 Error: Requested action aborted. Local error in processing.\r\n"
	#define PS3MAPI_ERROR_500 "500 Error: Syntax error, command unrecognized and the requested action did not take place.\r\n"
	#define PS3MAPI_ERROR_501 "501 Error: Syntax error in parameters or arguments.\r\n"
	#define PS3MAPI_ERROR_550 "550 Error: Requested action not taken.\r\n"
	#define PS3MAPI_ERROR_502 "502 Error: Command not implemented.\r\n"

	#define PS3MAPI_CONNECT_NOTIF 	 "PS3MAPI: Client connected [%s]\r\n"
	#define PS3MAPI_DISCONNECT_NOTIF "PS3MAPI: Client disconnected [%s]\r\n"

	CellRtcTick pTick;

	sys_net_sockinfo_t conn_info;
	sys_net_get_sockinfo(conn_s_ps3mapi, &conn_info, 1);

	char ip_address[16];
	char pasv_output[56];

	ssend(conn_s_ps3mapi, PS3MAPI_OK_220);

	sprintf(ip_address, "%s", inet_ntoa(conn_info.local_adr));
	for(u8 n = 0; n<strlen(ip_address); n++) if(ip_address[n] == '.') ip_address[n] = ',';

	ssend(conn_s_ps3mapi, PS3MAPI_OK_230);

	sprintf(buffer, PS3MAPI_CONNECT_NOTIF, inet_ntoa(conn_info.remote_adr)); show_msg(buffer);

	while(connactive == 1 && working)
	{

		if(working && (recv(conn_s_ps3mapi, buffer, PS3MAPI_RECV_SIZE, 0) > 0))
		{
			buffer[strcspn(buffer, "\n")] = '\0';
			buffer[strcspn(buffer, "\r")] = '\0';

			int split = ssplit(buffer, cmd, 19, param1, 383);
			if(strcasecmp(cmd, "DISCONNECT") == 0)
			{
				ssend(conn_s_ps3mapi, PS3MAPI_OK_221);
				connactive = 0;
			}
			else if(strcasecmp(cmd, "SERVER") == 0)
			{
				if(split == 1)
				{
					split = ssplit(param1, cmd, 19, param2, 383);
					if(strcasecmp(cmd, "GETVERSION") == 0)
					{
						sprintf(buffer, "200 %i\r\n", PS3MAPI_SERVER_VERSION);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "GETMINVERSION") == 0)
					{
						sprintf(buffer, "200 %i\r\n", PS3MAPI_SERVER_MINVERSION);
						ssend(conn_s_ps3mapi, buffer);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(strcasecmp(cmd, "CORE") == 0)
			{
				if(split == 1)
				{
					split = ssplit(param1, cmd, 19, param2, 383);
					if(strcasecmp(cmd, "GETVERSION") == 0)
					{
						int version = 0;
						{ system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_VERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "GETMINVERSION") == 0)
					{
						int version = 0;
						{ system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(strcasecmp(cmd, "PS3") == 0)
			{
				if(split == 1)
				{
					split = ssplit(param1, cmd, 19, param2, 383);
					if(strcasecmp(cmd, "SHUTDOWN") == 0)
					{
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						working = 0;
						{ DELETE_TURNOFF }
						{system_call_4(SC_SYS_POWER, SYS_SHUTDOWN, 0, 0, 0); }
						sys_ppu_thread_exit(0);
					}
					else if(strcasecmp(cmd, "REBOOT") == 0)
					{
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						working = 0;
						{ DELETE_TURNOFF }
						{system_call_3(SC_SYS_POWER, SYS_REBOOT, NULL, 0); }
						sys_ppu_thread_exit(0);
					}
					else if(strcasecmp(cmd, "SOFTREBOOT") == 0)
					{
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						working = 0;
						{ DELETE_TURNOFF }
						{system_call_3(SC_SYS_POWER, SYS_SOFT_REBOOT, NULL, 0); }
						sys_ppu_thread_exit(0);
					}
					else if(strcasecmp(cmd, "HARDREBOOT") == 0)
					{
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						working = 0;
						{ DELETE_TURNOFF }
						{system_call_3(SC_SYS_POWER, SYS_HARD_REBOOT, NULL, 0); }
						sys_ppu_thread_exit(0);
					}
					else if(strcasecmp(cmd, "GETFWVERSION") == 0)
					{
						int version = 0;
						{system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_VERSION); version = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", version);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "GETFWTYPE") == 0)
					{
						memset(param2, 0, sizeof(param2));
						{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_FW_TYPE, (u64)(u32)param2); }
						sprintf(buffer, "200 %s\r\n", param2);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "NOTIFY") == 0)
					{
						if(split == 1)
						{
							show_msg(param2);
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "BUZZER1") == 0)
					{
						{ BEEP1 }
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(strcasecmp(cmd, "BUZZER2") == 0)
					{
						{ BEEP2 }
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(strcasecmp(cmd, "BUZZER3") == 0)
					{
						{ BEEP3 }
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(strcasecmp(cmd, "LED") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u64 color = val(param1);
								u64 mode = val(param2);
								{system_call_2(SC_SYS_CONTROL_LED, color, mode); }
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "GETTEMP") == 0)
					{
						u32 cpu_temp = 0;
						u32 rsx_temp = 0;
						get_temperature(0, &cpu_temp);
						get_temperature(1, &rsx_temp);
						cpu_temp = cpu_temp >> 24;
						rsx_temp = rsx_temp >> 24;
						sprintf(buffer, "200 %i|%i\r\n", cpu_temp, rsx_temp);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "DISABLESYSCALL") == 0)
					{
						if(split == 1)
						{
							int num = val(param2);
							{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_DISABLE_SYSCALL, (u64)num); }
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "CHECKSYSCALL") == 0)
					{
						if(split == 1)
						{
							int num = val(param2);
							int check = 0;
							{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_CHECK_SYSCALL, (u64)num); check = (int)(p1); }
							sprintf(buffer, "200 %i\r\n", check);
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "PDISABLESYSCALL8") == 0)
					{
						if(split == 1)
						{
							int mode = val(param2);
							{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PDISABLE_SYSCALL8, (u64)mode); }
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "PCHECKSYSCALL8") == 0)
					{
						int check = 0;
						{ system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_PCHECK_SYSCALL8); check = (int)(p1); }
						sprintf(buffer, "200 %i\r\n", check);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "DELHISTORY+F") == 0)
					{
						delete_history(true);
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(strcasecmp(cmd, "DELHISTORY") == 0)
					{
						delete_history(false);
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(strcasecmp(cmd, "REMOVEHOOK") == 0)
					{
						{ system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_REMOVE_HOOK); }
						ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					}
					else if(strcasecmp(cmd, "GETIDPS") == 0)
					{
						u64 _new_idps[2] = { 0, 0};
						{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_IDPS, (u64)(u32)_new_idps);}
						sprintf(buffer, "200 %016llX%016llX\r\n", _new_idps[0], _new_idps[1]);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "SETIDPS") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u64 part1 = convertH(param1);
								u64 part2 = convertH(param2);
								{ system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_IDPS, part1, part2);}
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "GETPSID") == 0)
					{
						u64 _new_psid[2] = { 0, 0};
						{ system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PSID, (u64)(u32)_new_psid);}
						sprintf(buffer, "200 %016llX%016llX\r\n", _new_psid[0], _new_psid[1]);
						ssend(conn_s_ps3mapi, buffer);
					}
					else if(strcasecmp(cmd, "SETPSID") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u64 part1 = convertH(param1);
								u64 part2 = convertH(param2);
								{ system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PSID, (u64)part1, (u64)part2);}
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(strcasecmp(cmd, "PROCESS") == 0)
			{
				if(split == 1)
				{
					split = ssplit(param1, cmd, 19, param2, 383);
					if(strcasecmp(cmd, "GETNAME") == 0)
					{
						if(split == 1)
						{
							u32 pid = val(param2);
							memset(param2, 0, sizeof(param2));
							{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_NAME_BY_PID, (u64)pid, (u64)(u32)param2); }
							sprintf(buffer, "200 %s\r\n", param2);
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "GETALLPID") == 0)
					{
						u32 pid_list[16];
						memset(buffer, 0, sizeof(buffer));
						sprintf(buffer, "200 ");
						{system_call_3(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_PID, (u64)(u32)pid_list); }
						for(int i = 0; i < 16; i++)
						{
							sprintf(buffer + strlen(buffer), "%i|", pid_list[i]);
						}
						sprintf(buffer + strlen(buffer), "\r\n");
						ssend(conn_s_ps3mapi, buffer);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(strcasecmp(cmd, "MEMORY") == 0)
			{
				if(split == 1)
				{
					split = ssplit(param1, cmd, 19, param2, 383);
					if(strcasecmp(cmd, "GET") == 0)
					{
						if(data_s > 0)
						{
							if(split == 1)
							{
								split = ssplit(param2, param1, 383, param2, 383);
								if(split == 1)
								{
									u32 attached_pid = val(param1);
									split = ssplit(param2, param1, 383, param2, 383);
									if(split == 1)
									{
										u64 offset = convertH(param1);
										u32 size = val(param2);
										int rr = -4;
										sys_addr_t sysmem = 0;
										if(sys_memory_allocate(BUFFER_SIZE_PS3MAPI, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == 0)
										{
											char *buffer2 = (char*)sysmem;
											ssend(conn_s_ps3mapi, PS3MAPI_OK_150);
											rr = 0;
											while(working)
											{
												if(size > BUFFER_SIZE_PS3MAPI)
												{
													u32 sizetoread = BUFFER_SIZE_PS3MAPI;
													u32 leftsize = size;
													if(size < BUFFER_SIZE_PS3MAPI) sizetoread = size;
													while(0 < leftsize)
													{
														system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)attached_pid, offset, (u64)(u32)buffer2, (u64)sizetoread);
														if(send(data_s, buffer2, sizetoread, 0)<0) { rr = -3; break; }
														offset += sizetoread;
														leftsize -= sizetoread;
														if(leftsize < BUFFER_SIZE_PS3MAPI) sizetoread = leftsize;
														if(sizetoread == 0) break;
													}
													break;
												}
												else
												{
													system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MEM, (u64)attached_pid, (u64)offset, (u64)(u32)buffer2, (u64)size);
													if(send(data_s, buffer2, size, 0)<0) { rr = -3; break; }
													break;
												}
											}
											sys_memory_free(sysmem);
										}
										if(rr == 0) ssend(conn_s_ps3mapi, PS3MAPI_OK_226);
										else if(rr == -4) ssend(conn_s_ps3mapi, PS3MAPI_ERROR_550);
										else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
									}
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_425);
					}
					else if(strcasecmp(cmd, "SET") == 0)
					{
						if(data_s > 0)
						{
							if(split == 1)
							{
								split = ssplit(param2, param1, 383, param2, 383);
								if(split == 1)
								{
									u32 attached_pid = val(param1);
									u64 offset = convertH(param2);
									int rr = -1;
									sys_addr_t sysmem = 0;
									if(sys_memory_allocate(BUFFER_SIZE_PS3MAPI, SYS_MEMORY_PAGE_SIZE_64K, &sysmem) == 0)
									{
										char *buffer2 = (char*)sysmem;
										u64 read_e = 0;
										ssend(conn_s_ps3mapi, PS3MAPI_OK_150);
										rr = 0;
										while(working)
										{
											if((read_e = (u64)recv(data_s, buffer2, BUFFER_SIZE_PS3MAPI, MSG_WAITALL)) > 0)
											{
												system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_SET_PROC_MEM, (u64)attached_pid, offset, (u64)(u32)buffer2, read_e);
												offset += read_e;
											}
											else
											{
												break;
											}
										}
										sys_memory_free(sysmem);
									}
									if(rr == 0) ssend(conn_s_ps3mapi, PS3MAPI_OK_226);
									else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
								}
								else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_425);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(strcasecmp(cmd, "MODULE") == 0)
			{
				if(split == 1)
				{
					split = ssplit(param1, cmd, 19, param2, 383);
					if(strcasecmp(cmd, "GETNAME") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u32 pid = val(param1);
								s32 prxid = val(param2);
								memset(param2, 0, sizeof(param2));
								{system_call_5(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_NAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }
								sprintf(buffer, "200 %s\r\n", param2);
								ssend(conn_s_ps3mapi, buffer);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "GETFILENAME") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u32 pid = val(param1);
								s32 prxid = val(param2);
								memset(param2, 0, sizeof(param2));
								{system_call_5(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_PROC_MODULE_FILENAME, (u64)pid, (u64)prxid, (u64)(u32)param2); }
								sprintf(buffer, "200 %s\r\n", param2);
								ssend(conn_s_ps3mapi, buffer);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "GETALLPRXID") == 0)
					{
						if(split == 1)
						{
							s32 prxid_list[128];
							u32 pid = val(param2);
							memset(buffer, 0, sizeof(buffer));
							sprintf(buffer, "200 ");
							{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_ALL_PROC_MODULE_PID, (u64)pid, (u64)(u32)prxid_list); }
							for(int i = 0; i < 128; i++)
							{
								sprintf(buffer + strlen(buffer), "%i|", prxid_list[i]);
							}
							sprintf(buffer + strlen(buffer), "\r\n");
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "LOAD") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u32 pid = val(param1);
								{system_call_6(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_LOAD_PROC_MODULE, (u64)pid, (u64)(u32)param2, NULL, 0); }
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "UNLOAD") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								u32 pid = val(param1);
								s32 prx_id = val(param2);
								{system_call_4(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_UNLOAD_PROC_MODULE, (u64)pid, (u64)prx_id); }
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
							else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "LOADVSHPLUG") == 0)
					{
						if(split == 1)
						{
							split = ssplit(param2, param1, 383, param2, 383);
							if(split == 1)
							{
								unsigned int slot = val(param1);
								if ( slot ) {{system_call_5(8, SYSCALL8_OPCODE_LOAD_VSH_PLUGIN, (u64)slot, (u64)(u32)param2, NULL, 0); }}
								ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
							}
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "UNLOADVSHPLUGS") == 0)
					{
						if(split == 1)
						{
							unsigned int slot = val(param2);
							if ( slot ) {{system_call_2(8, SYSCALL8_OPCODE_UNLOAD_VSH_PLUGIN, (u64)slot); }}
							ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else if(strcasecmp(cmd, "GETVSHPLUGINFO") == 0)
					{
						if(split == 1)
						{
							unsigned int slot = val(param2);
							memset(param1, 0, sizeof(param1));
							memset(param2, 0, sizeof(param2));
							{system_call_5(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_VSH_PLUGIN_INFO, (u64)slot, (u64)(u32)param1, (u64)(u32)param2); }
							sprintf(buffer, "200 %s|%s\r\n", param1, param2);
							ssend(conn_s_ps3mapi, buffer);
						}
						else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
					}
					else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);
				}
				else
				{
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_501);
				}
			}
			else if(strcasecmp(cmd, "TYPE") == 0)
			{
				if(split == 1)
				{
					ssend(conn_s_ps3mapi, PS3MAPI_OK_200);
					if(strcmp(param1, "A") == 0) dataactive = 1;
					else if(strcmp(param1, "I") == 0)  dataactive = 0;
					else  dataactive = 1;
				}
				else
				{
					dataactive = 1;
				}
			}
			else if(strcasecmp(cmd, "PASV") == 0)
			{
				u8 pasv_retry = 0;
			pasv_again:
				if(!p1x)
				{
					cellRtcGetCurrentTick(&pTick);
					p1x = (((pTick.tick & 0xfe0000) >> 16) & 0xff) | 0x80; // use ports 32768 -> 65279 (0x8000 -> 0xFEFF)
					p2x = (((pTick.tick & 0x00ff00) >> 8) & 0xff);
				}
				data_ls = slisten(getPort(p1x, p2x), 1);

				if(data_ls >= 0)
				{
					sprintf(pasv_output, "227 Entering Passive Mode (%s,%i,%i)\r\n", ip_address, p1x, p2x);
					ssend(conn_s_ps3mapi, pasv_output);

					if((data_s = accept(data_ls, NULL, NULL)) > 0)
					{
						dataactive = 1;
					}
					else
					{
						ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
					}

				}
				else
				{
					p1x = 0;
					if(pasv_retry<10)
					{
						pasv_retry++;
						goto pasv_again;
					}
					ssend(conn_s_ps3mapi, PS3MAPI_ERROR_451);
				}
			}
			else ssend(conn_s_ps3mapi, PS3MAPI_ERROR_502);

			if(dataactive == 1) dataactive = 0;
			else
			{
				sclose(&data_s);
				if(data_ls>0) { sclose(&data_ls); data_ls = FAILED; }
			}
		}
		else
		{
			connactive = 0;
			break;
		}

		sys_timer_usleep(1668);
	}

	sprintf(buffer, PS3MAPI_DISCONNECT_NOTIF, inet_ntoa(conn_info.remote_adr));
	show_msg(buffer);
	sclose(&conn_s_ps3mapi);
	sclose(&data_s);
	sys_ppu_thread_exit(0);
}

static void ps3mapi_thread(u64 arg)
{
	int core_minversion = 0;
	{ system_call_2(8, SYSCALL8_OPCODE_PS3MAPI, PS3MAPI_OPCODE_GET_CORE_MINVERSION); core_minversion = (int)(p1); }
	if((core_minversion !=0) &&(PS3MAPI_CORE_MINVERSION == core_minversion)) //Check if ps3mapi core has a compatible min_version.
	{
		int list_s = FAILED;
	relisten:
		if(working) list_s = slisten(PS3MAPIPORT, 4);
		else goto end;

		if(working && (list_s<0))
		{
			sys_timer_sleep(3);
			if(working) goto relisten;
			else goto end;
		}

		//if(working && (list_s >= 0))
		{
			while(working)
			{
				sys_timer_usleep(1668);
				int conn_s_ps3mapi;
				if (!working) goto end;
				else
				if(working && (conn_s_ps3mapi = accept(list_s, NULL, NULL)) > 0)
				{
					sys_ppu_thread_t id;
					if(working) sys_ppu_thread_create(&id, handleclient_ps3mapi, (u64)conn_s_ps3mapi, -0x1d8, 0x10000, 0, THREAD02_NAME_PS3MAPI);
					else {sclose(&conn_s_ps3mapi); break;}
				}
				else
				if((sys_net_errno == SYS_NET_EBADF) || (sys_net_errno == SYS_NET_ENETDOWN))
				{
					sclose(&list_s);
					list_s = FAILED;
					if(working) goto relisten;
					else break;
				}
			}
		}
end:
		sclose(&list_s);
		sys_ppu_thread_exit(0);
	}
	else show_msg((char *)"PS3MAPI Server not loaded!");

	sys_ppu_thread_exit(0);
}

////////////////////////////////////////
///////////// PS3MAPI END //////////////
////////////////////////////////////////
#endif

static void wwwd_thread(uint64_t arg)
{

//	sys_timer_sleep(8);
//	u32 mode=0;
//	if(in_cobra(&mode)==0) cobra_mode=1;

	backup[0]=0;

	detect_firmware();

#ifdef COBRA_ONLY
	//cobra_lib_init();
	cobra_mode=1;
#else
	cobra_mode=0;
#endif

	View_Find = (void*)((int)getNIDfunc("paf", 0xF21655F3, 0));
	plugin_GetInterface = (void*)((int)getNIDfunc("paf", 0x23AFB290, 0));
#ifdef EXTRA_FEAT
	vsh_sprintf = (void*)((int)getNIDfunc("stdc", 0x273B9711, 0)); // sprintf
#endif

	//pokeq(0x8000000000003560ULL, 0x386000014E800020ULL); // li r3, 0 / blr
	//pokeq(0x8000000000003D90ULL, 0x386000014E800020ULL); // li r3, 0 / blr

	led(YELLOW, BLINK_FAST);

	WebmanCfg *webman_config = (WebmanCfg*) wmconfig;
	reset_settings();


	{struct CellFsStat buf; from_reboot = (cellFsStat((char*)WMNOSCAN, &buf)==CELL_FS_SUCCEEDED);} //is_rebug=isDir("/dev_flash/rebug");

	if(webman_config->blind) enable_dev_blind(NULL);

	{sys_map_path((char*)"/app_home", NULL);}

	set_buffer_sizes(webman_config->foot);

	init_running = 1;

	sys_ppu_thread_t id2;
	sys_ppu_thread_create(&id2, handleclient, (u64)START_DAEMON, -0x1d8, 0x20000, 0, "wwwd2");

	if(!webman_config->ftpd)
		sys_ppu_thread_create(&thread_id_ftp, ftpd_thread, NULL, -0x1d8,  0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_FTP);

#ifdef PS3MAPI
	///////////// PS3MAPI BEGIN //////////////
		sys_ppu_thread_create(&thread_id_ps3mapi, ps3mapi_thread, NULL, -0x1d8, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_PS3MAPI);
	///////////// PS3MAPI END //////////////
#endif

	led(YELLOW, OFF);
	sys_timer_sleep(5);

#ifdef USE_DEBUG
	u8 d_retries=0;
again_debug:
	debug_s=connect_to_server((char*)"192.168.100.209", 38009);
	if(debug_s<0) {d_retries++; sys_timer_sleep(2); if(d_retries<10) goto again_debug;}
	if(debug_s>=0) ssend(debug_s, "Connected...\r\n");
	sprintf(debug, "FC=%i T0=%i T1=%i\r\n", webman_config->fanc, webman_config->temp0, webman_config->temp1);
	ssend(debug_s, debug);
#endif

	max_temp=0;

	if(webman_config->fanc)
	{
		if(webman_config->temp0==0) max_temp=webman_config->temp1; else max_temp=0;
		fan_control(webman_config->temp0, 0);
	}

    sys_ppu_thread_create(&thread_id_poll, poll_thread, (u64)webman_config->poll, -0x1d8, 0x20000, SYS_PPU_THREAD_CREATE_JOINABLE, "poll_thread");

	while(init_running && working) sys_timer_usleep(100000);

	led(GREEN, ON);

//	{ DELETE_TURNOFF }

	int list_s = FAILED;

relisten:
#ifdef USE_DEBUG
	ssend(debug_s, "Listening on port 80...");
#endif
	if(working) list_s = slisten(WWWPORT, 4);
	else goto end;

	if((list_s<0) && working)
	{
		sys_timer_sleep(2);
		if(working) goto relisten;
		else goto end;
	}

	if((list_s >= 0) && working)
	{
#ifdef USE_DEBUG
		ssend(debug_s, " OK!\r\n");
#endif

		while(working)
		{
			sys_timer_usleep(10000);
			while(loading_html>2 && working)
			{
#ifdef USE_DEBUG
	sprintf(debug, "THREADS: %i\r\n", loading_html);
	ssend(debug_s, debug);
#endif
				sys_timer_usleep(300000);
			}
			int conn_s;
			if(!working) goto end;
			else
			if(working && (conn_s = accept(list_s, NULL, NULL)) > 0)
			{
				loading_html++;
				#ifdef USE_DEBUG
				ssend(debug_s, "*** Incoming connection... ");
				#endif
				sys_ppu_thread_t id;
				if(working) sys_ppu_thread_create(&id, handleclient, (u64)conn_s, -0x1d8, 0x20000, 0, "wwwd");
				else {sclose(&conn_s); break;}
			}
			else
			if((sys_net_errno==SYS_NET_EBADF) || (sys_net_errno==SYS_NET_ENETDOWN))
			{
				sclose(&list_s);
				list_s=FAILED;
				if(working) goto relisten;
				else break;
			}
		}

	}
end:
	sclose(&list_s);
	sys_ppu_thread_exit(0);
}


int wwwd_start(uint64_t arg)
{
	cellRtcGetCurrentTick(&rTick); gTick=rTick;

	sys_ppu_thread_create(&thread_id, wwwd_thread, NULL, -0x1d8, 0x4000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME);
#ifndef CCAPI
	_sys_ppu_thread_exit(0); // remove for ccapi compatibility
#endif
	return SYS_PRX_RESIDENT;
}

static void wwwd_stop_thread(uint64_t arg)
{
	while(init_running) sys_timer_usleep(500000); //Prevent unload too fast

	restore_fan(1); //restore & set static fan speed for ps2

	working = 0;
	sys_timer_usleep(500000);

	uint64_t exit_code;

/*
	sys_ppu_thread_t t;
 #ifndef LITE_EDITION
	sys_ppu_thread_create(&t, netiso_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t, &exit_code);
 #endif
	sys_ppu_thread_create(&t, rawseciso_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	sys_ppu_thread_join(t, &exit_code);

	while(netiso_loaded || rawseciso_loaded) {sys_timer_usleep(100000);}
*/
	//if(thread_id != (sys_ppu_thread_t)-1)
		sys_ppu_thread_join(thread_id, &exit_code);

	if(thread_id_ftp != (sys_ppu_thread_t)-1)
		sys_ppu_thread_join(thread_id_ftp, &exit_code);

#ifdef PS3MAPI
	///////////// PS3MAPI BEGIN //////////////
	if(thread_id_ps3mapi != (sys_ppu_thread_t)-1)
		sys_ppu_thread_join(thread_id_ps3mapi, &exit_code);
	///////////// PS3MAPI END //////////////
#endif

	if(wm_unload_combo !=1)
	{
		if(thread_id_poll != (sys_ppu_thread_t)-1)
			sys_ppu_thread_join(thread_id_poll, &exit_code);
	}

	sys_ppu_thread_exit(0);
}

static void stop_prx_module(void)
{
	show_msg((char*)STR_WMUNL);

	sys_prx_id_t prx = prx_get_module_id_by_address(stop_prx_module);
	int *result = NULL;

	{system_call_6(SC_STOP_PRX_MODULE, (u64)(u32)prx, 0, NULL, (u64)(u32)result, 0, NULL);}
}

static void unload_prx_module(void)
{

	sys_prx_id_t prx = prx_get_module_id_by_address(unload_prx_module);

	{system_call_3(SC_UNLOAD_PRX_MODULE, (u64)prx, 0, NULL);}

}

int wwwd_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	int ret = sys_ppu_thread_create(&t, wwwd_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
	if (ret == 0) sys_ppu_thread_join(t, &exit_code);

	sys_timer_usleep(500000);

	unload_prx_module();

//#ifndef CCAPI
	_sys_ppu_thread_exit(0); // remove for ccapi compatibility ???
//#endif

	return SYS_PRX_STOP_OK;
}

#ifdef COBRA_ONLY
static void select_ps1emu(void)
{
	CellPadData pad_data;
	pad_data.len=0;

	for(u8 n=0;n<10;n++)
	{
		if(cellPadGetData(0, &pad_data) != CELL_PAD_OK)
			if(cellPadGetData(1, &pad_data) != CELL_PAD_OK)
					cellPadGetData(2, &pad_data);

		if(pad_data.len > 0) break;
		sys_timer_usleep(100000);
	}

	if(pad_data.len>0)
    {
		if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_R2) {webman_config->ps1emu=1; save_settings();} else
		if(pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL2] & CELL_PAD_CTRL_L2) {webman_config->ps1emu=0; save_settings();}
    }

	char msg[100];

	if(webman_config->ps1emu)
	{
		sys_map_path((char*)"/dev_flash/ps1emu/ps1_netemu.self", (char*)"//dev_flash/ps1emu/ps1_emu.self");
		sys_map_path((char*)"/dev_flash/ps1emu/ps1_emu.self"   , (char*)"//dev_flash/ps1emu/ps1_netemu.self");

		sprintf(msg, "ps1_netemu.self %s", STR_ENABLED);
	}
	else
	{
		sys_map_path((char*)"/dev_flash/ps1emu/ps1_netemu.self", (char*)"//dev_flash/ps1emu/ps1_netemu.self");
		sys_map_path((char*)"/dev_flash/ps1emu/ps1_emu.self"   , (char*)"//dev_flash/ps1emu/ps1_emu.self");

		sprintf(msg, "ps1_emu.self %s",    STR_ENABLED);
	}

	show_msg((char*) msg);
}
#endif

static void eject_insert(u8 eject, u8 insert)
{
	u8 atapi_cmnd2[56];
	u8* atapi_cmnd = atapi_cmnd2;
	int dev_id;

	{system_call_4(SC_STORAGE_OPEN, BDVD_DRIVE, 0, (uint64_t)(u32) &dev_id, 0);}

	if(eject)
	{
		memset(atapi_cmnd, 0, 56);
		atapi_cmnd[0x00]=0x1b;
		atapi_cmnd[0x01]=0x01;
		atapi_cmnd[0x04]=0x02;
		atapi_cmnd[0x23]=0x0c;

		// Eject disc
		{system_call_7(SC_STORAGE_INSERT_EJECT, dev_id, 1, (uint64_t)(u32) atapi_cmnd, 56, NULL, 0, NULL);}
	}

	sys_timer_sleep(2);

	if(insert)
	{
		memset(atapi_cmnd, 0, 56);
		atapi_cmnd[0x00]=0x1b;
		atapi_cmnd[0x01]=0x01;
		atapi_cmnd[0x04]=0x03;
		atapi_cmnd[0x23]=0x0c;

		// Insert disc
		{system_call_7(SC_STORAGE_INSERT_EJECT, dev_id, 1, (uint64_t)(u32) atapi_cmnd, 56, NULL, 0, NULL);}
	}

	{system_call_1(SC_STORAGE_CLOSE, dev_id);}
}

#ifdef COBRA_ONLY
static void do_umount_iso(void)
{
	unsigned int real_disctype, effective_disctype, iso_disctype;

	cobra_get_disc_type(&real_disctype, &effective_disctype, &iso_disctype);

	// If there is an effective disc in the system, it must be ejected
	if(effective_disctype != DISC_TYPE_NONE)
	{
		cobra_send_fake_disc_eject_event();
		sys_timer_usleep(4000);
	}

	if(iso_disctype != DISC_TYPE_NONE) cobra_umount_disc_image();

	// If there is a real disc in the system, issue an insert event
	if(real_disctype != DISC_TYPE_NONE)
	{
		cobra_send_fake_disc_insert_event();
		for(u8 m=0; m<22; m++)
		{
			sys_timer_usleep(4000);

			if(isDir("/dev_bdvd")) break;
		}
		cobra_disc_auth();
	}
}
#endif

static void do_umount(bool clean)
{
	if(clean) cellFsUnlink((char*)WMTMP "/last_game.txt");

	fan_ps2_mode=false;

#ifdef COBRA_ONLY
	//if(cobra_mode)
	{
		do_umount_iso();
 #ifdef PS2_DISC
		do_umount_ps2disc(false);
 #endif
		sys_timer_usleep(20000);

		cobra_unset_psp_umd();

		{sys_map_path((char*)"/dev_bdvd", NULL);}
		{sys_map_path((char*)"/app_home", !isDir("/dev_hdd0/packages")?NULL:(char*)"/dev_hdd0/packages");}

		{sys_map_path((char*)"//dev_bdvd", NULL);}
		//{sys_map_path((char*)"//app_home", NULL);}

		{
			sys_ppu_thread_t t;
			uint64_t exit_code;
 #ifndef LITE_EDITION
			sys_ppu_thread_create(&t, netiso_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
			sys_ppu_thread_join(t, &exit_code);
 #endif
			sys_ppu_thread_create(&t, rawseciso_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, STOP_THREAD_NAME);
			sys_ppu_thread_join(t, &exit_code);
		}

		while(netiso_loaded || rawseciso_loaded) {sys_timer_usleep(100000);}
	}
#else
	{
		pokeq(0x8000000000000000ULL+MAP_ADDR, 0x0000000000000000ULL);
		pokeq(0x8000000000000008ULL+MAP_ADDR, 0x0000000000000000ULL);
		//eject_insert(1, 1);

		if(isDir("/dev_flash/pkg"))
			mount_with_mm((char*)"/dev_flash/pkg", 0);
	}
#endif //#ifdef COBRA_ONLY
}

#ifdef PS2_DISC
static void do_umount_ps2disc(bool mount)
{
	system_call_3(SC_FS_UMOUNT, (u64)(char*)"/dev_ps2disc", 0, 1);

    if(!mount) return;

	{system_call_8(SC_FS_MOUNT, (u64)(char*)"CELL_FS_IOS:BDVD_DRIVE", (u64)(char*)"CELL_FS_ISO9660", (u64)(char*)"/dev_ps2disc", 0, 1, 0, 0, 0);}
}

static bool mount_ps2disc(char *path)
{
	do_umount_ps2disc(true);

	if(!isDir(path)) return false;

 #ifdef COBRA_ONLY
	sys_map_path((char*)"/dev_ps2disc", path);
 #else

	max_mapped=0;
	add_to_map((char*)"/dev_ps2disc", path);
	add_to_map((char*)"//dev_ps2disc", path);

	u64 map_data  = (MAP_BASE);
	u64 map_paths = (MAP_BASE) + (max_mapped+1) * 0x20;

	for(u16 n=0; n<0x400; n+=8) pokeq(map_data + n, 0);

	for(u8 n=0; n<max_mapped; n++)
	{
		if(map_paths>0x80000000007FE800ULL) break;
		pokeq(map_data + (n * 0x20) + 0x10, map_paths);
		string_to_lv2(file_to_map[n].src, map_paths);
		map_paths+= (strlen(file_to_map[n].src)+8)&0x7f8;

		pokeq(map_data + (n * 0x20) + 0x18, map_paths);
		string_to_lv2(file_to_map[n].dst, map_paths);
		map_paths+= (strlen(file_to_map[n].dst)+8)&0x7f8;

		pokeq(map_data + (n * 0x20) + 0x08, strlen(file_to_map[n].dst));
		pokeq(map_data + (n * 0x20) + 0x00, strlen(file_to_map[n].src));
	}
 #endif //#ifdef COBRA_ONLY

	if(!isDir("/dev_ps2disc")) sys_timer_sleep(2);
	if(isDir("/dev_ps2disc")) return true;
	return false;
}
#endif //#ifdef PS2_DISC

static bool mount_with_mm(const char *_path0, u8 do_eject)
{
	if(is_mounting) return false;

	bool ret=true;
	fan_ps2_mode=false;

	is_mounting=true;

#ifndef COBRA_ONLY
	u64 sc_600=0;
	u64 sc_604=0;
	u64 sc_142=0;
#endif

	if(!dex_mode)
	{
		if(c_firmware==4.21f)
		{
			pokeq(0x8000000000296264ULL, 0x4E80002038600000ULL );
			pokeq(0x800000000029626CULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000057020ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x80000000000570E4ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000057090ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000057098ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005AA54ULL, 0x2F83000060000000ULL ); // fix 80010009 error
			pokeq(0x800000000005AA68ULL, 0x2F83000060000000ULL ); // fix 80010019 error

#ifndef COBRA_ONLY
			sc_600=0x33B2E0;
			sc_604=0x33B448;
			sc_142=0x2FD810;
#endif
		}
#ifndef COBRA_ONLY
        else
		if(c_firmware==4.30f)
		{
			pokeq(0x80000000002979D8ULL, 0x4E80002038600000ULL );
			pokeq(0x80000000002979E0ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000057170ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000057234ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error
			pokeq(0x8000000000057238ULL, 0x3BE000004BFFFF0CULL ); // introduced by me bug

			pokeq(0x80000000000571E0ULL, 0x419E00D860000000ULL );
			pokeq(0x80000000000571E8ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005ABA4ULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005ABB8ULL, 0x2F83000060000000ULL );

			sc_600=0x33D158; //35EEA0
			sc_604=0x33D2C0; //35EEC0
			sc_142=0x2FF460; //35E050
		}
        else
		if(c_firmware==4.31f)
		{
			pokeq(0x80000000002979E0ULL, 0x4E80002038600000ULL );
			pokeq(0x80000000002979E8ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000057174ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x800000000005723CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x80000000000571E8ULL, 0x600000002F840004ULL );
			pokeq(0x80000000000571F0ULL, 0x48000098E8629870ULL );
			pokeq(0x800000000005ABACULL, 0x60000000E8610188ULL );
			pokeq(0x800000000005ABA0ULL, 0x600000005463063EULL );

			sc_600=0x33D168;
			sc_604=0x33D2D0;
			sc_142=0x2FF470;
		}
        else
		if(c_firmware==4.40f)
		{
			pokeq(0x8000000000296DE8ULL, 0x4E80002038600000ULL );
			pokeq(0x8000000000296DF0ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000560BCULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000056180ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error +C4

			pokeq(0x800000000005612CULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000056134ULL, 0x2F84000448000098ULL );
			pokeq(0x8000000000059AF0ULL, 0x2F83000060000000ULL );
			pokeq(0x8000000000059B04ULL, 0x2F83000060000000ULL );

			sc_600=0x33D720;
			sc_604=0x33D888;
			sc_142=0x2FF9E0;
		}
        else
		if(c_firmware==4.41f)
		{
			pokeq(0x8000000000296DF0ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x8000000000296DF8ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000560C0ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000056184ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000056130ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000056138ULL, 0x2F84000448000098ULL );
			pokeq(0x8000000000059AF4ULL, 0x2F83000060000000ULL );
			pokeq(0x8000000000059B08ULL, 0x2F83000060000000ULL );

			sc_600=0x33D730;
			sc_604=0x33D898;
			sc_142=0x2FF9F0;
		}
#endif
        else
		if(c_firmware==4.46f)
		{
			pokeq(0x8000000000297310ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x8000000000297318ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000560C0ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000056184ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000056130ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000056138ULL, 0x2F84000448000098ULL );
			pokeq(0x8000000000059AF4ULL, 0x2F83000060000000ULL );
			pokeq(0x8000000000059B08ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x33DD40;
			sc_604=0x33DEA8;
			sc_142=0x2FFF58;
#endif
		}
        else
		if(c_firmware==4.50f)
		{
			pokeq(0x800000000026F61CULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x800000000026F624ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000560BCULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000056180ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x800000000005612CULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000056134ULL, 0x2F84000448000098ULL );
			pokeq(0x8000000000059AF0ULL, 0x2F83000060000000ULL );
			pokeq(0x8000000000059B04ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x33C180;
			sc_604=0x33C2E8;
			sc_142=0x302100;
#endif
		}
        else
		if(c_firmware==4.53f)
		{
			pokeq(0x800000000026F7F0ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x800000000026F7F8ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000560C0ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000056184ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000056130ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000056138ULL, 0x2F84000448000098ULL );
			pokeq(0x8000000000059AF4ULL, 0x2F83000060000000ULL );
			pokeq(0x8000000000059B08ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x33C308;
			sc_604=0x33C470;
			sc_142=0x302108;
#endif
		}
        else
		if(c_firmware==4.55f)
		{
			pokeq(0x800000000027103CULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x8000000000271044ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000056380ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000056444ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x80000000000563F0ULL, 0x419E00D860000000ULL );
			pokeq(0x80000000000563F8ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005A2ECULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005A300ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x33F5C8;
			sc_604=0x33F730;
			sc_142=0x3051D0;
#endif
		}
        else
		if(c_firmware==4.60f)
		{
			pokeq(0x80000000002925D8ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x80000000002925E0ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000056588ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x800000000005664CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x80000000000565F8ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000056600ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005A654ULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005A668ULL, 0x2F83000060000000ULL );

			pokeq(0x80000000002A1054ULL, 0x386000014E800020ULL); // fix 0x80010017 error   Original: 0xFBC1FFF0EBC225B0ULL
			pokeq(0x8000000000055C58ULL, 0x386000004E800020ULL); // fix 0x8001002B error   Original: 0xF821FE917C0802A6ULL

			//lv2poke32(0x8000000000058DACULL, 0x60000000);      // fix 0x80010017 error (found by @smhabib)

			// Booting of game discs and backups speed increased
			//lv2poke32(0x8000000000058DA0ULL, 0x38600001);
			//lv2poke32(0x800000000005A96CULL, 0x38600000);

			// enable new habib patches
			pokeq(0x8000000000058DACULL + 0x00, 0x60000000E8610098ULL);
			pokeq(0x8000000000058DACULL + 0x08, 0x2FA30000419E000CULL);
			pokeq(0x8000000000058DACULL + 0x10, 0x388000334800BE15ULL);
			pokeq(0x8000000000058DACULL + 0x18, 0xE80100F07FE307B4ULL);

			pokeq(0x8000000000055C5CULL + 0x00, 0x386000004E800020ULL);
			pokeq(0x8000000000055C5CULL + 0x08, 0xFBC10160FBE10168ULL);
			pokeq(0x8000000000055C5CULL + 0x10, 0xFB610148FB810150ULL);
			pokeq(0x8000000000055C5CULL + 0x18, 0xFBA10158F8010180ULL);

#ifndef COBRA_ONLY
			sc_600=0x340630; //0x363A18 + 600*8 = 00364CD8 -> 80 00 00 00 00 34 06 30
			sc_604=0x340798; //0x363A18 + 604*8 = 00364CF8 -> 80 00 00 00 00 34 07 98
			sc_142=0x306478; //0x363A18 + 142*8 = 00363E88 -> 80 00 00 00 00 30 64 78
#endif
		}
        else
		if(c_firmware==4.65f || c_firmware==4.66f)
		{
			//patches by deank
			pokeq(0x800000000026FDDCULL, 0x4E80002038600000ULL ); // fix 8001003C error  Original: 0x4E8000208003026CULL
			pokeq(0x800000000026FDE4ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error  Original: 0x3D6000463D201B43ULL
			pokeq(0x800000000005658CULL, 0x63FF003D60000000ULL ); // fix 8001003D error  Original: 0x63FF003D419EFFD4ULL
			pokeq(0x8000000000056650ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error  Original: 0x3FE0800163FF003EULL

			pokeq(0x80000000000565FCULL, 0x419E00D860000000ULL ); // Original: 0x419E00D8419D00C0ULL
			pokeq(0x8000000000056604ULL, 0x2F84000448000098ULL ); // Original: 0x2F840004409C0048ULL //PATCH_JUMP
			pokeq(0x800000000005A658ULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
			pokeq(0x800000000005A66CULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL

			pokeq(0x8000000000056230ULL, 0x386000012F830000ULL ); // ignore LIC.DAT check
			pokeq(0x80000000002302F0ULL, 0x38600000F8690000ULL ); // fix 0x8001002B / 80010017 errors (2015-01-03)

			pokeq(0x8000000000055C5CULL, 0xF821FE917C0802A6ULL ); // just restore the original
			pokeq(0x8000000000058DB0ULL, 0x419E0038E8610098ULL ); // just restore the original

		 /*
			//anti-ode patches by deank
			if(!is_rebug)
            {
				//pokeq(0x8000000000055C5CULL, 0xF821FE917C0802A6ULL ); //replaced by deank's patch (2015-01-03)
				pokeq(0x8000000000055C84ULL, 0x6000000060000000ULL );
				pokeq(0x8000000000055C8CULL, 0x600000003BA00000ULL );
            }

			//pokeq(0x80000000002A1060ULL, 0x386000014E800020ULL); // fix 0x80010017 error   Original: 0xFBC1FFF0EBC225B0ULL
			//pokeq(0x8000000000055C5CULL, 0x386000004E800020ULL); // fix 0x8001002B error   Original: 0xF821FE917C0802A6ULL

			// Booting of game discs and backups speed increased
			//lv2poke32(0x8000000000058DA4ULL, 0x38600001);
			//lv2poke32(0x800000000005A970ULL, 0x38600000);

			// habib patches (newest ones)
			pokeq(0x8000000000055C98ULL, 0x38600000EB610148ULL); //Original: 0x7FA307B4EB610148EB8101507C0803A6
			pokeq(0x8000000000058DCCULL, 0x38600000EBA100C8ULL); //Original: 0x7FE307B4EBA100C8EBC100D07C0803A6

			// enable new habib patches (now obsolete) //replaced by deank's patch (2015-01-03)
			pokeq(0x8000000000058DB0ULL + 0x00, 0x60000000E8610098ULL);
			pokeq(0x8000000000058DB0ULL + 0x08, 0x2FA30000419E000CULL);
			pokeq(0x8000000000058DB0ULL + 0x10, 0x388000334800BE15ULL);
			pokeq(0x8000000000058DB0ULL + 0x18, 0xE80100F07FE307B4ULL);

			pokeq(0x8000000000055C5CULL + 0x00, 0x386000004E800020ULL);
			pokeq(0x8000000000055C5CULL + 0x08, 0xFBC10160FBE10168ULL);
			pokeq(0x8000000000055C5CULL + 0x10, 0xFB610148FB810150ULL);
			pokeq(0x8000000000055C5CULL + 0x18, 0xFBA10158F8010180ULL);

			//patch to prevent blackscreen on usb games in jb format
			pokeq(0x8000000000055C84ULL, 0x386000002F830001ULL); //Original: 0x481DA6692F830001ULL
			pokeq(0x8000000000055C8CULL, 0x419E00303BA00000ULL); //Original: 0x419E00303BA00000ULL
		 */

#ifndef COBRA_ONLY
			sc_600=0x340640; //0x363A18 + 600*8 = 00364CD8 -> 80 00 00 00 00 34 06 40
			sc_604=0x3407A8; //0x363A18 + 604*8 = 00364CF8 -> 80 00 00 00 00 34 07 A8
			sc_142=0x306488; //0x363A18 + 142*8 = 00363E88 -> 80 00 00 00 00 30 64 88
#endif
		}
		else
		if(c_firmware==4.70f)
		{
			//patches by deank
			pokeq(0x80000000002670D8ULL, 0x4E80002038600000ULL ); // fix 8001003C error  Original: 0x4E8000208003026CULL //0x80000000002898DCULL??
			pokeq(0x80000000002670E0ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error  Original: 0x3D6000463D201B43ULL //0x80000000002898E4ULL??
			pokeq(0x8000000000056588ULL, 0x63FF003D60000000ULL ); // fix 8001003D error  Original: 0x63FF003D419EFFD4ULL
			pokeq(0x800000000005664CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error  Original: 0x3FE0800163FF003EULL

			pokeq(0x80000000000565F8ULL, 0x419E00D860000000ULL ); // Original: 0x419E00D8419D00C0ULL
			pokeq(0x8000000000056600ULL, 0x2F84000448000098ULL ); // Original: 0x2F840004409C0048ULL //PATCH_JUMP
			pokeq(0x800000000005A6DCULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
			pokeq(0x800000000005A6F0ULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL

			pokeq(0x800000000005622CULL, 0x386000012F830000ULL ); // ignore LIC.DAT check
			pokeq(0x80000000002275ECULL, 0x38600000F8690000ULL ); // fix 0x8001002B / 80010017 errors (2015-01-03)

			pokeq(0x8000000000055C58ULL, 0xF821FE917C0802A6ULL ); // just restore the original
			pokeq(0x8000000000058E18ULL, 0x419E0038E8610098ULL ); // just restore the original

#ifndef COBRA_ONLY
			sc_600=0x33FE88;
			sc_604=0x33FFF0;
			sc_142=0x306618;
#endif
		}
	}
	else
	{ //DEX

		if(c_firmware==4.21f)
		{
			pokeq(0x800000000029C8C0ULL, 0x4E80002038600000ULL );
			pokeq(0x800000000029C8C8ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x800000000005A938ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x800000000005A9FCULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x800000000005A9A8ULL, 0x419E00D860000000ULL );
			pokeq(0x800000000005A9B0ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005E36CULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005E380ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x3583F8;
			sc_604=0x3584D0;
			sc_142=0x318BA0;
#endif
		}
#ifndef COBRA_ONLY
		else
		if(c_firmware==4.30f)
		{
			pokeq(0x800000000029E034ULL, 0x4E80002038600000ULL );
			pokeq(0x800000000029E03CULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x800000000005AA88ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x800000000005AB4CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x800000000005AAF8ULL, 0x419E00D860000000ULL );
			pokeq(0x800000000005AB00ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005E4BCULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005E4D0ULL, 0x2F83000060000000ULL );

			sc_600=0x35A220;
			sc_604=0x35A2F8;
			sc_142=0x31A7A0;
		}
		else
		if(c_firmware==4.41f)
		{
			pokeq(0x800000000029D44CULL, 0x4E80002038600000ULL );
			pokeq(0x800000000029D454ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000599D8ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000059A9CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000059A48ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000059A50ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005D40CULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005D420ULL, 0x2F83000060000000ULL );

			sc_600=0x35AB40;
			sc_604=0x35AC18;
			sc_142=0x31B060;
		}
#endif
		else
		if(c_firmware==4.46f)
		{
			pokeq(0x800000000029D96CULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x800000000029D974ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x80000000000599D8ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000059A9CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000059A48ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000059A50ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005D40CULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005D420ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x35B150;
			sc_604=0x35B228;
			sc_142=0x31B5C8;
#endif
		}
#ifndef COBRA_ONLY
		else
		if(c_firmware==4.50f)
		{
			pokeq(0x8000000000275D38ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x8000000000275D40ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000059A8CULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000059B50ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000059AFCULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000059B04ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005D4C0ULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005D4D4ULL, 0x2F83000060000000ULL );

			sc_600=0x35EA90;
			sc_604=0x35EB68;
			sc_142=0x322B38;
		}
#endif
		else
		if(c_firmware==4.53f)
		{
			pokeq(0x8000000000275F0CULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x8000000000275F14ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000059A90ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000059B54ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000059B00ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000059B08ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005D4C4ULL, 0x2F83000060000000ULL );
			pokeq(0x800000000005D4D8ULL, 0x2F83000060000000ULL );

#ifndef COBRA_ONLY
			sc_600=0x3602A8; //0x385108 + 600*8 = 003863C8 -> 80 00 00 00 00 36 02 A8
			sc_604=0x360380; //0x385108 + 604*8 = 003863E8 -> 80 00 00 00 00 36 03 80
			sc_142=0x3242F0; //0x385108 + 142*8 = 00385578 -> 80 00 00 00 00 32 42 F0
#endif
		}
#ifndef COBRA_ONLY
        else
		if(c_firmware==4.55f)
		{
			pokeq(0x8000000000277758ULL, 0x4E80002038600000ULL ); // fix 8001003C error
			pokeq(0x8000000000277760ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error
			pokeq(0x8000000000059D50ULL, 0x63FF003D60000000ULL ); // fix 8001003D error
			pokeq(0x8000000000059E14ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error

			pokeq(0x8000000000059DC0ULL, 0x419E00D860000000ULL );
			pokeq(0x8000000000059DC8ULL, 0x2F84000448000098ULL );
			pokeq(0x800000000005DCB8ULL, 0x2F83000060000000ULL );
            pokeq(0x800000000005DCD0ULL, 0x2F83000060000000ULL );

			sc_600=0x3634F8; //0x388488 + 600*8 = 00389748 -> 80 00 00 00 00 36 34 F8
			sc_604=0x3635D0; //0x388488 + 604*8 = 00389768 -> 80 00 00 00 00 36 35 D0
			sc_142=0x327348; //0x388488 + 142*8 = 003888F8 -> 80 00 00 00 00 32 73 48
		}
#endif
		else
		if(c_firmware==4.65f || c_firmware==4.66f)
		{
			//patches by deank
			pokeq(0x80000000002764F8ULL, 0x4E80002038600000ULL ); // fix 8001003C error  Original: 0x4E8000208003026CULL
			pokeq(0x8000000000276500ULL, 0x7C6307B44E800020ULL ); // fix 8001003C error  Original: 0x3D6000463D201B43ULL
			pokeq(0x8000000000059F5CULL, 0x63FF003D60000000ULL ); // fix 8001003D error  Original: 0x63FF003D419EFFD4ULL
			pokeq(0x800000000005A020ULL, 0x3FE080013BE00000ULL ); // fix 8001003E error  Original: 0x3FE0800163FF003EULL

			pokeq(0x8000000000059FCCULL, 0x419E00D860000000ULL ); // Original: 0x419E00D8419D00C0ULL
			pokeq(0x8000000000059FD4ULL, 0x2F84000448000098ULL ); // Original: 0x2F840004409C0048ULL //PATCH_JUMP
			pokeq(0x800000000005E028ULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
			pokeq(0x800000000005E03CULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL

			pokeq(0x8000000000059C00ULL, 0x386000012F830000ULL ); // ignore LIC.DAT check
			pokeq(0x80000000002367CCULL, 0x38600000F8690000ULL ); // fix 0x8001002B / 80010017 errors (ported for DEX 2015-01-03)

			pokeq(0x800000000005962CULL, 0xF821FE917C0802A6ULL ); // just restore the original
			pokeq(0x800000000005C780ULL, 0x419E0038E8610098ULL ); // just restore the original

		 /*
			//anti-ode patches by deank
			if(!is_rebug)
			{
				//pokeq(0x800000000005962CULL, 0xF821FE917C0802A6ULL );
				pokeq(0x8000000000059654ULL, 0x6000000060000000ULL );
				pokeq(0x800000000005965CULL, 0x600000003BA00000ULL );
			}

			//pokeq(0x800000000005C780ULL, 0x60000000E8610098ULL );

			// habib patches (newest ones)
			pokeq(0x8000000000059668ULL, 0x38600000EB610148ULL); //Original: 0x7FA307B4EB610148EB8101507C0803A6
			pokeq(0x800000000005C79CULL, 0x38600000EBA100C8ULL); //Original: 0x7FE307B4EBA100C8EBC100D07C0803A6

			// enable new habib patches (now obsolete) //replaced by deank's patch (2015-01-03)
			pokeq(0x800000000005C780ULL + 0x00, 0x60000000E8610098ULL);
			pokeq(0x800000000005C780ULL + 0x08, 0x2FA30000419E000CULL);
			pokeq(0x800000000005C780ULL + 0x10, 0x388000334800BE15ULL);
			pokeq(0x800000000005C780ULL + 0x18, 0xE80100F07FE307B4ULL);

			pokeq(0x800000000005962CULL + 0x00, 0x386000004E800020ULL);
			pokeq(0x800000000005962CULL + 0x08, 0xFBC10160FBE10168ULL);
			pokeq(0x800000000005962CULL + 0x10, 0xFB610148FB810150ULL);
			pokeq(0x800000000005962CULL + 0x18, 0xFBA10158F8010180ULL);

			//patch to prevent blackscreen on usb games in jb format
			pokeq(0x8000000000059654ULL, 0x386000002F830141ULL);
			pokeq(0x800000000005965CULL, 0x9E00303BA0000000ULL);
		 */

#ifndef COBRA_ONLY
			sc_600=0x364DF0; //0x38A120 + 600*8 = 0038B3E0 -> 80 00 00 00 00 36 4D F0
			sc_604=0x364EC8; //0x38A120 + 604*8 = 0038B400 -> 80 00 00 00 00 36 4E C8
			sc_142=0x328E80; //0x38A120 + 142*8 = 0038A590 -> 80 00 00 00 00 32 8E 80
#endif
		}
		else
		if(c_firmware==4.70f)
		{
			//patches by deank
			pokeq(0x800000000026D7F4ULL, 0x4E80002038600000ULL ); // fix 8001003C error  Original: 0x4E80002038600000ULL //  0x800000000029E528ULL??
			pokeq(0x800000000026D7FCULL, 0x7C6307B44E800020ULL ); // fix 8001003C error  Original: 0x7C6307B44E800020ULL //  0x800000000029E530ULL??
            pokeq(0x8000000000059F58ULL, 0x63FF003D60000000ULL ); // fix 8001003D error  Original: 0x63FF003D419EFFD4ULL
			pokeq(0x800000000005A01CULL, 0x3FE080013BE00000ULL ); // fix 8001003E error  Original: 0x3FE0800163FF003EULL

			pokeq(0x8000000000059FC8ULL, 0x419E00D860000000ULL ); // Original: 0x419E00D8419D00C0ULL
			pokeq(0x8000000000059FD0ULL, 0x2F84000448000098ULL ); // Original: 0x2F840004409C0048ULL //PATCH_JUMP
			pokeq(0x800000000005E0ACULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL
			pokeq(0x800000000005E0C0ULL, 0x2F83000060000000ULL ); // fix 80010009 error  Original: 0x2F830000419E00ACULL

			pokeq(0x8000000000059BFCULL, 0x386000012F830000ULL ); // ignore LIC.DAT check
			pokeq(0x800000000022DAC8ULL, 0x38600000F8690000ULL ); // fix 0x8001002B / 80010017 errors (ported for DEX 2015-01-03)

			pokeq(0x8000000000059628ULL, 0xF821FE917C0802A6ULL ); // just restore the original
			pokeq(0x800000000005C7E8ULL, 0x419E0038E8610098ULL ); // just restore the original

#ifndef COBRA_ONLY
			sc_600=0x3647B8; //0x38A368 + 600*8 = 0038B628 -> 80 00 00 00 00 36 47 B8
			sc_604=0x364890; //0x38A368 + 604*8 = 0038B648 -> 80 00 00 00 00 36 48 90
			sc_142=0x329190; //0x38A368 + 142*8 = 0038A7D8 -> 80 00 00 00 00 32 91 90
#endif
		}
	}

	struct CellFsStat s; int fs;
	char titleID[10];

	char _path[MAX_PATH_LEN];

	strcpy(_path, _path0);

	int plen=strlen(_path)-9;

	if(_path[0]=='/' && plen>0)
	{
		for(int n=0; n<plen; n++)
			if(memcmp(_path + n, "/PS3_GAME", 9)==0) {_path[n]=0; break;}
	}

#ifndef LITE_EDITION
	if(!strcmp(_path, "/net0")) strcpy((char*)_path, "/net0/."); else
	if(!strcmp(_path, "/net1")) strcpy((char*)_path, "/net1/."); else
	if(!strcmp(_path, "/net2")) strcpy((char*)_path, "/net2/.");
#endif

	if(!strcmp(_path, "/dev_bdvd")) {do_umount(false); goto exit_mount;}

#ifndef COBRA_ONLY
 #ifdef EXT_GDATA
	if(do_eject==MOUNT_EXT_GDATA) goto patch;
 #endif
#endif

	// save lastgame.bin / process _next & _prev commands
	bool _prev=false, _next=false;
	if(do_eject)
	{
		//cobra_lib_init();

		//if(!strstr(_path0, "/PSPISO/") && !strstr(_path0, "/ISO/"))
		{
			int fd=0;
			_lastgames lastgames; memset(&lastgames, 0, sizeof(_lastgames)); lastgames.last=250;

			if(cellFsOpen((char*)WMTMP "/last_games.bin", CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				cellFsRead(fd, (void *)&lastgames, sizeof(_lastgames), NULL);
				cellFsClose(fd);
			}

			_next=(bool)(strstr(_path0, "_next")!=NULL);
			_prev=(bool)(strstr(_path0, "_prev")!=NULL);

			if(_next || _prev)
			{
				if(lastgames.last>(MAX_LAST_GAMES-1)) goto exit_mount;
				if(_prev)
				{
					if(lastgames.last==0) lastgames.last=(MAX_LAST_GAMES-1); else lastgames.last--;
				}
				if(_next)
				{
					if(lastgames.last==(MAX_LAST_GAMES-1)) lastgames.last=0; else lastgames.last++;
				}
				if(lastgames.game[lastgames.last][0]!='/') lastgames.last=0;
				if(lastgames.game[lastgames.last][0]!='/' || strlen(lastgames.game[lastgames.last])<7) goto exit_mount;
				strcpy(_path, lastgames.game[lastgames.last]);
			}
			else

			{
				if(lastgames.last==250)
				{
					lastgames.last=0;
					strcpy(lastgames.game[lastgames.last], _path);
				}
				else
				{
					bool found=false;
					for(u8 n=0;n<MAX_LAST_GAMES;n++)
					{
						if(!strcmp(lastgames.game[n], _path)) {found=true; break;}
					}
					if(!found)
					{
						lastgames.last++;
						if(lastgames.last>(MAX_LAST_GAMES-1)) lastgames.last=0;
						strcpy(lastgames.game[lastgames.last], _path);
					}
				}
			}

			if(cellFsOpen((char*)WMTMP "/last_games.bin", CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
			{
				cellFsWrite(fd, (void *)&lastgames, sizeof(_lastgames), NULL);
				cellFsClose(fd);
				cellFsChmod((char*)WMTMP "/last_games.bin", MODE);
			}
		}
	}

	// last mounted game
	if(_path[0]=='_' || strrchr(_path, '/')==NULL) goto exit_mount;
	else
	{
		char path2[MAX_PATH_LEN]; int fd;
		sprintf(path2, WMTMP "/last_game.txt");

		if(cellFsOpen(path2, CELL_FS_O_CREAT | CELL_FS_O_TRUNC | CELL_FS_O_WRONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			u64 written = 0;
			cellFsWrite(fd, (void *)_path, strlen(_path), &written);
			cellFsClose(fd);
			cellFsChmod(path2, MODE);
		}
	}

#ifdef COBRA_ONLY
 #ifdef FAKEISO
	// launch ntfs fake iso
	{
		if(strstr(_path, ".ntfs[BDFILE]") || (strstr(_path, ".ntfs[") && strstr(_path, "[raw]")))
		{

			bool found=false; u8 n;
			const char raw_iso_sprx[4][40] = {  "/dev_flash/vsh/module/raw_iso.sprx",
												"/dev_hdd0/raw_iso.sprx",
												"/dev_hdd0/plugins/raw_iso.sprx",
												"/dev_hdd0/game/IRISMAN00/sprx_iso" };

			for(n = 0; n < 4; n++)
				if(cellFsStat(raw_iso_sprx[n], &s)==CELL_FS_SUCCEEDED) {found = true; break;}

			if(found)
			{
				cellFsChmod(_path, MODE);

				int fdw;
				if(cellFsOpen((char*)_path, CELL_FS_O_RDONLY, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
				{
					uint64_t msiz = 0; uint8_t sprx_data[_64KB_];
					cellFsLseek(fdw, 0, CELL_FS_SEEK_SET, &msiz);
					cellFsRead(fdw, sprx_data, _64KB_, &msiz);
					cellFsClose(fdw);

					cobra_unload_vsh_plugin(0);

					do_umount(false);
					cobra_load_vsh_plugin(0, (char*)raw_iso_sprx[n], sprx_data, msiz);

					_path[strlen(_path)-13]=0;
					sprintf((char*)sprx_data, "\"%s\" %s", _path+20, STR_LOADED2);
					show_msg((char*)sprx_data);
					goto patch;
				}
			}
		}
	}
 #endif
#endif

	// Launch PS2 Classic
	if(!extcmp(_path, ".BIN.ENC", 8))
	{
		char temp[MAX_PATH_LEN];

		if(cellFsStat(PS2_CLASSIC_PLACEHOLDER, &s)==CELL_FS_SUCCEEDED)
		{
			sprintf(temp, "PS2 Classic\n%s", strrchr(_path, '/') + 1);
			copy_in_progress=true; copied_count = 0;
			show_msg(temp);

			if(c_firmware>=4.65f)
			{   // Auto create "classic_ps2 flag" for PS2 Classic (.BIN.ENC) on rebug 4.65.2
				do_umount(false);
				enable_classic_ps2_mode();
			}

			cellFsUnlink(PS2_CLASSIC_ISO_PATH);
			if(filecopy(_path, (char*)PS2_CLASSIC_ISO_PATH, COPY_WHOLE_FILE) == 0)
			{
				if(cellFsStat(PS2_CLASSIC_ISO_ICON ".bak", &s)!=CELL_FS_SUCCEEDED)
					filecopy((char*)PS2_CLASSIC_ISO_ICON, (char*)(PS2_CLASSIC_ISO_ICON ".bak"), COPY_WHOLE_FILE);

				sprintf(temp, "%s.png", _path);
				if(cellFsStat(temp, &s)!=CELL_FS_SUCCEEDED) sprintf(temp, "%s.PNG", _path);

				cellFsUnlink(PS2_CLASSIC_ISO_ICON);
				if(cellFsStat(temp, &s)==CELL_FS_SUCCEEDED)
					filecopy(temp, (char*)PS2_CLASSIC_ISO_ICON, COPY_WHOLE_FILE);
				else
					filecopy((char*)(PS2_CLASSIC_ISO_ICON ".bak"), (char*)PS2_CLASSIC_ISO_ICON, COPY_WHOLE_FILE);

				if(webman_config->fanc) restore_fan(1); //fan_control( ((webman_config->ps2temp*255)/100), 0);

				// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
				savefile((char*)WMNOSCAN, NULL, 0);

				sprintf(temp, "\"%s\" %s", strrchr(_path, '/') + 1, STR_LOADED2);
			}
			else
				{sprintf(temp, "PS2 Classic\n%s", STR_ERROR); ret=false;}

			show_msg(temp);
			copy_in_progress=false;
		}
		else
		{
			sprintf(temp, "PS2 Classic Placeholder %s", STR_NOTFOUND);
			show_msg(temp);
			ret=false;
		}

		goto patch;
	}

	if((c_firmware>=4.65f) && strstr(_path, "/PS2ISO/")!=NULL)
	{   // Auto remove "classic_ps2" flag for PS2 ISOs on rebug 4.65.2
		disable_classic_ps2_mode();
	}

#ifdef COBRA_ONLY
	//if(cobra_mode)
	{
 #ifdef EXT_GDATA
		{
			// auto-enable external GD
			if(do_eject!=1) ;
			else if(strstr(_path, "/GAME"))
			{
				int fdd=0; char extgdfile[540];
				sprintf(extgdfile, "%s/PS3_GAME/PS3GAME.INI", _path);
				if(cellFsOpen(extgdfile, CELL_FS_O_RDONLY, &fdd, NULL, 0) == CELL_FS_SUCCEEDED)
				{
					u64 read_e = 0;
					if(cellFsRead(fdd, (void *)&extgdfile, 12, &read_e) == CELL_FS_SUCCEEDED) extgdfile[read_e]=0;
					cellFsClose(fdd);
					if((extgd==0) &&  (extgdfile[10] & (1<<1))) set_gamedata_status(1, false); else
					if((extgd==1) && !(extgdfile[10] & (1<<1))) set_gamedata_status(0, false);
				}
				else if(extgd) set_gamedata_status(0, false);
			}
			else if((extgd==0) && (strstr(_path, "/PS3ISO")!=NULL) && (strstr(_path, "[gd]")!=NULL))
				set_gamedata_status(1, false);
			else if(extgd) set_gamedata_status(0, false);
		}
 #endif //#ifdef EXT_GDATA

		{
			// show loaded path
			char path2[MAX_PATH_LEN];
			char temp[MAX_PATH_LEN];
			sprintf(path2, "\"%s", (strrchr(_path, '/')+1));
			if(strstr(path2, ".ntfs[")) path2[strrchr(path2, '.')-path2]=0;
			if(strrchr(path2, '.')!=NULL) path2[strrchr(path2, '.')-path2]=0;
			if(path2[1]==NULL) sprintf(path2, "\"%s", _path);
			sprintf(temp, "\" %s", STR_LOADED2); strcat(path2, temp);
			show_msg(path2);
		}

		do_umount(false);

		sys_timer_usleep(4000);
		cobra_send_fake_disc_eject_event();
		sys_timer_usleep(4000);

		if( strstr(_path, "/PS3ISO/") || strstr(_path, "/BDISO/")    || strstr(_path, "/DVDISO/") || strstr(_path, "/PS2ISO/") ||
			strstr(_path, "/PSXISO/") || strstr(_path, "/PSXGAMES/") || strstr(_path, "/PSPISO/") || strstr(_path, "/ISO/")    ||
			strstr(_path,"/net0/")    || strstr(_path,"/net1/")      || strstr(_path, "/net2/")   || strstr(_path, ".ntfs[") )
		{
			if( strstr(_path, "/PSXISO/") || strstr(_path, "/PSXGAMES/") ) select_ps1emu();

			if(_next || _prev)
				sys_timer_sleep(1);
			else
				sys_timer_usleep(50000);

			u8 iso_num=1;
			char templn[MAX_LINE_LEN];
			char iso_list[16][MAX_PATH_LEN];
			char *cobra_iso_list[16];
			char path2[MAX_PATH_LEN];
			strcpy(iso_list[0], _path);
			cobra_iso_list[0] = (char*)iso_list[0];

			if(!extcasecmp(_path, ".iso.0", 6))
			{
				sprintf(path2, "%s", _path);
				path2[strlen(path2)-2]=0;
				for(u8 n=1;n<16;n++)
				{
					sprintf(templn, "%s.%i", path2, n);
					if(cellFsStat(templn, &s)==CELL_FS_SUCCEEDED)
					{
						iso_num++;
						strcpy(iso_list[n], templn);
						cobra_iso_list[n] = (char*)iso_list[n];
					}
					else
						break;
				}
			}

			if(strstr(_path, ".ntfs["))
			{
				int fdw;
				if(cellFsOpen(_path, CELL_FS_O_RDONLY, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
				{
					sys_addr_t addr=0;
					if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr)==0)
					{
						u8* sprx_data=(u8*)addr; uint64_t msiz = 0;
						cellFsLseek(fdw, 0, CELL_FS_SEEK_SET, &msiz);
						cellFsRead(fdw, sprx_data, (_64KB_), &msiz);
						cellFsClose(fdw);

						sys_ppu_thread_create(&thread_id_ntfs, rawseciso_thread, (uint64_t)addr, -0x1d8, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NTFS);

						if(strstr(_path, ".ntfs[PS3ISO]"))
						{
							get_name(iso_list[15], _path, 0); strcat(iso_list[15], ".SFO\0");

							if(cellFsStat(iso_list[15], &s)!=CELL_FS_SUCCEEDED)
							{
								for(u8 n=0;n<10;n++)
								{
									if(filecopy((char*)"/dev_bdvd/PS3_GAME/PARAM.SFO", iso_list[15], _4KB_)==CELL_FS_SUCCEEDED) break;
									sys_timer_usleep(500000);
								}
							}
							iso_list[15][strlen(iso_list[15])-4]=0; strcat(iso_list[15], ".PNG");
							if(cellFsStat(iso_list[15], &s)!=CELL_FS_SUCCEEDED)
							{
								for(u8 n=0;n<10;n++)
								{
									if(filecopy((char*)"/dev_bdvd/PS3_GAME/ICON0.PNG", iso_list[15], COPY_WHOLE_FILE)==CELL_FS_SUCCEEDED) break;
									sys_timer_usleep(500000);
								}
							}
						}

					}
				}
				goto patch;
				//return;
			}

 #ifndef LITE_EDITION
			if(strstr(_path, "/net0") || strstr(_path, "/net1") || strstr(_path, "/net2"))
			{
				sys_addr_t addr=0;
				if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &addr)==0)
				{
					netiso_args *mynet_iso	= (netiso_args*)addr;
					memset(mynet_iso, 0, _64KB_);

					if( (_path[4]=='0' && webman_config->netd0 && webman_config->neth0[0] && webman_config->netp0>0) ||
						(_path[4]=='1' && webman_config->netd1 && webman_config->neth1[0] && webman_config->netp1>0) ||
						(_path[4]=='2' && webman_config->netd2 && webman_config->neth2[0] && webman_config->netp2>0) )
					{
						if(_path[4]=='1')
						{
							sprintf(mynet_iso->server, "%s", webman_config->neth1);
							mynet_iso->port=webman_config->netp1;
						}
						else
						if(_path[4]=='2')
						{
							sprintf(mynet_iso->server, "%s", webman_config->neth2);
							mynet_iso->port=webman_config->netp2;
						}
						else
						{
							sprintf(mynet_iso->server, "%s", webman_config->neth0);
							mynet_iso->port=webman_config->netp0;
						}
					}
					else
					{
						sys_memory_free(addr);
						goto patch;
					}

					strcpy(mynet_iso->path, _path+5);
					if(strstr(_path, "/PS3ISO/")) mynet_iso->emu_mode=EMU_PS3; else
					if(strstr(_path, "/BDISO/" )) mynet_iso->emu_mode=EMU_BD;  else
					if(strstr(_path, "/DVDISO/")) mynet_iso->emu_mode=EMU_DVD; else
					if(strstr(_path, "/PSX"))
					{
						TrackDef tracks[1];
						tracks[0].lba = 0;
						tracks[0].is_audio = 0;
						mynet_iso->emu_mode=EMU_PSX;
						mynet_iso->num_tracks=1;

						memcpy(mynet_iso->tracks, tracks, sizeof(TrackDef));
					}
					else if(strstr(_path, "/GAMES/") || strstr(_path, "/GAMEZ/"))
					{
						mynet_iso->emu_mode=EMU_PS3;
						sprintf(mynet_iso->path, "/***PS3***%s", _path+5);
					}
					else
					{
						mynet_iso->emu_mode=EMU_DVD;
						sprintf(mynet_iso->path, "/***DVD***%s", _path+5);
					}

					sys_ppu_thread_create(&thread_id_net, netiso_thread, (uint64_t)addr, -0x1d8, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, THREAD_NAME_NET);

					if(mynet_iso->emu_mode==EMU_PS3)
					{
						sprintf(iso_list[15], WMTMP "/%s", (strrchr(_path, '/')+1));
						if(!strstr(mynet_iso->path, "/***PS3***")) iso_list[15][strlen(iso_list[15])-4]=0;
						strcat(iso_list[15], ".SFO\0");
						if(cellFsStat(iso_list[15], &s)!=CELL_FS_SUCCEEDED)
						{
							for(u8 n=0;n<30;n++)
							{
								if(filecopy((char*)"/dev_bdvd/PS3_GAME/PARAM.SFO", iso_list[15], _4KB_)==CELL_FS_SUCCEEDED) break;
								sys_timer_usleep(500000);
							}
						}
                        iso_list[15][strlen(iso_list[15])-4]=0; strcat(iso_list[15], ".PNG");
						if(cellFsStat(iso_list[15], &s)!=CELL_FS_SUCCEEDED)
						{
							for(u8 n=0;n<30;n++)
							{
								if(filecopy((char*)"/dev_bdvd/PS3_GAME/ICON0.PNG", iso_list[15], COPY_WHOLE_FILE)==CELL_FS_SUCCEEDED) break;
								sys_timer_usleep(500000);
							}
						}
					}
				}
				goto patch;
				//return;
			}
			else
 #endif //#ifndef LITE_EDITION
			{
				cellFsUnlink((char*)WMNOSCAN); // remove wm_noscan if PS2ISO was already mounted

				if(strstr(_path, "/PS3ISO/"))
				{
 #ifdef FIX_GAME
					if(webman_config->fixgame!=FIX_GAME_DISABLED)
					{
						fix_in_progress=true; fix_aborted = false;
						fix_iso(_path, 0x100000UL, true);
						fix_in_progress=false;
					}
 #endif //#ifdef FIX_GAME
					cobra_mount_ps3_disc_image(cobra_iso_list, iso_num);
					sys_timer_usleep(2500);
					cobra_send_fake_disc_insert_event();

					{
						get_name(iso_list[15], (strrchr(_path, '/')+1), 1); strcat(iso_list[15], ".SFO\0");

						if(cellFsStat(iso_list[15], &s)!=CELL_FS_SUCCEEDED)
						{
							for(u8 n=0;n<5;n++)
							{
								if(filecopy((char*)"/dev_bdvd/PS3_GAME/PARAM.SFO", iso_list[15], _4KB_)==CELL_FS_SUCCEEDED) break;
								sys_timer_usleep(500000);
							}
						}
						iso_list[15][strlen(iso_list[15])-4]=0; strcat(iso_list[15], ".PNG");
						if(cellFsStat(iso_list[15], &s)!=CELL_FS_SUCCEEDED)
						{
							for(u8 n=0;n<5;n++)
							{
								if(filecopy((char*)"/dev_bdvd/PS3_GAME/ICON0.PNG", iso_list[15], COPY_WHOLE_FILE)==CELL_FS_SUCCEEDED) break;
								sys_timer_usleep(500000);
							}
						}
					}
					goto patch;
					//return;
				}
				else if(strstr(_path, "/PSPISO/") || strstr(_path, "/ISO/"))
				{
					delete_history(false);

					cellFsUnlink((char*)"/dev_hdd0/game/PSPC66820/PIC1.PNG");
					cobra_unset_psp_umd();
					int result=cobra_set_psp_umd2(_path, NULL, (char*)"/dev_hdd0/tmp/psp_icon.png", 2);
					is_mounting=false;
					if(result==ENOTSUP || result==EABORT)
						return false;
					else if(!result)
					{
						cobra_send_fake_disc_insert_event();
						return true;
					}
				}
				else if(strstr(_path, "/BDISO/"))
					cobra_mount_bd_disc_image(cobra_iso_list, iso_num);
				else if(strstr(_path, "/DVDISO/"))
					cobra_mount_dvd_disc_image(cobra_iso_list, iso_num);
				else if(strstr(_path, "/PS2ISO/"))
				{
					TrackDef tracks[1];
					tracks[0].lba = 0;
					tracks[0].is_audio = 0;
					cobra_mount_ps2_disc_image(cobra_iso_list, 1, tracks, 1);
					if(webman_config->fanc) restore_fan(1); //fan_control( ((webman_config->ps2temp*255)/100), 0);

					// create "wm_noscan" to avoid re-scan of XML returning to XMB from PS2
					savefile((char*)WMNOSCAN, NULL, 0);
				}
				else if(strstr(_path, "/PSXISO/") || strstr(_path, "/PSXGAMES/"))
				{
					if(!extcasecmp(_path, ".cue", 4))
					{
						int flen=strlen(cobra_iso_list[0]);

						char extensions[8][8]={".BIN", ".bin", ".iso", ".ISO", ".img", ".IMG", ".mdf", ".MDF"};
						for(u8 e=0; e<8; e++)
						{
							cobra_iso_list[0][flen-4]=0; strcat(cobra_iso_list[0], extensions[e]);
							if(cellFsStat(cobra_iso_list[0], &s)==CELL_FS_SUCCEEDED) break;
						}

						unsigned int num_tracks=0;
						int fdw;

						sys_addr_t buf1=0;
						if(sys_memory_allocate(_64KB_, SYS_MEMORY_PAGE_SIZE_64K, &buf1)==0)
						{
							if(cellFsOpen(_path, CELL_FS_O_RDONLY, &fdw, NULL, 0)==CELL_FS_SUCCEEDED)
							{
								char *buf=(char*)buf1; uint64_t msiz = 0;
								cellFsLseek(fdw, 0, CELL_FS_SEEK_SET, &msiz);
								cellFsRead(fdw, (void *)buf, 65500, &msiz);
								cellFsClose(fdw);

								if(msiz>10)
								{

									TrackDef tracks[32];
									tracks[0].lba = 0;
									tracks[0].is_audio = 0;

									char tcode[MAX_LINE_LEN];
									u8 tmin=0, tsec=0, tfrm=0;
									u8 use_pregap=0;
									u32 lp=0;

									while(lp<msiz)// get_line ( templn, 512, buf1 ) != NULL )
									{
										u8 line_found=0;
										templn[0]=0;
										for(u32 l=0; l<511; l++)
										{
											if(l>=msiz) break;
											if(lp<msiz && buf[lp] && buf[lp]!='\n' && buf[lp]!='\r')
											{
												templn[l]=buf[lp];
												templn[l+1]=0;
											}
											else
											{
												templn[l]=0;
											}
											if(buf[lp]=='\n' || buf[lp]=='\r') line_found=1;
											lp++;
											if(buf[lp]=='\n' || buf[lp]=='\r') lp++;

											if(templn[l]==0) break;
										}

										if(!line_found) break;

										if(strstr(templn, "PREGAP")) {use_pregap=1; continue;}
										if(!strstr(templn, "INDEX 01") && !strstr(templn, "INDEX 1 ")) continue;

										sprintf(tcode, "%s", strrchr(templn, ' ')+1); tcode[8]=0;
										if(strlen(tcode)!=8 || tcode[2]!=':' || tcode[5]!=':') continue;
										tmin=(tcode[0]-'0')*10 + (tcode[1]-'0');
										tsec=(tcode[3]-'0')*10 + (tcode[4]-'0');
										tfrm=(tcode[6]-'0')*10 + (tcode[7]-'0');
										if(use_pregap && num_tracks) tsec+=2;

										if(num_tracks) tracks[num_tracks].is_audio = 1;
										tracks[num_tracks].lba=(tmin*60 + tsec)*75 + tfrm;

										num_tracks++; if(num_tracks>=32) break;
									}

									if(!num_tracks) num_tracks++;
									cobra_mount_psx_disc_image(cobra_iso_list[0], tracks, num_tracks);
								}
							}
							else
							{
								TrackDef tracks[1];
								tracks[0].lba = 0;
								tracks[0].is_audio = 0;
								cobra_mount_psx_disc_image_iso(cobra_iso_list[0], tracks, 1);
							}

							sys_memory_free(buf1);
						}
					}
					else
					{
						TrackDef tracks[1];
						tracks[0].lba = 0;
						tracks[0].is_audio = 0;
						cobra_mount_psx_disc_image_iso(cobra_iso_list[0], tracks, 1);
					}
				}

				sys_timer_usleep(2500);
				cobra_send_fake_disc_insert_event();
			}
		}
		else
		{
			int special_mode=0;

 #ifdef EXTRA_FEAT
			CellPadData pad_data;
			pad_data.len=0;

			for(u8 n=0;n<10;n++)
			{
				if(cellPadGetData(0, &pad_data) != CELL_PAD_OK)
					if(cellPadGetData(1, &pad_data) != CELL_PAD_OK)
							cellPadGetData(2, &pad_data);

				if(pad_data.len > 0) break;
				sys_timer_usleep(100000);
			}

			if(pad_data.len > 0 && (pad_data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT)) special_mode=true; //mount also app_home / eject disc
			sys_timer_usleep(10000);

			if(special_mode) eject_insert(1, 0);
 #endif //#ifdef EXTRA_FEAT

			// -- get TitleID from PARAM.SFO
			char filename[MAX_PATH_LEN];

			memset(titleID, 0, 10);

			sprintf(filename, "%s/PS3_GAME/PARAM.SFO", _path);
			if(cellFsOpen(filename, CELL_FS_O_RDONLY, &fs, NULL, 0)==CELL_FS_SUCCEEDED)
			{
				char paramsfo[_4KB_]; unsigned char *mem = (u8*)paramsfo;
				uint64_t msiz = 0;

				cellFsLseek(fs, 0, CELL_FS_SEEK_SET, &msiz);
				cellFsRead(fs, (void *)&paramsfo, _4KB_, &msiz);
				cellFsClose(fs);

 #ifndef FIX_GAME
				// get titleid
				fix_param_sfo(mem, titleID, 0);
 #else
				// fix ps3 extra
				char tmp_path[MAX_PATH_LEN]; sprintf(tmp_path, "%s/PS3_EXTRA", _path);
				if(webman_config->fixgame!=FIX_GAME_DISABLED && isDir(tmp_path) && fix_ps3_extra(mem))
				{
					savefile(filename, paramsfo, msiz);
				}

				tmp_path[10]=0;

				// get titleid & fix game folder if version is higher than cfw
				if((fix_param_sfo(mem, titleID, 0) || webman_config->fixgame==FIX_GAME_FORCED) && webman_config->fixgame!=FIX_GAME_DISABLED && !strstr(tmp_path, "/net") && !strstr(tmp_path, "/dev_bdvd"))
				{
					savefile(filename, paramsfo, msiz);

					sprintf(filename, "%s %s", STR_FIXING, _path);
					show_msg(filename);

					// fix game folder
					fix_in_progress=true; fix_aborted=false;

					sprintf(filename, "/dev_hdd0/game/%s/USRDIR/EBOOT.BIN", titleID); // has update on hdd0?

					if(cellFsStat(filename, &s)==CELL_FS_SUCCEEDED)
						sprintf(filename, "/dev_hdd0/game/%s/USRDIR", titleID);
					else
						sprintf(filename, "%s/PS3_GAME/USRDIR", _path);

					fix_game(filename);
					fix_in_progress=false;

					if(webman_config->fixgame==FIX_GAME_FORCED) {webman_config->fixgame=FIX_GAME_QUICK; save_settings();}
				}
 #endif //#ifndef FIX_GAME
			}
			// ----

			// -- reset USB bus
			if(!webman_config->bus)
			{
				if(strstr(_path, "/dev_usb") && isDir(_path))
				{
					// send fake eject event
					for(u8 f0=0; f0<8; f0++) fake_eject_event((f0<6)?USB_MASS_STORAGE_1(f0):USB_MASS_STORAGE_2(f0));

					sys_timer_sleep(1); u8 indx=0;

					if(strstr(_path, "/dev_usb00")) indx=_path[10]-'0';

					// send fake insert event for the current usb device
					fake_insert_event((indx<6)?USB_MASS_STORAGE_1(indx):USB_MASS_STORAGE_2(indx), DEVICE_TYPE_USB);

					sys_timer_sleep(3);

					// send fake insert event for the other usb devices
					for(u8 f0=0; f0<8; f0++)
					{
						if(f0!=indx) fake_insert_event((f0<6)?USB_MASS_STORAGE_1(f0):USB_MASS_STORAGE_2(f0), DEVICE_TYPE_USB);
					}
				}
			}

			// -- mount game folder
			if(titleID[0] && titleID[8]>='0')
				cobra_map_game(_path, titleID, &special_mode);
			else
				cobra_map_game(_path, (char*)"TEST00000", &special_mode);
		}
		//return;
	}
#endif //#ifdef COBRA_ONLY

patch:

#ifndef COBRA_ONLY

	if(c_firmware==0.0f) {ret=false; goto exit_mount;}

	//if(!cobra_mode)
	{
        //Remove Lv2 memory protection
		if(c_firmware==4.21f)
		{
			poke_lv1(HV_START_OFFSET_421 + 0x00, 0x0000000000000001ULL);
			poke_lv1(HV_START_OFFSET_421 + 0x08, 0xe0d251b556c59f05ULL);
			poke_lv1(HV_START_OFFSET_421 + 0x10, 0xc232fcad552c80d7ULL);
			poke_lv1(HV_START_OFFSET_421 + 0x18, 0x65140cd200000000ULL);
		}
		else
		if( (c_firmware>=4.30f && c_firmware<=4.53f) )
		{
			poke_lv1(HV_START_OFFSET_430 + 0x00, 0x0000000000000001ULL);
			poke_lv1(HV_START_OFFSET_430 + 0x08, 0xe0d251b556c59f05ULL);
			poke_lv1(HV_START_OFFSET_430 + 0x10, 0xc232fcad552c80d7ULL);
			poke_lv1(HV_START_OFFSET_430 + 0x18, 0x65140cd200000000ULL);
		}
        else
		if(c_firmware>=4.55f && c_firmware<=4.70f)
		{
			poke_lv1(HV_START_OFFSET_460 + 0x00, 0x0000000000000001ULL);
			poke_lv1(HV_START_OFFSET_460 + 0x08, 0xe0d251b556c59f05ULL);
			poke_lv1(HV_START_OFFSET_460 + 0x10, 0xc232fcad552c80d7ULL);
			poke_lv1(HV_START_OFFSET_460 + 0x18, 0x65140cd200000000ULL);
		}

		if(do_eject) eject_insert(1, 1);

		if(c_firmware>=4.30f && c_firmware<=4.70f)
		{	// add and enable lv2 peek/poke + lv1 peek/poke
			pokeq(0x800000000000171CULL + 0x00, 0x7C0802A6F8010010ULL);
			pokeq(0x800000000000171CULL + 0x08, 0x396000B644000022ULL);
			pokeq(0x800000000000171CULL + 0x10, 0x7C832378E8010010ULL);
			pokeq(0x800000000000171CULL + 0x18, 0x7C0803A64E800020ULL);
			pokeq(0x800000000000171CULL + 0x20, 0x7C0802A6F8010010ULL);
			pokeq(0x800000000000171CULL + 0x28, 0x396000B744000022ULL);
			pokeq(0x800000000000171CULL + 0x30, 0x38600000E8010010ULL);
			pokeq(0x800000000000171CULL + 0x38, 0x7C0803A64E800020ULL);
			pokeq(0x800000000000171CULL + 0x40, 0x7C0802A6F8010010ULL);
			pokeq(0x800000000000171CULL + 0x48, 0x7D4B537844000022ULL);
			pokeq(0x800000000000171CULL + 0x50, 0xE80100107C0803A6ULL);
			pokeq(0x800000000000171CULL + 0x58, 0x4E80002080000000ULL);
			pokeq(0x800000000000171CULL + 0x60, 0x0000170C80000000ULL);
			pokeq(0x800000000000171CULL + 0x68, 0x0000171480000000ULL);
			pokeq(0x800000000000171CULL + 0x70, 0x0000171C80000000ULL);
			pokeq(0x800000000000171CULL + 0x78, 0x0000173C80000000ULL);
			pokeq(0x800000000000171CULL + 0x80, 0x0000175C00000000ULL);

			pokeq(SYSCALL_PTR( 6), 0x8000000000001778ULL); //sc6
			pokeq(SYSCALL_PTR( 7), 0x8000000000001780ULL); //sc7
			pokeq(SYSCALL_PTR( 8), 0x8000000000001788ULL); //sc8
			pokeq(SYSCALL_PTR( 9), 0x8000000000001790ULL); //sc9
			pokeq(SYSCALL_PTR(10), 0x8000000000001798ULL); //sc10
		}
	}

	pokeq(0x8000000000000000ULL+MAP_ADDR, 0x0000000000000000ULL);
	pokeq(0x8000000000000008ULL+MAP_ADDR, 0x0000000000000000ULL);

	if(cobra_mode) goto exit_mount;

	if(base_addr==0) {ret=false; goto exit_mount;}

	// restore syscall table
	u64 sc_null = peekq(SYSCALL_TABLE);

	if(sc_null == peekq(SYSCALL_PTR(79)))
	{
		pokeq(SYSCALL_PTR(  35), sc_null);
		pokeq(SYSCALL_PTR(  36), sc_null);
		pokeq(SYSCALL_PTR(  37), sc_null);
		//pokeq(SYSCALL_PTR(1023), sc_null);

		if(sc_600)
		{   // restore original values
			sc_600|=0x8000000000000000ULL;
			sc_604|=0x8000000000000000ULL;
			sc_142|=0x8000000000000000ULL;

			pokeq(SYSCALL_PTR(600), sc_600); // sys_storage_open 600
			pokeq(SYSCALL_PTR(604), sc_604); // sys_storage_send_device_cmd 604
			pokeq(SYSCALL_PTR(142), sc_142); // sys_timer_sleep 142
		}
	}

	pokeq(0x80000000007FD000ULL, 0);

	// disable mM path table
	pokeq(0x8000000000000000ULL+MAP_ADDR, 0x0000000000000000ULL);
	pokeq(0x8000000000000008ULL+MAP_ADDR, 0x0000000000000000ULL);

	// disable Iris path table
	pokeq(0x80000000007FD000ULL,		  0x0000000000000000ULL);

	// restore hook used by all payloads)
	pokeq(open_hook + 0x00, 0xF821FF617C0802A6ULL);
	pokeq(open_hook + 0x08, 0xFB810080FBA10088ULL);
	pokeq(open_hook + 0x10, 0xFBE10098FB410070ULL);
	pokeq(open_hook + 0x18, 0xFB610078F80100B0ULL);
	pokeq(open_hook + 0x20, 0x7C9C23787C7D1B78ULL);

	// poke mM payload
	pokeq(base_addr + 0x00, 0x7C7D1B783B600001ULL);
	pokeq(base_addr + 0x08, 0x7B7BF806637B0000ULL | MAP_ADDR);
	pokeq(base_addr + 0x10, 0xEB5B00002C1A0000ULL);
	pokeq(base_addr + 0x18, 0x4D820020EBFB0008ULL);
	pokeq(base_addr + 0x20, 0xE8BA00002C050000ULL);
	pokeq(base_addr + 0x28, 0x418200CC7FA3EB78ULL);
	pokeq(base_addr + 0x30, 0xE89A001089640000ULL);
	pokeq(base_addr + 0x38, 0x892300005560063EULL);
	pokeq(base_addr + 0x40, 0x7F895800409E0040ULL);
	pokeq(base_addr + 0x48, 0x2F8000007CA903A6ULL);
	pokeq(base_addr + 0x50, 0x409E002448000030ULL);
	pokeq(base_addr + 0x58, 0x8964000089230000ULL);
	pokeq(base_addr + 0x60, 0x5560063E7F895800ULL);
	pokeq(base_addr + 0x68, 0x2F000000409E0018ULL);
	pokeq(base_addr + 0x70, 0x419A001438630001ULL);
	pokeq(base_addr + 0x78, 0x388400014200FFDCULL);
	pokeq(base_addr + 0x80, 0x4800000C3B5A0020ULL);
	pokeq(base_addr + 0x88, 0x4BFFFF98E89A0018ULL);
	pokeq(base_addr + 0x90, 0x7FE3FB7888040000ULL);
	pokeq(base_addr + 0x98, 0x2F80000098030000ULL);
	pokeq(base_addr + 0xA0, 0x419E00187C691B78ULL);
	pokeq(base_addr + 0xA8, 0x8C0400012F800000ULL);
	pokeq(base_addr + 0xB0, 0x9C090001409EFFF4ULL);
	pokeq(base_addr + 0xB8, 0xE8BA00087C632A14ULL);
	pokeq(base_addr + 0xC0, 0x7FA4EB78E8BA0000ULL);
	pokeq(base_addr + 0xC8, 0x7C842A1488040000ULL);
	pokeq(base_addr + 0xD0, 0x2F80000098030000ULL);
	pokeq(base_addr + 0xD8, 0x419E00187C691B78ULL);
	pokeq(base_addr + 0xE0, 0x8C0400012F800000ULL);
	pokeq(base_addr + 0xE8, 0x9C090001409EFFF4ULL);
	pokeq(base_addr + 0xF0, 0x7FFDFB787FA3EB78ULL);
	pokeq(base_addr + 0xF8, 0x4E8000204D4D504CULL); //blr + "MMPL"

	pokeq(MAP_BASE  + 0x00, 0x0000000000000000ULL);
	pokeq(MAP_BASE  + 0x08, 0x0000000000000000ULL);
	pokeq(MAP_BASE  + 0x10, 0x8000000000000000ULL);
	pokeq(MAP_BASE  + 0x18, 0x8000000000000000ULL);

	pokeq(0x8000000000000000ULL+MAP_ADDR, MAP_BASE);
	pokeq(0x8000000000000008ULL+MAP_ADDR, 0x80000000007FDBE0ULL);

	pokeq(open_hook + 0x20, (0x7C9C237848000001ULL | (base_addr-open_hook-0x24)));

	char expplg[128];
	char app_sys[128];
	struct CellFsStat buf2;

	char path[MAX_PATH_LEN];

	//------------------
	// re-load last game
	//------------------

 #ifdef EXT_GDATA
	if(do_eject==MOUNT_EXT_GDATA) // extgd
	{
		sprintf(_path, WMTMP "/last_game.txt"); int fd=0;
		if(cellFsOpen(_path, CELL_FS_O_RDONLY, &fd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			u64 read_e = 0;
			if(cellFsRead(fd, (void *)_path, MAX_PATH_LEN, &read_e) == CELL_FS_SUCCEEDED) _path[read_e]=0;
			cellFsClose(fd);
		}
		else
			_path[0]=0;
	}
 #endif //#ifdef EXT_GDATA

	if(_path[0] && strstr(_path, "/PS3_GAME/USRDIR/EBOOT.BIN")) _path[strlen(_path)-26]=0;

	sprintf(path, "%s", _path);

	if(!isDir(path)) _path[0]=path[0]=0;
	else if(do_eject)
	{   // show loaded path
		char path2[MAX_PATH_LEN];
		char temp[MAX_PATH_LEN];
		sprintf(path2, "\"%s", (strrchr(_path, '/')+1));
		if(path2[1]==NULL) sprintf(path2, "\"%s", _path);

		sprintf(temp, "\" %s", STR_LOADED2); strcat(path2, temp);
		show_msg(path2);
	}

	// -- get TitleID from PARAM.SFO
	char filename[MAX_PATH_LEN];

	sprintf(filename, "%s/PS3_GAME/PARAM.SFO", _path);
	if(cellFsOpen(filename, CELL_FS_O_RDONLY, &fs, NULL, 0)==CELL_FS_SUCCEEDED)
	{
		char paramsfo[_4KB_]; unsigned char *mem = (u8*)paramsfo;
		uint64_t msiz = 0;

		cellFsLseek(fs, 0, CELL_FS_SEEK_SET, &msiz);
		cellFsRead(fs, (void *)&paramsfo, _4KB_, &msiz);
		cellFsClose(fs);

 #ifndef FIX_GAME
		// get titleid
		memset(titleID, 0, 10);
		fix_param_sfo(mem, titleID, 0);
 #else
		// fix ps3 extra
		char tmp_path[MAX_PATH_LEN]; sprintf(tmp_path, "%s/PS3_EXTRA", _path);
		if(webman_config->fixgame!=FIX_GAME_DISABLED && isDir(tmp_path) && fix_ps3_extra(mem))
		{
			savefile(filename, paramsfo, msiz);
		}

		tmp_path[10]=0;

		// get titleid & fix game folder if version is higher than cfw
		if((fix_param_sfo(mem, titleID, 0) || webman_config->fixgame==FIX_GAME_FORCED) && webman_config->fixgame!=FIX_GAME_DISABLED && !strstr(tmp_path, "/dev_bdvd"))
		{
			savefile(filename, paramsfo, msiz);

			sprintf(filename, "%s %s", STR_FIXING, _path);
			show_msg(filename);

			// fix game folder
			fix_in_progress=true; fix_aborted=false;

			sprintf(filename, "/dev_hdd0/game/%s/USRDIR/EBOOT.BIN", titleID); // has update on hdd0?

			if(cellFsStat(filename, &s)==CELL_FS_SUCCEEDED)
				sprintf(filename, "/dev_hdd0/game/%s/USRDIR", titleID);
			else
				sprintf(filename, "%s/PS3_GAME/USRDIR", _path);

			fix_game(filename);
			fix_in_progress=false;

			if(webman_config->fixgame==FIX_GAME_FORCED) {webman_config->fixgame=FIX_GAME_QUICK; save_settings();}
		}
 #endif //#ifndef FIX_GAME
	}
	// ----

	//----------------------------------
	// map game to /dev_bdvd & /app_home
	//----------------------------------

	if(path[0])
	{
		if(do_eject)
		{
			add_to_map((char*)"/dev_bdvd", path);
			add_to_map((char*)"//dev_bdvd", path);

			char path2[strlen(_path)+24];

			sprintf(path2, "%s/PS3_GAME", _path);
			add_to_map((char*)"/app_home/PS3_GAME", path2);

			sprintf(path2, "%s/PS3_GAME/USRDIR", _path);
			add_to_map((char*)"/app_home/USRDIR", path2);

			sprintf(path2, "%s/PS3_GAME/USRDIR/", _path);
			add_to_map((char*)"/app_home/", path2);
		}

		add_to_map((char*)"/app_home", path);
	}

    //--------------------------------------------
	// auto-map /dev_hdd0/game to dev_usbxxx/GAMEI
    //---------------------------------------------
 #ifdef EXT_GDATA
    if(do_eject!=1) ;
	else if(strstr(_path, "/GAME"))
	{
		int fdd=0; char extgdfile[540];
		sprintf(extgdfile, "%s/PS3_GAME/PS3GAME.INI", _path);
		if(cellFsOpen(extgdfile, CELL_FS_O_RDONLY, &fdd, NULL, 0) == CELL_FS_SUCCEEDED)
		{
			u64 read_e = 0;
			if(cellFsRead(fdd, (void *)&extgdfile, 12, &read_e) == CELL_FS_SUCCEEDED) extgdfile[read_e]=0;
			cellFsClose(fdd);
			if((extgd==0) &&  (extgdfile[10] & (1<<1))) set_gamedata_status(1, false); else
			if((extgd==1) && !(extgdfile[10] & (1<<1))) set_gamedata_status(0, false);
		}
		else if(extgd) set_gamedata_status(0, false);
	}
 #endif

	sprintf(app_sys, MM_ROOT_STD "/sys");
	if(!isDir(app_sys))
		sprintf(app_sys, MM_ROOT_STL "/sys");
	if(!isDir(app_sys))
		sprintf(app_sys, MM_ROOT_SSTL "/sys");


    //----------------------------
	// Patched explore_plugin.sprx
    //----------------------------

    if(c_firmware==4.21f)
		sprintf(expplg, "%s/IEXP0_420.BIN", app_sys);
	else if(c_firmware==4.30f || c_firmware==4.31f)
		sprintf(expplg, "%s/IEXP0_430.BIN", app_sys);
	else if(c_firmware==4.40f || c_firmware==4.41f)
		sprintf(expplg, "%s/IEXP0_440.BIN", app_sys);
	else if(c_firmware==4.46f)
		sprintf(expplg, "%s/IEXP0_446.BIN", app_sys);
	else if(c_firmware==4.50f || c_firmware==4.53f || c_firmware==4.55f)
		sprintf(expplg, "%s/IEXP0_450.BIN", app_sys);
	else if(c_firmware==4.60f || c_firmware==4.65f || c_firmware==4.66f)
		sprintf(expplg, "%s/IEXP0_460.BIN", app_sys);
	else if(c_firmware==4.70f)
		sprintf(expplg, "%s/IEXP0_470.BIN", app_sys);
	else
        sprintf(expplg, "%s/none", app_sys);

	if(do_eject && cellFsStat(expplg, &buf2)==CELL_FS_SUCCEEDED)
		add_to_map( (char*)"/dev_flash/vsh/module/explore_plugin.sprx", expplg);


    //---------------
	// New libfs.sprx
    //---------------
	if((do_eject>0) && (c_firmware>=4.20f) && cellFsStat((char*)NEW_LIBFS_PATH, &buf2)==CELL_FS_SUCCEEDED)
		add_to_map((char*) ORG_LIBFS_PATH, (char*)NEW_LIBFS_PATH);

    //-----------------------------------------------//
	u64 map_data  = (MAP_BASE);
	u64 map_paths = (MAP_BASE) + (max_mapped+1) * 0x20;

	for(u16 n=0; n<0x400; n+=8) pokeq(map_data + n, 0);

	if(!max_mapped) {ret=false; goto exit_mount;}

	for(u8 n=0; n<max_mapped; n++)
	{
		if(map_paths>0x80000000007FE800ULL) break;
		pokeq(map_data + (n * 0x20) + 0x10, map_paths);
		string_to_lv2(file_to_map[n].src, map_paths);
		map_paths+= (strlen(file_to_map[n].src)+8)&0x7f8;

		pokeq(map_data + (n * 0x20) + 0x18, map_paths);
		string_to_lv2(file_to_map[n].dst, map_paths);
		map_paths+= (strlen(file_to_map[n].dst)+8)&0x7f8;

		pokeq(map_data + (n * 0x20) + 0x08, strlen(file_to_map[n].dst));
		pokeq(map_data + (n * 0x20) + 0x00, strlen(file_to_map[n].src));
	}

	if(isDir("/dev_bdvd")) sys_timer_sleep(2);

	//if(do_eject) eject_insert(0, 1);
#endif //#ifndef COBRA_ONLY

exit_mount:
	if(ret && extcmp(_path, ".BIN.ENC", 8))
	{
		waitfor((char*)"/dev_bdvd", 6);
		if(!isDir("/dev_bdvd")) ret = false;
	}

#ifdef FIX_GAME
	if(ret && (c_firmware<4.70f) && cellFsOpen("/dev_bdvd/PS3_GAME/PARAM.SFO", CELL_FS_O_RDONLY, &fs, NULL, 0)==CELL_FS_SUCCEEDED)
	{
		char paramsfo[_4KB_]; unsigned char *mem = (u8*)paramsfo;
		uint64_t msiz = 0;

		cellFsLseek(fs, 0, CELL_FS_SEEK_SET, &msiz);
		cellFsRead(fs, (void *)&paramsfo, _4KB_, &msiz);
		cellFsClose(fs);

		fix_param_sfo(mem, titleID, 1); // show warning (if fix is needed)
	}
#endif

	delete_history(false);

	if(!ret && !isDir("/dev_bdvd")) {char msg[MAX_PATH_LEN]; sprintf(msg, "%s %s", STR_ERROR, _path); show_msg(msg);}

#ifdef COBRA_ONLY
	{
		if(ret && (strstr(_path, ".PUP.ntfs[BD") || cellFsStat((char*)"/dev_bdvd/PS3UPDAT.PUP", &s)==CELL_FS_SUCCEEDED))
			sys_map_path((char*)"/dev_bdvd/PS3/UPDATE", (char*)"/dev_bdvd"); //redirect root of bdvd to /dev_bdvd/PS3/UPDATE

		sys_map_path((char*)"/dev_bdvd/PS3_UPDATE", (char*)"/dev_bdvd"); //redirect firmware update to root of bdvd
	}
#endif

	is_mounting=false;
	max_mapped=0;
    return ret;
}
