@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
if "%RDPW_BATCH_ELEVATED%"=="1" goto :elevated
"%~dp0RDPWInst.exe" --rdpw-run-batch "%~f0"
exit /b %errorlevel%

:elevated
set "RDPW_BATCH_ELEVATED="
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
