ZVM 开源文档
==================

欢迎来到嵌入式实时虚拟机ZVM (Zephyr-based Virtual Machine) 项目的开发文档，
本项目是一个基于 `Zephyr RTOS <https://github.com/zephyrproject-rtos/zephyr>`__ 开发的虚拟机管理器，

 由湖南大学教授、 嵌入式与网络计算湖南省重点实验室主任谢国琪老师团队开发，旨在实时嵌入式操作系统中构建一个虚拟化管理平台
 项目仓库中包含Zephyr RTOS内核及工具的一些源码，以及添加虚拟化支持所需的一些代码，共同构成了ZVM的代码仓。

ZVM使用
`zephyrproject-rtos <https://github.com/zephyrproject-rtos/zephyr>`__ 所遵守的
`Apache 2.0 许可证 <https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE>`__
，主要开发语言为C/C++语言。


介绍：什么是ZVM
------------------
基于实时操作系统Zephyr的虚拟机 Zephyr-based Virtual Machine（ZVM），
是一款实时虚拟机，其面向高性能嵌入式计算环境，提供嵌入式平台上操作系统级别的资源隔离和共享服务。

.. figure:: https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/zvm_demo.png
   :align: center
   :alt: zvm_demo


本项目以Zephyr实时操作系统为基础，使用C/C++在Zephyr RTOS中实现一个面向嵌入式平台的虚拟机管理器。
Zephyr RTOS是一个小型的实时操作系统，用于连接、资源受限和嵌入式设备，支持多种架构。
Zephyr包括内核、所有组件和库、设备驱动程序、协议栈、文件系统和固件更新，以开发连接、资源受限和嵌入式设备，
可用于各种应用和行业领域，如工业物联网、可穿戴设备、机器学习等。

文档目录
------------------

下面目录中包含ZVM系统介绍及系统的使用说明。

具体内容及简介：
^^^^^^^^^^^^^^

`1.系统介绍： <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/1_System_Design.rst>`__
*****************************************************************************************************
ZVM的整个系统各个模块的整体架构介绍，以及一些其他的功能介绍。

`2.主机开发环境搭建： <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst>`__
***************************************************************************************************************
Linux/Ubuntu主机开发环境的配置，zephyrproject SDK的配置及zvm仓库的初始化和简单sample的构建与测试等。

`3.在QEMU上运行ZVM： <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst>`__
********************************************************************************************************
在QEMU ARM64上面运行ZVM的教程。

`4.在RK3568上运行ZVM <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst>`__
********************************************************************************************************
在ROC_RK3568_PC上面运行ZVM的教程。

`5.加入我们： <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/5_Join_us.rst>`__
********************************************************************************************************
最后，我们介绍了我们团队的一些成员、如何加入ZVM进行开发以及开发过程中需要遵循的一些基本编码规则。
以及为ZVM的后续发展制定了一些计划安排，你可以在这里找到它们。



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
