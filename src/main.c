#include "reg.h"
#include "mmio.h"
#include "uart.h"
#include "mbox.h"
#include "interrupts.h"

int main() {
  uart_init();

  // Init the timer after uart init since the timer prints using the uart.
  // timer_init();

  uart_print("Hello, World!\n");

  uart_mem_dump(0x00005000, 8);
  uart_mem_dump(0x00006000, 8);

  return 0;
}
