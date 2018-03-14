@echo off
rem
rem MAGIC 4.0 install for Gemulator
rem
rem Copies the minimum set of MagiC 4.0 system files to C:
rem
cls
echo MAGIC 4.0 INSTALL FOR GEMULATOR
:disk1
echo Insert your MagiC 4.0 master diskette into drive A:
pause
if not exist a:\instmagc.prg goto disk1
if not exist a:\magx_1\gemsys\off002.osd goto disk1
if not exist a:\magx_1\gemsys\gemdesk\magxdesk.app goto disk1
mkdir c:\GEMSYS
mkdir c:\GEMSYS\GEMDESK
copy magx4.inf c:\magx.inf
echo Copying GEMSYS folder...
copy a:\magx_1\gemsys\*.* c:\gemsys
echo Copying GEMSYS\GEMDESK folder...
copy a:\magx_1\gemsys\gemdesk\*.* c:\gemsys\gemdesk
:disk2
echo Insert your MagiC 4.0 Disk 2 diskette into drive A:
pause
if not exist a:\magx_2\magic.ram goto disk2
echo Copying MAGIC.RAM...
copy a:\magx_2\magic.ram c:\
copy a:\magx_2\magxconf.acc c:\
copy a:\magx_2\magxdesk.rsc c:\gemsys\gemdesk
echo Done! MagiC 4.0 is installed on drive C:

