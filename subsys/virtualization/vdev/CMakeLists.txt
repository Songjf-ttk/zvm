# SPDX-License-Identifier: Apache-2.0
# Copyright 2021-2022 HNU-ESNL

zephyr_sources_ifdef(
    CONFIG_ZVM
    vgic_common.c
    vgic_v3.c
    serial.c
    virt_device.c
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
