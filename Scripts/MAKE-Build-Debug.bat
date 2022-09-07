@echo off
pushd %~dp0\..\
call make -j config=debug
popd

PAUSE