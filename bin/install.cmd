@echo off
setlocal
cd %~dp0

if defined PROCESSOR_ARCHITEW6432 (
	set ARCH=%PROCESSOR_ARCHITEW6432%
) else (
	set ARCH=%PROCESSOR_ARCHITECTURE%
)

if %ARCH%==x86   su32 cmd /C copy /B /-Y su32.exe "%WINDIR%\system32\su.exe" ^&^& pause
if %ARCH%==AMD64 su64 cmd /C copy /B /-Y su64.exe "%WINDIR%\system32\su.exe" ^&^& copy /B /-Y su32.exe "%WINDIR%\syswow64\su.exe" ^&^& pause
if %ARCH%==ARM64 su64a cmd /C copy /B /-Y su64a.exe "%WINDIR%\system32\su.exe" ^&^& copy /B /-Y su32.exe "%WINDIR%\syswow64\su.exe" ^&^& pause
