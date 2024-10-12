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

[ZVM: 基于Zephyr RTOS的嵌入式实时虚拟机](https://www.bilibili.com/video/BV1pe4y1A7o4/?spm_id_from=333.788.recommend_more_video.14&vd_source=64410f78d160e2b1870852fdc8e2e43a)

## 快速上手ZVM

#### 1. 系统简介 ：

系统介绍部分主要包含ZVM的总体涉及介绍以及基础的模块支持介绍，方便用户理解ZVM的设计思想和理念，详细内容见如下链接：[System_Design：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/1_System_Design.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/1_System_Design.rst)

#### 2. 开发环境搭建：

环境搭建部分主要介绍Linux/Ubuntu主机开发环境的配置，包括主机各类环境变量的配置与升级、zephyrproject SDK的配置、环境验证、zvm仓库的初始化以及简单zvm sample的构建与测试等，方便用户快速入手搭建开发环境，详细内容见如下链接：[Environment_Configuration：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst)

#### 3. ZVM运行与测试：

ZVM现在支持快速在qemu和rk3568板卡上进行验证，可以在上面运行多个虚拟机并进行虚拟化功能测试，下面是ZVM在各个平台上运行的详细教程：

- [ZVM on QEMU：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst)
- [ZVM on RK3568：https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst](https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst)

## ZVM性能测试

ZVM的性能测试在瑞芯微RK3568芯片上进行（开发板采用ROC-RK3568-PC），RK3568芯片是一款代表性的基于ARMv8.2架构（采用Cortex A55）的4核CPU国产SoC芯片。本次性能测试主要对比虚拟机操作系统（VM）在**裸板**（即RK3568裸板）和**ZVM**（即ZVM on RK3568）上的各种性能指标，包括系统线程的调度时延、中断处理时延、内核对象的操作时间、系统吞吐量等。本次测试采用的VM为Zephyr RTOS（3.7 LTS）和Debian（Linux 5.10内核），使用的测试集包括：1）Zephyr Latency  Measure测试集；2）Debian Cyclictest测试集；3）Zephyr Sys kernel测试集；4）Debian Unixbench测试集。前两个测试集关注VM的系统延迟，后两个测试集关注VM的整体性能，具体说明见下面测试中描述。

#### 测试1：**Zephyr RTOS的延迟测试**

我们分别在裸板和ZVM上运行Zephry RTOS，并在ZephyrRTOS中运行Zephyr Latency measure测试集，该基准测试测量选定内核功能的平均延迟，如线程上下文切换、中断处理、信号量收发的延迟，测试项的细节见：[Zephyr Latency measure](https://github.com/zephyrproject-rtos/zephyr/tree/main/tests/benchmarks/latency\_measure)中列出的基准项。

<p align="center"><img width="700" height="380"  src="https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/Zephyr_latency_measure.png"/>
<p align="center">   Zephyr中运行Latency Measure测试结果

测试结果如上图所示，横坐标代表各个基准项目的时延（单位为ns），纵坐标为各个基准测试项。结果显示，相比于裸机，Zephyr RTOS运行在ZVM上时，“Context switch between preemptive thread”测试项的延迟增加了7.1%， “Abort thread”测试项增加了2.4%，所有其他测试项延迟增幅均低于1%,且大部分是0增长。整体结果表明，ZVM对Zephyr RTOS的各项基准测试延迟的影响很小。

#### 测试2：**Debian的延迟测试**

我们分别在裸板和ZVM上运行Debian，并在Debian中运行Cyclictest测试集，Cyclictest 是一个用于测量 Debian系统实时性能的工具，尤其是测量系统的**中断延迟**和**调度延迟**。它通常用于测试系统在实时场景下的响应能力，以确保系统在关键任务中具备足够的实时性。见[Cyclictest](https://wiki.linuxfoundation.org/realtime/documentation/howto/tools/cyclictest/test-design)详细描述。在测试过程中，我们还通过stress工具逐步增加Debian的负载，依次将CPU密集型线程数量从1个增加到8个，每次Cyclictest测试的运行时间为30分钟，累计运行时间4小时。我们同时使用stress生成固定数量的IO和内存密集型任务，以模拟真实部署场景。

<p align="center"><img width="500" height="300"  src="https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/Debian_cyclictest.png"/>
<p align="center">   Debian中运行Cyclictest测试结果

测试结果如上图所示，横坐标为CPU密集型线程数量，纵坐标为具体的延迟数据指标（单位为us）。我们选择1，2，4，8等四个CPU密集型任务数量作为采样点。结果显示，相比于裸机，Debian运行在ZVM上时，最大延迟增加了6.2%，最小延迟增加了2.4%。随着系统负载的增加，Debian在ZVM运行的Cyclictest测试结果逐渐逼近裸机。具体原因是负载较轻情况下ZVM本身内核的线程存在资源竞争会会对Debian线程有一定影响，而当负载到达一定临界值时，裸机与ZVM的资源竞争都满量。

#### 测试3：**Zephyr RTOS开销测试**

我们分别在裸板和ZVM上运行Zephyr RTOS，并在Zephyr RTOS中运行Zephyr sys kernel基准测试集，其主要评估Sem、LIFO、FIFO、堆栈和Memslab等内核对象的操作效率，以体现系统开销。见[Zephyr sys kernel](https://github.com/zephyrproject-rtos/zephyr/tree/main/tests/benchmarks/sys_kernel)详细描述。

<p align="center"><img width="580" height="360"  src="https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/Zephyr_sys_kernel.png"/>
<p align="center">   Zephyr中运行Sys kernel测试结果

测试结果如上图所示，横坐标为各项基准测试项目，纵坐标为相关执行项的时间（单位为ns）。测试结果显示，相比于裸机平台，ZVM上运行Zephy RTOS的Semaphore 3测试项的损耗最大（8.7%），LIFO 3项的性能损耗为5.4%，其余损耗均在3%以内，且FIFO/Stack/Memslab等多个测试项的损耗为0。相比于裸机，ZVM引入的平均开销损耗仅为1.4%，说明ZVM上运行Zephyr RTOS的性能达到了裸机平台的98.6%。

#### 测试4：**Debian开销测试**

我们分别在裸板和ZVM上运行Debian，并在Debian中运行UnixBench基准测试集。UnixBench套件包含多项测试，每项测试评估Debian性能的不同方面，并提供相应的评分。最终，这些评分会被加权并平均，生成一个综合分数作为系统整体性能的评估值，即系统基准指数分数（System Benchmarks Index Score），详细描述见[UnixBench](https://github.com/kdlucas/byte-unixbench)。

<p align="center"><img width="700" height="320"  src="https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/Debian_unixbench.png"/>
<p align="center">   Debian中运行Unixbench测试结果

测试结果如上图所示，横坐标为各项基准测试项，纵坐标各个基准项的评分（Score数值）。测试结果显示，相比于裸机平台，ZVM上运行Debian的Process Creation测试项所引入的性能损耗最大（5.7%），其余项的性能损耗都在2%以内。ZVM和裸机对比，基准指数分数上的开销损耗仅为0.7%。说明ZVM上运行Debian的性能达到了裸机平台的99.3%，基本可以忽略不计。

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
