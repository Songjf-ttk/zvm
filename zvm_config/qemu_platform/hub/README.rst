仿真平台
==================
- qemu-system-aarch64: with zvm_config/qemu_platform/qemu/qemu-6.2.0-virt.c.patch


虚拟机镜像
==================

linux
------------------
- Image: v5.16 with simple filesystem made by busybox
- Image_oee: openEuler Embeded v23.09
- Image_debian: Debian v10
    - login:root 
    - passwd:123

zephyr
------------------
- zephyr.bin: Zephyr version 2.7.99

虚拟机设备树文件
==================

linux
------------------
- linux-qemu-virt.dtb: no virtio-mmio device
- linux-qemu-virtio.dtb: have ten virtio-mmio device