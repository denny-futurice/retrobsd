/*
 * Linker script for the RetroBSD m68k kernel on QEMU virt.
 *
 * QEMU loads the ELF image straight into RAM and jumps to the entry point, so
 * there is no ROM copy: text and data are linked and loaded where they run.
 * The kernel owns 0 to KERNEL_DATA_END (sys/m68k/machparam.h, D16):
 *
 *      0x00000000      exception vector table (256 vectors, VBR = 0)
 *      0x00000400      kernel text, rodata, data, bss
 *      ...             free
 *      u0              u area of process 0 / swap buffer (USIZE)
 *      u               u area of the running process (USIZE)
 *      u_end           kernel stack top = KERNEL_DATA_END
 *
 * QEMU places its bootinfo records just after the highest loaded address,
 * that is right after _end, so startup must read them before using that space.
 */
OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

KERNEL_DATA_START = 0x00000000;
KERNEL_DATA_END   = 0x00080000;
USIZE             = 4096;

u_end = KERNEL_DATA_END;
u     = u_end - USIZE;
u0    = u - USIZE;

PHDRS
{
  vectors PT_LOAD FLAGS (6);    /* RW: handlers may be installed at run time */
  text    PT_LOAD FLAGS (5);    /* RX */
  data    PT_LOAD FLAGS (6);    /* RW */
}

SECTIONS
{
  . = KERNEL_DATA_START;

  .vectors :
  {
    KEEP (*(.vectors))
    . = 0x400;
  } :vectors

  .text :
  {
    *(.text.startup .text.startup.*)
    *(.text .text.* .gnu.linkonce.t.*)
    *(.gnu.warning)
  } :text
  .rodata :
  {
    __rodata_start = .;
    *(.rodata .rodata.* .gnu.linkonce.r.*)
  }
  _etext = .;

  .data :
  {
    __data_start = .;
    *(.data .data.* .gnu.linkonce.d.*)
    . = ALIGN (4);
  } :data
  _edata = .;

  .bss :
  {
    __bss_start = .;
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN (4);
  }
  __bss_end = .;
  _end = .;

  ASSERT (SIZEOF (.vectors) == 0x400, "vector table overflows 0x400")
  ASSERT (_end <= u0, "kernel does not fit below the u areas")

  .comment        0 : { *(.comment) }
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
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
    *(.eh_frame) *(.gnu_debuglink)
    *(.interp) *(.dynamic) *(.dynsym) *(.dynstr) *(.hash) *(.gnu.hash)
    *(.got) *(.got.plt) *(.plt) *(.rela.*)
  }
}
