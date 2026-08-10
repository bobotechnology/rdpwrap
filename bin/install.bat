@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
"%~dp0RDPWInst.exe" -i -o
if errorlevel 1 goto :install_failed
if exist "%~dp0RDP_CnC.exe" start "" "%~dp0RDP_CnC.exe"
pause
exit /b 0

:install_failed
echo [-] RDP Wrapper installation failed.
pause
exit /b 1
:error
echo [-] Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 2
