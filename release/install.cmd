@echo off
setlocal
cd %~dp0

reg Query "HKLM\Hardware\Description\System\CentralProcessor\0" | find /i "x86" > NUL && set OSTYPE=32BIT || set OSTYPE=64BIT
if %OSTYPE%==32BIT su32 cmd /C copy /B /-Y su32.exe "%WINDIR%\system32\su.exe" ^&^& pause
if %OSTYPE%==64BIT su64 cmd /C copy /B /-Y su64.exe "%WINDIR%\system32\su.exe" ^&^& copy /B /-Y su32.exe "%WINDIR%\syswow64\su.exe" ^&^& pause
