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

- Reimplemented the legacy `RDPWInst` installer in C++17.
- Added a unified repository-level CMake build for `RDPWInst.exe`,
  `RDP_CnC.exe`, and `rdpwrap.dll`.
- Uses MSVC consistently for x86/x64 and experimental ARM32/ARM64 wrapper builds.
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
- Release binaries remain load-compatible with Windows Vista. Newer optional
  APIs are resolved dynamically, and CI rejects known post-Vista static imports;
  Vista is a compatibility target rather than a continuously executed test OS.
- Actual `termsrv.dll` support depends on whether the active architecture's
  `rdpwrap.ini` or `rdpwrap-arm-kb.ini` contains the exact Windows
  product-version section.
- The release pipeline builds x86, x64, ARM32, and ARM64 wrappers. ARM runtime
  support remains experimental and uses the separate `res/rdpwrap-arm-kb.ini`.
  The aggregate Installer embeds and selects all four wrapper architectures.

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

The ARM wrappers require the corresponding MSVC cross-compilers. ARM32 uses
the legacy v142 toolset:

```powershell
cmake -S . -B build-arm64 -G "Visual Studio 17 2022" -A ARM64
cmake --build build-arm64 --config Release --target rdpwrap --parallel

cmake -S . -B build-arm32 -G "Visual Studio 17 2022" `
  -A "ARM,version=10.0.19041.0" -T v142
cmake --build build-arm32 --config Release --target rdpwrap --parallel
```

MSVC compiles `src-installer/installer.rc` from the preserved system payload
files. A small MSVC build tool then adds maintained configuration, license, and
current build outputs to the final Installer; MinGW and `windres` are not
required.

Outputs are written below the build directory's `bin` folder. A native x64
build embeds its freshly built x64 `rdpwrap.dll` and `res/rdpwrap.ini` into
`RDPWInst.exe`. Release archives carry `RDP_CnC.exe` as a separate application
instead of duplicating it inside the Installer.

A Win32 aggregate Installer can run under the native or emulation layer on the
supported Windows architectures. Build the other wrappers separately and
provide them when configuring it:

```powershell
cmake -S . -B build-msvc32 -A Win32 `
  -DINSTALLER_RDPW64=C:/path/to/x64/rdpwrap.dll `
  -DINSTALLER_RDPWARM=C:/path/to/arm32/rdpwrap.dll `
  -DINSTALLER_RDPWARM64=C:/path/to/arm64/rdpwrap.dll
cmake --build build-msvc32 --config Release --parallel
```

The `Build MSVC release` GitHub Actions workflow builds all four wrapper
architectures and uploads them on every push and pull request. Its Win32
aggregate Installer contains the freshly built `RDPW32`, `RDPW64`, `RDPWARM`,
and `RDPWARM64` payloads. On ARM systems it selects `rdpwrap-arm-kb.ini` and
does not extract the x86/x64-only legacy system-component resources. The
workflow also produces a native ARM32 `RDPWInst-arm32.exe`; the Win32 aggregate
Installer remains the broadly compatible default on ARM64 Windows.

The manually triggered `Publish release` workflow supports `stable`, `alpha`,
`beta`, and `rc` channels. Starting from a stable release, select `patch`,
`minor`, or `major` and a prerelease channel to publish tags such as
`v1.8.8-beta.1`; rerunning the workflow with that prerelease checked out
continues the same line as `beta.2`, `beta.3`, and so on. Select `stable` to
promote the current prerelease line to its matching final tag (for example,
`v1.8.8-beta.2` to `v1.8.8`) without skipping to `v1.8.9`. Every release
commits the full version to `VERSION`, builds and verifies all architectures,
creates a ZIP plus SHA-256 file, and publishes generated GitHub Release notes.
Prereleases are not marked as Latest.

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
| `-u` | Uninstall the wrapper and managed files, disable Remote Desktop, and remove installer-owned firewall rules. |
| `-u -k` | Uninstall the wrapper and managed files while preserving the current Remote Desktop setting and installer-owned firewall rules. |
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
| `RDPWInst-arm32.exe` | Native ARM32 build of the Installer. |
| `RDP_CnC.exe` | Wrapper status and RDP configuration application. |
| `install.bat` | Online installation shortcut. |
| `update.bat` | INI update shortcut. |
| `uninstall.bat` | Safe uninstall shortcut (`RDPWInst.exe -u -k`). |
| `install-arm32.bat` | Native ARM32 installation shortcut. |
| `update-arm32.bat` | Native ARM32 update shortcut. |
| `uninstall-arm32.bat` | Native ARM32 safe uninstall shortcut (`RDPWInst-arm32.exe -u -k`). |

From the extracted release directory:

```bat
install.bat
update.bat
uninstall.bat

rem Native ARM32 alternatives:
install-arm32.bat
update-arm32.bat
uninstall-arm32.bat
```

The scripts propagate failures through their exit codes. Both uninstall scripts
intentionally use `-u -k`: they restore the system `termsrv.dll`, restart
Terminal Services, remove managed files and the updater, but do not change
`fDenyTSConnections` or remove the installer-owned firewall rules. Because
installation enables Remote Desktop and creates those rules, this keeps a remote
host reachable after uninstall. Run `RDPWInst.exe -u` directly only when Remote
Desktop should also be disabled and those firewall rules removed. The uninstaller
only deletes known project files and keeps a non-empty installation directory
rather than recursively deleting unknown user files.

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
