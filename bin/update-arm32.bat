@echo off
setlocal
if not exist "%~dp0RDPWInst-arm32.exe" goto :error
"%~dp0RDPWInst-arm32.exe" -w
if errorlevel 1 goto :update_failed
echo.
exit /b 0
:update_failed
echo [-] ARM32 configuration update failed.
exit /b 1
:error
echo [-] ARM32 Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
exit /b 2
