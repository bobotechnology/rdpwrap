========================================================================
    动态链接库。RDPWrap项目概述
========================================================================

这个RDPWrap DLL库是通过应用程序向导自动创建的。

本文件提供了应用程序RDPWrap中所有文件内容的摘要。

RDPWrap.vcxproj
    这是VC++项目的主要文件，由应用程序向导创建。它包含了创建此文件时使用的Visual C++语言版本的数据，以及通过应用程序向导选择的平台、配置和项目功能的详细信息。

RDPWrap.vcxproj.filters
    这是由应用程序向导创建的VC++项目的过滤器文件。它包含了项目文件与过滤器之间的映射关系。这些映射在IDE环境中用于将具有相同扩展名的文件分组到一个节点中（例如，CPP文件映射到“源文件”过滤器）。

RDPWrap.cpp
    这是DLL库的主要源文件。

	在创建此DLL库时，不执行符号导出。因此，在构建时不生成LIB文件。如果此项目需要依赖于另一个项目，应添加代码以从DLL库中导出符号以确保生成导出库，或者将“跳过输入库”属性设置为“是”，在项目属性对话框的“链接器”文件夹的“常规”属性页中进行设置。

/////////////////////////////////////////////////////////////////////////////
其他标准文件：

StdAfx.h, StdAfx.cpp
    这些文件用于构建名为RDPWrap.pch的预编译头文件和名为StdAfx.obj的预编译类型文件。

/////////////////////////////////////////////////////////////////////////////
其他注意事项。

应用程序向导使用“TODO:”注释来标记需要补充或修改的源代码片段。
/////////////////////////////////////////////////////////////////////////////

