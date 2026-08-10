@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
"%~dp0RDPWInst.exe" -w
if errorlevel 1 goto :update_failed
echo.
pause
exit /b 0
:update_failed
echo [-] Configuration update failed.
pause
exit /b 1
:error
echo [-] Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 2
