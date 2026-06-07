
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
  // Wait for transmitter to be idle.
  while (!(mmio_read(AUX_MU_LSR_REG) & 0x20));

  // Write data to transmit FIFO.
  mmio_write(AUX_MU_IO_REG, c);
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

char nibble_to_char(int nibble) {
  if (nibble >= 0 && nibble <= 9) {
    return '0' + nibble;
  } else if (nibble >= 10 && nibble <= 15) {
    return 'A' + (nibble - 10);
  } else {
    return 26; // ASCII substitute character
  } 
}

void uart_print_hex(unsigned int value) {
  uart_send('0');
  uart_send('x');

  // TODO: Make the size a constant.
  int nibbles[4];

  int idx = 0;
  while (idx < 4) {
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

void uart_print_char_hex(char value) {
  static const int kMask = (1 << 4) - 1;

  int nibble0 = value & kMask; 
  value = value >> 4;

  int nibble1 = value & kMask;
  
  uart_send(nibble_to_char(nibble1));
  uart_send(nibble_to_char(nibble0));   
}

void uart_dump(void* mem, int size) {
  char* ptr = (char*)mem;

  while (size > 0) {
    uart_print_char_hex(*ptr);
    ptr++;

    size--;
  }
}

extern char _bss_start;

int main() {
  uart_init();

  uart_print("Hello, World!\n");

  char* bss_start = (char*)&_bss_start - 8;
  uart_dump(bss_start, 32);

  uart_print("\n");

  return 0;
}
