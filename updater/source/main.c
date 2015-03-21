#include <string.h>

#include <unistd.h>
#include <ppu-lv2.h>
#include <lv2/process.h>
#include <sys/process.h>

#include <sys/file.h>
#include <lv2/sysfs.h>

#include <io/pad.h>

bool lite=false;

int main()
{
//--- hold CROSS
    unsigned button = 0;

    padInfo padinfo;
    padData paddata;

    ioPadInit(7);

    int n, r;
    for(r=0; r<10; r++)
    {
        ioPadGetInfo(&padinfo);
        for(n = 0; n < 7; n++)
        {
            if(padinfo.status[n])
            {
                ioPadGetData(n, &paddata);
                button = (paddata.button[2] << 8) | (paddata.button[3] & 0xff);
                break;
            }
        }
        if(button) break; else usleep(20000);
    }
    ioPadEnd();

	if(button) return 0;
//---

    sysProcessExitSpawn2((const char*)"/dev_hdd0/game/HTSS00003/USRDIR/showtime.self", NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
    exit(0);

	return 0;
}
