@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
if "%RDPW_BATCH_ELEVATED%"=="1" goto :elevated
"%~dp0RDPWInst.exe" --rdpw-run-batch "%~f0"
exit /b %errorlevel%

:elevated
set "RDPW_BATCH_ELEVATED="
"%~dp0RDPWInst.exe" -u
if errorlevel 1 goto :uninstall_failed
SCHTASKS /DELETE /TN "RDPWUpdater" /F >nul 2>&1
del /Q "%ProgramFiles%\RDP Wrapper\RDPWInst.exe" 2>nul
rmdir "%ProgramFiles%\RDP Wrapper" 2>nul
if exist "%ProgramFiles%\RDP Wrapper\" echo [!] Installation directory contains unrecognized files and was retained.
echo.
pause
exit /b 0
:uninstall_failed
echo [-] Uninstall failed; updater task and installation directory were left intact.
pause
exit /b 1
:error
echo [-] Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 2
