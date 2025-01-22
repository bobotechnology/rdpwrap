### 已知问题：
- 从Windows 8开始**在平板电脑上**，系统会注销非活动会话 - [更多信息](https://github.com/stascorp/rdpwrap/issues/37)
- 从Windows 10开始，你可能会意外锁定自己 - [更多信息](https://github.com/stascorp/rdpwrap/issues/50)
- 从Windows 10家庭版的创作者更新开始，RDP Wrapper将不再工作，声称监听器是`[未监听]`，因为`rfxvmt.dll`缺失 - [更多信息](https://github.com/stascorp/rdpwrap/issues/194#issuecomment-323564111)，[下载链接](https://github.com/stascorp/rdpwrap/issues/194#issuecomment-325627235)
- 安装某些更新后，终端服务无法启动或出现“访问被拒绝”问题 - [#215](https://github.com/stascorp/rdpwrap/issues/215)，[#101](https://github.com/stascorp/rdpwrap/issues/101)
- RDP Wrapper不支持启用RemoteFX的主机 - [#127](https://github.com/stascorp/rdpwrap/issues/127)，[#208](https://github.com/stascorp/rdpwrap/issues/208)，[#216](https://github.com/stascorp/rdpwrap/issues/216)
- RDP正常工作，但termsrv.dll在登录尝试时崩溃 - Windows Vista Starter RTM x86（termsrv.dll `6.0.6000.16386`）
- 如果终端服务在启动时挂起，请尝试将**`rdpwrap.dll`**添加到防病毒排除项中。还可以通过以下命令尝试将RDP Wrapper与其他共享服务隔离：
<br>`sc config TermService type= own`
- RDP Wrapper可能会被AVG Free Antivirus和[Norton Antivirus](https://github.com/stascorp/rdpwrap/issues/191)删除 - 首先确保你从GitHub下载了[官方版本](https://github.com/stascorp/rdpwrap/releases)，然后将其添加到排除项中。

---

### 安装说明：
- 下载最新的发布二进制文件并解压文件
- 右键单击 **`install.bat`** 并选择以管理员身份运行
- 查看命令输出以获取详细信息

更新INI文件：
- 右键单击 **`update.bat`** 并选择以管理员身份运行
- 查看命令输出以获取详细信息

卸载：
- 转到你解压文件的目录
- 右键单击 **`uninstall.bat`** 并选择以管理员身份运行
- 查看命令输出以获取详细信息

### 确认工作：
- Windows Vista Starter (x86 - Service Pack 1及更高版本)
- Windows Vista Home Basic
- Windows Vista Home Premium
- Windows Vista Business
- Windows Vista Enterprise
- Windows Vista Ultimate
- Windows Server 2008
- Windows 7 Starter
- Windows 7 Home Basic
- Windows 7 Home Premium
- Windows 7 Professional
- Windows 7 Enterprise
- Windows 7 Ultimate
- Windows Server 2008 R2
- Windows 8开发者预览
- Windows 8消费者预览
- Windows 8发布预览
- Windows 8
- Windows 8单语言版
- Windows 8专业版
- Windows 8企业版
- Windows Server 2012
- Windows 8.1预览
- Windows 8.1
- Windows 8.1连接版（带Bing）
- Windows 8.1单语言版
- Windows 8.1连接单语言版（带Bing）
- Windows 8.1专业版
- Windows 8.1企业版
- Windows Server 2012 R2
- Windows 10技术预览
- Windows 10专业技术预览
- Windows 10家庭版
- Windows 10家庭单语言版
- Windows 10专业版
- Windows 10企业版
- Windows Server 2016技术预览

---

### 更新日志：

#### 2017.12.27
- 版本1.6.2
- 安装程序更新
- 包含最新Windows版本的更新INI文件
- 添加对支持的Windows版本的检查（[#155](https://github.com/stascorp/rdpwrap/issues/155)）
- 添加从当前目录获取INI文件的功能（[#300](https://github.com/stascorp/rdpwrap/issues/300)）
- 添加恢复缺失的rfxvmt.dll的功能（在Windows 10家庭版中缺失 [#194](https://github.com/stascorp/rdpwrap/issues/194)）
- RDP配置更新
- 添加允许自定义启动程序的功能（[#13 (comment)](https://github.com/stascorp/rdpwrap/issues/13#issuecomment-77651843)）
- 添加MSI安装包（[#14](https://github.com/stascorp/rdpwrap/issues/14)）

#### 2016.08.01
- 版本1.6.1
- 包含最新Windows版本的更新INI文件
- 安装程序更新
- 添加在线安装模式
- 添加卸载时保留设置的功能
- RDP配置更新
- 修复RDP端口更改时更新防火墙规则
- 添加隐藏用户登录的功能

#### 2015.08.12
- 版本1.6
- 添加对Windows 10的支持
- INI文件现在更小 - 所有注释已移至KB文件
- 安装程序更新
- 添加1056错误的解决方法（尽管这不是错误）
- 添加安装程序的更新支持
- 最新的RDPClip版本包含在安装程序中
- RDP检查器更新
- 更改连接IP为127.0.0.2
- 更新一些文本消息
- RDP配置更新
- 添加所有可能的投影模式
- 还会将设置写入组策略

#### 2014.12.11
- 版本1.5
- 添加INI配置支持
- 配置现在存储在INI文件中
- 我们可以在不构建新二进制文件的情况下扩展版本支持
- 添加对Windows 8.1和KB3000850的支持
- 添加对Windows 10技术预览更新2的支持
- 安装程序更新
- RDP配置更新
- 添加诊断功能到RDP配置

#### 2014.11.14
- 版本1.4
- 添加对Windows 10技术预览更新1的支持
- 添加对Windows Vista SP2和KB3003743的支持
- 添加对Windows 7 SP1和KB3003743的支持
- 添加新的RDP配置程序

#### 2014.10.21
- 安装程序更新
- 添加将RDP Wrapper安装到System32目录的功能
- 修复安装程序中的问题 - NLA设置现在保持不变
- 本地RDP检查器更新
- 在检查开始时更改SecurityLayer和UserAuthentification值
- RDP检查器在退出时恢复值

#### 2014.10.20
- 版本1.3
- 添加对Windows 10技术预览的支持
- 添加对Windows 7和KB2984972的支持
- 添加对Windows 8和KB2973501的支持
- 添加对Windows Vista（SP0、SP1和SP2）的扩展支持
- 添加对Windows 7（SP0和SP1）的扩展支持
- 源代码中的一些改进
- 安装程序更新到v2.2
- 修复Vista x64中的安装错误（错误的展开路径）
- 本地RDP检查器更新
- 为错误0x708添加描述

#### 2014.07.26
- 版本1.2
- 添加对Windows 8开发者预览的支持
- 添加对Windows 8消费者预览的支持
- 添加对Windows 8发布预览的支持
- 添加对Windows 8.1预览的支持
- 添加对Windows 8.1的支持
- 源代码中更多详细信息
- 安装程序更新到v2.1

#### 2013.12.09
- RDP Wrapper的C++移植由Fusix完成
- 现在支持x64架构
- 添加新的命令行安装程序v2.0
- 添加本地RDP检查器
- 源代码（C++移植、安装程序2.0、本地RDP检查器）也包含在内

#### 2013.10.25
- 版本1.1源代码可用

#### 2013.10.22
- 版本1.1
- 稳定版本
- 改进的包装器（现在可以包装内部未导出的termsrv.dll SL策略函数）
- 添加对Windows 8单语言版的支持（在带有Intel Atom Z2760的Acer平板电脑上测试）

#### 2013.10.19
- 版本1.0
- 第一个[beta]版本
- 基本的SL政策包装器

---

#### 支持的终端服务版本：
- 6.0.X.X（Windows Vista / Server 2008）
- 6.0.6000.16386（Windows Vista）
- 6.0.6001.18000（Windows Vista SP1）
- 6.0.6002.18005（Windows Vista SP2）
- 6.0.6002.19214（Windows Vista SP2与KB3003743 GDR）
- 6.0.6002.23521（Windows Vista SP2与KB3003743 LDR）
- 6.1.X.X（Windows 7 / Server 2008 R2）
- 6.1.7600.16385（Windows 7）
- 6.1.7600.20890（Windows 7与KB2479710）
- 6.1.7600.21316（Windows 7与KB2750090）
- 6.1.7601.17514（Windows 7 SP1）
- 6.1.7601.21650（Windows 7 SP1与KB2479710）
- 6.1.7601.21866（Windows 7 SP1与KB2647409）
- 6.1.7601.22104（Windows 7 SP1与KB2750090）
- 6.1.7601.18540（Windows 7 SP1与KB2984972 GDR）
- 6.1.7601.22750（Windows 7 SP1与KB2984972 LDR）
- 6.1.7601.18637（Windows 7 SP1与KB3003743 GDR）
- 6.1.7601.22843（Windows 7 SP1与KB3003743 LDR）
- 6.1.7601.23403（Windows 7 SP1与KB3125574）
- 6.1.7601.24234（Windows 7 SP1与KB4462923）
- 6.2.8102.0（Windows 8开发者预览）
- 6.2.8250.0（Windows 8消费者预览）
- 6.2.8400.0（Windows 8发布预览）
- 6.2.9200.16384（Windows 8 / Server 2012）
- 6.2.9200.17048（Windows 8与KB2973501 GDR）
- 6.2.9200.21166（Windows 8与KB2973501 LDR）
- 6.3.9431.0（Windows 8.1预览）
- 6.3.9600.16384（Windows 8.1 / Server 2012 R2）
- 6.3.9600.17095（Windows 8.1与KB2959626）
- 6.3.9600.17415（Windows 8.1与KB3000850）
- 6.3.9600.18692（Windows 8.1与KB4022720）
- 6.3.9600.18708（Windows 8.1与KB4025335）
- 6.3.9600.18928（Windows 8.1与KB4088876）
- 6.3.9600.19093（Windows 8.1与KB4343891）
- 6.4.9841.0（Windows 10技术预览）
- 6.4.9860.0（Windows 10技术预览更新1）
- 6.4.9879.0（Windows 10技术预览更新2）
- 10.0.9926.0（Windows 10专业技术预览）
- 10.0.10041.0（Windows 10专业技术预览更新1）
- 10.0.10240.16384（Windows 10 RTM）
- 10.0.10586.0（Windows 10 TH2发布151029-1700）
- 10.0.10586.589（Windows 10 TH2发布160906-1759与KB3185614）
- 10.0.11082.1000（Windows 10 RS1发布151210-2021）
- 10.0.11102.1000（Windows 10 RS1发布160113-1800）
- 10.0.14251.1000（Windows 10 RS1发布160124-1059）
- 10.0.14271.1000（Windows 10 RS1发布160218-2310）
- 10.0.14279.1000（Windows 10 RS1发布160229-1700）
- 10.0.14295.1000（Windows 10 RS1发布160318-1628）
- 10.0.14300.1000（Windows Server 2016技术预览5）
- 10.0.14316.1000（Windows 10 RS1发布160402-2227）
- 10.0.14328.1000（Windows 10 RS1发布160418-1609）
- 10.0.14332.1001（Windows 10 RS1发布160422-1940）
- 10.0.14342.1000（Windows 10 RS1发布160506-1708）
- 10.0.14352.1002（Windows 10 RS1发布160522-1930）
- 10.0.14366.0（Windows 10 RS1发布160610-1700）
- 10.0.14367.0（Windows 10 RS1发布160613-1700）
- 10.0.14372.0（Windows 10 RS1发布160620-2342）
- 10.0.14379.0（Windows 10 RS1发布160627-1607）
- 10.0.14383.0（Windows 10 RS1发布160701-1839）
- 10.0.14385.0（Windows 10 RS1发布160706-1700）
- 10.0.14388.0（Windows 10 RS1发布160709-1635）
- 10.0.14393.0（Windows 10 RS1发布160715-1616）
- 10.0.14393.1198（Windows 10 RS1发布安全版170427-1353与KB4019472）
- 10.0.14393.1737（Windows 10 RS1发布市场版170914-1249与KB4041691）
- 10.0.14393.2457（Windows 10 RS1发布市场版180822-1743与KB4343884）
- 10.0.14901.1000（Windows 10 RS预发布160805-1700）
- 10.0.14905.1000（Windows 10 RS预发布160811-1739）
- 10.0.14915.1000（Windows 10 RS预发布160826-1902）
- 10.0.14926.1000（Windows 10 RS预发布160910-1529）
- 10.0.14931.1000（Windows 10 RS预发布160916-1700）
- 10.0.14936.1000（Windows 10 RS预发布161003-1929）
- 10.0.14946.1000（Windows 10 RS预发布161007-1700）
- 10.0.14951.1000（Windows 10 RS预发布161014-1700）
- 10.0.14955.1000（Windows 10 RS预发布161020-1700）
- 10.0.14959.1000（Windows 10 RS预发布161026-1700）
- 10.0.14965.1001（Windows 10 RS预发布161104-1700）
- 10.0.14971.1000（Windows 10 RS预发布161111-1700）
- 10.0.14986.1000（Windows 10构建160101.0800）
- 10.0.14997.1001（Windows 10构建160101.0800）
- 10.0.15002.1001（Windows 10构建160101.0800）
- 10.0.15007.1000（Windows 10构建160101.0800）
- 10.0.15014.1000（Windows 10构建160101.0800）
- 10.0.15019.1000（Windows 10 RS预发布170121-1513）
- 10.0.15025.1000（Windows 10 RS预发布170127-1750）
- 10.0.15031.0（Windows 10 RS2发布170204-1546）
- 10.0.15042.0（Windows 10 RS2发布170219-2329）
- 10.0.15046.0（Windows 10构建160101.0800）
- 10.0.15048.0（Windows 10构建160101.0800）
- 10.0.15055.0（Windows 10构建160101.0800）
- 10.0.15058.0（Windows 10构建160101.0800）
- 10.0.15061.0（Windows 10构建160101.0800）
- 10.0.15063.0（Windows 10构建160101.0800）
- 10.0.15063.296（Windows 10构建160101.0800）
- 10.0.15063.994（Windows 10构建160101.0800）
- 10.0.15063.1155（Windows 10构建160101.0800）
- 10.0.16179.1000（Windows 10构建160101.0800）
- 10.0.16184.1001（Windows 10构建160101.0800）
- 10.0.16199.1000（Windows 10构建160101.0800）
- 10.0.16215.1000（Windows 10构建160101.0800）
- 10.0.16232.1000（Windows 10构建160101.0800）
- 10.0.16237.1001（Windows 10构建160101.0800）
- 10.0.16241.1001（Windows 10构建160101.0800）
- 10.0.16251.0（Windows 10构建160101.0800）
- 10.0.16251.1000（Windows 10构建160101.0800）
- 10.0.16257.1（Windows 10构建160101.0800）
- 10.0.16257.1000（Windows 10构建160101.0800）
- 10.0.16273.1000（Windows 10构建160101.0800）
- 10.0.16275.1000（Windows 10构建160101.0800）
- 10.0.16278.1000（Windows 10构建160101.0800）
- 10.0.16281.1000（Windows 10构建160101.0800）
- 10.0.16288.1（Windows 10构建160101.0800）
- 10.0.16291.0（Windows 10构建160101.0800）
- 10.0.16294.1（Windows 10构建160101.0800）
- 10.0.16296.0（Windows 10构建160101.0800）
- 10.0.16299.0（Windows 10构建160101.0800）
- 10.0.16299.15（Windows 10构建160101.0800）
- 10.0.16353.1000（Windows 10构建160101.0800）
- 10.0.16362.1000（Windows 10构建160101.0800）
- 10.0.17004.1000（Windows 10构建160101.0800）
- 10.0.17017.1000（Windows 10构建160101.0800）
- 10.0.17025.1000（Windows 10构建160101.0800）
- 10.0.17035.1000（Windows 10构建160101.0800）
- 10.0.17046.1000（Windows 10构建160101.0800）
- 10.0.17063.1000（Windows 10构建160101.0800）
- 10.0.17115.1（Windows 10构建160101.0800）
- 10.0.17128.1（Windows 10构建160101.0800）
- 10.0.17133.1（Windows 10构建160101.0800）
- 10.0.17134.1（Windows 10构建160101.0800）
- 10.0.17723.1000（Windows 10构建160101.0800）
- 10.0.17763.1（Windows 10构建160101.0800）