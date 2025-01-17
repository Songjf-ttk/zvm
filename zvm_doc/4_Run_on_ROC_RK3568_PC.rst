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

    zvm_vm_image/roc_rk3568_pc/linux_sp/*  #linux vm相关镜像
    zvm_vm_image/roc_rk3568_pc/zephyr/* #zephyr vm相关镜像

如果需要自定义构建相关的zephyr vm或者linux vm的文件，参考qemu章节的`自定义构建镜像方法`，
最终，所有需要的文件及说明如下：

.. code:: shell

   zvm_host.bin                     #主机镜像
   zephyr.bin                       #zephyr vm 镜像
   Image                            #linux vm 内核镜像
   rk3568-firefly-roc-pc-simple.dtb #Linux设备树文件
   initramfs.cpio.gz                #Linux rootfs

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

注：进行测试时尽可能3个串口都连接上，不然可能出现由于串口浮空导致的异常
中断问题；

.. code:: shell

   mw 0xfdc60000 0xffff0022                 #写入串口uart3配置

   mw 0xfdc60074 0x04400440                 #写入串口uart9配置
   mw 0xfdc60310 0xffff0100                 #写入串口uart9配置


下载各个镜像到rk3568板卡：

.. code:: shell

   tftp 0x10000000 zvm_host.bin                         #下载zvm镜像
   tftp 0x01000000 zephyr.bin                           #下载zephyr vm镜像
   tftp 0xe0000000 Image                                #下载linux vm镜像
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


4.  串口调试说明
-----------------------
前面已经说过，本文给到的例子中，涉及的串口为uart2,3,9。即正常启动两个虚拟机时两个虚拟机各
分配一个串口。现有的sample中分配如下：

- uart2: Linux 虚拟机；
- uart3: Zephyr 虚拟机；
- uart9: zvm虚拟机管理器；

这么分配的原因是Linux虚拟机只能使用uart2作为其debug串口（和瑞芯微rk3568的bsp相关）。
串口的具体分配原理如下，涉及的多个串口的设备树在*rk3568.dtsi*和*roc_rk3568_pc_smp.overlay*文件中，如下：

.. code:: dts

   uart9: serial@fe6d0000 {
      compatible = "rockchip,rk3568-uart", "ns16550";
      reg = <0x0 0xfe6d0000 0x0 0x10000>;
      interrupts = <GIC_SPI 125 IRQ_TYPE_EDGE IRQ_DEFAULT_PRIORITY>;
      clock-frequency = <12000000>;
      reg-shift = <2>;
      status = "disabled";
      label = "UART9";
   };

   uart3: serial@fe670000 {
         compatible = "rockchip,rk3568-uart", "ns16550";
         reg = <0x0 0xfe670000 0x0 0x10000>;
         interrupts = <GIC_SPI 119 IRQ_TYPE_EDGE IRQ_DEFAULT_PRIORITY>;
         clocks = <&uartclk>;
         clock-frequency = <12000000>;
         reg-shift = <2>;
         status = "reserved";
         current-speed = <1500000>;
         label = "UART3";
   };

   uart2: serial@fe660000 {
         compatible = "rockchip,rk3568-uart", "ns16550";
         reg = <0x0 0xfe660000 0x0 0x10000>;
         interrupts = <GIC_SPI 118 IRQ_TYPE_EDGE IRQ_DEFAULT_PRIORITY>;
         clocks = <&uartclk>;
         clock-frequency = <12000000>;
         reg-shift = <2>;
         status = "reserved";
         current-speed = <1500000>;
         label = "UART2";
   };

正常来说，这些串口都是使能的，因此会在zvm以及虚拟机启动过程中分配给不同的系统。
分配的顺序是从地址高位到地址低位，例如我们先启动zvm，zvm会分配uart9，此时还剩下uart2和uart3，
接着启动zephyr虚拟机，zephyr虚拟机会分配uart3，最后启动linux虚拟机，linux虚拟机会分配uart2，
此时系统能正常启动。
如果你的板卡上不是这些个串口，你仅需要修改overlay中的uart设备树文件即可。例如你的板卡能用uart8,
那么你只需要将uart9/uart3改为uart8即可。

下面介绍板卡上分别使用两个串口和一个串口的情况：

1）板卡上只有两个串口（例如串口上只有uart9和uart2）

此时，仅需要将uart3的label从*UART3*修改*PTUART3*即可（debug串口的识别逻辑是匹配UARTx字符串）。
这个时候的串口分配如下：

- uart2: Linux/Zephyr 虚拟机；
- uart9: zvm虚拟机管理器；

2）板卡上只有一个串口（例如串口上只有uart2，分配给Linux虚拟机）

此时，我们可以通过编写自定义的VM启动程序，在zvm启动后直接启动相关的虚拟机，但不给其分配串口。

.. code:: c++

   void auto_run(void)
   {
      zvm_init();
      //启动Zephyr虚拟机
      start_linux_vm();

      //启动Linux虚拟机
      start_linux_vm();
   }

此时，我们考虑将uart3/uart9都关掉。这个时候的串口分配如下：

- uart2: Linux 虚拟机；

此时需要修改*prj.conf*中的多个配置项，如下：

.. code:: shell

      CONFIG_SHELL=n
      CONFIG_ZVM_DEBUG_LOG_INFO=n

同时，需要修改roc_rk3568_pc_smp.dts中的内容，将uart9的status改为disabled。

.. code:: dts

      &uart9 {
         status = "disabled";
         current-speed = <1500000>;
      };

最后，将uart3关闭，在*roc_rk3568_pc_smp.overlay*中将串口的label改为*PTUART3*。

.. code:: dts

      uart3: serial@fe670000 {
         compatible = "rockchip,rk3568-uart", "ns16550";
         reg = <0x0 0xfe670000 0x0 0x10000>;
         interrupts = <GIC_SPI 119 IRQ_TYPE_EDGE IRQ_DEFAULT_PRIORITY>;
         clocks = <&uartclk>;
         clock-frequency = <12000000>;
         reg-shift = <2>;
         status = "disabled";
         current-speed = <1500000>;
         label = "PTUART3";
      };

最后，编写自动启动虚拟机的代码，将其编译进zvm_host.bin中即可。

`Prev>> 在QEMU上运行ZVM <https://gitee.com/openeuler/zvm/blob/master/zvm_doc/3_Run_on_ARM64_QEMU.rst>`__
