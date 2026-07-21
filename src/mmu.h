#ifndef MMU_H_
#define MMU_H_

#include "stdint.h"

void mmu_map_device_mem(uint64_t virt_addr, uint64_t phys_addr);

#endif
