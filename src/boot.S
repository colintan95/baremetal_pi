.global ".text.boot" 

.global _start

_start:
  mrs x1, mpidr_el1
  and x1, x1, 3

  cbz x1, main

wait:
  wfe
  b wait

main:
  mov w0, #0x5000
  movk w0, #0x3f21, lsl #16

  // AUX_ENABLE
  add x1, x0, #0x4
  mov w2, 1
  str w2, [x1]

  // AUX_MU_LCR_REG
  add x1, x0, #0x4c
  mov w2, 3
  str w2, [x1]

  // AUX_MU_BAUD_REG
  add x1, x0, #0x68
  mov w2, 270
  str w2, [x1]

  // GPFSEL1
  mov w1, #0x0004
  movk w1, #0x3f20, lsl #16
  ldr w2, [x1]

  // GPIO14, GPIO15
  and w2, w2, -258049
   
  // ALT5
  mov w3, 8192
  movk w3, 0x1, lsl #16
  orr w2, w2, w3

  str w2, [x1]

  // GPPUD
  mov w1, #0x0094
  movk w1, #0x3f20, lsl #16
  mov w2, wzr
  str w2, [x1]

  // AUX_MU_CNTL_REG
  add x1, x0, #0x60
  mov w2, 3
  str w2, [x1]

  // AUX_MU_LSR_REG
  add x1, x0, #0x54 

loop:
  ldr w2, [x1]
  and w2, w2, #0x20
  cbz w2, loop

  // AUX_MU_IO_REG
  add x1, x0, #0x40
  mov w2, 65 
  str w2, [x1]

  b wait 
