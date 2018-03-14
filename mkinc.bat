
@rem ************************************************************************
@rem 
@rem Incremental build script for Gemulator / Xformer / SoftMac
@rem
@rem Requires Microsoft Visual Studio 98 / VC 6.0, VC 7.x, VC 10, or VC11
@rem For other versions of Visual Studio, convert the ATARIST.SLN file first.
@rem
@rem ************************************************************************

del build\debug\atarist.exe
del build\ship\atarist.exe

@setlocal
pushd .

@rem check for legacy Visual Studio 98 variables first
cd /D %MsDevDir%
if exist bin\msdev.exe goto vc6

@rem check for Visual Studio 2013 / Dev12
cd /D %VS120COMNTOOLS%
if exist ..\ide\devenv.exe goto vcdev

@rem check for Visual Studio 2012 / Dev11
cd /D %VS110COMNTOOLS%
if exist ..\ide\devenv.exe goto vcdev

@rem check for Visual Studio 2010
cd /D %VS100COMNTOOLS%
if exist ..\ide\devenv.exe goto vcdev

@rem check for Visual Studio 2008
cd /D %VS90COMNTOOLS%
if exist ..\ide\devenv.exe goto vcdev

@rem check for Visual Studio 2005
cd /D %VS80COMNTOOLS%
if exist ..\ide\devenv.exe goto vcdev

@rem check for older Visual Studio 200x (might no longer work)
cd /D %VSCOMNTOOLS%
if exist webdbg.exe goto vcdev

@rem goto vcdev

@rem when all fails it is likely Visual Studio 98 / VC6
if "%%VSCOMNTOOLS%%"== "" goto vc6
if "%%VSCOMNTOOLS%%"== "%VSCOMNTOOLS%" goto vc6
if "%VSCOMNTOOLS%"  == "" goto vc6

@rem Common case for DEVENV.EXE based builds, VC 7.0 and higher

:vcdev
set PATH=%PATH%;%CD%\..\ide;
popd
set CLT=%CL%
set CL=%CL% -FAcs -MT -Gy -GF -Zi -O2 -arch:SSE2
set CL=%CL% -GL -GA -GS-
set LINKT=%LINK%
set LINK=%LINK% /subsystem:windows,6.00 /version:9.21
set LINK=%LINK% /nodefaultlib:libc /nodefaultlib:libcmt /defaultlib:msvcrt
set LINK=%LINK% /entry:WinMainCRTStartup /largeaddressaware:no
set LINK=%LINK% /swaprun:cd /swaprun:net /release
set LINK=%LINK% /opt:ref /opt:icf,999
set LINK=%LINK% /safeseh:no

if     "%1" == "all"     devenv atarist.sln /%REBUILD%build "Debug|Win32"
if     "%1" == "all"     devenv atarist.sln /%REBUILD%build "Debug|x64"
if     "%1" == "all"     devenv atarist.sln /%REBUILD%build "Release|Win32"
if     "%1" == "all"     devenv atarist.sln /%REBUILD%build "Release|x64"

if     "%1" == "debug"   devenv atarist.sln /%REBUILD%build "Debug|Win32"
if     "%1" == "debug64" devenv atarist.sln /%REBUILD%build "Debug|x64"

if     "%1" == "ship"    devenv atarist.sln /%REBUILD%build "Release|Win32"
if     "%1" == "ship64"  devenv atarist.sln /%REBUILD%build "Release|x64"

if     "%1" == "arm"     devenv atarist.sln /%REBUILD%build "Release|ARM"
if     "%1" == "armd'    devenv atarist.sln /%REBUILD%build "Debug|ARM"

if     "%1" == ""        devenv atarist.sln /%REBUILD%build "Release|Win32"

goto done

@rem MSDEV.EXE based builds, VC 6.x

:vc6
popd
pushd src\atari8.vm
call makeasmc.bat
popd

set CLT=%CL%
set CL=%CL% -FAcs -MT -Gy -G6 -GF -Zi
set CL=%CL% -GA -GS-
set LINKT=%LINK%
set LINK=%LINK% /subsystem:windows,5.01 /version:9.21
set LINK=%LINK% /nodefaultlib:libc /defaultlib:libcmt
set LINK=%LINK% /entry:WinMainCRTStartup /fixed /largeaddressaware:no
set LINK=%LINK% /swaprun:cd /swaprun:net /ws:aggressive /release
set LINK=%LINK% /opt:ref /opt:icf,999 /opt:nowin98

if     "%1" == "debug" msdev atarist.dsp /make "GEMUL8R - Win32 (80x86) Debug"
if     "%1" == "all"   msdev atarist.dsp /make "GEMUL8R - Win32 (80x86) Debug"

if     "%1" == "all"   msdev atarist.dsp /make "GEMUL8R - Win32 (80x86) Release"
if     "%1" == "ship"  msdev atarist.dsp /make "GEMUL8R - Win32 (80x86) Release"

if     "%1" == ""      msdev atarist.dsp /make "GEMUL8R - Win32 (80x86) Release"

:done
@endlocal
set CL=%CLT%
set LINK=%LINKT%
set REBUILD=

