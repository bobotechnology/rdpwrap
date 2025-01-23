# Third Party RDP Wrapper Library by bobo

[![Telegram](https://img.shields.io/badge/chat-Telegram-blue.svg)](https://t.me/rdpwrap)
![Environment](https://img.shields.io/badge/Windows-Vista,%207,%208,%2010-brightgreen.svg)
[![Release](https://img.shields.io/github/release/stascorp/rdpwrap.svg)](https://github.com/stascorp/rdpwrap/releases)
![License](https://img.shields.io/github/license/stascorp/rdpwrap.svg)
![Downloads](https://img.shields.io/github/downloads/stascorp/rdpwrap/latest/total.svg)
![TotalDownloads](https://img.shields.io/github/downloads/stascorp/rdpwrap/total.svg)

#### All thanks for RDP Wrapper Library to: [stascorp](https://github.com/stascorp/rdpwrap)

The goal of this project is to enable Remote Desktop Host support and concurrent RDP sessions on reduced functionality systems for home usage.

RDP Wrapper works as a layer between Service Control Manager and Terminal Services, so the original termsrv.dll file remains untouched. Also this method is very strong against Windows Update.

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

| NT Version    | Screenshots |
| ------------- | ----------- |
| Windows Vista | [![Windows Vista Starter][pVistaST]][fVistaST] [![Windows Vista Home Basic][pVistaHB]][fVistaHB] |
| Windows 7     | [![Windows 7 Starter][pWin7ST]][fWin7ST] [![Windows 7 Home Basic][pWin7HB]][fWin7HB] |
| Windows 8     | [![Windows 8 Developer Preview][pWin8DP]][fWin8DP] [![Windows 8 Consumer Preview][pWin8CP]][fWin8CP] [![Windows 8 Release Preview][pWin8RP]][fWin8RP] [![Windows 8][pWin8]][fWin8] |
| Windows 8.1   | [![Windows 8.1 Preview][pWin81P]][fWin81P] [![Windows 8.1][pWin81]][fWin81] |
| Windows 10    | [![Windows 10 Technical Preview][pWin10TP]][fWin10TP] [![Windows 10 Pro Technical Preview][pWin10PTP]][fWin10PTP] [![Windows 10][pWin10]][fWin10] |
---
[WinPPE]: http://forums.mydigitallife.info/threads/39411-Windows-Product-Policy-Editor

This solution was inspired by [Windows Product Policy Editor][WinPPE], big thanks to **kost** :)

— binarymaster

### Attention:
It's recommended to have original termsrv.dll file with the RDP Wrapper installation. If you have modified it before with other patchers, it may become unstable and crash in any moment.

### Information:
- Source code is available, so you can build it on your own
- RDP Wrapper does not patch termsrv.dll, it loads termsrv with different parameters
- RDPWInst and RDPChecker can be redistributed without development folder and batch files
- RDPWInst can be used for unattended installation / deployment
- Windows 2000, XP and Server 2003 will not be supported

### Key features:
- RDP host server on any Windows edition beginning from Vista
- Console and remote sessions at the same time
- Using the same user simultaneously for local and remote logon (see configuration app)
- Up to [15 concurrent sessions](https://github.com/stascorp/rdpwrap/issues/192) (the actual limitation depends on your hardware and OS version)
- Console and RDP session shadowing (using [Task Manager in Windows 7](http://cdn.freshdesk.com/data/helpdesk/attachments/production/1009641577/original/remote_control.png?1413476051) and lower, and [Remote Desktop Connection in Windows 8](http://woshub.com/rds-shadow-how-to-connect-to-a-user-session-in-windows-server-2012-r2/) and higher)
- Full [multi-monitor support](https://github.com/stascorp/rdpwrap/issues/163) for RDP host
- ...and if you find a new feature not listed here, [tell us](https://github.com/stascorp/rdpwrap/issues/new) ;)

### Porting to other platforms:
- **ARM** for Windows RT (see links below)
- **IA-64** for Itanium-based Windows Server? *Well, I have no idea* :)

### Building the binaries:
- **x86 Delphi version** can be built with *Embarcadero RAD Studio 2010*
- **x86/x64 C++ version** can be built with *Microsoft Visual Studio 2013*

[andrewblock]:   http://web.archive.org/web/20150810054558/http://andrewblock.net/enable-remote-desktop-on-windows-8-core/
[mydigitallife]: http://forums.mydigitallife.info/threads/55935-RDP-Wrapper-Library-(works-with-Windows-8-1-Basic)
[xda-dev]:       http://forum.xda-developers.com/showthread.php?t=2093525&page=3
[yt-offsets]:    http://www.youtube.com/watch?v=FiD86tmRBtk

### Links:
- Official GitHub repository:
<br>https://github.com/stascorp/rdpwrap/
- Official Telegram chat:
<br>https://t.me/rdpwrap
- Active discussion in the comments here:
<br>[Enable remote desktop on Windows 8 core / basic - Andrew Block .net][andrewblock]
- MDL Projects and Applications thread here:
<br>[RDP Wrapper Library (works with Windows 8.1 Basic)][mydigitallife]
- Some ideas about porting to ARM for Windows RT (post #23):
<br>[\[Q\] Mod Windows RT to enable Remote Desktop][xda-dev]
- Adding «Remote Desktop Users» group:
<br>http://superuser.com/questions/680572/

#### Tutorial videos:
- [How to find offsets for new termsrv.dll versions][yt-offsets]

### Files in release package:

| File name | Description |
| --------- | ----------- |
| `RDPWInst.exe`  | RDP Wrapper Library installer/uninstaller |
| `RDPCheck.exe`  | Local RDP Checker (you can check the RDP is working) |
| `RDPConf.exe`   | RDP Wrapper Configuration |
| `install.bat`   | Quick install batch file |
| `uninstall.bat` | Quick uninstall batch file |
| `update.bat`    | Quick update batch file |

### Frequently Asked Questions

> Where can I download the installer or binaries?

In the [GitHub Releases](https://github.com/stascorp/rdpwrap/releases) section.

> Is it legal to use this application?

There is no definitive answer, see [this discussion](https://github.com/stascorp/rdpwrap/issues/26).

> The installer tries to access the Internet, is it normal behaviour?

Yes, it works in online mode by default. You may disable it by removing `-o` flag in the `install.bat` file.

> What is online install mode?

Online install mode introduced in version 1.6.1. When you installing RDP Wrapper first time using this mode, it will download [latest INI file](https://github.com/stascorp/rdpwrap/blob/master/res/rdpwrap.ini) from GitHub. See [this discussion](https://github.com/stascorp/rdpwrap/issues/132).

> What is INI file and why we need it?

INI file was introduced in version 1.5. It stores system configuration for RDP Wrapper — general wrapping settings, binary patch codes, and per build specific data. When new `termsrv.dll` build comes out, developer adds support for it by updating INI file in repository.

> Config Tool shows `[not supported]` and RDP doesn't work. What can I do?

Make sure you're connected to the Internet and run `update.bat`.

> Update doesn't help, it still shows `[not supported]`.

Visit [issues](https://github.com/stascorp/rdpwrap/issues) section, and check whether your `termsrv.dll` build is listed here. If you can't find such issue, create a new — specify your build version for adding to support.

> Why `RDPCheck` doesn't allow to change resolution and other settings?

`RDPCheck` is a very simple application and only for testing purposes. You need to use Microsoft Remote Desktop Client (`mstsc.exe`) if you want to customize the settings. You can use `127.0.0.1` or `127.0.0.2` address for loopback connection.

### Known issues:
- Beginning with Windows 8 **on tablet PCs** inactive sessions will be logged out by system - [more info](https://github.com/stascorp/rdpwrap/issues/37)
- Beginning with Windows 10 you can accidentally lock yourself from PC - [more info](https://github.com/stascorp/rdpwrap/issues/50)
- Beginning with the Creators Update for Windows 10 Home, RDP Wrapper will no longer work, claiming that the listener is `[not listening]` because of `rfxvmt.dll` is missing - [more info](https://github.com/stascorp/rdpwrap/issues/194#issuecomment-323564111), [download links](https://github.com/stascorp/rdpwrap/issues/194#issuecomment-325627235)
- Terminal Service does not start after installing some updates or "Access Denied" issue - [#215](https://github.com/stascorp/rdpwrap/issues/215), [#101](https://github.com/stascorp/rdpwrap/issues/101)
- RDP Wrapper does not work with RemoteFX enabled hosts - [#127](https://github.com/stascorp/rdpwrap/issues/127), [#208](https://github.com/stascorp/rdpwrap/issues/208), [#216](https://github.com/stascorp/rdpwrap/issues/216)
- RDP works, but termsrv.dll crashes on logon attempt - Windows Vista Starter RTM x86 (termsrv.dll `6.0.6000.16386`)
- If Terminal Services hangs at startup, try to add **`rdpwrap.dll`** to antivirus exclusions. Also try to isolate RDP Wrapper from other shared services by the command:
<br>`sc config TermService type= own`
- RDP Wrapper can be removed by AVG Free Antivirus and [Norton Antivirus](https://github.com/stascorp/rdpwrap/issues/191) - first make sure you downloaded [official release](https://github.com/stascorp/rdpwrap/releases) from GitHub, then add it to exclusions.

---

### Change log:

### How to use
Installation instructions:
- Download latest release binaries and unpack files
- Right-click on **`install.bat`** and select Run as Administrator
- See command output for details

To update INI file:
- Right-click on **`update.bat`** and select Run as Administrator
- See command output for details

To uninstall:
- Go to the directory where you extracted the files
- Right-click on **`uninstall.bat`** and select Run as Administrator
- See command output for details
