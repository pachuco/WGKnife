@echo off

set gccbase=G:\p_files\rtdk\i686-8.1.0-win32-dwarf-rt_v6-rev0\mingw32
::set gccbase=G:\p_files\rtdk\mingw32-gcc5
set PATH=%PATH%;%gccbase%\bin

set opts=-std=c99 -mconsole -Os -s -Wall -Wextra
set link=-lshlwapi
set bin=.
set includes=

set compiles=wgknife.c common.c
set outname=wgknife.exe
del %bin%\%outname%

pushd src
gcc -o ..\%bin%\%outname% %includes% %compiles% %opts% %link% 2> ..\compile.log
IF %ERRORLEVEL% NEQ 0 (
    echo oops %outname%!
    pause
)
popd

wgknife.exe -sd WINGROOV.TPD
wgknife.exe -sfz WINGROOV.TPD
wgknife.exe -d WINGROOV.TPD > A5.txt