#!/bin/bash

# operation string and platform string
OPS=$1
PLAT=$2

ops_build="build"
ops_debug="debugserver"

plat_qemu="qemu"
plat_fvp="fvp"

# Build system
if [ "$OPS" = "$ops_build" ]; then
    rm -rf build/

    if [ "$PLAT" = "$plat_qemu" ]; then
        west build -b qemu_cortex_max_smp samples/_zvm
    elif [ "$PLAT" = "$plat_fvp" ]; then
        west build -b fvp_cortex_a55 samples/_zvm \
        -DARMFVP_BL1_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin \
        -DARMFVP_FIP_FILE=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin
    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./z_auto.sh build qemu. "
    fi

elif [ "$OPS" = "$ops_debug" ]; then

    if [ "$PLAT" = "$plat_qemu" ]; then

        /home/xiong/project/qemu/build/qemu-system-aarch64 -cpu max -nographic -machine virt,virtualization=on,gic-version=3 \
        -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on \
        -serial chardev:con -mon chardev=con,mode=readline -smp cpus=4 \
        -device loader,file=/home/yourpathname/ZephyrVisor/zephyr/build/zephyr/zephyr.bin,addr=0x45000000,force-raw=on \
        -kernel /home/xiong/zvm_new/zephyr/build/zephyr/zephyr.elf -s -S

    elif [ "$PLAT" = "$plat_fvp" ]; then

#        /opt/arm/developmentstudio-2021.2/bin/FVP_Base_Cortex-A55x4+Cortex-A75x2 	\
        /home/xiong/software/FVP_Base/Base_RevC_AEMvA_pkg/models/Linux64_GCC-9.3/FVP_Base_RevC-2xAEMvA \
        -C pctl.startup=0.0.0.* \
        -C bp.secure_memory=0 \
        -C cache_state_modelled=0 \
        -C bp.pl011_uart0.untimed_fifos=1 -C bp.pl011_uart0.unbuffered_output=1 -C bp.pl011_uart0.out_file=uart0.log \
        -C bp.pl011_uart1.out_file=uart1.log \
        -C bp.terminal_0.terminal_command=/usr/bin/gnome-terminal -C bp.terminal_0.mode=raw \
        -C bp.terminal_1.terminal_command=/usr/bin/gnome-terminal -C bp.terminal_1.mode=raw \
        -C bp.secureflashloader.fname=/home/xiong/trusted-firmware-a/build/fvp/release/bl1.bin \
        -C bp.flashloader0.fname=/home/xiong/trusted-firmware-a/build/fvp/release/fip.bin \
        -C bp.ve_sysregs.mmbSiteDefault=0 -C bp.ve_sysregs.exit_on_shutdown=1 \
        --data cluster0.cpu0=/home/xiong/zephyrproject/zephyr/build/zephyr/zephyr.bin@0xa0000000 \
        --data cluster0.cpu0=/home/xiong/project/fvp_platform/Image@0xb0000000 \
        --data cluster0.cpu0=/home/xiong/project/fvp_platform/fvp-base-gicv3-psci.dtb@0xc0000000   \
        --cpulimit 4 \
        --iris-server

    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./z_auto.sh build qemu. "
    fi

fi 