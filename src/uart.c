#include "uart.h"

#include "interrupts.h"
#include "mmio.h"
#include "mbox.h"
#include "reg.h"

#define GPFSEL1 (MMIO_BASE + 0x200004)

#define UART_BASE (MMIO_BASE + 0x201000)

#define UART_DR (UART_BASE)
#define UART_FR (UART_BASE + 0x18)

// Baud rate divisor = clock rate / (16 * baud rate)
// Fractional part is 6 bits

// Baud rate divisor integer part
#define UART_IBRD (UART_BASE + 0x24)

// Baud rate divisor fractional part
#define UART_FBRD (UART_BASE + 0x28)

#define UART_LCRH (UART_BASE + 0x2c)
#define UART_CR (UART_BASE + 0x30)

// Interrupt mask set register
#define UART_IMSC (UART_BASE + 0x38)

// Interrupt clear register
#define UART_ICR (UART_BASE + 0x44)

void uart_init() {
  volatile unsigned int __attribute__((aligned(16))) buffer[9];

  buffer[0] = 9 * 4; // Size of buffer
  buffer[1] = MBOX_REQUEST;

  // Tag
  buffer[2] = 0x38002; // Set clock rate
  buffer[3] = 12; // Request length

  // Value
  buffer[4] = 0; // Request code
  buffer[5] = 2; // UART clock id
  buffer[6] = 4000000; // 4Mhz
  buffer[7] = 0; // No turbo

  // End tag
  buffer[8] = MBOX_TAG_LAST;

  mbox_call((unsigned int*)buffer, MBOX_CHANNEL_TAGS);

  unsigned int select = reg_read(GPFSEL1);

  // Set gpio 14 and 15 to alt0 - to use them for uart0.
  select &= ~((7 << 12) | (7 << 15));
  select |= (4 << 12) | (4 << 15);

  reg_write(GPFSEL1, select);

  // Set the baud rate to 115200
  // Baud rate divisor - 4MHz / (16 * 115200) = 2.170
  reg_write(UART_IBRD, 2);
  reg_write(UART_FBRD, 11);

  // Clear all interrupts - bits 0 to 10
  reg_write(UART_ICR, 0x7ff);

  // Enable UART transmit and receive
  reg_write(UART_CR, (1 << 0) | (1 << 8) | (1 << 9));

  // Enable the transmit and receive FIFOs and set the word length to 8 bits
  reg_write(UART_LCRH, (1 << 6) | (1 << 5) | (1 << 4));

  // Enable UART interrupts
  irq_enable_irq(IRQ_UART);

  // Enable receive timeout and receive FIFO interrupts
  reg_write(UART_IMSC, (1 << 6) | (1 << 4));
}

void uart_clear_interrupts() {
  // Clear the receive interrupts
  reg_write(UART_ICR, (1 << 6) | (1 << 4));
}

void uart_send(char c) {
  // Transmit FIFO full
  while (reg_read(UART_FR) & (1 << 5));

  reg_write(UART_DR, c);
}

int uart_read(char* c) {
  // Receive FIFO empty
  if (reg_read(UART_FR) & (1 << 4)) {
    return -1;
  }

  unsigned int data = reg_read(UART_DR);

  // The char to read is in bits 0 to 7
  *c = (char)(data & ((1 << 8) - 1));

  return 0;
}

void uart_print(const char* s) {
  while (*s != '\0') {
    // Add carriage return for newlines.
    if (*s == '\n') {
      uart_send('\r');
    }

    uart_send(*s);
    s++;
  }
}

static char nibble_to_char(int nibble) {
  if (nibble >= 0 && nibble <= 9) {
    return '0' + nibble;
  } else if (nibble >= 10 && nibble <= 15) {
    return 'A' + (nibble - 10);
  } else {
    return 26; // ASCII substitute character
  }
}

void uart_print_hex(unsigned int value) {
  static const int kNibbles = 8;
  int nibbles[kNibbles];

  int idx = 0;
  while (idx < kNibbles) {
    nibbles[idx] = value & ((1 << 4) - 1);
    value = value >> 4;

    idx++;
  }

  idx--;

  while (idx >= 0) {
    uart_send(nibble_to_char(nibbles[idx]));
    idx--;
  }
}

