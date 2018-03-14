
@rem ************************************************************************
@rem 
@rem Auxilary build script for Gemulator / Xformer / SoftMac
@rem
@rem Requires Microsoft Visual Studio 98 / VC 6.0 or higher!!!
@rem
@rem ************************************************************************

@rem Rebuild blit functions

cd src
ml -c -coff -Fl -Zi utiln.asm
link -lib -out:utiln.lib utiln.obj

@rem Rebuild 6502 interpreter

cd atari8.vm
call makeasm.bat

@rem Rebuild 680x0 interpreter

cd ..\680x0.cpu
call makeasm.bat

@rem Rebuild the block device library

cd ..\blocklib
call m.bat
cd ..\..

