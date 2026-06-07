
#define MMIO_BASE 0x3f000000

#define GPFSEL1 (MMIO_BASE + 0x200004)

#define AUX_BASE (MMIO_BASE + 0x215000)

#define AUX_ENABLES (AUX_BASE + 0x4)
#define AUX_MU_IO_REG (AUX_BASE + 0x40)
#define AUX_MU_LCR_REG (AUX_BASE + 0x4c)
#define AUX_MU_LSR_REG (AUX_BASE + 0x54)
#define AUX_MU_CNTL_REG (AUX_BASE + 0x60)
#define AUX_MU_BAUD_REG (AUX_BASE + 0x68)

void mmio_write(long reg, unsigned int value) {
  *(volatile unsigned int*)reg = value;
}

unsigned int mmio_read(long reg) {
  return *(volatile unsigned int*)reg;
}

void uart_init() {
  unsigned int enables = mmio_read(AUX_ENABLES);

  // Enable uart1.
  enables |= 1;
  mmio_write(AUX_ENABLES, enables);

  // Use 8-bit mode.
  mmio_write(AUX_MU_LCR_REG, 3);

  // Set baud rate to 115200 at 250MHz.
  mmio_write(AUX_MU_BAUD_REG, 270);

  unsigned int select = mmio_read(GPFSEL1);
  
  // Set gpio 14 and 15 to alt5 - to use them for uart1. 
  select &= ~((7 << 12) | (7 << 15));
  select |= (2 << 12) | (2 << 15);

  mmio_write(GPFSEL1, select);

  // Enable transmitter and receiver.
  mmio_write(AUX_MU_CNTL_REG, 3);
}

void uart_send(char c) {
  while (!(mmio_read(AUX_MU_LSR_REG) & 0x20));

  mmio_write(AUX_MU_IO_REG, c);
}

void uart_print(const char* str) {
  while (*str != '\0') {
    // Add carriage return for newlines.
    if (*str == '\n') {
      uart_send('\r');
    }

    uart_send(*str);
    str++;
  }
}

int main() {
  uart_init();

  uart_send('A');

  uart_send('\r');
  uart_send('\n');

  return 0;
}
