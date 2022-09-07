@echo off
pushd %~dp0\..\
call ThirdParty\Premake\premake5.exe gmake2
popd

PAUSE