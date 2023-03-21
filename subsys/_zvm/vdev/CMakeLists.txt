# SPDX-License-Identifier: Apache-2.0
# Copyright 2021-2022 HNU


zephyr_sources_ifdef(
    CONFIG_ZVM
    uart.c
)
