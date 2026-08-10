@echo off
setlocal
if not exist "%~dp0RDPWInst-arm32.exe" goto :error
"%~dp0RDPWInst-arm32.exe" -u
if errorlevel 1 goto :uninstall_failed
SCHTASKS /DELETE /TN "RDPWUpdater" /F >nul 2>&1
del /Q "%ProgramFiles%\RDP Wrapper\RDPWInst.exe" 2>nul
rmdir "%ProgramFiles%\RDP Wrapper" 2>nul
if exist "%ProgramFiles%\RDP Wrapper\" echo [!] Installation directory contains unrecognized files and was retained.
echo.
exit /b 0
:uninstall_failed
echo [-] ARM32 uninstall failed; updater task and installation directory were left intact.
exit /b 1
:error
echo [-] ARM32 Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
exit /b 2
