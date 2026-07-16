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

  // Bit 31 - 1: el1 execute at aarch64
  mov x0, #(1 << 31)
  msr hcr_el2, x0

  // Bit 4:0 - 0b100: return to el1 with sp_el0 (el1t)
  mov x0, 4
  msr spsr_el2, x0

  // TODO: Assign this to a better address
  // Set the stack for sp_el1 - this will be used when handling interrupts
  mov x0, 0x40000
  msr sp_el1, x0

  // Set the return address of el2 to the el1 entry point.
  adr x0, 1f
  msr elr_el2, x0

  // Ensure all the above instructions have run.
  isb

  // Switch to el1.
  eret

1:
  // Start the stack just below the code segment - sp here should be sp_el0
  mov sp, 0x80000

  // Clear the bss.

  ldr x0, =_bss_start
  ldr w1, =_bss_num_stores

1:
  cbz w1, 2f

  str xzr, [x0]

  add x0, x0, 8
  sub w1, w1, 1

  b 1b

2:
  // Install the interrupt handlers.
  adr x0, vectors
  msr vbar_el1, x0

  // Now start running main() in our C code.
  bl main

wait:
  wfe
  b wait

// TODO: Test out the saving and restoring of registers

.macro handler_start
  // Mask all interrupts when executing the handler
  msr daifset, 0b1111

  // When handling an interrupt, save all the registers onto the stack
  sub sp, sp, 16 * 16
  stp x0, x1, [sp]
  stp x2, x3, [sp, 16]
  stp x4, x5, [sp, 16 * 2]
  stp x6, x7, [sp, 16 * 3]
  stp x8, x9, [sp, 16 * 4]
  stp x10, x11, [sp, 16 * 5]
  stp x12, x13, [sp, 16 * 6]
  stp x14, x15, [sp, 16 * 7]
  stp x16, x17, [sp, 16 * 8]
  stp x18, x19, [sp, 16 * 9]
  stp x20, x21, [sp, 16 * 10]
  stp x22, x23, [sp, 16 * 11]
  stp x24, x25, [sp, 16 * 12]
  stp x26, x27, [sp, 16 * 13]
  stp x28, x29, [sp, 16 * 14]
  str x30, [sp, 16 * 15]
.endm

.macro handler_end
  // Restores all registers
  ldp x0, x1, [sp]
  ldp x2, x3, [sp, 16]
  ldp x4, x5, [sp, 16 * 2]
  ldp x6, x7, [sp, 16 * 3]
  ldp x8, x9, [sp, 16 * 4]
  ldp x10, x11, [sp, 16 * 5]
  ldp x12, x13, [sp, 16 * 6]
  ldp x14, x15, [sp, 16 * 7]
  ldp x16, x17, [sp, 16 * 8]
  ldp x18, x19, [sp, 16 * 9]
  ldp x20, x21, [sp, 16 * 10]
  ldp x22, x23, [sp, 16 * 11]
  ldp x24, x25, [sp, 16 * 12]
  ldp x26, x27, [sp, 16 * 13]
  ldp x28, x29, [sp, 16 * 14]
  ldr x30, [sp, 16 * 15]
  add sp, sp, 16 * 16

  // Unmask interrupts
  msr daifclr, 0b1111
.endm

irq_handler_el1_sp_el0:
  handler_start

  bl irq_handler

  handler_end
  eret

  // Vector table for interrupt handlers.
  //
  // Table must be aligned to 2048 bytes, and each entry must be aligned to
  // 128 bytes.

  // Make sure the .align comes before the label else the label will point
  // to the address before the alignment is applied.
  .align 11

vectors:
  // Handlers for el1 with sp_el0

  .align 7
  eret

  .align 7
  b irq_handler_el1_sp_el0

  .align 7
  eret

  .align 7
  eret

  // Handlers for el1 with sp_el1

  .align 7
  eret

  .align 7
  eret

  .align 7
  eret

  .align 7
  eret

  // Handlers from lower el - aarch64

  .align 7
  eret

  .align 7
  eret

  .align 7
  eret

  .align 7
  eret

  // Handlers from lower el - aarch32

  .align 7
  eret

  .align 7
  eret

  .align 7
  eret

  .align 7
  eret
