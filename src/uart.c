#include "uart.h"

#include "reg.h"
#include "mmio.h"
#include "mbox.h"

#define GPFSEL1 (MMIO_BASE + 0x200004)

#define AUX_BASE (MMIO_BASE + 0x215000)

#define AUX_ENABLES (AUX_BASE + 0x4)
#define AUX_MU_IO_REG (AUX_BASE + 0x40)
#define AUX_MU_LCR_REG (AUX_BASE + 0x4c)
#define AUX_MU_LSR_REG (AUX_BASE + 0x54)
#define AUX_MU_CNTL_REG (AUX_BASE + 0x60)
#define AUX_MU_BAUD_REG (AUX_BASE + 0x68)

#define UART_BASE (MMIO_BASE + 0x201000)

#define UART_DR (UART_BASE)
#define UART_FR (UART_BASE + 0x18)
#define UART_IBRD (UART_BASE + 0x24)
#define UART_FBRD (UART_BASE + 0x28)
#define UART_LCRH (UART_BASE + 0x2c)
#define UART_CR (UART_BASE + 0x30)
#define UART_ICR (UART_BASE + 0x44)

#define UART0 1

#ifdef UART0
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

  // Baud rate divisor = Clock rate / (16 * Baud rate)

  // Baud rate divisor integer part
  reg_write(UART_IBRD, 2);

  // Baud rate divisor fractional part
  // 6-bit fractional - round(remainder * 16 + 0.5)
  reg_write(UART_FBRD, 11);

  // Clear interrupts - see which bits need to actually be set.
  reg_write(UART_ICR, 0x7ff);

  // Bit 5 & 6 - 8 bit word length. Bit 4 - Enable transmit and receive FIFO.
  reg_write(UART_LCRH, (1 << 6) | (1 << 5) | (1 << 4));

  // Bit 0 - Enable UART. Bit 8 - Transmit enable. Bit 9 - Receive enable.
  reg_write(UART_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_send(char c) {
  // Bit 5 - Transmit FIFO full.
  while (reg_read(UART_FR) & (1 << 5));

  reg_write(UART_DR, c);
}
#else
void uart_init() {
  unsigned int enables = reg_read(AUX_ENABLES);

  // Enable uart1.
  enables |= 1;
  reg_write(AUX_ENABLES, enables);

  // Use 8-bit mode.
  reg_write(AUX_MU_LCR_REG, 3);

  // Set baud rate to 115200 at 250MHz.
  reg_write(AUX_MU_BAUD_REG, 270);

  unsigned int select = reg_read(GPFSEL1);

  // Set gpio 14 and 15 to alt5 - to use them for uart1.
  select &= ~((7 << 12) | (7 << 15));
  select |= (2 << 12) | (2 << 15);

  reg_write(GPFSEL1, select);

  // Enable transmitter and receiver.
  reg_write(AUX_MU_CNTL_REG, 3);
}

void uart_send(char c) {
  // Wait for transmitter to be idle.
  while (!(reg_read(AUX_MU_LSR_REG) & 0x20));

  // Write data to transmit FIFO.
  reg_write(AUX_MU_IO_REG, c);
}
#endif

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

