# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Huang Jiajia
# Email: Huangjj2020@hnu.edu.cn
# Copyright (c) 2022 xcl (xiongcl@hnu.edu.cn)


zephyr_sources_ifdef(
    CONFIG_ZVM
    uart.c
)
