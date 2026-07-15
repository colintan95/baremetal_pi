#include "reg.h"
#include "mmio.h"

#define IRQ_BASE (MMIO_BASE + 0xb000)

#define IRQ_PENDING_BASIC_REG (IRQ_BASE + 0x200)

// If there's a pending irq in register 1
#define IRQ_PENDING_1_REG_BIT (1 << 8)

// For irqs 0 to 31
#define IRQ_PENDING_1_REG (IRQ_BASE + 0x204)

// Irq bits
#define IRQ_TIMER_MATCH_1_BIT (1 << 1)

#define TIMER_BASE (MMIO_BASE + 0x3000)

// System timer control/status
#define TIMER_CS_REG (TIMER_BASE)

void handle_irq() {
  unsigned int pending_basic = reg_read(IRQ_PENDING_BASIC_REG);

  if (pending_basic & IRQ_PENDING_1_REG_BIT) {
    unsigned int pending = reg_read(IRQ_PENDING_1_REG);

    if (pending & IRQ_TIMER_MATCH_1_BIT) {
      // Clear system timer match 1
      reg_write(TIMER_CS_REG, 2);

      reg_write(0x10000000, 42);
    }
  }
}
