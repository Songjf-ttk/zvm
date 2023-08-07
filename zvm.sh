zvm_config/qemu_platform/hub/qemu-system-aarch64 \
    -cpu max -m 4G -nographic -machine virt,virtualization=on,gic-version=3 \
    -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on -serial chardev:con \
    -mon chardev=con,mode=readline -serial pty -serial pty -smp cpus=4 \
    -device loader,file=zvm_config/qemu_platform/hub/zephyr.bin,addr=0xf2000000,force-raw=on \
    -device loader,file=zvm_config/qemu_platform/hub/Image,addr=0xf3000000,force-raw=on \
    -device loader,file=zvm_config/qemu_platform/hub/linux-qemu-virt.dtb,addr=0xfb000000 \
    -kernel build/zephyr/zvm_host.elf 