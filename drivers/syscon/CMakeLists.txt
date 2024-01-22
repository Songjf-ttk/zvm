# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SYSCON          syscon.c)
zephyr_library_sources_ifdef(CONFIG_RK3568_SYSCON   rk3568_syscon.c)
