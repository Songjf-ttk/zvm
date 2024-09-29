ZVM 开源文档
==================

 欢迎来到嵌入式实时虚拟机ZVM (Zephyr-based Virtual Machine) 项目的开发文档。

本项目是在Linux基金会下流行的开源实时操作系统 `Zephyr RTOS <https://github.com/zephyrproject-rtos/zephyr>`__ 
上构建的虚拟机管理器ZVM，其继承了Zephyr RTOS的优秀开源生态和特性，同时提供了一个原创的嵌入式虚拟化解决方案。

ZVM由湖南大学教授、 嵌入式与网络计算湖南省重点实验室主任谢国琪老师团队开发，旨在嵌入式实现一款开源、实时、安全、易用
的虚拟化管理平台，填补Linux-KVM虚拟化方案在嵌入式领域的空白，成为嵌入式领域的“Linux-KVM”。
项目仓库中包含Zephyr RTOS内核、工具及虚拟化支持所需的一些代码，共同构成了ZVM的代码仓。

ZVM使用 `zephyrproject-rtos <https://github.com/zephyrproject-rtos/zephyr>`__ 所遵守的
`Apache 2.0 许可证 <https://github.com/zephyrproject-rtos/zephyr/blob/main/LICENSE>`__
，主要开发语言为C/C++语言。Apache 2.0许可证是一种自由软件许可证，允许用户自由使用、修改和分发软件，
不影响用户的商业使用，因此可以直接作为商业软件发布和销售。


ZVM特点
------------------
ZVM面向高性能嵌入式计算环境，提供嵌入式平台上操作系统级别的资源隔离和共享服务。可用于各种应用和行业领域，如工业物联网、可穿戴设备、机器学习等。
ZVM架构图如下所示：

.. figure:: https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/zvm_demo.png
   :align: center
   :alt: zvm_demo


虚拟机操作系统
^^^^^^^^^^^^^^^^^^^^^^
ZVM支持的虚拟机操作系统列表如下, 包括：

- Zephyr RTOS
- openEuler Embedded
- Debian
- xxx


底层硬件平台
^^^^^^^^^^^^^^^^^^^^^^
ZVM 支持的平台如下, 包括多核的ARMv8平台：

- QEMU ARM64 virt (qemu-max)
- RK3568 SoC (roc_rk3568_pc/ok3568/lubancat2)
- ARM Fixed platform (cortex-a55x4)
- xxx



文档目录
------------------

下面目录中包含ZVM系统介绍及系统的使用说明。

具体内容及简介：
^^^^^^^^^^^^^^

`1.系统介绍： <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/1_System_Design.rst>`__
*****************************************************************************************************
ZVM的架构及各个功能模块的介绍。

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
--------------------

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
ZVM作为Zephyr实时操作系统生态在国内的关键一环，致力于构建国内开源hypervisor生态，
且正处于快速发展的时期，我们欢迎对ZVM及Zephyr感兴趣的小伙伴加入本项目。

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
