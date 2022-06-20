@echo off
pushd ..\
call vendor\premake\premake5.exe vs2022
popd

PAUSE
