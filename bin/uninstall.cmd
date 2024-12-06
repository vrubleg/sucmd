@echo off
setlocal
cd %~dp0

if defined PROCESSOR_ARCHITEW6432 (
	set ARCH=%PROCESSOR_ARCHITEW6432%
) else (
	set ARCH=%PROCESSOR_ARCHITECTURE%
)

if %ARCH%==x86   su32 cmd /C del /P "%WINDIR%\system32\su.exe" ^&^& pause
if %ARCH%==AMD64 su64 cmd /C del /P "%WINDIR%\system32\su.exe" ^&^& del /P "%WINDIR%\syswow64\su.exe" ^&^& pause
if %ARCH%==ARM64 su64a cmd /C del /P "%WINDIR%\system32\su.exe" ^&^& del /P "%WINDIR%\syswow64\su.exe" ^&^& pause
