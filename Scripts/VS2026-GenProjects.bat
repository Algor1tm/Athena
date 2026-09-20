@echo off
pushd %~dp0\..\
call cmake -B Build/Projects -G "Visual Studio 18 2026"
popd
PAUSE
