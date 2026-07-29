#include "mmu.h"

#define MMU_PAGES_BASE_ADDR 0x00004000
#define MMU_PAGE_SIZE 4096

struct mmu_page {
  uint64_t entries[MMU_PAGE_SIZE / 8];
};

static struct mmu_page* s_root_page = 0;

static uint64_t s_top_page_addr = MMU_PAGES_BASE_ADDR;

static void clear_mem(uint64_t addr, int size) {
  // TODO: Do this more efficiently (e.g. write 8 bytes at a time)
  while (size > 0) {
    *(unsigned char*)addr = 0;

    addr++;
    size--;
  }
}

static struct mmu_page* alloc_page() {
  uint64_t addr = s_top_page_addr;
  s_top_page_addr += MMU_PAGE_SIZE;

  clear_mem(addr, MMU_PAGE_SIZE);

  return (struct mmu_page*)addr;
}

// Virtual address bits
// - Level 0 - Bits 47:39
// - Level 1 - Bits 38:30
// - Level 2 - Bits 29:21
// - Level 3 - Bits 20:12

// Layout for page table entries:
// - Bit 47:12 - Physical address bits 12 to 47
// - Bit 10    - 0b1: Don't fault on first access
// - Bit 4:2   - Memory attributes index (in mair_el1)
// - Bit 1     - 0b0: Block descriptor, 0b1: Table descriptor
// - Bit 0     - 0b1: Active entry

uint64_t mmu_init() {
  // Allocate the page tables for the kernel's virtual memory. For now this is
  // only 2MB since allocating a 1GB page would eat into the peripherals
  // memory region at 0x3f00000000
  //
  // We map device memory later in main()

  struct mmu_page* level_0 = alloc_page();
  s_root_page = level_0;

  struct mmu_page* level_1 = alloc_page();
  struct mmu_page* level_2 = alloc_page();

  // Level 1 table
  level_0->entries[0] = (uint64_t)level_1 | 0x403;

  // Level 2 table
  level_1->entries[0] = (uint64_t)level_2 | 0x403;

  // 2MB block for VA 0x00000000 - 0x00200000
  level_2->entries[0] = 0x401; // PA 0x00000000

  // 2MB block for VA 0x10000000 - 0x10200000
  level_2->entries[128] = 0x401; // PA 0x00000000

  return (uint64_t)s_root_page;
}

static struct mmu_page* get_next_page(struct mmu_page* page, int entry_idx) {
  uint64_t entry = page->entries[entry_idx];

  uint64_t addr_mask = ((1ull << 36) - 1) << 12;
  uint64_t valid_mask = 1ull;

  struct mmu_page* next_page = 0;

  if ((entry & valid_mask) != 0) {
    // Table already exists
    uint64_t next_addr = entry & addr_mask;
    next_page = (struct mmu_page*)next_addr;

  } else {
    // Table not yet allocated. Allocate a new one and update the previous
    // page's entry
    next_page = alloc_page();
    page->entries[entry_idx] = (uint64_t)next_page | 0x403;
  }

  return next_page;
}

void mmu_map_device_mem(uint64_t virt_addr, uint64_t phys_addr) {
  uint64_t mask = (1 << 9) - 1;

  uint32_t lvl_0_idx = (virt_addr >> 39) & mask;
  uint32_t lvl_1_idx = (virt_addr >> 30) & mask;
  uint32_t lvl_2_idx = (virt_addr >> 21) & mask;
  uint32_t lvl_3_idx = (virt_addr >> 12) & mask;

  // Traverse the pages
  struct mmu_page* lvl_0_page = s_root_page;
  struct mmu_page* lvl_1_page = get_next_page(lvl_0_page, lvl_0_idx);
  struct mmu_page* lvl_2_page = get_next_page(lvl_1_page, lvl_1_idx);
  struct mmu_page* lvl_3_page = get_next_page(lvl_2_page, lvl_2_idx);

  // Set the level 3 entry
  lvl_3_page->entries[lvl_3_idx] = phys_addr | 0x407;
}
