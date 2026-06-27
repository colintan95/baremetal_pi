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

  // Set the return address of el2 to the el1 entry point.
  adr x0, 1f
  msr elr_el2, x0

  // TODO: Remove after testing is done for interrupt handling.
  mov w0, 0x01000000
  str xzr, [x0];

  // Ensure all the above instructions have run.
  isb

  // Switch to el1.
  eret

1:
  // Start the stack just below the code segment.
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

  // 0x3f00b210 - interrupt enable register 1.
  mov w0, 0xb210
  movk w0, 0x3f00, lsl 16

  // Bit 1: enable irq 1 - system timer match 1.
  mov w1, 2
  str w1, [x0]

  // 0x3f003004 - system timer counter lower 32 bits.
  mov w0, 0x3004
  movk w0, 0x3f00, lsl 16

  // Get the current timer counter.
  ldr w1, [x0]

  mov w0, 0x02000000
  str w1, [x0]

  // Increment the counter value.
  mov w2, 0x2000
  add w1, w1, w2

  // 0x3f003010 - system timer compare 1.
  mov w0, 0x3010
  movk w0, 0x3f00, lsl 16

  // Store the new counter in the compare register - trigger irq when hit
  str w1, [x0]

  // Now start running main() in our C code.
  bl main

wait:
  wfe
  b wait

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

  // IRQ
  .align 7

  // 0x3f003000 - system timer control/status
  mov w0, 0x3000
  movk w0, 0x3f00, lsl 16

  // Bit 1: clear system timer match 1
  mov w1, 2
  str w1, [x0]

  // TODO: Remove after testing
  mov w0, 0x01000000
  mov w1, 42
  str w1, [x0]

  eret

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
