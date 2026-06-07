SECTIONS
{
  /* Raspberry Pi starts executing at 0x80000, so make sure the text secion starts there. */
  . = 0x80000;
  .text : { *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }

  /* bss is NOLOAD since it does not need to be loaded at runtime. */ 
  .bss (NOLOAD) : {
    /* Align to 8 bytes so we can zero the bss region with aligned stores. */
    . = ALIGN(8);
    _bss_start = .;

    *(.bss) *(.bss.*)

    . = ALIGN(8);
    _bss_end = .;
  }
  
  _bss_size = _bss_end - _bss_start;

  /* For convenience calculate the number of 8 byte stores we need here. */
  _bss_num_stores = _bss_size >> 3;
}
