在ARM QEMU上运行ZVM
======================


1. QEMU 平台构建ZVM镜像
-----------------------

拉取镜像并进入工作区：

.. code:: shell

   cd ~/zvm_workspace/zvm

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_zvm.sh build qemu

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
    cp -r zvm_vm_image/qemu_max/linux/*  zvm/zvm_config/qemu_platform/hub
    cp -r zvm_vm_image/qemu_max/zephyr/* zvm/zvm_config/qemu_platform/hub

此时，在zvm_config/qemu_platform/hub目录下有Linux和zephyr虚拟机的镜像，直接执行如下命令即可运行：
（注1：上述仓库中镜像可以选择下单个或者多个）
（注2：上述仓库中zephyr镜像提供bin，elf二种格式，ZVM的默认配置下是支持ELF镜像运行）
（注3：如若想要运行bin格式的镜像，请修改auto_zvm.sh:46处文件名为对应bin文件名,并在ZVM项目中修改samples/_zvm/prj.conf文件，
在该文件中注释掉CONFIG_ZVM_ELF_LOADER宏）
（注4：如果要运行linux guest os，请修改auto_zvm.sh:47行处linux镜像名为自己想运行的linux镜像）

.. code:: shell

   ./auto_zvm.sh debug qemu


3. QEMU 平台自定义构建镜像方法
-------------------------------

构建 Zephyr VM 镜像
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

在构建Zephyr的镜像过程中，需要使用zephyrproject的工程。
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
      cp ../zvm_config/qemu_platform/.config_qemu path-to/kernel/
      # add filesystem's *.cpio.gz file to kernel image by chosing it in menuconfig.
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
      # build kernel
      make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image

最终在qemu平台，生成如下文件：

.. code:: shell

   zephyr.bin, linux-qemu-virt.dtb, Image, initramfs.cpio.gz

再将其复制到zvm_config/qemu_platform/hub文件夹中，并运行：

.. code:: shell

   ./auto_zvm.sh debug qemu

`Prev>> 主机开发环境搭建 <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/2_Environment_Configuration.rst>`__

`Next>> 在RK3568上运行ZVM <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/4_Run_on_ROC_RK3568_PC.rst>`__


参考资料：
-----------
[1] https://docs.zephyrproject.org/latest/index.html

[2] https://gitee.com/cocoeoli/arm-trusted-firmware-a
