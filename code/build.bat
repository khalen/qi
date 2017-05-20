@echo off

set SOURCE_DIR=w:\qi\code

pushd ..\..\build

rem Debug build
set DEFINES=-DQI_DEV_BUILD=1 -DQI_WIN32=1 -DQI_PROFILE_BUILD=1
set OPTIMIZATIONS=-Oi

rem Fast profile build
rem set DEFINES=-DQI_FASE_BUILD=1 -DQI_WIN32=1 -DQI_PROFILE_BUILD=1
rem set OPTIMIZATIONS=-Oi

rem Release build
rem set DEFINES=-DQI_FASE_BUILD=1 -DQI_WIN32=1
rem set OPTIMIZATIONS=-Oi

set WARNINGS=-WX -W4 -wd4201 -wd4100 -wd4189
set DEBUG=-Z7
set ALLBUILDOPTS=-GR- -EHa -MT
set LIBS=user32.lib gdi32.lib winmm.lib

cl -nologo %ALLBUILDOPS% %WARNINGS% %OPTIMIZATIONS% %DEBUG% %DEFINES% %SOURCE_DIR%\win32_qi.cpp %LIBS%

popd
