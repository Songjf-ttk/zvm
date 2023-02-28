# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)


zephyr_sources_ifdef(
    CONFIG_ZVM_ELF_LOADER
    elfreloc_aarch64.c
    elfloader.c
)

zephyr_sources_ifdef(
    CONFIG_ZVM_TIME_MEASURE
    latency_measure.c
)
