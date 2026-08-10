@echo off
setlocal
if not exist "%~dp0RDPWInst.exe" goto :error
if "%RDPW_BATCH_ELEVATED%"=="1" goto :elevated
"%~dp0RDPWInst.exe" --rdpw-run-batch "%~f0"
set "RDPW_RESULT=%errorlevel%"
if "%RDPW_RESULT%"=="0" if exist "%~dp0RDP_CnC.exe" start "" "%~dp0RDP_CnC.exe"
exit /b %RDPW_RESULT%

:elevated
set "RDPW_BATCH_ELEVATED="
"%~dp0RDPWInst.exe" -i -o
if errorlevel 1 goto :install_failed

copy /y "%~dp0RDPWInst.exe" "%ProgramFiles%\RDP Wrapper\RDPWInst.exe" >nul
if errorlevel 1 goto :copy_failed

SCHTASKS /CREATE /F /SC ONSTART /DELAY 0002:00 /TN "RDPWUpdater" /TR "\"%ProgramFiles%\RDP Wrapper\RDPWInst.exe\" -w" /RL HIGHEST /RU SYSTEM /NP
if errorlevel 1 goto :task_failed

pause
exit /b 0

:install_failed
echo [-] RDP Wrapper installation failed.
pause
exit /b 1
:copy_failed
echo [-] Installed, but failed to deploy the updater executable.
pause
exit /b 2
:task_failed
echo [-] Installed, but failed to create the RDPWUpdater scheduled task.
pause
exit /b 3
:error
echo [-] Installer executable not found.
echo Please extract all files from the downloaded package or check your anti-virus.
pause
exit /b 4
