# SPDX-License-Identifier: Apache-2.0
# Copyright 2021-2022 HNU

add_subdirectory_ifdef(CONFIG_ZVM os)
add_subdirectory_ifdef(CONFIG_ZVM tools)
add_subdirectory_ifdef(CONFIG_ZVM vdev)

zephyr_sources_ifdef(
    CONFIG_ZVM
    vm_console.c
    vm_cpu.c
    vm_device.c
    vm_irq.c
    vm_manager.c
    vm_mm.c
    vm.c
    zvm.c
)

if(CONFIG_SHELL AND CONFIG_ZVM)
    zephyr_sources(zvm_shell.c)
endif()
