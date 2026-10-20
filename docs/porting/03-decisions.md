# Decision record

Short records of porting decisions. Newest last. Status: Accepted unless noted.

## D1: Use vanilla (upstream) GCC, not LLVM or the Amiga GCC fork
- **Context:** LLVM's M68k backend is not sufficient for typical user-mode code
  at present. The bebbo `amiga-gcc` fork adds AmigaOS-specific behaviour
  (Hunk output, libnix/ixemul specs, register-argument attributes) that this
  port does not need.
- **Decision:** Use stock GCC's m68k backend from Debian.
- **Consequences:** `target.mk` and `Makefile.kconf` need a GCC variant;
  Clang-only flags are dropped.

## D2: Cross-compile with `m68k-linux-gnu-` as a freestanding toolchain
- **Context:** Debian packages `m68k-linux-gnu` only; there is no `m68k-elf`
  package.
- **Decision:** Use `m68k-linux-gnu-gcc` with `-ffreestanding -nostdlib
  -nostdinc -fno-pic -fno-pie -fno-stack-protector`, static linking and our own
  linker scripts and `crt0`. Build `m68k-elf` from source only if this proves
  problematic.
- **Consequences:** Linux-specific defaults must be neutralised with flags.

## D3: Kernel is libc-free; userland uses the RetroBSD libc
- **Decision:** No external libc. Soft float and 64-bit helpers come from
  `src/libc/runtime/`, not the Linux-built `libgcc`.

## D4: Build host is Debian Testing

## D5: CPU is 68030, no MMU use
- **Decision:** Target `-mcpu=68030` but keep RetroBSD's current no-MMU model
  (fixed user region, whole-process swapping, no inter-process protection).
- **Consequences:** Simplest port. PMMU protection is a possible later upgrade.

## D6: First (and for now only) platform is QEMU `virt`
- **Decision:** `qemu-system-m68k -M virt -cpu m68030`. Fall back to `m68040`
  if the 030 is rejected.
- **Devices:** Goldfish TTY, PIC and timer; virtio-mmio block.

## D7: Amiga hardware is out of scope
- **Context:** Too far downstream of the current backlog.
- **Decision:** No Amiga work. Keep board code behind the kconfig board
  abstraction (`sys/m68k/<board>/`) so another board can be added later.

## D8: Native on-target compilers and tools are out of scope
- **Decision:** `as`, `ld`, `cc`, `adb`, `aout`, PCC/`ccom`, `lcc`, `lccom`,
  `smlrc` and similar are not ported. They are excluded from the m68k build.
  All code is cross-built on the host.

## D9: Keep the 2.11BSD filesystem format; big-endian on disk
- **Status:** Proposed; to be confirmed during the M4 endianness audit.
- **Decision:** Retain the existing block and inode structures, with host tools
  (`fsutil`, `elf2aout`) made endian-correct.
