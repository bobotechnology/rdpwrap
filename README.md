# RDP Wrapper Enhanced Edition

English | [简体中文](README_CN.md)

[![Windows](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows)](https://github.com/bobotechnology/rdpwrap)
[![Build](https://img.shields.io/badge/build-CMake%20%7C%20MSVC-success)](CMakeLists.txt)
[![Release](https://img.shields.io/github/v/release/bobotechnology/rdpwrap?include_prereleases)](https://github.com/bobotechnology/rdpwrap/releases)
[![License](https://img.shields.io/github/license/bobotechnology/rdpwrap)](LICENSE)

> This is a community-maintained, secondary-development edition. It is not an
> official release of the original RDP Wrapper project and is not affiliated
> with Microsoft.

This edition keeps the RDP Wrapper architecture while modernizing its installer,
configuration application, build system, update process, and runtime checks.
`termsrv.dll` is loaded through the wrapper and is not directly replaced or
binary-patched on disk.

## Project lineage and credits

This repository is derived from the original
[stascorp/rdpwrap](https://github.com/stascorp/rdpwrap) project. Thanks to
Stas'M Corp., binarymaster, kost, the original contributors, and the wider RDP
Wrapper community. The synchronized compatibility configuration is based on
[sebaxakerhtc/rdpwrap.ini](https://github.com/sebaxakerhtc/rdpwrap.ini).

Development, releases, issue tracking, and support for **this edition** belong
to this repository:

- Repository: <https://github.com/bobotechnology/rdpwrap>
- Releases: <https://github.com/bobotechnology/rdpwrap/releases>
- Issues: <https://github.com/bobotechnology/rdpwrap/issues>

Please do not report edition-specific installer, build, or `RDP_CnC` problems
to the original project.

## Changes in this edition

- Reimplemented the Delphi `RDPWInst` installer in C++17.
- Added a unified repository-level CMake build for `RDPWInst.exe`,
  `RDP_CnC.exe`, and `rdpwrap.dll`.
- Uses MSVC consistently for the maintained x86/x64 build path.
- Uses `dwProductVersionMS` and `dwProductVersionLS` consistently when matching
  `termsrv.dll` against INI sections.
- Preserves command output in the original CMD during UAC elevation by relaying
  output over a named pipe; no extra persistent console window is required.
- Embeds the maintained `res/rdpwrap.ini` and current build outputs into the
  installer instead of silently shipping stale resources.
- Adds validated HTTPS INI updates, size/time limits, atomic replacement, and
  more reliable service/process error handling.
- Provides the native `RDP_CnC` status and configuration application.
- Uses installer-owned firewall rules instead of modifying Windows' generic
  `Remote Desktop` rules.
- Hardens the scheduled INI synchronization workflow and removes forced pushes.

## Requirements and scope

- Administrator permission is required for installation and system changes.
- Windows 10/11 are the primary supported and tested targets for this edition.
- Actual `termsrv.dll` support depends on whether the active `rdpwrap.ini`
  contains the exact Windows product-version section.
- The maintained root CMake build currently targets x86/x64. ARM/ARM64 files in
  the repository are legacy/experimental paths and are not part of the verified
  release pipeline.

Before installation, restore an original Microsoft `termsrv.dll` if another
patcher modified it. Create a restore point or other recovery path before
changing remote-access configuration, especially on a machine that can only be
reached through RDP.

## Build

Build from the repository root with Visual Studio 2022 and MSVC:

```powershell
cmake -S . -B build-msvc -A x64
cmake --build build-msvc --config Release --parallel
```

The legacy Delphi `resource.res` is linked directly by MSVC. A small build tool
then replaces the maintained payloads in the resulting installer; MinGW and
`windres` are not required.

Outputs are written below the build directory's `bin` folder. A native x64
build embeds its freshly built x64 `rdpwrap.dll`, `RDP_CnC.exe`, and
`res/rdpwrap.ini` into `RDPWInst.exe`.

A Win32 installer can run on both x86 and x64 Windows and therefore needs both
wrapper architectures. Build the x64 DLL separately and provide it when
configuring the Win32 aggregate installer:

```powershell
cmake -S . -B build-msvc32 -A Win32 `
  -DINSTALLER_RDPW64=C:/path/to/x64/rdpwrap.dll
cmake --build build-msvc32 --config Release --parallel
```

The `Build MSVC release` GitHub Actions workflow performs both builds and
uploads a ready-to-use artifact on every push and pull request. Its Win32
installer contains the freshly built `RDPW32` and `RDPW64` payloads.

## Installer commands

```text
RDPWInst.exe -l
RDPWInst.exe -i [-s] [-o]
RDPWInst.exe -u [-k]
RDPWInst.exe -w [HTTPS_URL]
RDPWInst.exe -r
```

| Option | Description |
| --- | --- |
| `-l` | Display the bundled license. |
| `-i` | Install using a validated adjacent or embedded INI. |
| `-i -o` | Download and validate the current online INI before installation. |
| `-i -s` | Install the wrapper DLL into System32; not recommended for normal deployments. |
| `-u` | Uninstall the wrapper and its managed files. |
| `-u -k` | Uninstall while retaining Terminal Services/firewall configuration. |
| `-w` | Update the installed INI from the default HTTPS source. |
| `-w URL` | Update from a specified HTTPS source. |
| `-r` | Restart Terminal Services. |

Every command uses the same code-controlled UAC path. You can launch it from an
ordinary CMD; accept the UAC prompt and output will continue in that CMD.

## Quick use

Release packages normally contain:

| File | Purpose |
| --- | --- |
| `RDPWInst.exe` | Installer, uninstaller, updater, and service control tool. |
| `RDP_CnC.exe` | Wrapper status and RDP configuration application. |
| `install.bat` | Online installation shortcut. |
| `update.bat` | INI update shortcut. |
| `uninstall.bat` | Safe uninstall shortcut. |

From the extracted release directory:

```bat
install.bat
update.bat
uninstall.bat
```

The scripts propagate failures through their exit codes. The uninstaller only
deletes known project files and keeps a non-empty installation directory rather
than recursively deleting unknown user files.

## Updates and diagnostics

- The default online mode retrieves a compatibility INI, validates its
  structure, and writes it atomically.
- `RDP_CnC.exe` displays the installed wrapper state, TermService state,
  listener state, product version, and support status.
- If the status is `not supported`, run `update.bat` first. If the exact product
  version is still absent, open an issue in
  [this repository](https://github.com/bobotechnology/rdpwrap/issues) and include
  the complete `termsrv.dll` product version, Windows architecture, and relevant
  installer output.

## Important notes

- This software changes Windows service, registry, and firewall configuration.
- Antivirus products may flag service wrappers or in-memory hooking behavior.
  Review and build the source yourself if supply-chain assurance is required.
- Windows updates can introduce a new `termsrv.dll` product version before a
  matching INI section is available.
- Enabling concurrent sessions may be restricted by Windows licensing terms or
  local law. Users are responsible for verifying their authorization and
  compliance.
- No warranty is provided. See [LICENSE](LICENSE).
