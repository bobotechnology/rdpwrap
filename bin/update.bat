@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
"%~dp0RDPWInst.exe" -w
if errorlevel 1 goto :update_failed
echo.
exit /b 0
:update_failed
echo [-] Configuration update failed.
exit /b 1
:error
echo [-] Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
exit /b 2
