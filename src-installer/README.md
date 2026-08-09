# RDPWInst C++ port

This directory contains a C++17/Win32 port of the original Delphi installer.
The command-line interface is preserved. Repository builds replace the legacy
embedded INI and current-architecture binaries with the maintained/build
outputs.

## Build

Build with Visual Studio 2022 and MSVC:

```powershell
cmake -S . -B build -A Win32
cmake --build build --config Release
```

The executable is written to `build/Release/RDPWInst.exe`. The original Delphi
target was Win32, but both x86 and x64 C++ builds are supported. MSVC links the
legacy Delphi `resource.res` directly, then a small MSVC build tool replaces
the maintained payloads and embeds the `asInvoker` application manifest.

Every invocation relaunches itself through the `runas` verb when needed.
The relaunch uses `SEE_MASK_NO_CONSOLE`, but UAC broker behavior is not
consistent enough to rely on console inheritance alone. The unelevated process
therefore creates a named pipe, the elevated child writes its wide output to
that pipe, and the parent forwards it to the original CMD in real time.
`FreeConsole`/`AttachConsole` remains only as a fallback. `SW_HIDE` suppresses
any temporary console allocated by the broker. This code-based elevation is
intentional: a `requireAdministrator` manifest elevates before the program can
set up reliable output forwarding.

When built from the repository root, `CONFIG`, the native-architecture wrapper
DLL, and `RDP_CnC.exe` are embedded from the current source/build. A Win32
aggregate installer can receive the separately built x64 DLL through the
`INSTALLER_RDPW64` CMake cache variable.

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
