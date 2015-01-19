// Mysis rec_plugin.h v0.1
typedef struct
{
	int (*DoUnk0)();
	int (*start)(); //RecStart
	int (*stop)(); //RecStop
	int (*close)(int isdiscard);
	int (*geti)(int giprm);  // RecGetInfo
	int (*md)(void * mdarg,int); //RecSetInfo
	int (*etis)(int start_time_msec); //RecSetInfo
	int (*etie)(int end_time_msec); //RecSetInfo
} rec_plugin_interface;

int (*reco_open)(int)=0;
int (*vsh_E7C34044)(int)=0;

rec_plugin_interface * rec_interface;
uint32_t * recOpt = 0;

