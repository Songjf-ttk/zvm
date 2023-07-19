核心模块设计
================

ZVM是基于实时操作系统Zephyr设计的虚拟机管理器（Hypervisor），系统架构如下图所示，
主要功能模块包括包括CPU虚拟化、内存虚拟化、中断虚拟化、时钟虚拟化及I/O虚拟化等功能。
本文档介绍各个核心功能模块的实现。


核心功能模块
---------------

1.处理器虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

CPU虚拟化模块主要功能在于为每个虚拟机虚拟出单独隔离的上下文、程序执行及异常状态，在系统中，
每个vCPU以一个线程的形式存在，并由Hypervisor统一调度。为了更好的提高vCPU的性能，
ZVM利用ARMv8架构为Hypervisors提供了一个单独的特权级模式，具有比Kernel更高的特权级，便于对VM进行监管。 

CPU虚拟化模块主要功能在于为每个虚拟机虚拟出单独隔离的上下文、程序执行及异常状态，在系统中，
每个vCPU以一个线程的形式存在，并由Hypervisor统一调度。
为了更好的提高vCPU的性能，ARMv8架构为Hypervisors提供了一个单独的特权级模式，
具有比Kernel更高的特权级，便于对OS Kernel进行监管。

具体来说，vCPU部分初始化及运行流程如下：

(1)首先在创建VM时根据VM信息创建vCPU结构体，并创建vCPU线程加入等待队列，即为VM提供了vCPU的支持。

(2)同时在运行VM时根据OS信息初始化vCPU数据结构并检查系统状态，初始化各类子系统。

(3)其次，系统在进行Guest Entry或 Exit时的上下文切换，并在每次Handler时进行处理，以处理虚拟机异常。
而在上下文切换过程中，通过错误码判断处理机制，sync处理过程和irq分开，加快处理速度，保证虚拟机的切换性能，
整体vCPU模拟执行逻辑如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vcpu_logic.png

此外，为了更好的减少系统性能开销，我们利用arm VHE技术来减少主机操作系统与客户机操作系统之间的上下文切换次数。

(1)VHE的实现可以使Host OS整体移动到EL2特权级模式下，并在不更改操作原始系统代码的情况下实现迁移。

(2)VHE模式下主要硬件实现了寄存器的重定向，如图4所示，因此不再需要更改Zephyr OS内核的代码即可实现将其迁移到EL2层，
以减少了虚拟机切换的性能开销，VHE上下文切换如下图所示。


.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vhe_context_switch.png

2.内存虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

内存虚拟化模块主要功能是实现虚拟机（VM）内存地址的访问与隔离，系统也需要将不同虚拟机的内存空间隔离，
并监控虚拟机对实际物理内存的访问，实现对主机物理内存的保护。

ARM提供了两阶段的地址查找策略，

(1)即第1阶段从VM的客户机虚拟地址到VM的客户机物理地址的转化。

(2)到第2阶段从VM的客户机物理地址到主机物理地址的转换，具体内存地址虚拟化逻辑如下图所示，
  虚拟机的地址空间由相应的数据结构实现，并在VM初始化过程中初始化其地址空间，以支持VM的内存访问。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vmem_logic.png

为了实现这一功能，ARM专门为第2阶段的转换提供了单独的硬件，以提高地址转换的性能。同时，考虑实时系统的时间开销。
ZVM先接单采用的线性映射的逻辑，即将虚拟机所需要的物理地址空间，一整块映射到设备的物理地址空间，
如图下图所示，而不需要考虑Hypervisor级别的页错误，以达到接近物理机器的地址转换开销。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vmem_mapping.png

3.中断虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

中断虚拟化主要基于ARM GIC设备的特性，为每个VM提供中断控制器，以支持VM的虚拟中断处理，
  具体如下图所示。

首先，系统将初始化vGIC相关的数据结构，并注册相应的虚拟中断处理逻辑，并在内存空间分配一块地址来实现对中断控制器的模拟，
  以此为VM提供虚拟中断服务。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/virq_logic.png

此外，在ZVM系统中，在中断触发逻辑中，我们通过配置相关寄存器，统一将VM的中断路由到Hypervisor，
并由其分配给不同的vCPU。而在虚拟中断触发方式上，我们具体是通过GIC中的Virtual CPU interface和List Register来实现虚拟中断的注入。
如下图所示，这一逻辑部分可由硬件接管，以减少软件仿真的开销，进而提高虚拟中断的性能。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/virq_routing.png


4.时钟虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

在ZVM的时钟虚拟化部分，现阶段的设计主要包含两个时钟虚拟化逻辑。ARM平台上的时钟由CPU内部的寄存器控制，
因此，在虚拟机独占某个CPU时，vTimer通过访问虚拟时钟寄存器模拟，
即对虚拟机来说，有一套单独的计数寄存器，用于记录VM的时间。

在这个逻辑中，每个CPU定义了一组虚拟时钟寄存器，它们单独计数并在预定的时间过后抛出中断，并由主机转发至VM。
  此外，在虚拟机切换过程中（即非独占状态），虚拟时钟将通过一系列的数据结构，计算出VM实际运行的时间，
  并补偿虚拟机退出的时间，进而校准vCPU时钟偏差，同时将时钟触发事件转移的主机物理时钟寄存器中，
  以支持为虚拟机提供定时器服务，具体如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vtimer_logic.png


5.设备虚拟化模块
^^^^^^^^^^^^^^^^^^^^^^

在I/O虚拟化方面，在本研究采用的ARM架构中MMIO的方式将设备地址映射的物理内存地址空间进行访问。
具体实现上，我们通过构建Virtual MMIO Device设备及全虚拟化的方式，并在创建VM的过程中将设备分配给指定的VM，
以实现I/O的虚拟化，如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vdev_logic.png

ZVM系统现阶段具体支持了两类设备，如下图所示，对于主机独占设备，通过完全虚拟化的方式模拟，而其他设备，
支持采用透传直通的方式，以减少开销。此外，我们还将支持virtIO半虚拟化方式，具体在后续章节介绍。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vdev_mode.png

(1)完全虚拟化的设备（如GIC设备）。

  由于GIC设备控制整个系统的中断配置信息，对其进行修改将会影响ZVM系统的配置，而每个ARM架构的OS又都必须要一个中断控制器，
  因此我们为每个VM提供了一个完全虚拟化的GIC设备，并为其在内存中分配一段地址，模拟GIC的IO地址空间，
  并存储当前vGIC的配置信息，当执行VM时，将配置信息通过Hypervisor控制写入物理GIC地址当中或者完全通过软件模拟操作GIC读写操作，
  以实现GIC设备的虚拟化。

(2)直通的设备（如UART设备）。

  由于对于常见的平台来说，UART设备在系统中往往不止一个，且设备之间一般无影响，因此我们将其直接分配给VM使用。
  具体实现上，即通过在ZVM初始化过程中初始化相应的UART设备，并建立起VM IO地址到Host OS IO地址的映射关系。
  并在VM初始化过程中分配给指定的VM，实现VM对该设备的直接使用，而Hypervisor在此过程中只需要记录设备分配给了哪个VM，
  不需要进行具体设备功能的模拟，减小了系统的开销。具体的I/O 虚拟化的过程如下，当系统刚开始发生IO trap时，
  处理器由EL1陷入到EL2特权级，并判断是否为直通设备，如果是，则直接将赋予IO空间访问权限；
  否则将建立陷入EL2模式中进行IO模拟，并最终判断是否访问物理IO空间，从图中可以看出，
  非直通设备在首次赋予权限后的访问不再需要trap-emulate流程，减少了系统的开销。


6.VM生命周期
^^^^^^^^^^^^^^^^^^^^^^

本项目中虚拟机的状态有以下几种：

| (1)未运行状态（VM_STATUS_NEVER_RUN）：此状态VM未运行，只是创建了一个VM实体，等待首次调度。
| (2)运行状态（VM_STATUS_RUNNING）：此状态VM正在运行，并占用处理器资源。
| (3)挂起状态（VM_STATUS_PAUSE）：此状态VM暂停，相关线程在等待队列中，不占用物理CPU，随时可以调度。
| (4)停止状态（VM_STATUS_STOP）：此状态VM停止，相关线程都停止执行，相关内存和I/O资源开始释放。

一个完整的生命周期包括VM的创建、VM的运行、VM的暂停和VM的退出等一系列流程。
用户通过相应的shell接口实现对VM在各种状态下的切换。为了提供用户操作VM的接口，
我们为VM提供了直通的串口，并直接向串口发送打印信息来判断VM能否正常运行。

因此，在每个平台上，均支持了两个串口：serial0和serial1，如图12所示，其中serial0通过分配给Host OS
来下达控制命令，而serial1通过分配给VM来打印VM的信息，
同时serial1必须支持在虚拟机之间进行切换。直通串口支持shell的方式如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vuart_passthrough.png

7.vCPU分配及调度策略
^^^^^^^^^^^^^^^^^^^^^^

在ZVM系统中，vCPU都是以线程的形式进行调度，因此可以使用Zephyr自身的调度系统进行调度。
Zephyr采用的是基于可变优先级的可抢占式调度算法，并支持时间片轮转调度。

可以通过配置项，将系统配置为基于可变优先级的时间片轮转算法，
时间片轮转调度是指系统给处于就绪态的最高优先级的每个同优先级的任务一个相同的执行时间片。

一个任务所分配的时间片用完后，就进行任务切换，停止当前运行的任务，将它放入就绪列表最高优先级任务链表末尾，
并开始执行就绪队列中的下一个任务。具体使用的轮转调度算法如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/time_stamp.png

同时，为了减少系主机和从机的系统切换开销，ZVM在此阶段设计采用主从核设计模式，在设计过
程中将vCPU线程和主机线程尽量绑定在不同的物理处理器上，并通过核间中断(Inter-Processor Interrupt, IPI)
实现主从核的通信，进而实现主机对从机的控制。

具体来说，如下图所示，cpu0负责主机的任务调度，如shell输入产生的中断将路由至cpu0进行处理，执行控制指令。
cpu0与cpuN间的IPI通信通过方式实现，cpu0通过IPI通知cpuN执行任务。vCPU线程在初始化过程中绑定一个物理CPU，
并在启动时部署到指定cpu上执行，直到异常发生。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/vcpu_allocate.png


`Prev>> 主机开发环境搭建 <https://gitee.com/cocoeoli/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst>`__

`Next>> ZVM系统构建 <https://gitee.com/cocoeoli/zvm/blob/master/zvm_doc/4_System_Build.rst>`__



