@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
rem Keep Terminal Services and RDP Wrapper firewall configuration so a remote session remains reachable after uninstall.
"%~dp0RDPWInst.exe" -u -k
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
