@echo off

pushd .\build
cl /Zi .\..\win32_handmade.cpp user32.lib Gdi32.lib
popd
