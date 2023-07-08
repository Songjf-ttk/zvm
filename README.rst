ZVM 开源文档
==================

欢迎来到Zephyr based Virtual Machine(ZVM) 项目开发的文档，本项目是一个基于
`Zephyr RTOS <https://github.com/zephyrproject-rtos/zephyr>`__ 开发的虚拟机管理器项目，
由湖南大学信息科学与工程学院谢国琪教授团队开发，旨在于实时嵌入式操作系统中构建一个虚拟化管理平台，
在需要虚拟化支持的环境中实现多个系统运行在统一的硬件平台上。
项目仓库中包含Zephyr RTOS内核及工具的一些源码，以及添加虚拟化支持所需的一些代码，共同构成了ZVM的代码仓。

ZVM使用
`zephyrproject-rtos <https://github.com/zephyrproject-rtos/zephyr>`__ 所遵守的
`Apache 2.0 许可证 <https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE>`__ 
，主要开发语言为C（C++）语言。


介绍：什么是ZVM
------------------
基于实时操作系统Zephyr的虚拟机 Zephyr-based Virtual Machine（ZVM），
是一种参考Kernel-based Virtual Machine（KVM）实现的虚拟机，其面向高性能嵌入式计算环境，
提供嵌入式平台上操作系统级别的资源隔离和共享服务。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/refactor/zvm_doc/figure/zvm_demo.png
   :alt: zvm_demo

   zvm_demo


本项目以Zephyr实时操作系统为基础，使用C（C++）在Zephyr RTOS中实现一个面向嵌入式平台的虚拟机管理器。
Zephyr RTOS是一个小型的实时操作系统，用于连接、资源受限和嵌入式设备，支持多种架构，发布于Apache License 2.0下。
Zephyr包括内核、所有组件和库、设备驱动程序、协议栈、文件系统和固件更新，以开发连接、资源受限和嵌入式设备。
Zephyr RTOS易于部署、安全、连接和管理。它具有不断增长的软件库集，可用于各种应用和行业领域，
如工业物联网、可穿戴设备、机器学习等。

Zephyr系统整体结构和Linux类似，本项目参考基于内核的虚拟机（Kernel-based Virtual Machine, KVM）思路，
实现基于Zephyr RTOS的虚拟化管理平台ZVM，有助于更好的理解KVM等复杂虚拟化系统的实现原理。
同时考虑在嵌入式等资源受限场景下的虚拟化实现需要注意哪些内容。

文档目录
------------------

下面目录中内容包含ZVM系统介绍及系统的使用说明。

具体内容及简介：
^^^^^^^^^^^

`系统简介： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/1_System_Overview.rst>`__
*****************************************************************************************************
ZVM的整个系统各个模块的整体架构介绍，以及一些其他的功能介绍。

`主机开发环境搭建： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/2_Environment_Configuration.rst>`__
****************************************************************************************************************************
Linux主机开发环境的配置，zephyrproject SDK的配置及zvm仓库的初始化和简单sample的构建与测试等。

`核心模块介绍： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/3_Key_Modules.rst>`__
********************************************************************************************************
各个模块的详细实现介绍，包括虚拟处理器、虚拟内存、虚拟设备等各个模块的技术实现。

`ZVM系统构建： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/4_System_Build.rst>`__
********************************************************************************************************
ZVM主机的构建、Linux和zephyr虚拟机的构建以及其他ZVM运行所需要环境的配置流程。

`ZVM运行与调试： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/5_Running_and_Debugging.rst>`__
********************************************************************************************************
说明如何在主机中运行及调试相应的模块，包括基础指令的测试，以及如何对系统进行调试的步骤。

`拓展技术介绍： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/6_Expansion_Technology.rst>`__
*******************************************************************************************************
为了优化ZVM在嵌入式系统中的运行支持，我们拟支持一些额外的技术，保证ZVM系统能更好的支持各类应用场景。

`ZVM后续规划： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/7_Todo_List.rst>`__
**************************************************************************************************
我们对ZVM的后续发展制定了一些计划安排，你可以在这里找到。

`加入我们： <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/8_Join_us.rst>`__
************************************************************************************************
最后，我们介绍了我们团队的一些成员、如何加入ZVM进行开发以及开发过程中需要遵循的一些基本编码规则。


视频介绍
----------
以往关于一些ZVM的分享视频。

EOSS分享视频：
^^^^^^^^^^^^^^^^^^^^^^

`ZVM: An Embedded Real-time Virtual Machine Based on Zephyr RTOS <https://mp.weixin.qq.com/s/igDKghI7CptV01wu9JrwRA>`__
*************************************************************************************************************************************

Sig-Zephyr分享视频：
^^^^^^^^^^^^^^^^^^^^^^

`ZVM:基于Zephyr RTOSI的嵌入式实时虚拟机 <https://www.bilibili.com/video/BV1pe4y1A7o4/?spm_id_from=333.788.recommend_more_video.14&vd_source=64410f78d160e2b1870852fdc8e2e43a>`__
******************************************************************************************************************************************************************************************


参与贡献
^^^^^^^^^^^^^^^^^^^^^^

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


特技
^^^^^^^^^^^^^^^^^^^^^^

1.  使用 Readme\_XXX.rst 来支持不同的语言，例如 Readme\_en.rst, Readme\_zh.rst
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
