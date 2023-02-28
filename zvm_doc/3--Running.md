
# Running

## Arm FVP
#### Platform

- FVP Model: Cortex-A55x4_Cortex-A75x2(Installed with Arm DS2021)
- paramters: `-C bp.secure_memory=0 -C cache_state_modelled=0 -C pctl.startup=0.0.*.* -C bp.refcounter.non_arch_start_at_default=1 -C bp.terminal_0.mode=raw -C bp.terminal_1.mode=raw -C bp.terminal_2.mode=raw`

#### Load image
> Host: 'zvm_host.elf'

Load zvm_host.elf to address: 0x80000000.

> guest: 'zephyr.elf' and 'linux-system.axf'

Load zephyr.elf to address: 0xa0000000
Load linux-system.axf to address: 0xb0000000

#### Run ZVM
Open Arm DS2021 and click the `run` button



## QEMU
#### Platform

- QEMU model: board `virt` with cpu `max`
- Run command:
```shell
path-to/qemu-system-aarch64 -cpu max -m 4G -nographic -machine virt,virtualization=on,gic-version=3 -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on -serial chardev:con -mon chardev=con,mode=readline -serial pty -serial pty -smp cpus=4 -device loader,file=path-to/zephyr.bin,addr=0x48000000,force-raw=on -device loader,file=path-to/Image,addr=0x80000000,force-raw=on -device loader,file=path-to/virt.dtb,addr=0x88000000 -kernel path-to/zvm/zephyr/build/zephyr/zvm_host.elf 
```
> Warning: Due to the requriment that we should use multiple console. This qemu exec file has been modified. After input the command upon, we should to create another terminal and using `screen` to connect the zvm's second console. And then, we can using VM's console.
