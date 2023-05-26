Running
=======

Arm FVP
-------

Platform
~~~~~~~~

-  FVP Model: Cortex-A55x4_Cortex-A75x2(Installed with Arm DS2021)
-  paramters:
   ``-C pctl.startup=0.0.0.* \
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
      --iris-server``

Load image
~~~~~~~~~~

   Host: ‘zvm_host.elf’

Load zvm_host.elf to address: 0x80000000.

   guest: ‘zephyr.bin’ and ‘linux-system.axf’

Load zephyr.bin to address: 0xa0000000 Load linux-system.axf to address: 0xb0000000

Run ZVM
~~~~~~~

Open Arm DS2021 and click the ``run`` button



QEMU
----

.. _platform-1:

Platform
~~~~~~~~

-  QEMU model: board ``virt`` with cpu ``max``
-  Run command:

.. code:: shell

   /path-to/qemu-system-aarch64 \
        -cpu max -m 4G -nographic -machine virt,virtualization=on,gic-version=3 \
        -net none -pidfile qemu.pid -chardev stdio,id=con,mux=on \
        -serial chardev:con -mon chardev=con,mode=readline -serial pty -serial pty -smp cpus=4 \
        -device loader,file=/path-to/zephyr.bin,addr=0x48000000,force-raw=on \
        -device loader,file=/path-to/Image,addr=0x80000000,force-raw=on \
        -device loader,file=/path-to/linux-qemu-virt.dtb,addr=0x88000000 \
        -kernel /path-to/zvm_host.elf -s -S

..

   Warning: Due to the requriment that we should use multiple console.
   This qemu exec file has been modified. After input the command upon,
   we should to create another terminal and using ``screen`` to connect
   the zvm’s second console. And then, we can using VM’s console.
