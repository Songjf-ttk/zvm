#!/bin/bash
# operation string and platform string
OPS=$1
PLAT=$2

ops_array=("build" "debugserver")
plat_array=("qemu_cortex_max_smp" "fvp_cortex_a55x4_a75x2_smp" "roc_rk3568_pc_smp")

ops_found=false
for i in "${ops_array[@]}"
do
    if [ "$1" = "$i" ]; then
        ops_found=true
        break
    fi
done

if [ "$ops_found" = false ]; then
    echo "Invalid operation. Please use one of the following:"
    for index in "${!ops_array[@]}"; do
        echo "Argument $(($index+1)): ${ops_array[$index]}"
    done
    echo "For example: ./auto_zvm.sh ${ops_array[0]} ${plat_array[0]}"
    exit 1
fi

plat_found=false
for i in "${plat_array[@]}"
do
    if [ "$2" = "$i" ]; then
        plat_found=true
        break
    fi
done

if [ "$plat_found" = false ]; then
    echo "Invalid platform. Please use one of the following:"
    for index in "${!plat_array[@]}"; do
        echo "Argument $(($index+1)): ${plat_array[$index]}"
    done
    echo "For example: ./auto_zvm.sh ${ops_array[0]} ${plat_array[0]}"
    exit 1
fi

# Build system
if [ "$OPS" = "${ops_array[0]}" ]; then
    rm -rf build/
    if [ "$PLAT" = "${plat_array[0]}" ]; then
        west build -b qemu_cortex_max_smp samples/_zvm
    elif [ "$PLAT" = "${plat_array[1]}" ]; then
        west build -b fvp_cortex_a55x4_a75x2_smp samples/_zvm \
        -DARMFVP_BL1_FILE=$(pwd)/zvm_config/fvp_platform/hub/bl1.bin \
        -DARMFVP_FIP_FILE=$(pwd)/zvm_config/fvp_platform/hub/fip.bin
    elif [ "$PLAT" = "${plat_array[2]}" ]; then
        west build -b roc_rk3568_pc_smp samples/_zvm
    else
        echo "Error arguments for this auto.sh! \n Please input command like: ./auto_build.sh build qemu. "
    fi
# debug system
elif [ "$OPS" = "${ops_array[1]}" ]; then
    if [ "$PLAT" = "${plat_array[0]}" ]; then
        $(pwd)/zvm_config/qemu_platform/hub/qemu-system-aarch64 \
        -cpu max -m 8G -nographic -machine virt,virtualization=on,gic-version=3 \
        -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on \
        -serial chardev:con -mon chardev=con,mode=readline -serial pty -serial pty -smp cpus=4 \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/zephyr.bin,addr=0xc8000000,force-raw=on \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/Image_rfss,addr=0xe0000000,force-raw=on \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/linux-qemu-virt.dtb,addr=0xf2a00000 \
        -device loader,file=$(pwd)/zvm_config/qemu_platform/hub/debian.cpio.gz,addr=0xe4000000 \
        -kernel $(pwd)/build/zephyr/zvm_host.elf
### using gdb to connect it:
# gdb-multiarch -q -ex 'file ./build/zephyr/zvm_host.elf' -ex 'target remote localhost:1234'

    elif [ "$PLAT" = "${plat_array[1]}" ]; then
        /opt/arm/developmentstudio-2021.2/bin/FVP_Base_Cortex-A55x4+Cortex-A75x2 	\
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
