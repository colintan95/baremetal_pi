#include "mbox.h"

#include "reg.h"

#define MBOX_READ (MBOX_BASE)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_WRITE (MBOX_BASE + 0x20)

#define MBOX_EMPTY 0x40000000
#define MBOX_FULL 0x80000000

int mbox_call(unsigned int* buffer, char channel) {
  while (reg_read(MBOX_STATUS) & MBOX_FULL);

  // Raspberry pi only uses 32-bit addresses so this should be safe.
  unsigned int addr = (unsigned int)(unsigned long)buffer;

  // First 28 bits is the address to the buffer. Last 4 bits is the channel
  // number.
  unsigned int value = (addr & ~0xf) | (channel & 0xf);

  reg_write(MBOX_WRITE, value);

  while (1) {
    while (reg_read(MBOX_STATUS) & MBOX_EMPTY);

    if (reg_read(MBOX_READ) == value) {
      if (buffer[1] == MBOX_RESPONSE) {
        return 1;
      } else {
        return 0;
      }
    }
  }

  return 0;
}
