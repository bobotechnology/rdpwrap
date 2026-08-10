# RDP Wrapper 二次开发增强版

[English](README.md) | 简体中文

[![Windows](https://img.shields.io/badge/platform-Windows-0078D6?logo=windows)](https://github.com/bobotechnology/rdpwrap)
[![Build](https://img.shields.io/badge/build-CMake%20%7C%20MSVC-success)](CMakeLists.txt)
[![Release](https://img.shields.io/github/v/release/bobotechnology/rdpwrap?include_prereleases)](https://github.com/bobotechnology/rdpwrap/releases)
[![License](https://img.shields.io/github/license/bobotechnology/rdpwrap)](LICENSE)

> 本仓库是由社区维护的二次开发增强版，不是原始 RDP Wrapper 项目的官方
> 发布，也不隶属于 Microsoft。

本版本保留 RDP Wrapper 的基本架构，并重新整理了安装器、配置程序、构建
系统、在线更新及运行时检查。Wrapper 负责加载 `termsrv.dll`，不会在磁盘上
直接替换或二进制修改 Microsoft 原始 DLL。

## 项目来源与致谢

本仓库基于原始项目
[stascorp/rdpwrap](https://github.com/stascorp/rdpwrap) 二次开发。感谢
Stas'M Corp.、binarymaster、kost、原项目贡献者及 RDP Wrapper 社区。
兼容性配置同步自
[sebaxakerhtc/rdpwrap.ini](https://github.com/sebaxakerhtc/rdpwrap.ini)。

**本版本**的开发、发布、问题反馈和支持均以当前仓库为准：

- 项目仓库：<https://github.com/bobotechnology/rdpwrap>
- 版本下载：<https://github.com/bobotechnology/rdpwrap/releases>
- 问题反馈：<https://github.com/bobotechnology/rdpwrap/issues>

本版本的安装器、构建系统或 `RDP_CnC` 问题请勿提交到原始项目。

## 本版本的主要改动

- 使用 C++17 重新实现原 Delphi `RDPWInst` 安装器。
- 使用仓库根目录 CMake 统一构建 `RDPWInst.exe`、`RDP_CnC.exe` 和
  `rdpwrap.dll`。
- x86/x64 及实验性的 ARM32/ARM64 Wrapper 构建统一使用 MSVC。
- Installer、Wrapper 与 CnC 统一使用 `dwProductVersionMS` 和
  `dwProductVersionLS` 匹配 `termsrv.dll` 的 INI 配置节。
- UAC 提权时通过命名管道把输出转发回原 CMD，不再依赖不稳定的控制台继承，
  也不会留下额外的常驻黑窗。
- 构建安装器时嵌入维护中的 `res/rdpwrap.ini` 和本次构建产物，避免继续携带
  陈旧资源。
- 在线更新增加 HTTPS、文件大小、结构、超时和原子替换检查。
- 提供原生 `RDP_CnC` 状态检查与配置程序。
- 使用项目专属防火墙规则，不再修改 Windows 通用的 `Remote Desktop` 规则。
- 加固 INI 自动同步工作流，并取消强制推送。

## 系统要求与支持范围

- 安装和修改系统配置需要管理员权限。
- 本二次开发版本主要面向并验证 Windows 10/11。
- 发布二进制文件保持 Windows Vista 加载兼容性。较新的可选 API 会动态解析，CI 会拒绝
  已知的 Vista 之后静态导入；Vista 属于兼容目标，并非持续执行测试的系统。
- 是否支持当前系统，取决于活动 `rdpwrap.ini` 中是否存在与
  `termsrv.dll` **产品版本**完全一致的配置节。
- 发布流水线会构建 x86、x64、ARM32 和 ARM64 Wrapper。ARM 运行时支持仍属于
  实验功能，并使用独立的 `res/rdpwrap-arm-kb.ini`。聚合 Installer 会嵌入并选择
  全部四种 Wrapper 架构。

如果此前使用其他工具修改过 `termsrv.dll`，请先恢复 Microsoft 原始文件。
修改远程访问配置前建议创建还原点或其他恢复方式；如果机器只能通过 RDP
访问，更应提前准备本地或带外恢复通道。

## 构建

请使用 Visual Studio 2022 和 MSVC，从仓库根目录构建：

```powershell
cmake -S . -B build-msvc -A x64
cmake --build build-msvc --config Release --parallel
```

ARM Wrapper 需要安装对应的 MSVC 交叉编译器，其中 ARM32 使用旧版 v142 工具集：

```powershell
cmake -S . -B build-arm64 -G "Visual Studio 17 2022" -A ARM64
cmake --build build-arm64 --config Release --target rdpwrap --parallel

cmake -S . -B build-arm32 -G "Visual Studio 17 2022" `
  -A "ARM,version=10.0.19041.0" -T v142
cmake --build build-arm32 --config Release --target rdpwrap --parallel
```

MSVC 会直接链接旧 Delphi `resource.res`。随后由一个 MSVC 构建的小工具在最终
安装器中替换当前维护的资源，不再需要 MinGW 或 `windres`。

产物位于构建目录的 `bin` 子目录。原生 x64 构建会把刚构建的 x64
`rdpwrap.dll`、`RDP_CnC.exe` 和 `res/rdpwrap.ini` 嵌入 `RDPWInst.exe`。

Win32 聚合 Installer 可通过原生或模拟层运行在支持的 Windows 架构上。请先单独构建
其他架构的 Wrapper，再配置聚合安装器：

```powershell
cmake -S . -B build-msvc32 -A Win32 `
  -DINSTALLER_RDPW64=C:/path/to/x64/rdpwrap.dll `
  -DINSTALLER_RDPWARM=C:/path/to/arm32/rdpwrap.dll `
  -DINSTALLER_RDPWARM64=C:/path/to/arm64/rdpwrap.dll
cmake --build build-msvc32 --config Release --parallel
```

`Build MSVC release` GitHub Actions 工作流会在每次 push 和 pull request 时构建全部四种
Wrapper 架构并上传产物。Win32 聚合 Installer 包含本次构建的 `RDPW32`、`RDPW64`、
`RDPWARM` 和 `RDPWARM64`。在 ARM 系统上会选择 `rdpwrap-arm-kb.ini`，并跳过仅适用于
x86/x64 的旧系统组件资源。工作流还会生成原生 ARM32 `RDPWInst-arm32.exe`；在 ARM64
Windows 上仍以兼容范围更广的 Win32 聚合 Installer 作为默认选择。

## 安装器命令

```text
RDPWInst.exe -l
RDPWInst.exe -i [-s] [-o]
RDPWInst.exe -u [-k]
RDPWInst.exe -w [HTTPS_URL]
RDPWInst.exe -r
```

| 参数 | 说明 |
| --- | --- |
| `-l` | 显示内置许可证。 |
| `-i` | 使用通过校验的同目录或内置 INI 安装。 |
| `-i -o` | 安装前下载并校验当前在线 INI。 |
| `-i -s` | 把 Wrapper DLL 安装到 System32；普通部署不建议使用。 |
| `-u` | 卸载 Wrapper 及项目管理的文件。 |
| `-u -k` | 卸载但保留终端服务和防火墙配置。 |
| `-w` | 从默认 HTTPS 地址更新已安装的 INI。 |
| `-w URL` | 从指定 HTTPS 地址更新 INI。 |
| `-r` | 重启终端服务。 |

所有命令使用同一套代码提权流程。可以直接在普通 CMD 中运行；同意 UAC
提示后，输出会继续显示在原 CMD 中。

## 快速使用

发布包通常包含：

| 文件 | 用途 |
| --- | --- |
| `RDPWInst.exe` | 安装、卸载、更新和服务控制工具。 |
| `RDP_CnC.exe` | Wrapper 状态及 RDP 配置程序。 |
| `install.bat` | 在线安装快捷脚本。 |
| `update.bat` | INI 更新快捷脚本。 |
| `uninstall.bat` | 安全卸载快捷脚本。 |

在解压后的发布目录中运行：

```bat
install.bat
update.bat
uninstall.bat
```

脚本会正确返回失败状态。卸载逻辑只删除项目已知文件；如果安装目录中存在
未知文件，会保留该目录，不再递归删除用户文件。

## 更新与问题诊断

- 默认在线模式会下载兼容性 INI，完成结构校验后再原子替换。
- `RDP_CnC.exe` 会显示 Wrapper、TermService、监听器、产品版本和支持状态。
- 如果显示 `not supported`，请先运行 `update.bat`。如果仍然没有对应的产品
  版本，请在[当前仓库 Issues](https://github.com/bobotechnology/rdpwrap/issues)
  反馈，并提供完整 `termsrv.dll` 产品版本、Windows 架构和相关安装器输出。

## 重要说明

- 本软件会修改 Windows 服务、注册表和防火墙配置。
- 杀毒软件可能会对服务 Wrapper 或内存 Hook 行为报警。如果对供应链有要求，
  建议自行审查并构建源代码。
- Windows 更新可能先发布新的 `termsrv.dll`，而对应 INI 配置节尚未更新。
- 并发会话功能可能受到 Windows 授权条款或当地法律限制，使用者应自行确认
  已获得授权并符合相关要求。
- 本项目不提供任何担保，详情见 [LICENSE](LICENSE)。
