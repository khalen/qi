@echo off

set SOURCE_DIR=w:\qi\code

pushd ..\..\build

cl /Zi %SOURCE_DIR%\win32_qi.cpp user32.lib gdi32.lib

popd
