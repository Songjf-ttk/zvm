# SPDX-License-Identifier: Apache-2.0
# Copyright 2021-2022 HNU

add_subdirectory(os)
add_subdirectory(tools)
add_subdirectory(vdev)

zephyr_sources_ifdef(
    CONFIG_ZVM
    vm_console.c
    vm_cpu.c
    vm_dev.c
    vm_irq.c
    vm_manager.c
    vm_mm.c
    vm.c
    zvm_shell.c
    zvm.c
)
