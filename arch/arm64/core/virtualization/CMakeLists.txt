# Copyright 2021-2022 HNU

# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
  hyp_entry.S
  hyp_vector.S
  cpu.c
  mm.c
  vtimer.c
  trap_handler.c
  switch.c
  cpu_irq.c
)


