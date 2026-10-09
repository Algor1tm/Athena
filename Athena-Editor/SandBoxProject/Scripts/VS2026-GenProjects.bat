@echo off
call cmake -B Build/Projects -G "Visual Studio 18 2026"
popd
exit 0
