/*
 * Copyright 2025 Syswonder
 * SPDX-License-Identifier: MulanPSL-2.0
 */

#include "arch.h"
#include "generated/autoconf.h"

#if defined(CONFIG_TARGET_ARCH_AARCH64)
extern struct arch_ops aarch64_ops;
#define ARCH_OPS &aarch64_ops
#elif defined(CONFIG_TARGET_ARCH_LOONGARCH64)
extern struct arch_ops loongarch64_ops;
#define ARCH_OPS &loongarch64_ops
#elif defined(CONFIG_TARGET_ARCH_RISCV64)
extern struct arch_ops riscv64_ops;
#define ARCH_OPS &riscv64_ops
#else
#error "Unsupported target architecture"
#endif

#if !defined(CONFIG_ENABLE_SERIAL)
static void arch_serial_init_noop(void) {}
static void arch_put_char_noop(char c) { (void)c; }
static void arch_get_char_noop(char *c) { (void)c; }
#endif

struct arch_ops *arch_ops = NULL;

void arch_detect_and_init(void) {
  arch_ops = ARCH_OPS;
  ARCH_EARLY_INIT();
  ARCH_INIT();
  ARCH_MEMORY_INIT();
  ARCH_SERIAL_INIT();
#if !defined(CONFIG_ENABLE_SERIAL)
  arch_ops->serial.init = arch_serial_init_noop;
  arch_ops->serial.put_char = arch_put_char_noop;
  arch_ops->serial.get_char = arch_get_char_noop;
#endif
}