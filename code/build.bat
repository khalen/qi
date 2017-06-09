@echo off

set SOURCE_DIR=w:\qi\code
set THIRDPARTY=w:\qi\thirdparty

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

set INCLUDE_DIRS=-I%THIRDPARTY%\OpenAL\include
set WARNINGS=-WX -W4 -wd4201 -wd4100 -wd4189
set DEBUG=-Z7
set ALLBUILDOPTS=-GR- -EHa -MT

set LINK_OPTS=/LIBPATH:%THIRDPARTY%\OpenAL\libs\Win64
set LIBS=user32.lib gdi32.lib winmm.lib OpenAL32.lib xaudio2.lib

cl -nologo %ALLBUILDOPS% %WARNINGS% %OPTIMIZATIONS% %DEBUG% %DEFINES% %INCLUDE_DIRS% %SOURCE_DIR%\win32_qi.cpp %LIBS% /link %LINK_OPTS%

popd
