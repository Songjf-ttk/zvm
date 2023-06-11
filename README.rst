Zephyr-based Virtual Machine Manager
====================================


什么是ZVM
---------
基于实时操作系统Zephyr的虚拟机 Zephyr-based Virtual Machine（ZVM），是一种参考Kernel-based Virtual Machine（KVM）实现的虚拟机，其面向高性能嵌入式计算环境，提供嵌入式平台上操作系统级别的资源隔离和共享服务。

本项目以Zephyr实时操作系统为基础，使用C（C++）在Zephyr RTOS中实现一个面向嵌入式平台的虚拟机管理器。Zephyr RTOS是一个小型的实时操作系统，用于连接、资源受限和嵌入式设备，支持多种架构，发布于Apache License 2.0下。Zephyr包括内核、所有组件和库、设备驱动程序、协议栈、文件系统和固件更新，以开发连接、资源受限和嵌入式设备。Zephyr RTOS易于部署、安全、连接和管理。它具有不断增长的软件库集，可用于各种应用和行业领域，如工业物联网、可穿戴设备、机器学习等。

Zephyr系统整体结构和Linux类似，本项目参考基于内核的虚拟机（Kernel-based Virtual Machine, KVM）思路，实现基于Zephyr RTOS的虚拟化管理平台ZVM，有助于更好的理解KVM等复杂虚拟化系统的实现原理。同时考虑在嵌入式等资源受限场景下的虚拟化实现需要注意哪些内容。


开发环境
------------
-  ARCH: x86_64
-  OS : ubuntu18.04+ 
-  zephyr sdk version: zephyr-sdk-0.13.2+

支持的硬件及仿真平台：
------------

-  FVP Platform: Arm FVP(Installed with Arm DS)
-  QEMU Platform: qemu-6.2.0(Modified)
-  Firefly ROC_RK3568_PC(正在进行适配)

支持的虚拟机操作系统：
-------------

-  Linux
-  Zephyr

代码架构说明
---------
基于Zephyr RTOS工程，主要实现代码的目录为：
    ./arch/arm64/_zvm

    ./samples/_zvm

    ./subsys/_zvm

    ./zvm_config
    
    ./zvm_doc
板卡支持目录(fvp_cortex_a55x4_a75x2_smp, qemu_cortex_max, roc_rk3568_pc)：
    ./board/arm64/*
    
    ./dts/arm64/*
    
    ./soc/arm64/*


实现计划
---------
- [√] 能够在QEMU模拟平台上运行，能够支持启动至少两个及以上虚拟机（Linux, Zephyr）。
- [√] 能够在QEMU模拟平台上运行，能够支持启动至少两个及以上虚拟机（Linux, Zephyr）。
- [ ] 能够支持虚拟机间的通信 。
- [ ] 能够支持虚拟机上运行实时应用 。
- [ ] 能够支持virtIO等设备虚拟化框架 。

手册目录
============

本手册包含ZVM如何构建、使用、开发的相关内容。具体内容存放在（目录地址）地址下。

`1–Overview.rst <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/1--Overview.rst>`__

`2–Building.rst <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/2--Building.rst>`__

`3–Running.rst <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/3--Running.rst>`__

`4–Test
command.rst <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/4--Test%20system.rst>`__

`5–Developing
Help.rst <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/5--Developing%20Help.rst>`__
