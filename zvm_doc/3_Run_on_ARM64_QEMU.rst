在ARM QEMU上运行ZVM
======================


1. QEMU 平台构建ZVM镜像
-----------------------

拉取镜像并进入工作区：

.. code:: shell

   cd ~/zvm_workspace/zvm

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_zvm.sh build qemu_cortex_max_smp

或者使用命令行构建镜像:

.. code:: shell

   west build -b qemu_cortex_max_smp samples/_zvm


2） 生成ZVM镜像文件如下:

.. code:: shell

    build/zephyr/zvm_host.elf


2. QEMU 平台运行ZVM(非定制镜像)
-------------------------------

如果不想自己去定制Linux和Zephyr的镜像文件，本项目提供了直接可以在平台上执行的镜像文件，
可以在使用如下方法拉取已经定制好的镜像：

.. code:: shell
    cd ../                            #返回到zvm_workspace
    git clone https://gitee.com/hnu-esnl/zvm_vm_image.git

随后将镜像放置代码仓指定位置：

.. code:: shell

    git clone https://gitee.com/hnu-esnl/zvm_vm_image.git
    cp -r zvm_vm_image/qemu_max/linux_sp/*  zvm/zvm_config/qemu_platform/hub
    cp -r zvm_vm_image/qemu_max/zephyr/* zvm/zvm_config/qemu_platform/hub

此时，在zvm_config/qemu_platform/hub目录下有Linux和zephyr虚拟机的镜像，直接执行如下命令即可运行：
（注1：上述仓库中镜像可以选择下单个或者多个）

（注2：上述仓库中zephyr镜像提供bin，elf二种格式，ZVM的默认配置下是支持BIN镜像运行）

（注3：如若想要运行ELF格式的镜像，请修改auto_zvm.sh:46处文件名为对应ELF文件名,并在ZVM项目中修改samples/_zvm/prj.conf文件，
在该文件中启动CONFIG_ZVM_ELF_LOADER宏）

（注4：如果要运行linux guest os，请修改auto_zvm.sh:47行处linux镜像名为自己想运行的linux镜像）

.. code:: shell

   ./auto_zvm.sh debugserver qemu_cortex_max_smp


3. QEMU 平台自定义构建镜像方法
-------------------------------

构建 Zephyr VM 镜像
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

在构建Zephyr镜像的过程中，需要使用zephyrproject的工程。
首先进入zephyrproject工程并镜像构建具体过程如下。
Supported board: qemu_cortex_a53

.. code:: shell

   west build -b qemu_cortex_a53 samples/subsys/shell/shell_module/

最终生成如下镜像文件：

.. code:: shell

   build/zephyr/zephyr.bin

构建 Linux VM 镜像
~~~~~~~~~~~~~~~~~~~~~~~~~~~

构建linux OS过程中，需要先拉取linux kernel源码，并构建设备树及文件系统，
最终构建linux vm镜像：


（1） 生成dtb文件.
+++++++++++++++++++++++++++++

.. code:: shell

   # build dtb file for linux os, the dts file is locate at ../zvm_config/qemu_platform/linux-qemu-virt.dts 
   dtc linux-qemu-virt.dts -I dts -O dtb > linux-qemu-virt.dtb

   # if CONFIG_VIRTIO_MMIO=y && CONFIG_VIRTIO_BLK=y
   dtc linux-qemu-virtio.dts -I dts -O dtb > linux-qemu-virtio.dtb

（2） 生成文件系统.
+++++++++++++++++++++++++++++

构建initramfs根文件系统，这此处借助了BusyBox构建极简initramfs，提供基本的用户态可执行程序编译
BusyBox，配置CONFIG_STATIC参数，编译静态版BusyBox，编译好的可执行文件busybox不依赖动态链接库
，可以独立运行，方便构建initramfs


1） 编译调试版内核

   .. code:: shell

      $ cd linux-4.14
      $ make menuconfig
      #修改以下内容
      Kernel hacking  --->
      [*] Kernel debugging
      Compile-time checks and compiler options  --->
      [*] Compile the kernel with debug info
      [*]   Provide GDB scripts for kernel debugging
      $ make -j 20


2） 拉取busybox包

   .. code:: shell

      # 在busybox官网拉取busybox包
      # 官网 ref="https://busybox.net/"

3）编译busybox，配置CONFIG_STATIC参数，编译静态版BusyBox

   .. code:: shell

      $ cd busybox-1.28.0
      $ make menuconfig
      #勾选Settings下的Build static binary (no shared libs)选项
      $ make -j 20
      $ make install
      #此时会安装在_install目录下
      $ ls _install
      bin  linuxrc  sbin  usr

4）创建initramfs，启动脚本init

   .. code:: shell

      $ mkdir initramfs
      $ cd initramfs
      $ cp ../_install/* -rf ./
      $ mkdir dev proc sys
      $ sudo cp -a /dev/{null, console, tty, tty1, tty2, tty3, tty4} dev/
      $ rm linuxrc
      $ vim init
      $ chmod a+x init
      $ ls
      $ bin   dev  init  proc  sbin  sys   usr
      #init文件内容：
      #!/bin/busybox sh
      mount -t proc none /proc
      mount -t sysfs none /sys

      exec /sbin/init

5）打包initramfs

   .. code:: shell

      $ find . -print0 | cpio --null -ov --format=newc | gzip -9 > ../initramfs.cpio.gz


（3） 编译 kernel.
+++++++++++++++++++++++++++++

   .. code:: shell

      # Download Linux-5.16.12 or other version’s kernel.
      # chose the debug info, the .config file that is show on ../zvm_config/qemu_platform/.config_qemu
      cp ../zvm_config/qemu_platform/.config_qemu_sp path-to/kernel/
      # add filesystem's *.cpio.gz file to kernel image by chosing it in menuconfig.
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
      # build kernel
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image

最终在qemu平台，生成如下文件：

.. code:: shell

   # maybe you need to change the name of rootfs in auto_zvm.sh
   zephyr.bin, linux-qemu-virt.dtb, Image, initramfs.cpio.gz

再将其复制到zvm_config/qemu_platform/hub文件夹中，并运行：

.. code:: shell

   ./auto_zvm.sh debug qemu


4. QEMU 平台使用zvm启动虚拟机
-------------------------------

运行zvm平台后可见以下内容：

.. figure:: https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/zvm_platform.png
   :align: center

其中前两行表明平台的输出端口被重定向到/dev/pts/1和/dev/pts/2，标签分别为serial1和serial2

在zvm平台上输入如下命令查看平台支持的指令：

.. code:: shell

   zvm help


启动Linux虚拟机
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

（1）创建Linux虚拟机:
+++++++++++++++++++++++++++++

   .. code:: shell

      zvm new -t linux


（2）运行Linux虚拟机:
+++++++++++++++++++++++++++++

   .. code:: shell

      zvm run -n 2

(-n后面的数是虚拟机的对应ID，假设创建所得虚拟机的VM-ID：2)

启动Zephyr虚拟机
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

（1）创建Zephyr虚拟机:
+++++++++++++++++++++++++++++

   .. code:: shell

      zvm new -t zephyr


（2）运行Zephyr虚拟机:
+++++++++++++++++++++++++++++

   .. code:: shell

      zvm run -n 0

(-n后面的数是虚拟机的对应ID，假设创建所得虚拟机的VM-ID：0)


连接对应的虚拟机
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

新建窗口

.. code:: shell


   cd ~/zvm_workspace/zvm

   screen /dev/pts/1


(screen后接对应虚拟机的输出端口重定向去向)

在完成对应虚拟机的创建和运行之后，新窗口上会出现相应的虚拟机终端界面。

成功运行
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: https://gitee.com/openeuler/zvm/raw/master/zvm_doc/figure/Run%20successfully.png
   :align: center


`Prev>> 主机开发环境搭建 <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst>`__

`Next>> 在RK3568上运行ZVM <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst>`__


参考资料：
-----------
[1] https://docs.zephyrproject.org/latest/index.html

[2] https://gitee.com/cocoeoli/arm-trusted-firmware-a
