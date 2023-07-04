ZVM系统构建
================


QEMU platform
~~~~~~~~~~~~~~~~~~~~~

1. 在ZVM的工作目录下构建ZVM镜像
+++++++++++++++++++++++++++++++++++++++++++

.. code:: shell

   cd ~/zvm_workspace/zvm

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_build.sh build qemu

2）或者使用命令行构建镜像:

.. code:: shell

   west build -b qemu_cortex_max_smp samples/_zvm 


2. 生成ZVM镜像文件如下: 
++++++++++++++++++++++++++

.. code:: shell

    build/zephyr/zvm_host.elf


Arm FVP platform
~~~~~~~~~~~~~~~~~~

1. 在ZVM的工作目录构建ZVM镜像
++++++++++++++++++++++++++++++
进入zvm工作目录：

.. code:: shell

   cd ~/zvm_workspace/zvm

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_build.sh build fvp

2）或者使用命令行构建镜像:

.. code:: shell

   west build -b fvp_cortex_a55x4_a75x2_smp samples/_zvm \
   -DARMFVP_BL1_FILE=path-to/trusted-firmware-a/build/fvp/release/bl1.bin \
   -DARMFVP_FIP_FILE=path-to/trusted-firmware-a/build/fvp/release/fip.bin 

后面的'arm-trusted-fireware-a'为arm 平台的安全启动工具，

2. 生成ZVM镜像文件如下: 
+++++++++++++++++++++++++++

.. code:: shell

   build/zephyr/zvm_host.elf



构建虚拟机镜像
--------------------------

因为本项目的zvm系统搭建的是同构虚拟化平台，现阶段实现的虚拟机和主机运行的平台是一致的，因此下面分别介绍针对qemu以及
fvp平台的os镜像构建过程。

Building Zephyr OS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

在构建Zephyr os的镜像过程中，需要使用zephyrproject的工程，分别生成适用于qemu和fvp版本的虚拟机镜像，镜像构建具体过程如下。
需要注意的是，本项目中在zephyr vm生成过程中，如果是fvp平台，需要考虑arm trusted-firmware-a的启动配置，arm trusted-firmware-a
相关仓库和配置参考资料[2]，直接将代码拉取下来，再编译构建即可。


构建zephyr vm镜像(qemu)：
+++++++++++++++++++++++++++++

Supported board: fvp_base_revc_2xaemv8a

.. code:: shell

   west build -b fvp_base_revc_2xaemv8a samples/subsys/shell/shell_module/  \
   -DARMFVP_BL1_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin \ 
   -DARMFVP_FIP_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin 


构建zephyr vm镜像(fvp)：
++++++++++++++++++++++++++++++++++

Supported board: qemu_cortex_a53

.. code:: shell

   west build -b qemu_cortex_a53 samples/subsys/shell/shell_module/


最终生成如下镜像文件：

.. code:: shell

   build/zephyr/zephyr.bin


Building linux OS
~~~~~~~~~~~~~~~~~~~~~~~~~~~

构建linux OS过程中，需要先拉取linux kernel源码，并构建设备树及文件系统，
最终构建linux vm镜像(qemu)：


1. Build dtb.
+++++++++++++++++++++++++++++
.. code:: shell

   # build dtb file for linux os, the dts file is locate at ../zvm_config/qemu_platform/linux-qemu-virt.dts 
   dtc linux-qemu-virt.dts -I dts -O dtb > linux-qemu-virt.dtb

2. Build filesystem.
++++++++++++++++++++++++++++++++++++++++++++++++++

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


3. Build kernel
+++++++++++++++++++++++++++++

这里我们构建可以运行在虚拟机上的Linux内核。

   .. code:: shell

      # Download Linux-5.16.12 or other version’s kernel.
      # chose the debug info, the .config file that is show on ../zvm_config/qemu_platform/.config_qemu
      cp ../zvm_config/qemu_platform/.config_qemu path-to/kernel/
      # add filesystem's *.cpio.gz file to kernel image by chosing it in menuconfig.
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
      # build kernel
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image



构建linux vm镜像(fvp)：
~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Download Linux-5.16.12 or other version’s kernel.
++++++++++++++++++++++++++++++++++++++++++++++++++++++

2. Build kernel.
++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. code:: shell

   # chose the debug info, the .config file that is show on ../zvm_config/fvp_platform/.config_fvp
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
   # build kernel, generate image in: ./zvm_configs/fvp_platform/Image
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image

3. Build dtb.
+++++++++++++++++++++++++++++++++++++++++++++++++++

.. code:: shell

   # build dtb file for linux os, the dts file is locate at ../zvm_config/fdts/* 
   dtc fdts/fvp-base-gicv3-psci.dts -I dts -O dtb > fvp-base-gicv3-psci.dtb

4. Build filesystem.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++

.. code:: shell

   # build the filesystem and generate the filesystem image
   # Using busybox to build it, ref: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/. 

5. Build linux image.
+++++++++++++++++++++++++++++++++++++++++++++++++++++

.. code:: shell

   # using boot-wrapper to build linux image.
   wget https://git.kernel.org/pub/scm/linux/kernel/git/mark/boot-wrapper-aarch64.git/snapshot/boot-wrapper-aarch64-ed60963595855e66ffc06a8a543cbb429c7ede03.tar.gz
   tar -xf boot-wrapper-aarch64-ed60963595855e66ffc06a8a543cbb429c7ede03.tar.gz
   cd boot-wrapper-aarch64-ed60963595855e66ffc06a8a543cbb429c7ede03/
   autoreconf -i
   ./configure --enable-psci --enable-gicv3 --with-kernel-dir=path-to/linux-5.16.12/ --with-dtb=path-to/fvp-base-gicv3-psci.dtb --with-initrd=path-to/initramfs.cpio.gz --host=aarch64-linux-gnu
   make

   # And the final generated image file: linux-system.axf


最终生成文件
~~~~~~~~~~~~~~~~~~~~~~~~~~~

在qemu平台，成功生成如下文件：
++++++++++++++++++++++++++++++++++++++++++++
.. code:: shell 

   zvm_host.elf, zephyr.bin, linux-qemu-virt.dtb, Image, initramfs.cpio.gz

在fvp平台，成功生成如下文件：
+++++++++++++++++++++++++++++++++++++++++++++
.. code:: shell 
   
   zvm_host.elf, zephyr.bin, linux-system.axf(包含内核镜像，文件系统及设备树等文件)



参考资料：
---------------------------
[1] https://docs.zephyrproject.org/latest/index.html 

[2] https://gitee.com/cocoeoli/arm-trusted-firmware-a 

`Prev>> 核心模块介绍 <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/3_Key_Modules.rst>`__

`Next>> ZVM运行与调试 <https://gitee.com/cocoeoli/zvm/blob/refactor/zvm_doc/5_Running_and_Debugging.rst>`__

