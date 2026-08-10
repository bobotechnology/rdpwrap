@echo off
setlocal
if not exist "%~dp0RDPWInst-arm32.exe" goto :error
"%~dp0RDPWInst-arm32.exe" -i -o
if errorlevel 1 goto :install_failed
if exist "%~dp0RDP_CnC.exe" start "" "%~dp0RDP_CnC.exe"
pause
exit /b 0

:install_failed
echo [-] RDP Wrapper ARM32 installation failed.
pause
exit /b 1
:error
echo [-] ARM32 Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 2
