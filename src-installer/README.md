# RDPWInst C++ port

This directory contains the maintained C++17/Win32 Installer.
The command-line interface is preserved. Repository builds replace the legacy
embedded INI and current-architecture binaries with the maintained/build
outputs.

## Build

Build with Visual Studio 2022 and MSVC:

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release
```

The executable is written to `build/Release/RDPWInst.exe`. MSVC compiles
`installer.rc` from the preserved system payload files, then a small MSVC
build tool adds maintained resources and embeds the `asInvoker` application
manifest.

Every invocation relaunches itself through the `runas` verb when needed.
The relaunch uses `SEE_MASK_NO_CONSOLE`, but UAC broker behavior is not
consistent enough to rely on console inheritance alone. The unelevated process
therefore creates a named pipe, the elevated child writes its wide output to
that pipe, and the parent forwards it to the original CMD in real time.
`FreeConsole`/`AttachConsole` remains only as a fallback. `SW_HIDE` suppresses
any temporary console allocated by the broker. This code-based elevation is
intentional: a `requireAdministrator` manifest elevates before the program can
set up reliable output forwarding.

Repository builds refresh `CONFIG`, `CONFIG_ARM`, and the current-architecture
Wrapper. Release archives provide `RDP_CnC.exe` separately. A Win32 aggregate
Installer can receive separately built wrappers through `INSTALLER_RDPW64`,
`INSTALLER_RDPWARM`, and `INSTALLER_RDPWARM64`. At runtime it selects `RDPW32`,
`RDPW64`, `RDPWARM`, or `RDPWARM64` from the native processor architecture.
ARM uses `rdpwrap-arm-kb.ini` and never extracts the legacy x86/x64 system
components.

## Commands

```text
RDPWInst.exe [-l|-i[-s][-o]|-w[url]|-u[-k]|-r]
```

See `RDPWInst.exe` without arguments for the full usage text.

## Reliability notes

- Command options are validated before any system changes are made.
- Downloaded and adjacent INI files are size-limited and structurally
  validated; downloads use bounded network timeouts.
- INI replacement is atomic, so a failed write does not truncate the active
  configuration.
- Service start/stop operations have timeouts and verify the resulting state.
- A failed installation restores the original `ServiceDll` and attempts to
  restart Terminal Services.
- WOW64 file-system redirection is restored automatically during error exits.
- Firewall rules use the installer-owned names `RDP Wrapper TCP 3389` and
  `RDP Wrapper UDP 3389`, avoiding modification of Windows' generic
  `Remote Desktop` rules.
