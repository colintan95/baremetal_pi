#include "reg.h"
#include "mmio.h"
#include "uart.h"

#define IRQ_BASE (MMIO_BASE + 0xb000)

#define IRQ_PENDING_BASIC (IRQ_BASE + 0x200)
#define IRQ_PENDING_1 (IRQ_BASE + 0x204)

#define IRQ_ENABLE_1 (IRQ_BASE + 0x210)
#define IRQ_ENABLE_2 (IRQ_BASE + 0x214)

#define IRQ_UART_PENDING (1 << 19)

// If there's a pending irq in register 1
#define IRQ_REG_1_PENDING (1 << 8)

// IRQ bits
#define IRQ_TIMER_MATCH_1_BIT (1 << 1)

#define TIMER_BASE (MMIO_BASE + 0x3000)

// Timer control/status
#define TIMER_CS (TIMER_BASE)

// Timer counter lower 32 bits
#define TIMER_CLO (TIMER_BASE + 0x4)

// Timer compare 1
#define TIMER_C0 (TIMER_BASE + 0x10)

// The timer should be running at 1MHz, so this should be 1 second
#define TIMER_POLL_CLOCKS 1000000

static void set_timer() {
  unsigned int counter = reg_read(TIMER_CLO);

  // Set the timer to fire in one second.
  counter += TIMER_POLL_CLOCKS;

  // Set the new counter - triggers irq when hit
  reg_write(TIMER_C0, counter);
}

void irq_handler() {
  unsigned int pending_basic = reg_read(IRQ_PENDING_BASIC);

  if (pending_basic & IRQ_UART_PENDING) {
    uart_clear_interrupts();

    char c;
    while (uart_read(&c) != -1) {
      // Echo back whatever that's typed
      uart_send(c);

      // Need to do this to print newlines
      if (c == '\r') {
        uart_send('\n');
      }
    }

  } else if (pending_basic & IRQ_REG_1_PENDING) {
    unsigned int pending = reg_read(IRQ_PENDING_1);

    if (pending & IRQ_TIMER_MATCH_1_BIT) {
      // Clear the irq
      reg_write(TIMER_CS, 2);

      // Set the next timer poll
      set_timer();
    }
  }
}

void timer_init() {
  // Enable the timer match irq
  reg_write(IRQ_ENABLE_1, IRQ_TIMER_MATCH_1_BIT);

  set_timer();
}

void irq_enable_irq(int irq) {
  if (irq < 32) {
    // IRQs 0 to 31 uses register 1
    reg_write(IRQ_ENABLE_1, 1 << irq);
  } else {
    // IRQs 32 to 63 uses register 2
    reg_write(IRQ_ENABLE_2, 1 << (irq - 32));
  }
}
