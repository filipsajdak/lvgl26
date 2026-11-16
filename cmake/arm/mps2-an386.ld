/* Memory of QEMU's mps2-an386 board (an Arm Cortex-M4): 4 MB for code at 0, 4 MB of RAM. */
MEMORY
{
  CODE (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
  RAM  (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

ENTRY(reset)

SECTIONS
{
  .text :
  {
    KEEP(*(.vectors))
    *(.text .text.* .gnu.linkonce.t.*)
    *(.rodata .rodata.* .gnu.linkonce.r.*)
    KEEP(*(.init)) KEEP(*(.fini))
    . = ALIGN(4);
    __preinit_array_start = .; KEEP(*(.preinit_array)) __preinit_array_end = .;
    __init_array_start = .; KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array)) __init_array_end = .;
    __fini_array_start = .; KEEP(*(SORT(.fini_array.*))) KEEP(*(.fini_array)) __fini_array_end = .;
  } > CODE

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > CODE
  __exidx_start = .;
  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } > CODE
  __exidx_end = .;
  __data_load = .;

  .data : AT(__data_load)
  {
    __data_start = .;
    *(.data .data.* .gnu.linkonce.d.*)
    . = ALIGN(4);
    __data_end = .;
  } > RAM

  .bss (NOLOAD) :
  {
    __bss_start = .;
    __bss_start__ = .;
    *(.bss .bss.* .gnu.linkonce.b.*) *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
    __bss_end__ = .;
  } > RAM

  . = ALIGN(8);
  end = .;                     /* where newlib's heap starts */
  __stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
