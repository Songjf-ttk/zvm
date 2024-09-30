在ROC_RK3568_PC上运行ZVM
======================

1.  构建面向RK3568平台的ZVM镜像
-----------------------

拉取镜像并进入工作区：

.. code:: shell

   cd ~/zvm_workspace/zvm

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_zvm.sh build roc_rk3568_pc_smp

或者使用命令行构建镜像:

.. code:: shell

   west build -b roc_rk3568_pc_smp samples/_zvm


2） 生成ZVM镜像文件如下:

.. code:: shell

    build/zephyr/zvm_host.bin


2. RK3568 平台运行ZVM(非定制镜像)
-------------------------------

如果不想自己去定制Linux和Zephyr的镜像文件，本项目提供了直接可以在平台上执行的镜像文件，
可以在使用如下方法拉取已经定制好的镜像：

.. code:: shell
    cd ../                             #返回到zvm_workspace
    git clone https://gitee.com/hnu-esnl/zvm_vm_image.git

其中，需要的镜像在roc_rk3568_pc目录下：

.. code:: shell

    zvm_vm_image/roc_rk3568_pc/linux/*  #linux vm相关镜像
    zvm_vm_image/roc_rk3568_pc/zephyr/* #zephyr vm相关镜像

如果需要自定义构建相关的zephyr vm或者linux vm的文件，参考qemu章节的`自定义构建镜像方法`，
最终，所有需要的文件及说明如下：

.. code:: shell

   zvm_host.bin                     #主机镜像
   zephyr.bin                       #zephyr vm 镜像
   Image                            #linux vm 内核镜像
   rk3568-firefly-roc-pc-simple.dtb #Linux设备树文件
   debian_rt.cpio.gz                #Linux rootfs文件

准备好这些镜像后，需要将其统一烧录到rk3568的板卡上。具体来说，就是需要通过tftp协议将这些镜像
烧录到开发板上。包括如下步骤：

（1）搭建主机tftp服务器（ubuntu服务器）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1）安装依赖包
++++++++++++++++++++++++++++++++++++++

.. code:: shell
    sudo apt-get install tftp-hpa tftpd-hpa

2）配置tftp-server目录
++++++++++++++++++++++++++++++++++++++

.. code:: shell
    sudo vim /etc/default/tftpd-hpa

将文件中内容`TFTP_DIRECTORY`修改成自己指定的地址,
例如：TFTP_DIRECTORY="./zvm_workspace/tftp-ser"，然后将上面说的镜像
放入该目录即可。

（2）下载并运行镜像
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1）将两个机器放于同一网段
++++++++++++++++++++++++++++++++++++++

这里可以选择直接连接板卡和主机的网卡，或者连接统一局域网的交换机即可。
例如：

主机ip：192.168.1.101
板卡ip：192.168.1.109

2）下载并运行镜像
++++++++++++++++++++++++++++++++++++++

rk3568板卡通电，使用串口助手连接板卡后，启动时长按`ctrl + c`
进入uboot启动界面，在uboot命令行配置网络：

.. code:: shell

   setenv ipaddr 192.168.1.109              #配置板卡网络
   setenv serverip 192.168.1.101            #配置tftp服务器网络
   ping 192.168.1.101                       #测试tftp服务器地址是否可用，出现active说明正常

由于使用rk3568运行zvm时，主机使用的板卡串口为串口uart3，因此需要主动配置
板卡的uart相关的gpio端口为串口模式，才能正常使用uart3串口。同时，启动两个
虚拟机时，还需要一个串口uart9分配给其他虚拟机，示例中为：

- uart2: 分配给Linux虚拟机
- uart3: 分配给Zephyr虚拟机
- uart9: 分配给其他虚拟机

.. code:: shell

   mw 0xfdc60000 0xffff0022                 #写入串口uart3配置

   mw 0xfdc60074 0x04400440                 #写入串口uart9配置
   mw 0xfdc60310 0xffff0100                 #写入串口uart9配置


下载各个镜像到rk3568板卡：

.. code:: shell

   tftp 0x10000000 zvm_host.bin                         #下载zvm镜像
   tftp 0x01000000 zephyr.bin                           #下载zephyr vm镜像
   tftp 0x60000000 Image                                #下载linux vm镜像
   tftp 0x99000000 rk3568-firefly-roc-pc-simple.dtb     #下载linux 设备树镜像
   tftp 0x69000000 debian_rt.cpio.gz                    #下载linux rootfs镜像   

运行镜像：

.. code:: shell

   dcache flush; icache flush                           #刷新数据和指令cache
   dcache off;icache off;go 0x10000000                  #关闭数据和指令cache
   go 0x10000000                                        #将pc指针指0x10000000

此时，打开uart3串口，即可使用zvm的shell来输入命令并启动两个虚拟机。

3.  RK3568平台的ZVM上运行Paddle Lite
-----------------------

修改/zvm/samples/_zvm/boards/roc_rk3568_pc_smp.overlay的zephyr_ddr的vm_reg_size为600：

.. code:: shell

   vm_reg_size = <DT_SIZE_M(600)>;

1） 使用脚本文件构建ZVM镜像：

.. code:: shell

   ./auto_zvm.sh build roc_rk3568_pc_smp

或者使用命令行构建镜像:

.. code:: shell

   west build -b roc_rk3568_pc_smp samples/_zvm


2） 生成ZVM镜像文件如下:

.. code:: shell

   build/zephyr/zvm_host.bin

3） 参照RK3568 平台运行ZVM步骤，相关文件在AI文件夹下，运行如下命令:

.. code:: shell

   tftp 0x10000000 zvm_host.bin                         #下载zvm镜像
   tftp 0x48000000 zephyr.bin                           #下载zephyr vm镜像
   tftp 0x80000000 Image                                #下载linux vm镜像
   tftp 0x48000000 rk3568-firefly-roc-pc-simple.dtb     #下载linux 设备树镜像
   tftp 0xa0000000 mobilenet_v1.nb                      #下载mobilenetv1模型

运行镜像：

.. code:: shell

   dcache flush; icache flush                           #刷新数据和指令cache
   dcache off;icache off;go 0x10000000                  #关闭数据和指令cache
   go 0x10000000                                        #将pc指针指0x10000000

4.  注意
-----------------------

由于zvm运行需要使用到多个串口，因此主机必须连接至少两个串口，
这里使用的时uart2和uart3。

uart2: 分配给虚拟机

uart3: 用作主机shell控制

具体主机如何连接到串口uart3，需要看不同板卡的设计手册并自主引出串口线。

`Prev>> 在QEMU上运行ZVM <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst>`__

`Next>> 加入我们 <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/5_Join_us.rst>`__
