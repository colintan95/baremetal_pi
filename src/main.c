#include "reg.h"
#include "mmio.h"
#include "uart.h"

#define MBOX_BASE (MMIO_BASE + 0xb880)

#define MBOX_READ (MBOX_BASE)
#define MBOX_STATUS (MBOX_BASE + 0x18)
#define MBOX_WRITE (MBOX_BASE + 0x20)

#define MBOX_CHANNEL_TAGS 8

#define MBOX_TAG_LAST 0
#define MBOX_TAG_GET_SERIAL 0x10004

#define MBOX_REQUEST 0
#define MBOX_RESPONSE 0x80000000

#define MBOX_EMPTY 0x40000000
#define MBOX_FULL 0x80000000

// The buffer must be aligned to 16 bytes because we can only pass a 28-bit
// address to the mailbox.
int mailbox_call(unsigned int* buffer, unsigned char channel) {
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

void query_serial_number() {
  volatile unsigned int __attribute__((aligned(16))) buffer[8];

  buffer[0] = 8 * 4; // Buffer size in bytes
  buffer[1] = MBOX_REQUEST;

  // Tag
  buffer[2] = MBOX_TAG_GET_SERIAL; // Get board serial
  buffer[3] = 8; // Value buffer size in bytes 
  buffer[4] = 0; // Request code 

  // Value buffer
  buffer[5] = 0;
  buffer[6] = 0;

  // End tag
  buffer[7] = MBOX_TAG_LAST;

  if (mailbox_call((unsigned int*)buffer, MBOX_CHANNEL_TAGS) == 1) {
    uart_print_hex(buffer[6]);
    uart_print_hex(buffer[5]);

    uart_print("\n");
  }
}

unsigned long bus_to_physical_addr(unsigned long bus_addr) {
  // Physical address starts at 0x00000000 while bus address starts at
  // 0xc0000000. So convert bus address to physical by removing the top
  // 2 bits.
  return bus_addr & 0x3fffffff;
}

struct framebuf_fb {
  void* addr;

  unsigned int width;
  unsigned int height;

  unsigned int pitch;
};

int framebuf_init(struct framebuf_fb* fb) {
  volatile unsigned int __attribute__((aligned(16))) buffer[36];

  buffer[0] = 35 * 4; // Buffer size in bytes
  buffer[1] = MBOX_REQUEST;

  buffer[2] = 0x48003; // Set physical display width/height
  buffer[3] = 8;
  buffer[4] = 0;
  buffer[5] = 1024; // Width
  buffer[6] = 768; // Height

  buffer[7] = 0x48004; // Set virtual buffer width/height
  buffer[8] = 8;
  buffer[9] = 0;
  buffer[10] = 1024;
  buffer[11] = 768;

  buffer[12] = 0x48009; // Set virtual offset
  buffer[13] = 8;
  buffer[14] = 0;
  buffer[15] = 0; // x offset
  buffer[16] = 0; // y offset

  buffer[17] = 0x48005; // Set depth
  buffer[18] = 4;
  buffer[19] = 0;
  buffer[20] = 32; // Depth

  buffer[21] = 0x48006; // Set pixel order
  buffer[22] = 4;
  buffer[23] = 0;
  buffer[24] = 1; // RGB

  buffer[25] = 0x40001; // Get framebuffer and alignment
  buffer[26] = 8;
  buffer[27] = 0;
  buffer[28] = 4096; // Request: alignment, response: framebuffer address
  buffer[29] = 0; // Response: framebuffer size

  buffer[30] = 0x40008; // Get pitch 
  buffer[31] = 4;
  buffer[32] = 0;
  buffer[33] = 0; // Response: pitch

  buffer[34] = MBOX_TAG_LAST;

  if (mailbox_call((unsigned int*)buffer, MBOX_CHANNEL_TAGS) == 1) {
    if (buffer[28] != 0 && buffer[20] == 32) {
      fb->addr = (void*)bus_to_physical_addr(buffer[28]);

      fb->width = buffer[5];
      fb->height = buffer[6];

      fb->pitch = buffer[33];

      return 1;

    } else {
      uart_print("Could not set screen resolution.\n");
    }
  }

  return 0;
}

int main() {
  uart_init();

  uart_print("Hello, World!\n");

  struct framebuf_fb fb;
  
  int fb_valid = framebuf_init(&fb);

  if (fb_valid == 1) {
    for (int irow = 0; irow < fb.height; irow++) {
      for (int icol = 0; icol < fb.width; icol++) {
        char* row_ptr = (char*)fb.addr + irow * fb.pitch;

        char r = irow * 256 / fb.height;
        char g = icol * 256 / fb.width;
        char b = 0;

        unsigned int* pixel = (unsigned int*)row_ptr + icol; 
        *pixel = b << 16 | g << 8 | r;
      }
    }

    uart_print_hex(*(unsigned int*)fb.addr);
    uart_print("\n");
  } 

  query_serial_number();

  return 0;
}
