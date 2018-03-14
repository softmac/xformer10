
@rem deltree /y *.obj *.map *.lib *.exe *.pdb *.pch
del /f *.obj *.map *.lib *.exe *.pdb *.pch

set DF=-DNDEBUG -Zi -O1 -Oi -G6 -GF -Gy -FAsc
if "%1"=="debug" set DF=-Od

set LF=-opt:ref -opt:nowin98

cl %DF% -DMAKEDSK makedsk.c -Fodsk.obj /link %LF% -out:MAKEDSK.EXE
cl %DF% -DMAKEIMG makedsk.c -Foimg.obj /link %LF% -out:MAKEIMG.EXE

cl -c -D_WINDOWS -Gz %DF% blockapi.c sectorio.c mac_hfs.c makedsk.c
link -lib -out:blockdev.lib blockapi.obj mac_hfs.obj sectorio.obj makedsk.obj

cl %DF% testmac.c  -link -opt:ref -map blockdev.lib user32.lib
cl %DF% testscsi.c -link -opt:ref -map blockdev.lib user32.lib

