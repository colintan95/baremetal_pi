
#define MMIO_BASE 0x3f000000

#define GPFSEL1 (MMIO_BASE + 0x200004)

#define AUX_BASE (MMIO_BASE + 0x215000)

#define AUX_ENABLES (AUX_BASE + 0x4)
#define AUX_MU_IO_REG (AUX_BASE + 0x40)
#define AUX_MU_LCR_REG (AUX_BASE + 0x4c)
#define AUX_MU_LSR_REG (AUX_BASE + 0x54)
#define AUX_MU_CNTL_REG (AUX_BASE + 0x60)
#define AUX_MU_BAUD_REG (AUX_BASE + 0x68)

#define MBOX_BASE (MMIO_BASE + 0xb880)

#define MBOX_READ (MBOX_BASE)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_WRITE (MBOX_BASE + 0x20)

#define MBOX_CHANNEL_TAGS 8

#define MBOX_TAG_LAST 0
#define MBOX_TAG_GETSERIAL 0x10004

#define MBOX_REQUEST 0
#define MBOX_RESPONSE 0x80000000

#define MBOX_EMPTY 0x40000000
#define MBOX_FULL 0x80000000

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

void query_serial_number() {
  // Must be aligned to 16 bytes because we can only pass a 28-bit
  // address to the mailbox.
  volatile unsigned int __attribute__((aligned(16))) buffer[8];

  buffer[0] = 8 * 4; // Buffer size in bytes
  buffer[1] = MBOX_REQUEST;

  // Tag
  buffer[2] = MBOX_TAG_GETSERIAL; // Get board serial
  buffer[3] = 8; // Value buffer size in bytes 
  buffer[4] = 0; // Request code 

  // Value buffer
  buffer[5] = 0;
  buffer[6] = 0;

  // End tag
  buffer[7] = MBOX_TAG_LAST;

  while (mmio_read(MBOX_STATUS) & MBOX_FULL);

  // Raspberry pi only uses 32-bit addresses so this should be safe.
  unsigned int addr = (unsigned int)(unsigned long)&buffer;

  // First 28 bits is the address to the buffer. Last 4 bits is the
  // channel number.
  unsigned int value = (addr & ~0xf) | MBOX_CHANNEL_TAGS;  

  mmio_write(MBOX_WRITE, value);

  while (1) {
    while (mmio_read(MBOX_STATUS) & MBOX_EMPTY);  

    if (mmio_read(MBOX_READ) == value) {
      if (buffer[1] == MBOX_RESPONSE) {
        // Prints the serial address.
        uart_print_hex(buffer[6]);
        uart_print_hex(buffer[5]);

        uart_print("\n"); 
      } 

      return;
    }
  }
}

int main() {
  uart_init();

  uart_print("Hello, World!\n");

  query_serial_number();

  return 0;
}
