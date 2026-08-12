# RDP_CnC

Native Win32 C++17 configuration utility for RDP Wrapper and Windows Remote
Desktop. The application reads and writes Terminal Server settings, reports
Wrapper/TermService/listener status, checks the architecture-specific
`rdpwrap.ini` or `rdpwrap-arm-kb.ini` compatibility,
launches local `mstsc` tests, updates the INI, and restarts TermService.

This directory contains the C++17 Win32 configuration utility. The repository
root owns the CMake project.

## Build with Visual Studio

```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release
```

The executable requests administrator privileges because its settings are
stored below `HKEY_LOCAL_MACHINE` and service control also requires elevation.
The application uses the Service Control Manager and Windows Firewall COM APIs
directly. Its INI update action launches only the adjacent `RDPWInst.exe` with
the fixed `-w` argument; it does not provide a general-purpose hidden command
runner.

## Language

The interface automatically uses Simplified Chinese when the Windows display
language is Chinese, and English otherwise. The automatic choice can be
overridden when launching the program:

```powershell
RDP_CnC.exe /lang=zh-CN
RDP_CnC.exe /lang=en-US
```
