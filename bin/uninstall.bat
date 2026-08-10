@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
"%~dp0RDPWInst.exe" -u
if errorlevel 1 goto :uninstall_failed
echo.
pause
exit /b 0
:uninstall_failed
echo [-] Uninstall failed.
pause
exit /b 1
:error
echo [-] Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 2
