@echo off
set PS3SDK=/c/PSDK3v2
set WIN_PS3SDK=C:/PSDK3v2
set PATH=%WIN_PS3SDK%/mingw/msys/1.0/bin;%WIN_PS3SDK%/mingw/bin;%WIN_PS3SDK%/ps3dev/bin;%WIN_PS3SDK%/ps3dev/ppu/bin;%WIN_PS3SDK%/ps3dev/spu/bin;%WIN_PS3SDK%/mingw/Python27;%PATH%;
set PSL1GHT=%PS3SDK%/psl1ght
set PS3DEV=%PS3SDK%/ps3dev

make pkg

if exist webMAN_MOD_1.41.xx_Updater.pkg del webMAN_MOD_1.41.xx_Updater.pkg>nul
ren updater.pkg webMAN_MOD_1.41.xx_Updater.pkg
del updater.elf
del updater.self
del /s/q build>nul
rd /q/s build

pause
