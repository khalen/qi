@echo off

set SOURCE_DIR=w:\qi\code
set BUILD_DIR=w:\build
set DATA_DIR=w:\qi\data
set THIRDPARTY=w:\qi\thirdparty

pushd ..\..\build

rem Debug build
set DEFINES=-DQI_DEV_BUILD=1 -DQI_WIN32=1 -DQI_PROFILE_BUILD=1
set OPTIMIZATIONS=-Oi

rem Fast profile build
rem set DEFINES=-DQI_FASE_BUILD=1 -DQI_WIN32=1 -DQI_PROFILE_BUILD=1
rem set OPTIMIZATIONS=-O2

rem Release build
rem set DEFINES=-DQI_FASE_BUILD=1 -DQI_WIN32=1
rem set OPTIMIZATIONS=-O2

set INCLUDE_DIRS=
set WARNINGS=-WX -W4 -wd4201 -wd4100 -wd4189
set DEBUG=-Z7
set ALLBUILDOPTS=-GR- -EHa -MT

set LINK_OPTS=/incremental:no
set LIBS=user32.lib gdi32.lib winmm.lib xaudio2.lib

set TMPADD=%TIME::=_%
set TMPADD=%TMPADD:.=_%
set PDBNAME=qi_%TMPADD%.pdb

del qi*.pdb > NUL 2>&1
cl -nologo %ALLBUILDOPS% %WARNINGS% %OPTIMIZATIONS% %DEBUG% %DEFINES% %INCLUDE_DIRS% %SOURCE_DIR%\qi.cpp /LD /link /PDB:%PDBNAME% %LINK_OPTS%
cl -nologo %ALLBUILDOPS% %WARNINGS% %OPTIMIZATIONS% %DEBUG% %DEFINES% %INCLUDE_DIRS% %SOURCE_DIR%\win32_qi.cpp %LIBS% /link %LINK_OPTS%

popd

pushd %DATA_DIR%
copy %BUILD_DIR%\qi.dll %DATA_DIR%
copy %BUILD_DIR%\%PDBNAME% %DATA_DIR%
popd
