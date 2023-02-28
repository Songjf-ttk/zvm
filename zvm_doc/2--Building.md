# Building system

## For Arm FVP platform
### Build host system.

On path-to/zvm/zephyr/ dir：

For auto build the zvm, using z_auto.sh to build it.

```shell
./z_auto.sh build fvp
```

Or input below command:

```shell
west build -b fvp_cortex_a55 samples/_zvm -DARMFVP_BL1_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin -DARMFVP_FIP_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin
```

Eventually generate output files below:
> build/zephyr/zvm_host.elf

> build/zephyr/zvm_host.bin


### Building
#### Building Zephyr OS
On path-to/zephyr/ dir：

Supported board: fvp_base_revc_2xaemv8a

```shell
west build -b fvp_base_revc_2xaemv8a samples/subsys/shell/shell_module/ -DARMFVP_BL1_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin -DARMFVP_FIP_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin 
```

Eventually generate image files below:
> build/zephyr/zephyr.elf

> build/zephyr/zephyr.bin


#### Building Linux OS
1. Download Linux-5.16.12 or other version's kernel.
2. Build kernel.
```shell
# chose the debug info, the .config file that is show on path-to/zvm_configs/fvp_platform/.config_fvp
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
# build kernel, generate image in: ./zvm_configs/fvp_platform/Image
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image
```
3. Build dtb.
```shell
# build dtb file for linux os, the dts file is locate at path-to/zvm_configs/fdts/* and dtb file is on path-to/zvm_configs/fvp-base-gicv3-psci.dtb
$ dtc fdts/fvp-base-gicv3-psci.dts -I dts -O dtb > fvp-base-gicv3-psci.dtb
```
4. Build filesystem.
```shell
# build the filesystem and generate the filesystem image
# Using busybox to build it, ref: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/. Finally generate path-to/zvm_config/fvp_platform/initramfs.cpio.gz
```
5. Build linux image.
```shell
# using boot-wrapper to build linux image.
$ wget https://git.kernel.org/pub/scm/linux/kernel/git/mark/boot-wrapper-aarch64.git/snapshot/boot-wrapper-aarch64-ed60963595855e66ffc06a8a543cbb429c7ede03.tar.gz
$ tar -xf boot-wrapper-aarch64-ed60963595855e66ffc06a8a543cbb429c7ede03.tar.gz
$ cd boot-wrapper-aarch64-ed60963595855e66ffc06a8a543cbb429c7ede03/
$ autoreconf -i
$ ./configure --enable-psci --enable-gicv3 --with-kernel-dir=path-to/linux-5.16.12/ --with-dtb=path-to/fvp-base-gicv3-psci.dtb --with-initrd=path-to/initramfs.cpio.gz --host=aarch64-linux-gnu
$ make

# And the final generated image file: linux-system.axf
```

## For QEMU platform
### Host Compile

For auto build the zvm, using z_auto.sh to build it.
```shell
./z_auto.sh build qemu
```

On path-to/zvm/zephyr/ dir
```shell
west build -b qemu_cortex_max_smp samples/_zvm/
```
Generated image files below:
> build/zephyr/zvm_host.elf

> build/zephyr/zvm_host.bin


### Guest Compile
#### zephyr os:
on path-to/zephyr dir：
```shell
west build -b qemu_cortex_a53 samples/subsys/shell/shell_module/
```

Generate image files below:
> build/zephyr/zephyr.elf

> build/zephyr/zephyr.bin


#### Linux os:
1. Build dtb.
```shell
# build dtb file for linux os, the dts file is locate at ./zvm_configs/qemu_platform/virt.dts and dtb file is on ./z_configs/qemu_platform/virt.dtb
$ dtc virt.dts -I dts -O dtb > virt.dtb
```
2. Build filesystem.
```shell
# build the filesystem and generate the filesystem image
# Using busybox to build it, ref: https://consen.github.io/2018/01/17/debug-linux-kernel-with-qemu-and-gdb/. Finally generate ./zvm_config/qemu_platform/initramfs.cpio.gz
```
3. Build kernel.
```shell
# chose the debug info, the .config file that is show on ./zvm_configs/qemu_platform/.config_qemu
# add filesystem's *.cpio.gz file to kernel image by chosing it in menuconfig.
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
# build kernel
$ make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image
```
