@echo on
@rem
@rem Builds the 68000.asm module and the special opcodes (6 7 A F)
@rem

@setlocal
cls
del 68000.obj ops6.obj opsa.obj opsf.obj

set ML=%ML% -Fl

echo Building x86 files...
ml -c -Fl -coff -Zi -DDEBUG=0 68000.asm ops6.asm ops7.asm opsa.asm opsf.asm

del 68k.lib
link -lib -out:68k.lib 68000.obj ops*.obj

@endlocal

