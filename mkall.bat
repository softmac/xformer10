
@rem ************************************************************************
@rem 
@rem Main build script for Gemulator / Xformer / SoftMac
@rem
@rem Requires Microsoft Visual Studio 98 / VC 6.0 or higher!!!
@rem
@rem ************************************************************************

@rem Wipe temporary build files
@rem
@rem On Windows 9x use DELTREE /Y in place of DEL /S

del /s *.cod
del /s *.lib
del /s *.lst
del /s *.map
del /s *.obj
del /s *.pch
del /s *.pdb

@rem Build the 6502 and 680x0 interpreters and libraries

call mkallasm.bat

set REBUILD=re

@rem Build the C sources and link the EXE

call mkinc.bat %1

