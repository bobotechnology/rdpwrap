# Third Party RDP Wrapper Library by bobo

[English](https://github.com/bobotechnology/rdpwrap/blob/master/README.md) | 简体中文

[![Telegram](https://img.shields.io/badge/chat-Telegram-blue.svg)](https://t.me/rdpwrap)
![Environment](https://img.shields.io/badge/Windows-Vista,%207,%208,%2010,%2011-brightgreen.svg)
[![Release](https://img.shields.io/github/release/stascorp/rdpwrap.svg)](https://github.com/stascorp/rdpwrap/releases)
![License](https://img.shields.io/github/license/stascorp/rdpwrap.svg)
![Downloads](https://img.shields.io/github/downloads/stascorp/rdpwrap/latest/total.svg)
![TotalDownloads](https://img.shields.io/github/downloads/stascorp/rdpwrap/total.svg)

#### 所有感谢都归功于 RDP Wrapper 库的：[stascorp](https://github.com/stascorp/rdpwrap)

这个项目的目标是为家庭用户在功能受限的系统上启用远程桌面主机支持和并发 RDP 会话。

RDP Wrapper 作为服务控制管理器和终端服务之间的一层，因此原始的 termsrv.dll 文件保持不变。此外，这种方法，即使Windows更新也不受到影响。

[pVistaST]:  http://stascorp.com/images/rdpwrap/pVistaST.jpg
[pVistaHB]:  http://stascorp.com/images/rdpwrap/pVistaHB.jpg
[pWin7ST]:   http://stascorp.com/images/rdpwrap/pWin7ST.jpg
[pWin7HB]:   http://stascorp.com/images/rdpwrap/pWin7HB.jpg
[pWin8DP]:   http://stascorp.com/images/rdpwrap/pWin8DP.jpg
[pWin8CP]:   http://stascorp.com/images/rdpwrap/pWin8CP.jpg
[pWin8RP]:   http://stascorp.com/images/rdpwrap/pWin8RP.jpg
[pWin8]:     http://stascorp.com/images/rdpwrap/pWin8.jpg
[pWin81P]:   http://stascorp.com/images/rdpwrap/pWin81P.jpg
[pWin81]:    http://stascorp.com/images/rdpwrap/pWin81.jpg
[pWin10TP]:  http://stascorp.com/images/rdpwrap/pWin10TP.jpg
[pWin10PTP]: http://stascorp.com/images/rdpwrap/pWin10PTP.jpg
[pWin10]:    http://stascorp.com/images/rdpwrap/pWin10.jpg

[fVistaST]:  http://stascorp.com/images/rdpwrap/VistaST.png
[fVistaHB]:  http://stascorp.com/images/rdpwrap/VistaHB.png
[fWin7ST]:   http://stascorp.com/images/rdpwrap/Win7ST.png
[fWin7HB]:   http://stascorp.com/images/rdpwrap/Win7HB.png
[fWin8DP]:   http://stascorp.com/images/rdpwrap/Win8DP.png
[fWin8CP]:   http://stascorp.com/images/rdpwrap/Win8CP.png
[fWin8RP]:   http://stascorp.com/images/rdpwrap/Win8RP.png
[fWin8]:     http://stascorp.com/images/rdpwrap/Win8.png
[fWin81P]:   http://stascorp.com/images/rdpwrap/Win81P.png
[fWin81]:    http://stascorp.com/images/rdpwrap/Win81.png
[fWin10TP]:  http://stascorp.com/images/rdpwrap/Win10TP.png
[fWin10PTP]: http://stascorp.com/images/rdpwrap/Win10PTP.png
[fWin10]:    http://stascorp.com/images/rdpwrap/Win10.png

| NT 版本    | 截图 |
| ------------- | ----------- |
| Windows Vista | [![Windows Vista Starter][pVistaST]][fVistaST] [![Windows Vista Home Basic][pVistaHB]][fVistaHB] |
| Windows 7     | [![Windows 7 Starter][pWin7ST]][fWin7ST] [![Windows 7 Home Basic][pWin7HB]][fWin7HB] |
| Windows 8     | [![Windows 8 Developer Preview][pWin8DP]][fWin8DP] [![Windows 8 Consumer Preview][pWin8CP]][fWin8CP] [![Windows 8 Release Preview][pWin8RP]][fWin8RP] [![Windows 8][pWin8]][fWin8] |
| Windows 8.1   | [![Windows 8.1 Preview][pWin81P]][fWin81P] [![Windows 8.1][pWin81]][fWin81] |
| Windows 10    | [![Windows 10 Technical Preview][pWin10TP]][fWin10TP] [![Windows 10 Pro Technical Preview][pWin10PTP]][fWin10PTP] [![Windows 10][pWin10]][fWin10] |
---
[WinPPE]: http://forums.mydigitallife.info/threads/39411-Windows-Product-Policy-Editor

这个解决方案的灵感来源于 [Windows 产品策略编辑器][WinPPE]，非常感谢 **kost** :)

— binarymaster

### 注意：
建议在安装 RDP Wrapper 时保留原始的 termsrv.dll 文件。如果您之前使用其他补丁程序修改过它，它可能会变得不稳定并随时崩溃。

### 信息：
- 源代码是可用的，因此您可以自行构建
- RDP Wrapper 不会直接修改 termsrv.dll，而是使用不同的参数来加载它，以确保原始文件保持不变。
- RDPWInst 和 RDPChecker 可以在没有开发文件夹和批处理文件的情况下重新分发
- RDPWInst 可用于无人值守安装/部署
- 不支持 Windows 2000、XP 和 Server 2003

### 主要特性：
- 在从 Vista 开始的任何 Windows 版本上作为 RDP 主机服务器
- 控制台会话和 RDP 会话可以同时进行
- 同时使用同一用户进行本地和远程登录（请参见配置应用程序）
- 支持最多 [15 个并发会话](https://github.com/stascorp/rdpwrap/issues/192)（实际限制取决于您的硬件和操作系统版本）
- 控制台和 RDP 会话 影子模式（使用 [Windows 7 中的任务管理器](https://img.picui.cn/free/2025/01/22/679110d27de7b.png) 及更低版本，以及 [Windows 8 中的远程桌面连接](http://woshub.com/rds-shadow-how-to-connect-to-a-user-session-in-windows-server-2012-r2/) 及更高版本）
- 完整的 [多显示器支持](https://github.com/stascorp/rdpwrap/issues/163) 适用于 RDP 主机
- ...如果您发现这里未列出的新功能，请 [告诉我们](https://github.com/stascorp/rdpwrap/issues/new) ;)


### 移植到其他平台：
- **ARM** 用于 Windows RT（请参见下面的链接）
- **IA-64** 用于基于 Itanium 的 Windows Server？*嗯，我不知道* :)

### 构建二进制文件：
- **x86 Delphi 版本** 可以使用 *Embarcadero RAD Studio 2010* 构建
- **x86/x64 C++ 版本** 可以使用 *Microsoft Visual Studio 2022* 构建

[andrewblock]:   http://web.archive.org/web/20150810054558/http://andrewblock.net/enable-remote-desktop-on-windows-8-core/
[mydigitallife]: http://forums.mydigitallife.info/threads/55935-RDP-Wrapper-Library-(works-with-Windows-8-1-Basic)
[xda-dev]:       http://forum.xda-developers.com/showthread.php?t=2093525&page=3
[yt-offsets]:    http://www.youtube.com/watch?v=FiD86tmRBtk

### 链接：
- 官方 GitHub 仓库：
<br>https://github.com/stascorp/rdpwrap/
- 官方 Telegram 聊天：
<br>https://t.me/rdpwrap
- 此处的活跃讨论：
<br>[在 Windows 8 核心/基本版上启用远程桌面 - Andrew Block .net][andrewblock]
- 此处的 MDL 项目和应用线程：
<br>[RDP Wrapper 库（适用于 Windows 8.1 基本版）][mydigitallife]
- 关于在 Windows RT 上移植到 ARM 的一些想法（帖子 #23）：
<br>[\[Q\] 修改 Windows RT 以启用远程桌面][xda-dev]
- 添加 «远程桌面用户» 组：
<br>http://superuser.com/questions/680572/

#### 教程视频：
- [如何找到新版本 termsrv.dll 的偏移量][yt-offsets]

### 发布包中的文件：

| 文件名 | 描述 |
| --------- | ----------- |
| `RDPWInst.exe`  | RDP Wrapper 库安装程序/卸载程序 |
| `RDPCheck.exe`  | 本地 RDP 检查工具（您可以检查 RDP 是否正常工作） |
| `RDPConf.exe`   | RDP Wrapper 配置工具 |
| `install.bat`   | 快速安装批处理文件 |
| `uninstall.bat` | 快速卸载批处理文件 |
| `update.bat`    | 快速更新批处理文件 |

### 常见问题解答

> 我在哪里可以下载安装程序或二进制文件？

在 [GitHub Releases](https://github.com/stascorp/rdpwrap/releases) 部分。

> 使用这个应用程序是否合法？

没有明确的答案，请参见 [这个讨论](https://github.com/stascorp/rdpwrap/issues/26)。

> 安装程序尝试访问互联网，这正常吗？

是的，默认情况下它在在线模式下工作。您可以通过在 `install.bat` 文件中删除 `-o` 标志来禁用它。

> 什么是在线安装模式？

在线安装模式在版本 1.6.1 中引入。当您第一次使用此模式安装 RDP Wrapper 时，它将从 GitHub 下载 [最新的 INI 文件](https://github.com/stascorp/rdpwrap/blob/master/res/rdpwrap.ini)。请参见 [这个讨论](https://github.com/stascorp/rdpwrap/issues/132)。

> 什么是 INI 文件，我们为什么需要它？

INI 文件在版本 1.5 中引入。它存储 RDP Wrapper 的系统配置——一般包装设置、二进制补丁代码和每个构建特定的数据。当新的 `termsrv.dll` 构建发布时，开发者通过更新存储库中的 INI 文件来添加对其的支持。

> 配置工具显示 `[not supported]`，RDP 无法工作。我该怎么办？

确保您已连接到互联网并运行 `update.bat`。

> 更新没有帮助，仍然显示 `[not supported]`。

访问 [issues](https://github.com/stascorp/rdpwrap/issues) 部分，检查您的 `termsrv.dll` 构建是否在此列出。如果找不到相关问题，请创建一个新问题——指定您的构建版本以便添加支持。

> 为什么 `RDPCheck` 不允许更改分辨率和其他设置？

`RDPCheck` 是一个非常简单的应用程序，仅用于测试目的。如果您想自定义设置，需要使用 Microsoft 远程桌面客户端 (`mstsc.exe`)。您可以使用 `127.0.0.1` 或 `127.0.0.2` 地址进行本地连接。

### 已知问题：
- 从 Windows 8 **在平板电脑上**开始，系统将自动注销非活动会话 - [更多信息](https://github.com/stascorp/rdpwrap/issues/37)
- 从 Windows 10 开始，您可能会意外锁定自己无法访问 PC - [更多信息](https://github.com/stascorp/rdpwrap/issues/50)
- 从 Windows 10 家庭版的创作者更新开始，RDP Wrapper 将不再工作，声称监听器为 `[not listening]`，因为 `rfxvmt.dll` 缺失 - [更多信息](https://github.com/stascorp/rdpwrap/issues/194#issuecomment-323564111)，[下载链接](https://github.com/stascorp/rdpwrap/issues/194#issuecomment-325627235)
- 安装某些更新后，终端服务无法启动或出现“访问被拒绝”问题 - [#215](https://github.com/stascorp/rdpwrap/issues/215)，[#101](https://github.com/stascorp/rdpwrap/issues/101)
- RDP Wrapper 不支持启用 RemoteFX 的主机 - [#127](https://github.com/stascorp/rdpwrap/issues/127)，[#208](https://github.com/stascorp/rdpwrap/issues/208)，[#216](https://github.com/stascorp/rdpwrap/issues/216)
- RDP 可以使用，但在登录尝试时 termsrv.dll 崩溃 - Windows Vista Starter RTM x86 (termsrv.dll `6.0.6000.16386`)
- 如果终端服务在启动时挂起，请尝试将 **`rdpwrap.dll`** 添加到防病毒软件的排除列表中。还可以通过以下命令将 RDP Wrapper 与其他共享服务隔离：
<br>`sc config TermService type= own`
- RDP Wrapper 可能会被 AVG Free Antivirus 和 [Norton Antivirus](https://github.com/stascorp/rdpwrap/issues/191) 删除 - 首先确保您从 GitHub 下载了 [官方版本](https://github.com/stascorp/rdpwrap/releases)，然后将其添加到排除列表中。

---

### 更新日志：

### 使用方法

安装：
- 下载最新版本的二进制文件并解压
- 右键点击 **`install.bat`** 并选择“以管理员身份运行”
- 查看命令输出以获取详细信息

更新 INI 文件：
- 右键点击 **`update.bat`** 并选择“以管理员身份运行”
- 查看命令输出以获取详细信息

卸载：
- 进入你解压文件的目录
- 右键点击 **`uninstall.bat`** 并选择“以管理员身份运行”
- 查看命令输出以获取详细信息