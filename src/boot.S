.global ".text.boot" 

.global _start

_start:
  mrs x0, mpidr_el1
  and x0, x0, 3

  cbnz x0, wait 

  // Set stack to below code 
  mov sp, 0x80000 

  // Clear the bss

  ldr x0, =_bss_start
  ldr w1, =_bss_num_stores

bss_clear:
  cbz w1, bss_clear_end

  str xzr, [x0] 

  add x0, x0, 8
  sub w1, w1, 1

  b bss_clear

bss_clear_end:
  bl main

wait:
  wfe
  b wait
