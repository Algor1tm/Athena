@echo off
pushd %~dp0\..\
call make -j config=release
popd

PAUSE