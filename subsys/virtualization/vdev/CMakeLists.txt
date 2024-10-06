# SPDX-License-Identifier: Apache-2.0
# Copyright 2021-2022 HNU-ESNL

zephyr_sources_ifdef(
    CONFIG_ZVM
    vgic_common.c
    vgic_v3.c
    vpsci.c
)

# board specific configuration
zephyr_sources_ifdef(
    CONFIG_SOC_RK3568
    pt_device_roc_rk3568_pc.c
)

zephyr_sources_ifdef(
    CONFIG_SOC_QEMU_CORTEX_MAX
    pt_device_qemu_cortex_max.c
)

zephyr_sources_ifdef(
    CONFIG_VM_VIRTIO_MMIO
    virtio.c
    virtio_mmio.c
)

zephyr_sources_ifdef(
    CONFIG_VM_VIRTIO_BLOCK
    virtio_blk.c
)

zephyr_sources_ifdef(
    CONFIG_VM_FIQ_DEBUGGER
    fiq_debugger.c
)

zephyr_sources_ifdef(
    CONFIG_VM_CLOCK_SYSTEM_CONTROLLER
    virt_clock_syscon.c
)

zephyr_sources_ifdef(
    CONFIG_VM_SYSCON_CONTROLLER
    virt_syscon.c
)

zephyr_sources_ifdef(
    CONFIG_VM_SHMEM
    shmem.c
)

zephyr_sources_ifdef(
    CONFIG_VM_SHMEMRW
    shmem_rw.c
)
