#!/bin/bash

# operation string and platform string
ops=$1
plat=$2

ops_build="build"
ops_debug="debug"
plat_qemu="qemu"
plat_fvp="fvp"

# Build system
if [ "$ops" = "$ops_build" ]; then

rm -rf build/

if [ "$plat" = "$plat_qemu" ]; then
    west build -b qemu_cortex_max_smp samples/_zvm
else
if [ "$plat" = "$plat_fvp" ]; then
    west build -b fvp_cortex_a55 samples/_zvm -DARMFVP_BL1_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin -DARMFVP_FIP_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin
else
    echo "Error arguments for this auto.sh! Please input command like: ./z_auto.sh build qemu "
fi
fi
else

# Debug system(Just run on qemu)
if [ "$ops" = "$ops_debug" ]; then
    /home/xiong/zephyr-sdk-0.13.2/sysroots/x86_64-pokysdk-linux/usr/bin/qemu-system-aarch64 -cpu max -nographic -machine virt,virtualization=on,gic-version=3 -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on -serial chardev:con -mon chardev=con,mode=readline -s -S -smp cpus=4 -device loader,file=/home/yourpathname/ZephyrVisor/zephyr/build/zephyr/zephyr.bin,addr=0x45000000,force-raw=on -kernel /home/xiong/zvm_new/zephyr/build/zephyr/zephyr.elf
fi


fi
