/*
 * Linker script for RetroBSD m68k user programs.
 *
 * There is no MMU: every program is linked at the fixed USER_DATA_START address
 * from sys/m68k/machparam.h (D16).  tools/elf2aout turns the ELF into an OMAGIC
 * a.out, and the kernel (exec_aout.c) reads text and data in one piece to
 * USER_DATA_START, so text, data and bss must be contiguous and in that order.
 * The stack grows down from USER_DATA_END.
 */
OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

USER_DATA_START = 0x00080000;
USER_DATA_END   = 0x000c0000;

PHDRS
{
  text PT_LOAD FLAGS (5);       /* RX: becomes a.out text */
  data PT_LOAD FLAGS (6);       /* RW: becomes a.out data and bss */
}

SECTIONS
{
  . = USER_DATA_START;

  .text :
  {
    *(.text.startup .text.startup.*)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.gnu.warning)
  } :text
  .rodata :
  {
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }
  .eh_frame : { KEEP (*(.eh_frame)) }
  PROVIDE (_etext = .);
  PROVIDE (etext = .);

  .data :
  {
    __data_start = .;
    *(.data .data.* .gnu.linkonce.d.*)
    SORT (CONSTRUCTORS)
  } :data
  PROVIDE (_edata = .);
  PROVIDE (edata = .);

  .bss :
  {
    __bss_start = .;
    *(.dynbss)
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (4);
  }
  _end = .;
  PROVIDE (end = .);

  ASSERT (_end <= USER_DATA_END, "program does not fit in the user region")

  /* Stabs and DWARF debugging sections, kept for gdb; elf2aout ignores them. */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .comment       0 : { *(.comment) }
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  .debug_ranges   0 : { *(.debug_ranges) }
  .debug_line_str 0 : { *(.debug_line_str) }
  .debug_loclists 0 : { *(.debug_loclists) }
  .debug_rnglists 0 : { *(.debug_rnglists) }
  .debug_addr     0 : { *(.debug_addr) }
  .debug_str_offsets 0 : { *(.debug_str_offsets) }

  /DISCARD/ :
  {
    *(.note.GNU-stack) *(.note.gnu.build-id) *(.note.*)
    *(.gnu_debuglink) *(.gnu.lto_*)
    *(.interp) *(.dynamic) *(.dynsym) *(.dynstr) *(.hash) *(.gnu.hash)
    *(.got) *(.got.plt) *(.plt) *(.rela.*)
  }
}
