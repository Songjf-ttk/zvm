Building system
===============

开发环境配置
---------------
Zephyr-Based Virtual Machine 基于 ZephyrProject 代码库进行开发，构建和运行工具与沿用ZephyrProject原先的
west 工具，本项目已经将west.yml文件进行了配置，只需要使用west对系统进行初始化即可。但是由于系统编译过程中将要使用到许多依赖项目，
这里参考zephyrproject的说明文档[1]下载相应的依赖。

安装相关依赖
~~~~~~~~~~~~~~~~~~

1. 升级软件仓

.. code:: shell

   sudo apt update
   sudo apt upgrade

2. 升级Kitware archive

.. code:: shell

   wget https://apt.kitware.com/kitware-archive.sh
   sudo bash kitware-archive.sh

3. 升级相关依赖仓库

.. code:: shell

   sudo apt install --no-install-recommends git cmake ninja-build gperf \
      ccache dfu-util device-tree-compiler wget \
      python3-dev python3-pip python3-setuptools python3-tk python3-wheel xz-utils file \
      make gcc gcc-multilib g++-multilib libsdl2-dev libmagic1


4. 安装west工具(这里选择全局安装，若是想要在python-env中安装，参考[1]中资料)

.. code:: shell

   pip3 install --user -U west
   echo 'export PATH=~/.local/bin:"$PATH"' >> ~/.bashrc
   source ~/.bashrc


创建并初始化工作区
~~~~~~~~~~~~~~~~~~

1. 创建工作区并拉取zvm仓库镜像

.. code:: shell

   cd ~
   mkdir zvm_workspace && cd zvm_workspace
   git clone https://gitlab.eduxiji.net/202310532111672/project1466467-177494.git


2. 初始化工作仓

.. code:: shell

   cd project1466467-177494
   west init -l /path-to/project1466467-177494

上面的'path-to'修改为自己的目录路径，执行完上面命令后，在'zvm_workspace'目录下将会生成.west文件夹，
其中'config'文件中存放了west的相关配置。此时可以通过执行如下命令查看'west'配置是否成功：

.. code:: shell

   west -h

显示有west信息后，即说明工作仓初始化成功，可以进行主机操作系统和客户机操作系统的开发。



构建主机ZVM镜像
---------------


QEMU platform
~~~~~~~~~~~~~~~~~~

1. 在ZVM的工作目录构建ZVM镜像：
进入zvm工作目录：

.. code:: shell

   cd ~/zvm_workspace/zvm

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_build.sh build qemu

2）或者使用命令行构建镜像:

.. code:: shell

   west build -b qemu_cortex_max_smp samples/_zvm 


2. 生成ZVM镜像文件如下: 

..

    build/zephyr/zvm_host.elf


Arm FVP platform
~~~~~~~~~~~~~~~~~~

1. 在ZVM的工作目录构建ZVM镜像：
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

..

    build/zephyr/zvm_host.elf



构建虚拟机镜像
---------------

因为本项目的zvm系统搭建的是同构虚拟化平台，现阶段实现的虚拟机和主机运行的平台是一致的，因此下面分别介绍针对qemu以及
fvp平台的os镜像构建过程。

Building Zephyr OS
~~~~~~~~
在构建Zephyr os的镜像过程中，需要使用zephyrproject的工程，分别生成适用于qemu和fvp版本的虚拟机镜像，镜像构建具体过程如下。
需要注意的是，本项目中在zephyr vm生成过程中，如果是fvp平台，需要考虑arm trusted-firmware-a的启动配置，arm trusted-firmware-a
相关仓库和配置参考资料[2]，直接将代码拉取下来，再编译构建即可。

构建zephyr vm镜像(qemu)：
^^^^^^^^^^^^^^^^^^

Supported board: fvp_base_revc_2xaemv8a

.. code:: shell

   west build -b fvp_base_revc_2xaemv8a samples/subsys/shell/shell_module/  \
   -DARMFVP_BL1_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin \ 
   -DARMFVP_FIP_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin 


构建zephyr vm镜像(fvp)：
^^^^^^^^^^^^^^^^^^

Supported board: qemu_cortex_a53

.. code:: shell

   west build -b qemu_cortex_a53 samples/subsys/shell/shell_module/


最终生成如下镜像文件：

..

   build/zephyr/zephyr.bin




Building linux OS
~~~~~~~~
构建linux OS过程中，需要先拉取linux kernel源码，并构建设备树及文件系统，最终

构建linux vm镜像(qemu)：
^^^^^^^^^^^^^^^^^^
1. Build dtb.

.. code:: shell

   # build dtb file for linux os, the dts file is locate at ../zvm_config/qemu_platform/linux-qemu-virt.dts 
   dtc linux-qemu-virt.dts -I dts -O dtb > linux-qemu-virt.dtb

2. Build filesystem.

.. code:: shell

   # build the filesystem and generate the filesystem image
   # Using busybox to build it, ref: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/. 

3. Build kernel.

.. code:: shell

   # Download Linux-5.16.12 or other version’s kernel.
   # chose the debug info, the .config file that is show on ../zvm_config/qemu_platform/.config_qemu
   cp ../zvm_config/qemu_platform/.config_qemu path-to/kernel/
   # add filesystem's *.cpio.gz file to kernel image by chosing it in menuconfig.
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
   # build kernel
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image


构建linux vm镜像(fvp)：
^^^^^^^^^^^^^^^^^^

1. Download Linux-5.16.12 or other version’s kernel.

2. Build kernel.

.. code:: shell

   # chose the debug info, the .config file that is show on ../zvm_config/fvp_platform/.config_fvp
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
   # build kernel, generate image in: ./zvm_configs/fvp_platform/Image
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image

3. Build dtb.

.. code:: shell

   # build dtb file for linux os, the dts file is locate at ../zvm_config/fdts/* 
   dtc fdts/fvp-base-gicv3-psci.dts -I dts -O dtb > fvp-base-gicv3-psci.dtb

4. Build filesystem.

.. code:: shell

   # build the filesystem and generate the filesystem image
   # Using busybox to build it, ref: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/. 

5. Build linux image.

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
~~~~~~~~

在qemu平台，成功生成如下文件：
^^^^^^^^^^^^^^^^^^
   zvm_host.elf, zephyr.bin, linux-qemu-virt.dtb, Image, initramfs.cpio.gz

在fvp平台，成功生成如下文件：
^^^^^^^^^^^^^^^^^^
   zvm_host.elf, zephyr.bin, linux-system.axf(包含内核镜像，文件系统及设备树等文件)



参考资料：
-------
[1] https://docs.zephyrproject.org/latest/index.html 

[2] https://gitee.com/cocoeoli/arm-trusted-firmware-a 
