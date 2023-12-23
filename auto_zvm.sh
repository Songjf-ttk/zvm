#!/bin/bash
# operation string and platform string
OPS=$1
PLAT=$2

ops_build="build"
ops_debug="debug"

plat_qemu="qemu"
plat_fvp="fvp"
plat_rk3568="rk3568"

if [[ "$1" != "$ops_build" && "$1" != "$ops_debug" ]]; then
    echo "Invalid build command. "
    echo "Please use either 'build' or 'debug'."
    exit 1
fi
if [[ "$2" != "$plat_qemu" && "$2" != "$plat_rk3568" && "$2" != "$plat_fvp" ]]; then
    echo "Invalid platform."
    echo "Please use either 'qemu', 'rk3568', or 'fvp'."
    exit 1
fi

# Build system
if [ "$OPS" = "$ops_build" ]; then
    rm -rf build/
    if [ "$PLAT" = "$plat_qemu" ]; then
        west build -b qemu_cortex_max_smp samples/_zvm
    elif [ "$PLAT" = "$plat_fvp" ]; then
        west build -b fvp_cortex_a55x4_a75x2_smp samples/_zvm \
        -DARMFVP_BL1_FILE=$(pwd)/zvm_config/fvp_platform/hub/bl1.bin \
        -DARMFVP_FIP_FILE=$(pwd)/zvm_config/fvp_platform/hub/fip.bin
    elif [ "$PLAT" = "$plat_rk3568" ]; then
        west build -b roc_rk3568_pc_smp samples/_zvm
        cp build/zephyr/zvm_host.bin /home/xiong/tftp_server
    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./auto_build.sh build qemu. "
    fi

elif [ "$OPS" = "$ops_debug" ]; then
    if [ "$PLAT" = "$plat_qemu" ]; then
        $(pwd)/zvm_config/qemu_platform/hub/qemu-system-aarch64 \
        -cpu max -m 4G -nographic -machine virt,virtualization=on,gic-version=3 \
        -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on \
        -serial chardev:con -mon chardev=con,mode=readline -serial pty -serial pty -smp cpus=4 \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/zephyr.bin,addr=0xf2000000,force-raw=on \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/Image,addr=0xf3000000,force-raw=on \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/linux-qemu-virt.dtb,addr=0xfb000000 \
        -kernel $(pwd)/build/zephyr/zvm_host.elf

### using gdb to connect it:
# gdb-multiarch -q -ex 'file ./build/zephyr/zvm_host.elf' -ex 'target remote localhost:1234'
# -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/Image,addr=0xf3000000,force-raw=on \
# -device loader,file=/home/xiong/linux_workspace/linux-5.10.200/arch/arm64/boot/Image,addr=0xf3000000,force-raw=on \

    elif [ "$PLAT" = "$plat_fvp" ]; then
#        /opt/arm/developmentstudio-2021.2/bin/FVP_Base_Cortex-A55x4+Cortex-A75x2 	\
        /path-to/FVP_Base_RevC-2xAEMvA \
        -C pctl.startup=0.0.0.* \
        -C bp.secure_memory=0 \
        -C cache_state_modelled=0 \
        -C bp.pl011_uart0.untimed_fifos=1 -C bp.pl011_uart0.unbuffered_output=1 -C bp.pl011_uart0.out_file=uart0.log \
        -C bp.pl011_uart1.out_file=uart1.log \
        -C bp.terminal_0.terminal_command=/usr/bin/gnome-terminal -C bp.terminal_0.mode=raw \
        -C bp.terminal_1.terminal_command=/usr/bin/gnome-terminal -C bp.terminal_1.mode=raw \
        -C bp.secureflashloader.fname=/path-to/bl1.bin \
        -C bp.flashloader0.fname=/path-to/fip.bin \
        -C bp.ve_sysregs.mmbSiteDefault=0 -C bp.ve_sysregs.exit_on_shutdown=1 \
        --data cluster0.cpu0=/path-to/zephyr.bin@0xa0000000 \
        --data cluster0.cpu0=/path-to/Image@0xb0000000 \
        --data cluster0.cpu0=/path-to/fvp-base-gicv3-psci.dtb@0xc0000000   \
        --cpulimit 4 \
        --iris-server
    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./z_auto.sh build qemu. "
    fi

fi
