# ZVM 开源文档

[![License](https://img.shields.io/badge/license-Apache%202-blue.svg)](LICENSE)

ZVM项目是在开源实时操作系统 [Zephyr RTOS](https://github.com/zephyrproject-rtos/zephyr)（由Linux基金会托管）上构建的一款虚拟机管理器（Hypervisor），ZVM一方面继承了Zephyr RTOS的开源生态和特性，另一方面为业界提供了一个原创的面向嵌入式场景的开源虚拟化平台解决方案。

ZVM由湖南大学教授、嵌入式与网络计算湖南省重点实验室主任谢国琪老师主导开发，这是一款面向嵌入式场景的开源、实时、安全、轻量及易用的虚拟机管理平台，致力于成为嵌入式领域的“Linux-KVM”，构建混合关键部署的开源生态。项目仓库中包含Zephyr RTOS内核、工具及虚拟化支持所需的一些代码，共同构成了ZVM的代码仓。

## 架构设计

ZVM面向高性能嵌入式计算环境，提供嵌入式平台上操作系统级别的资源隔离和共享服务。可用于各种应用和行业领域，如工业物联网、可穿戴设备、机器学习等。ZVM架构图如下所示：

<p align="center"><img width="600" height="500"  src="https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/overview.png"/>

## 持续集成

ZVM将持续支持多种虚拟机操作系统和底层硬件平台，拓展软硬件生态。

#### 虚拟机操作系统

ZVM支持的虚拟机操作系统列表如下, 包括：

- Zephyr RTOS
- openEuler Embedded
- Debian
- **欢迎您提交新的虚拟机OS支持到ZVM社区**

#### 底层硬件平台

ZVM 支持的平台如下, 包括多核的ARMv8平台：

- QEMU ARM64 virt (qemu-max)
- RK3568 SoC (roc_rk3568_pc/ok3568/lubancat2)
- ARM Fixed platform (cortex-a55x4)
- **欢迎您提交新的芯片支持到ZVM社区**

## 视频介绍

视频部分主要为ZVM在各类技术分享会议上的录屏，主要介绍了ZVM的架构和功能，方便用户快速了解ZVM。

#### EOSS分享视频

[ZVM: An Embedded Real-time Virtual Machine Based on Zephyr RTOS](https://mp.weixin.qq.com/s/igDKghI7CptV01wu9JrwRA)

#### Sig-Zephyr分享视频

[ZVM: 基于Zephyr RTOSI的嵌入式实时虚拟机](https://www.bilibili.com/video/BV1pe4y1A7o4/?spm_id_from=333.788.recommend_more_video.14&vd_source=64410f78d160e2b1870852fdc8e2e43a)

## 快速上手ZVM

#### 1. 系统简介 ：

系统介绍部分主要包含ZVM的总体涉及介绍以及基础的模块支持介绍，方便用户理解ZVM的设计思想和理念，详细内容见如下链接：[System_Design：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/1_System_Design.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/1_System_Design.rst)

#### 2. 开发环境搭建：

环境搭建部分主要介绍Linux/Ubuntu主机开发环境的配置，包括主机各类环境变量的配置与升级、zephyrproject SDK的配置、环境验证、zvm仓库的初始化以及简单zvm sample的构建与测试等，方便用户快速入手搭建开发环境，详细内容见如下链接：[Environment_Configuration：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst)

#### 3. ZVM运行与测试：

ZVM现在支持快速在qemu和rk3568板卡上进行验证，可以在上面运行多个虚拟机并进行虚拟化功能测试，下面是ZVM在各个平台上运行的详细教程：

- [ZVM on QEMU：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst)
- [ZVM on RK3568：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst)

## 参与贡献

ZVM作为Zephyr实时操作系统生态在国内的关键一环，致力于构建国内开源hypervisor生态，且正处于快速发展的时期，我们欢迎对ZVM及Zephyr感兴趣的小伙伴加入本项目。

1. Fork 本仓库
2. 新建 Feat_xxx 分支
3. 提交代码
4. 新建 Pull Request

## 交流与反馈

* 技术交流QQ群：如有相关问题可以加入ZVM开发者群进行交流；

<p align="center"><img width="400" height="400"  src="https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/QQCode.png"/>
<p align="center">   ZVM开发者交流QQ群

#### 研发团队：

研发团队依托 [嵌入式与网络计算湖南省重点实验室](http://esnl.hnu.edu.cn/index.htm) 及 [车网智能融合技术研创中心](http://cyy.hnu.edu.cn/yjly1/cwznrhjs1.htm) 等平台，核心成员如下：

**谢国琪教授**: [个人主页](http://csee.hnu.edu.cn/people/xieguoqi)

**熊程来**，邮箱：xiongcl@hnu.edu.cn

**胡星宇**，邮箱：huxingyu@hnu.edu.cn

**王中甲**，邮箱：zjwang@hnu.edu.cn

**韦东升**，邮箱：weidongsheng@hnu.edu.cn

**赵思蓉**，邮箱：zhaosr@hnu.edu.cn

**温浩**，邮箱：wenhao@stu.ynu.edu.cn

**胡宇昊**，邮箱：ahui@hun.edu.cn

**魏胜骏**，邮箱：weishengjun@hnu.edu.cn

欢迎大家反馈开发中遇到的问题，可以联系上面邮箱或者加入技术交流群。

## 版权与许可证

ZVM使用 [zephyrproject-rtos](https://gitee.com/link?target=https%3A%2F%2Fgithub.com%2Fzephyrproject-rtos%2Fzephyr) 所遵守的 [Apache 2.0 许可证](https://gitee.com/link?target=https%3A%2F%2Fgithub.com%2Fzephyrproject-rtos%2Fzephyr%2Fblob%2Fmain%2FLICENSE) ，主要开发语言为C/C++语言。Apache 2.0许可证是一种自由软件许可证，允许用户自由使用、修改和分发软件， 不影响用户的商业使用。
