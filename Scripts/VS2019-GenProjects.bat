@echo off
pushd %~dp0\..\
call ThirdParty\Premake\premake5.exe vs2019
popd

PAUSE
