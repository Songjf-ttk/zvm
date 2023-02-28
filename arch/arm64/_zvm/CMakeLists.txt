# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  hyp_entry.S
  hyp_vector.S
  arm.c
  mm.c
  vgic_common.c
  vgic_v3.c
  vtimer.c
  trap_handler.c
  switch.c
  sysreg.c
)


