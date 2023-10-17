拓展技术支持
============

1.内存设计优化方案
^^^^^^^^^^^^^^^^^^^^^^

(1)整体架构
~~~~~~~~~~~~~~~~

ZVM 提供了两阶段的内存映射，第一阶段是从zephyr的内核空间映射到物理内存地址，
第二阶段是虚拟机的物理地址映射到zephyr的物理地址空间。第一阶段的映射主要是把对应内核镜像映射到zephyr物理地址空间中，
第二阶段的映射是使用vm_mem_partition 和 vm_mem_block 进行映射，具体如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/mem_opt_arch.png


(2)动态分配内存
~~~~~~~~~~~~~~~~

ZVM 提供了 CONFIG_VM_DYNAMIC_MEMORY 这一个宏可以让用户自由的选择是否动态分配内存，
如果选择静态的内存分配，每一个虚拟机都将得到一个vm_mem_partition 记录整体的内存分配状态，
否则将使用vm_mem_block记录内存的映射。在vm_mem_partion 中维护一条关于block的双向链表，
block的大小和映射范围可以动态的变化，这样就实现了内存的动态分配。基于双向链表的静态内存记录如下图所示。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/mem_opt_list.png


(3)压缩分区
~~~~~~~~~~~~~~~~

为了减少嵌入式系统的内存损耗，我们拟在ZVM 在内存初始化的过程中从 zephyr
的heap中分配了一块压缩分区，当内存紧张的时候，将会把一部分不常用的block压缩至压缩分区中，
之后如果有请求再把它从压缩分区中恢复。我们采用 LZO 算法进行无损压缩，LZO 具有较高的压缩速度和较低的内存需求，
缺点是压缩率不是很高，符合ZVM的使用场景。内存压缩方案概览图如下图所示。


.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/mem_compress.png


2.virtIO虚拟化方案
^^^^^^^^^^^^^^^^^^^^^^

(1)整体架构
~~~~~~~~~~~~~~~~

在虚拟化系统中，I/O资源是有限的，为了满足多个Guest OS的需求，VMM必须通过I/O虚拟化的方式来复用有限的I/O资源。
现有的I/O虚拟化方案可以分为三类：全虚拟化、半虚拟化和 I/O 透传。
其中全虚拟化方案就是通过纯软件的形式来模拟I/O设备并处理虚拟机的 I/O 请求，
虽然因为无需对操作系统做修改而获得了较好的可移植性和兼容性，但大量的上下文切换也造成了巨大的性能开销。
半虚拟化是一种软硬件结合的方式，它提供了一种机制，用于接收并转发Guest端的I/O请求到Host端，
最终由主机的硬件处理这些I/O请求，同时也可以接收并转发Host端的I/O响应到Guest端。这样既能够有序的处理I/O请求，
又能够减少性能开销。I/O透传技术让虚拟机独占一个物理设备，并像宿主机一样的使用物理设备，
因此其需要依赖虚拟内存技术，以实现不同虚拟机之间内存空间的隔离。
基于ZVM的嵌入式应用场景以及Zephyr操作系统的实时性要求，本方案选用半虚拟化的I/O虚拟化方案，
以Linux系统中的VirtI/O框架作为设计参考。

整体虚拟化架构如图所示，共分为三个部分：前端的驱动程序virtio-driver，
后端的虚拟设备virtio-device以及用于连接二者的virtio-queue。
前端的virtio-driver以内核模块的形式存在于Guest OS中，其核心职责是: 接收来自用户进程的I/O请求，
将这些 I/O 请求转移到相应的后端虚拟设备中，并从virtio-device中接收已经处理完的I/O响应数据。
后端的virtio-device存在于ZVM中，ZVM以内核模块的形式载入主机操作系统Zephyr。其核心职责是：
接收来自相应前端virtio驱动程序的I/O请求，使用物理硬件来处理这些I/O请求，并将响应数据暴露给前端驱动程序。
virtio-queue是一种数据结构，其位于主机和虚拟机都能访问的共享内存中，
其是前端驱动程序和后端虚拟设备消息传输的通道，对I/O请求和I/O响应的操作满足生产者-消费者模型。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/virtIO_arch.png


(2)virtio-queue设计
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
如下图所示，virtio-queue是一组缓冲区块组成的队列，每一个缓冲区块都可以设置为可读或可写。
virtio-driver和virtio-device通过virtio-queue进行数据交流，
每一个virtio-device绑定了一定数量的virtio-queue。Guest OS中的virtio-driver捕获I/O请求之后，
将I/O请求信息写入一个缓冲区块，并将其添加到相应设备的virtio-queue中。
而VMM中的virtio-device从相应设备的virtio-queue中读取并处理I/O请求，
并将响应信息写回到相应的virtio-queue中。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/virtIO_queue.png


(3)virtio-driver设计
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
不同的外设需要设计不同的驱动程序，具体表现为绑定的virtio-queue数量，
virtio-queue中缓冲区的结构以及对缓冲区的操作不同，本方案暂只对块设备驱动程序的设计方法进行说明。
块设备只绑定了一个virtio-queue，这个virtio-queue即用于保存I/O请求，也用于保存I/O响应数据。
virtio-queue中的缓冲区结构如图3（左）所示，在原始的virtio框架中，
每个缓冲区的前16个字节总是一个只读的描述符结构，该描述符结构如图3（右）所示，type成员用于指示该缓冲区是只读、
只写还是通用的SCSI命令以及在这个命令之前是否应该有一个写障碍。
ioprio成员用于指示该缓冲区中保存的I/O请求的优先级，值越大则优先级越高。sector成员指示磁盘操作的偏移量。
缓冲区的最后一个字节是只写的，如果请求成功则写入0，失败则写入1，不支持该请求则写入2。
剩余的缓冲区部分的长度以及类型依据于请求的类型而定。

.. figure:: https://gitee.com/cocoeoli/zvm/raw/master/zvm_doc/figure/virtIO_driver.png


(4)virtio-device设计
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
后端的virtio-device主要需要进行的是I/O事件的通知，当从virtio-queue中读取到一个I/O请求时，
虚拟设备需要通知真实的物理设备对I/O请求进行处理，在本方案中，拟设计一个API将I/O请求分发到Host的I/O调度器上，
由Host完成之后的操作。



`Prev>> ZVM运行与调试 <https://gitee.com/cocoeoli/zvm/blob/master/zvm_doc/5_Running_and_Debugging.rst>`__


`Next>> ZVM后续规划 <https://gitee.com/cocoeoli/zvm/blob/master/zvm_doc/7_Todo_List.rst>`__

