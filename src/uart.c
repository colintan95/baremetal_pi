#include "uart.h"

#include "reg.h"
#include "mmio.h"

#define GPFSEL1 (MMIO_BASE + 0x200004)

#define AUX_BASE (MMIO_BASE + 0x215000)

#define AUX_ENABLES (AUX_BASE + 0x4)
#define AUX_MU_IO_REG (AUX_BASE + 0x40)
#define AUX_MU_LCR_REG (AUX_BASE + 0x4c)
#define AUX_MU_LSR_REG (AUX_BASE + 0x54)
#define AUX_MU_CNTL_REG (AUX_BASE + 0x60)
#define AUX_MU_BAUD_REG (AUX_BASE + 0x68)

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

