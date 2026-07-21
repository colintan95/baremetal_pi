#include "interrupts.h"
#include "mbox.h"
#include "mmio.h"
#include "mmu.h"
#include "reg.h"
#include "uart.h"

static void map_device_mem() {
  mmu_map_device_mem(0x3f00b000, 0x3f00b000); // IRQ, mailbox

  mmu_map_device_mem(0x3f200000, 0x3f200000); // GPIO
  mmu_map_device_mem(0x3f201000, 0x3f201000); // UART
}

int main() {
  map_device_mem();

  uart_init();

  uart_print("Hello, World!\n");

  uart_mem_dump(0x00004000, 8);
  uart_mem_dump(0x00005000, 8);

  return 0;
}
