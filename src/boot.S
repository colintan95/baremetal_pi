.global ".text.boot"

.global _start

_start:
  // The id of the core is stored in lowest bits of the register. Mask by the
  // first 2 bits since the pi only has 4 cores.
  mrs x0, mpidr_el1
  and x0, x0, 3

  // Puts all cores except core 0 in a busy wait.
  //
  // Based on testing only core 0 seems to be executing when we run this code,
  // but just have this check here in case.
  cbnz x0, wait

  // Switch to execution level (el) 1. Based on testing we will be at el2 at
  // this point.

  // Set el1 to AArch64.
  mov x0, #(1 << 31)
  msr hcr_el2, x0

  // Set return level of el2 to el1. Bit [3:0] - 0b100: el1 with sp_el0.
  mov x0, 4
  msr spsr_el2, x0

  // Set the return address of el2 to the el1 entry point.
  adr x0, el1_entry
  msr elr_el2, x0

  // Ensure all the above instructions have run.
  isb

  // Switch to el1.
  eret

el1_entry:
  // Start the stack just below the code segment.
  mov sp, 0x80000

  // CurrentEL should be 0x4 for el1.
  mrs x0, CurrentEL
  mov x1, 0x10000000
  str w0, [x1]

  // Bit 4 should be 0 for AArch64.
  mrs x0, spsr_el1
  mov x1, 0x01000000
  str w0, [x1]

  // Clear the bss.

  ldr x0, =_bss_start
  ldr w1, =_bss_num_stores

bss_clear:
  cbz w1, bss_clear_end

  str xzr, [x0]

  add x0, x0, 8
  sub w1, w1, 1

  b bss_clear

bss_clear_end:
  // Now start running main() in our C code.
  bl main

wait:
  wfe
  b wait
