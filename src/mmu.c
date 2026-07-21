#include "mmu.h"

#define MMU_PAGES_BASE_ADDR 0x00005000

#define MMU_PAGE_SIZE 4096

// TODO: Move into a stdint header
typedef unsigned long long uint64_t;

static void clear_mem(uint64_t addr, int size) {
  // TODO: Do this more efficiently (e.g. write 8 bytes at a time)
  while (size > 0) {
    *(unsigned char*)addr = 0;

    addr++;
    size--;
  }
}

// Virtual memory regions
// 0x00000000 - 2MB of normal memory
// 0x3f00b000 - IRQ, mailbox
// 0x3f200000 - GPIO
// 0x3f201000 - UART
//
// Page tables
//
// 0x00000000 - Level 0
// - 0x0: 0x1 (Table)
//
// 0x00001000 - Level 1
// - 0x0: 0x2 (Table)
//
// 0x00002000 - Level 2
// - 0x000: 0x0 (Block) (Normal Mem)
// - 0xFC0: 0x3 (Table) (Entry 504)
// - 0xFC8: 0x4 (Table) (Entry 505)
//
// 0x00003000 - Level 3 (1)
// - 0x58: 0x3f00b (Page) (Device Mem) (Entry 11)
//
// 0x00004000 - Level 3 (2)
// - 0x0: 0x3f200 (Page) (Device Mem)
// - 0x8: 0x3f201 (Page) (Device Mem) (Entry 1)

// Virtual address bits
// - Level 0 - Bits 47:39
// - Level 1 - Bits 38:30
// - Level 2 - Bits 29:21
// - Level 3 - Bits 20:12

// Layout for page table entries:
// - Bit 42:12 - Physical address bits 12 to 48
// - Bit 10    - 0b1: Don't fault on first access
// - Bit 4:2   - Memory attributes index
// - Bit 1     - 0b0: Block descriptor, 0b1: Table descriptor
// - Bit 0     - 0b1: Active entry

unsigned int mmu_init() {
  uint64_t base_addr = MMU_PAGES_BASE_ADDR;
  int page_size = MMU_PAGE_SIZE;

  uint64_t addr = base_addr;
  int num_pages = 5;

  clear_mem(addr, page_size * num_pages);

  uint64_t* level0_ptr = (uint64_t*)addr;
  addr += page_size;

  uint64_t* level1_ptr = (uint64_t*)addr;
  addr += page_size;

  uint64_t* level2_ptr = (uint64_t*)addr;
  addr += page_size;

  uint64_t* level3_1_ptr = (uint64_t*)addr;
  addr += page_size;

  uint64_t* level3_2_ptr = (uint64_t*)addr;
  addr += page_size;

  // Level 1 table entry
  *level0_ptr = (uint64_t)level1_ptr | 0x403;

  // Level 2 table entry
  *level1_ptr = (uint64_t)level2_ptr | 0x403;

  // 2 MB block entry for VA 0x00000000 - 0x00100000
  *level2_ptr = 0x401;

  // Level 3 (1) table entry
  *(level2_ptr + 504) = (uint64_t)level3_1_ptr | 0x403;

  // Level 3 (2) table entry
  *(level2_ptr + 505) = (uint64_t)level3_2_ptr | 0x403;

  // IRQ, mailbox
  *(level3_1_ptr + 11) = 0x3f00b407;

  // GPIO
  *level3_2_ptr = 0x3f200407;

  // UART
  *(level3_2_ptr + 1) = 0x3f201407;

  return base_addr;
}
