@echo off
rem
rem MAGIC 2.0 install for Gemulator
rem
rem Copies the minimum set of MagiC 2.0 system files to C:
rem
cls
echo MAGIC 2.0 INSTALL FOR GEMULATOR
:disk1
echo Insert your MagiC 2.0 master diskette into drive A:
pause
if not exist a:\instmagx.prg goto disk1
if not exist a:\magx\magic.ram goto disk1
if not exist a:\magx\magxdesk.app goto disk1
mkdir c:\GEMSYS
mkdir c:\GEMSYS\GEMDESK
copy magx2.inf c:\magx.inf
echo Copying MAGIC.RAM...
copy a:\magx\magic.ram c:\
echo Copying GEMDESK folder
copy a:\magx\magxdes*.* c:\gemsys\gemdesk
echo Done! MagiC 2.0 is installed on drive C:

