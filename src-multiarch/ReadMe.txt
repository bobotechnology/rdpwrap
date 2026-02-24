========================================================================
RDPWrap 项目（src-multiarch）
========================================================================

简介
------------------------------------------------------------------------
RDPWrap 是一个 Windows 远程桌面服务（RDP）包装库项目，
通过对相关接口进行 Hook 与策略处理，实现对多会话等行为的扩展支持。

当前仓库结构（主要文件）
------------------------------------------------------------------------
RDPWrap.sln
    Visual Studio 解决方案文件。

RDPWrap.vcxproj
    Visual Studio C++ 项目文件，包含编译选项、链接配置与依赖。

Export.def
    模块导出定义文件（如 ServiceMain、SvchostPushServiceGlobals）。

dllmain.cpp
    DLL 入口实现。

rdpwrap_core.h
    核心声明与公共接口定义。

rdpwrap_entry.cpp
    入口相关实现。

rdpwrap_globals.cpp
    全局变量与全局状态实现。

rdpwrap_hook.cpp
    Hook 逻辑核心实现。

rdpwrap_policy.cpp
    策略处理相关实现。

rdpwrap_utils.cpp
    通用工具函数实现。

stdafx.h / stdafx.cpp
    预编译头相关文件。

targetver.h
    Windows SDK 目标版本定义。

cpp_configparser/
    内置 INI 配置解析库（源码形式引入）。
    - include/ini/parser.hpp：解析接口定义
    - src/parser.cpp：解析实现
    - tests/basic_test.cpp：基础测试

构建说明
------------------------------------------------------------------------
1) 使用 Visual Studio 打开 RDPWrap.sln。
2) 选择目标平台。
3) 选择配置（Debug/Release）并执行生成。

说明
------------------------------------------------------------------------
- 本 README 以当前仓库文件为准进行整理。
- 若后续新增/重命名源文件，请同步更新本文件。

========================================================================
