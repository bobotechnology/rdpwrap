$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$root = (Resolve-Path (Join-Path $PSScriptRoot "../..")).Path
Set-Location $root

function Invoke-CMake {
    & cmake @args
    if ($LASTEXITCODE -ne 0) {
        throw "cmake failed with exit code ${LASTEXITCODE}: $($args -join ' ')"
    }
}

function Assert-PeMachine([string]$Path, [uint16]$Expected) {
    $bytes = [System.IO.File]::ReadAllBytes((Join-Path $root $Path))
    if ($bytes.Length -lt 64) { throw "Invalid PE file: $Path" }
    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($peOffset -lt 0 -or $peOffset + 6 -gt $bytes.Length) {
        throw "Invalid PE header: $Path"
    }
    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    if ($machine -ne $Expected) {
        throw "Unexpected PE machine 0x$($machine.ToString('X4')) in $Path"
    }
}

Invoke-CMake -S . -B build-x64 -G "Visual Studio 17 2022" -A x64
Invoke-CMake --build build-x64 --config Release --target rdpwrap --parallel

Invoke-CMake -S . -B build-arm64 -G "Visual Studio 17 2022" -A ARM64
Invoke-CMake --build build-arm64 --config Release --target rdpwrap --parallel

Invoke-CMake -S . -B build-arm32 -G "Visual Studio 17 2022" `
    -A "ARM,version=10.0.19041.0" -T v142
Invoke-CMake --build build-arm32 --config Release --target rdpwrap --parallel

$rdpw64 = Join-Path $root "build-x64/bin/Release/rdpwrap.dll"
$rdpwArm = Join-Path $root "build-arm32/bin/Release/rdpwrap.dll"
$rdpwArm64 = Join-Path $root "build-arm64/bin/Release/rdpwrap.dll"
$rdpCnc = Join-Path $root "build-x86/bin/Release/RDP_CnC.exe"
foreach ($wrapper in $rdpw64, $rdpwArm, $rdpwArm64) {
    if (-not (Test-Path -LiteralPath $wrapper)) { throw "Missing wrapper: $wrapper" }
}
Invoke-CMake -S . -B build-x86 -G "Visual Studio 17 2022" -A Win32 `
    "-DINSTALLER_RDPW64=$rdpw64" `
    "-DINSTALLER_RDPWARM=$rdpwArm" `
    "-DINSTALLER_RDPWARM64=$rdpwArm64"
Invoke-CMake --build build-x86 --config Release `
    --target RDPWInst RDP_CnC --parallel

if (-not (Test-Path -LiteralPath $rdpCnc)) { throw "Missing RDP_CnC: $rdpCnc" }

$hostPatcher = Join-Path $root `
    "build-x86/src-installer/Release/installer_resource_patcher.exe"
foreach ($payload in $hostPatcher, $rdpwArm, $rdpwArm64) {
    if (-not (Test-Path -LiteralPath $payload)) {
        throw "Missing ARM32 Installer input: $payload"
    }
}
Invoke-CMake -S src-installer -B build-installer-arm32 `
    -G "Visual Studio 17 2022" -A "ARM,version=10.0.19041.0" -T v142 `
    "-DINSTALLER_RESOURCE_PATCHER=$hostPatcher" `
    "-DINSTALLER_RDPWARM=$rdpwArm" `
    "-DINSTALLER_RDPWARM64=$rdpwArm64"
Invoke-CMake --build build-installer-arm32 --config Release `
    --target RDPWInst --parallel

Assert-PeMachine "build-x86/bin/Release/rdpwrap.dll" 0x014c
Assert-PeMachine "build-x64/bin/Release/rdpwrap.dll" 0x8664
Assert-PeMachine "build-arm32/bin/Release/rdpwrap.dll" 0x01c4
Assert-PeMachine "build-arm64/bin/Release/rdpwrap.dll" 0xaa64
Assert-PeMachine "build-x86/bin/Release/RDPWInst.exe" 0x014c
Assert-PeMachine "build-installer-arm32/Release/RDPWInst.exe" 0x01c4

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -property installationPath
$dumpbin = Get-ChildItem "$vsPath\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe" |
    Sort-Object FullName -Descending | Select-Object -First 1
if (-not $dumpbin) { throw "dumpbin.exe was not found" }
$forbiddenImports = @(
    "CreateFile2",
    "GetDpiForWindow",
    "GetDpiForSystem",
    "GetSystemTimePreciseAsFileTime",
    "GetTempPath2W",
    "SetThreadDescription"
)
$releaseBinaries = @(
    "build-x86/bin/Release/RDPWInst.exe",
    "build-installer-arm32/Release/RDPWInst.exe",
    "build-x86/bin/Release/RDP_CnC.exe",
    "build-x86/bin/Release/rdpwrap.dll",
    "build-x64/bin/Release/rdpwrap.dll",
    "build-arm32/bin/Release/rdpwrap.dll",
    "build-arm64/bin/Release/rdpwrap.dll"
)
foreach ($binary in $releaseBinaries) {
    $imports = (& $dumpbin.FullName /nologo /imports $binary 2>&1 | Out-String)
    if ($LASTEXITCODE -ne 0) { throw "dumpbin failed for $binary" }
    foreach ($api in $forbiddenImports) {
        if ($imports -match "\b$([regex]::Escape($api))\b") {
            throw "Post-Vista static import $api found in $binary"
        }
    }
}

$dist = Join-Path $root "dist"
if (Test-Path -LiteralPath $dist) {
    $resolvedDist = (Resolve-Path -LiteralPath $dist).Path
    if (-not $resolvedDist.StartsWith($root, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to replace dist outside workspace: $resolvedDist"
    }
    Remove-Item -LiteralPath $resolvedDist -Recurse -Force
}
New-Item -ItemType Directory -Path $dist | Out-Null
Copy-Item "build-x86/bin/Release/RDPWInst.exe" $dist
Copy-Item "build-installer-arm32/Release/RDPWInst.exe" `
    (Join-Path $dist "RDPWInst-arm32.exe")
Copy-Item "build-x86/bin/Release/RDP_CnC.exe" $dist
Copy-Item `
    "bin/install.bat", "bin/update.bat", "bin/uninstall.bat", `
    "bin/install-arm32.bat", "bin/update-arm32.bat", `
    "bin/uninstall-arm32.bat" $dist
Get-ChildItem $dist | Format-Table Name, Length
