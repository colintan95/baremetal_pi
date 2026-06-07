.global ".text.boot" 

.global _start

_start:
  mrs x1, mpidr_el1
  and x1, x1, 3

  cbnz x1, wait 

  // Set stack to below code 
  mov sp, 0x80000 

  bl main

wait:
  wfe
  b wait
