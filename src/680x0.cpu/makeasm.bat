@echo on
@rem
@rem Builds the 680x0 interpreter
@rem

@setlocal
cls
del ops*.obj
del sthw.obj
del 68000.obj
del 68k.lib

set ML=%ML% -Fl

echo Building x86 files...
ml -c -Fl -coff -Zi -DDEBUG=0 68000.asm
ml -c     -coff -Zi -DDEBUG=0 ops0.asm
ml -c     -coff -Zi -DDEBUG=0 ops1.asm
ml -c     -coff -Zi -DDEBUG=0 ops2.asm
ml -c     -coff -Zi -DDEBUG=0 ops3.asm
ml -c     -coff -Zi -DDEBUG=0 ops4.asm
ml -c     -coff -Zi -DDEBUG=0 ops5.asm
ml -c     -coff -Zi -DDEBUG=0 ops6.asm
ml -c     -coff -Zi -DDEBUG=0 ops7.asm
ml -c     -coff -Zi -DDEBUG=0 ops8.asm
ml -c     -coff -Zi -DDEBUG=0 ops9.asm
ml -c     -coff -Zi -DDEBUG=0 opsA.asm
ml -c     -coff -Zi -DDEBUG=0 opsB.asm
ml -c     -coff -Zi -DDEBUG=0 opsC.asm
ml -c     -coff -Zi -DDEBUG=0 opsD.asm
ml -c     -coff -Zi -DDEBUG=0 opsE.asm
ml -c     -coff -Zi -DDEBUG=0 opsF.asm

del 68k.lib
link -lib -out:68k.lib 68000.obj ops*.obj

@endlocal

