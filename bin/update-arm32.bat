@echo off
setlocal
if not exist "%~dp0RDPWInst-arm32.exe" goto :error
if "%RDPW_BATCH_ELEVATED%"=="1" goto :elevated
"%~dp0RDPWInst-arm32.exe" --rdpw-run-batch "%~f0"
exit /b %errorlevel%

:elevated
set "RDPW_BATCH_ELEVATED="
"%~dp0RDPWInst-arm32.exe" -w
if errorlevel 1 goto :update_failed
echo.
pause
exit /b 0
:update_failed
echo [-] ARM32 configuration update failed.
pause
exit /b 1
:error
echo [-] ARM32 Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 2
