// Thanks to Mysis & 3141card

#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/timer.h>
#include <sys/syscall.h>

#include <sys/sys_time.h>
#include <sys/timer.h>
#include <sys/time_util.h>

#include <stdbool.h>

#include <cell/cell_fs.h>
#include <cell/pad.h>
#include <cell/rtc.h>

#include "vsh_exports.h"
#include "network.h"

#define PLUGIN_NAME "rec_plugin"

SYS_MODULE_INFO(TEST, 0, 1, 0);
SYS_MODULE_START(plugin_start);
SYS_MODULE_STOP(plugin_stop);


static sys_ppu_thread_t thread_id=-1;
static int32_t done = 0;

int32_t plugin_start(uint64_t arg);
int32_t plugin_stop(void);


static inline void _sys_ppu_thread_exit(uint64_t val)
{
	system_call_1(41, val);
}

static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
	system_call_1(461, (uint64_t)(uint32_t)addr);
	return (int)p1;
}


//////////////////////////////////////////////////////////////////////////////

uint32_t *recOpt = NULL;              // recording utility vsh options struct
int32_t (*reco_open)(int32_t) = NULL; // base pointer

bool rec_start(void);

bool rec_start()
{
	//dbg_printf("Setup Recording Settings in VSH\n");

	recOpt[1] = 0x4660;//CELL_REC_PARAM_VIDEO_FMT_M4HD_HD720_5000K_30FPS | 0x2100; //CELL_REC_PARAM_VIDEO_FMT_AVC_BL_MIDDLE_512K_30FPS
	recOpt[2] = 0x0000; //CELL_REC_PARAM_AUDIO_FMT_AAC_96K
	recOpt[5] = (vsh_memory_container_by_id(1) == -1 ) ? vsh_memory_container_by_id(0) : vsh_memory_container_by_id(1);
	recOpt[0x208] = 0x80; // 0x90 show XMB || reduce memsize // 0x80; // allow show XMB

	CellRtcDateTime t;
	cellRtcGetCurrentClockLocalTime(&t);

	char g[0x120];
	game_interface = (game_plugin_interface *)plugin_GetInterface(View_Find("game_plugin"), 1);

	game_interface->DoUnk8(g);

	//dbg_printf("  -> [%s]\n", g+4);
	//dbg_printf("  -> [%s]\n", g+0x14);

	//sprintf((char *)&recOpt[0x6], "/dev_hdd0/game/PRXLOADER/USRDIR/%s_%04d.%02d.%02d_%02d_%02d.mp4",
	//                             g+4, t.year, t.month, t.day, t.hour, t.minute);

	cellFsMkdir((char*)"/dev_hdd0/VIDEO", 0777);
	sprintf((char*)&recOpt[0x6], "/dev_hdd0/VIDEO/%s_04d.%02d.%02d_%02d_%02d_%02d.mp4",
								   g+4, t.year, t.month, t.day, t.hour, t.minute, t.second);

	//dbg_printf(" -> File: [%s]\n", (char*)&recOpt[0x6]);
	//dbg_printf("Record Open\n ");


	reco_open(-1); // memory container
	sys_timer_sleep(4);


	if(View_Find("rec_plugin") != 0)
	{
		//dbg_printf("rec_plugin detected.\n");

		//dbg_printf("Getting Interface.\n");
		rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

		if(rec_interface != 0)
		{
			//dbg_printf("rec_interface->start()\n");
			rec_interface->start();
			return true;
		}
		else
		{
			//dbg_printf("Unable to get Recording Interface\n");
			return false;
		}
	}
	else
	{
		//dbg_printf("Trying -1 memCtr\n");
		reco_open(-1); //reco_open((vsh_E7C34044(1) == -1 ) ? vsh_E7C34044(0) : vsh_E7C34044(1));
		sys_timer_sleep(3);

		if(View_Find("rec_plugin") != 0)
		{
			//dbg_printf("rec_plugin detected.\n");
			//dbg_printf("Getting Interface.\n");

			rec_interface = (rec_plugin_interface *)plugin_GetInterface(View_Find("rec_plugin"), 1);

			//dbg_printf("rec_interface->start()\n");
			rec_interface->start();
			return true;
		}
		else
		{
			vshtask_notify("No rec_plugin view found.");
			return false;
		}
	}
}

/***********************************************************************
* plugin main thread
***********************************************************************/
static void plugin_thread(uint64_t arg)
{
	//dbg_init();
	//dbg_printf("programstart:\n");
	sys_timer_sleep(13);

	vshtask_notify(PLUGIN_NAME " loaded ...");

	CellPadData data;

	//////////////////////////////////////////////////////////////////////
	// plugin: class usage
	char nickname[0x80];
	int32_t nickname_len;

	xsetting_0AF1F161()->GetSystemNickname(nickname, &nickname_len);
	//dbg_printf("GetSystemNickname() [%s] [%d]\n", nickname, nickname_len);
	//////////////////////////////////////////////////////////////////////


	// get functions pointer for sub_163EB0() aka reco_open()
	reco_open = vshmain_BEF63A14; // base pointer, the export nearest to sub_163EB0()

	reco_open -= (50*8); // reco_open_opd (50 opd's above vshmain_BEF63A14_opd)

	// fetch recording utility vsh options struct (build address from instructions...)
	uint32_t addr = (*(uint32_t*)(*(uint32_t*)reco_open+0xC) & 0x0000FFFF) -1;
	recOpt = (uint32_t*)((addr << 16) + ((*(uint32_t*)(*(uint32_t*)reco_open+0x14)) & 0x0000FFFF)); // (uint32_t*)0x72EEC0;


	bool recording = false; int delay=0;

	while(!done)
	{
		if(delay==0 && cellPadGetData(0, &data) == CELL_PAD_OK && data.len > 0)
		{
			bool r3 = (data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_R3) && !(data.button[CELL_PAD_BTN_OFFSET_DIGITAL1] & CELL_PAD_CTRL_SELECT);

			if(r3)
			{
				if(View_Find("game_plugin") != 0)    // if game_plugin is loaded -> there is a game/app running and we can recording...
				{
					if(recording == false)
					{
					  // not recording yet
						vshtask_notify("Recording started");

						if(rec_start() == false)
						{
							vshtask_notify("Recording Error!");
						}
						else
						{
							recording = true;
						}
					}
					else
					{
						// we are already recording
						//dbg_printf("rec_interface->stop()\n");
						rec_interface->stop();
						//dbg_printf("rec_interface->close()\n");
						rec_interface->close(0);
						vshtask_notify("Recording finished");
						recording = false;
					}
				}
				delay = 3*6; // 3 seconds
			}
		}
		sys_timer_usleep(16668);

		if(delay>0) delay--;
	}

	sys_ppu_thread_exit(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int plugin_start(uint64_t arg)
{
	sys_ppu_thread_create(&thread_id, plugin_thread, 0, 3000, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, "plugin_thread");
	_sys_ppu_thread_exit(0);
	return SYS_PRX_RESIDENT;
}

static void plugin_stop_thread(uint64_t arg)
{
	done = 1;
	if (thread_id != (sys_ppu_thread_t)-1){
		uint64_t exit_code;
		sys_ppu_thread_join(thread_id, &exit_code);
	}

	sys_ppu_thread_exit(0);
}

static void finalize_module(void)
{
	uint64_t meminfo[5];

	sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);

	meminfo[0] = 0x28;
	meminfo[1] = 2;
	meminfo[3] = 0;

	system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);
}

int plugin_stop(void)
{
	sys_ppu_thread_t t;
	uint64_t exit_code;

	sys_ppu_thread_create(&t, plugin_stop_thread, 0, 0, 0x2000, SYS_PPU_THREAD_CREATE_JOINABLE, "plugin_stop_thread");
	sys_ppu_thread_join(t, &exit_code);

	finalize_module();
	_sys_ppu_thread_exit(0);
	return SYS_PRX_STOP_OK;
}
