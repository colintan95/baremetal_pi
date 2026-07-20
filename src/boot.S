.global ".text.boot"

.global _start

_start:
  // The core id is stored in the lowest bits. Since the pi only has 4 cores
  // we can just mask by the 2 bits to get the id
  mrs x0, mpidr_el1
  and x0, x0, 3

  // Only run core 0 for now. Based on testing only core 0 seems to be
  // executing at this point but have this here just in case
  cbnz x0, wait

  // Zero out all the page tables

  ldr w0, =0x00000000
  ldr w1, =0x00005000

1:
  cmp w0, w1
  b.ge 2f

  str xzr, [x0]
  add x0, x0, 8

  b 1b

2:
  // Virtual memory regions
  // 0x00000000 - 2MB of normal memory
  // 0x3f00b000 - IRQ, mailbox
  // 0x3f200000 - GPIO
  // 0x3f201000 - UART
  //
  // Page tables
  //
  // 0x00000000 - Level 0
  // - 0x0: 0x1 (Table)
  //
  // 0x00001000 - Level 1
  // - 0x0: 0x2 (Table)
  //
  // 0x00002000 - Level 2
  // - 0x000: 0x0 (Block) (Normal Mem)
  // - 0xFC0: 0x3 (Table) (Entry 504)
  // - 0xFC8: 0x4 (Table) (Entry 505)
  //
  // 0x00003000 - Level 3 (1)
  // - 0x58: 0x3f00b (Page) (Device Mem) (Entry 11)
  //
  // 0x00004000 - Level 3 (2)
  // - 0x0: 0x3f200 (Page) (Device Mem)
  // - 0x8: 0x3f201 (Page) (Device Mem) (Entry 1)

  // Layout for page table entries:
  // - Bit 42:12 - Physical address bits 12 to 48
  // - Bit 10    - 0b1: Don't fault on first access
  // - Bit 4:2   - Memory attributes index
  // - Bit 1     - 0b0: Block descriptor, 0b1: Table descriptor
  // - Bit 0     - 0b1: Active entry

  // Level 0 table

  ldr w0, =0x00000000 // Level 1 table entry

  ldr x1, =0x1403
  str x1, [x0]

  // Level 1 table

  ldr w0, =0x00001000 // Level 2 table entry

  mov x1, 0x2403
  str x1, [x0]

  // Level 2 table

  ldr w0, =0x00002000 // 2MB block entry for 0x00000000

  mov x1, 0x401
  str x1, [x0]

  ldr w0, =0x00002fc0 // Level 3 (1) table entry

  mov x1, 0x3403
  str w1, [x0]

  ldr w0, =0x00002fc8 // Level 3 (2) table entry

  mov x1, 0x4403
  str w1, [x0]

  // Level 3 table (1)

  ldr w0, =0x00003058 // IRQ, mailbox

  ldr x1, =0x3f00b407
  str w1, [x0]

  // Level 3 table (2)

  ldr w0, =0x00004000 // GPIO

  ldr x1, =0x3f200407
  str w1, [x0]

  ldr w0, =0x00004008 // UART

  ldr x1, =0x3f201407
  str w1, [x0]

  // Switch to el1. With the pi we would be at el2 at this point

  // Memory attributes
  // Index 0: Normal memory, inner cacheable, outer non-cacheable
  // Index 1: Device memory, nGnRnE
  mov x1, 0x4f
  msr mair_el1, x1

  // Set translation properties
  // Bit 34:32 - 0b101: 48 bits physical address size
  //
  // ttbr0:
  // Bit 15:14 - 0b00: 4KB granules
  // Bit 5:0   - Size offset: 16 (48-bit virtual addresses)
  ldr x0, =0x500000010
  msr tcr_el1, x0

  // Page table starts at address 0x00000000
  // Bit 47:1 - Table base address
  // Bit 0    - 0b1: Cores share translation tables
  mov x0, 1
  msr ttbr0_el1, x0

  mrs x0, sctlr_el1

  // Turn on the mmu and caches
  // Bit 12 - Enable instruction cache
  // Bit 2  - Enable data caches
  // Bit 0  - Enable MMU
  mov w1, 0x1005
  orr x0, x0, x1

  msr sctlr_el1, x0

  // Set to Aarch64 mode
  mov x0, #(1 << 31)
  msr hcr_el2, x0

  // Use sp_el0 for the stack pointer (el1t)
  mov x0, 4
  msr spsr_el2, x0

  // TODO: Set this to a better address
  // Set the stack for sp_el1 - this will be used when handling interrupts
  mov x0, 0x40000
  msr sp_el1, x0

  // Set the return address of el2 to the el1 entry point
  adr x0, 1f
  msr elr_el2, x0

  // Ensure all the above instructions have run
  isb

  // Switch to el1
  eret

1:
  // Start the stack just below the code segment - sp here should be sp_el0
  mov sp, 0x80000

  // Clear the bss

  ldr x0, =_bss_start
  ldr w1, =_bss_num_stores

1:
  cbz w1, 2f

  str xzr, [x0]

  add x0, x0, 8
  sub w1, w1, 1

  b 1b

2:
  // Install the interrupt handlers
  adr x0, vectors
  msr vbar_el1, x0

  // Now start running main() in our C code
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
