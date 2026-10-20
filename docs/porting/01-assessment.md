# RetroBSD → 68030 on QEMU `virt`: feasibility assessment

**Scope:** vanilla GCC on Debian, a freestanding kernel, RetroBSD's own libc for
userland, no-MMU operation (on a 68030), QEMU `virt` bring-up only.

Out of scope: Amiga hardware, and the native on-target compilers and tools
(`as`, `ld`, `cc`, `adb`, `aout`, PCC/`ccom`, `lcc`, `lccom`, `smlrc`, ...).
Everything is cross-built on the host.

This is a desk assessment based on a survey of the repository (file sizes and
structure). Nothing has been built yet.

**Verdict: viable and well bounded.** A minimal port that boots to a shell on
QEMU is estimated at about 2-3 person-months.

## 1. Toolchain (low risk)

- Debian provides `gcc-*-m68k-linux-gnu`, `binutils-m68k-linux-gnu` and
  `qemu-system-m68k`. It does not provide an `m68k-elf` target, so
  `m68k-linux-gnu-gcc` is used as a freestanding cross compiler.
- Flags: `-mcpu=68030 -ffreestanding -nostdlib -nostdinc -fno-pic
  -fno-pie -fno-stack-protector -std=gnu99`; link with `-static -no-pie`.
  Kernel code also uses `-fno-builtin`.
- Floating point uses the hardware 68881/68882 FPU (D12); soft float is
  rejected, so the FP code in `src/libc/runtime/` is not used. The 64-bit
  integer helpers (`__divdi3` etc.) in `runtime/` are still needed, and the
  Linux-built `libgcc` is not used.
- Build system: add a `CROSS = m68k-linux-gnu-` variant of `target.mk` and
  `sys/pic32/Makefile.kconf`, replacing Clang-only options and
  `clang-config.mk`; add an ELF m68k linker script. Expect a first pass of
  `-Werror` fixes for old K&R-style code under GCC.
- `tools/elf2aout` needs big-endian ELF32 and EM_68K support (about 800 lines,
  mechanical).

## 2. Kernel (medium risk)

- The machine-independent kernel (`sys/kernel/`, about 18k lines, 2.11BSD
  heritage) is already 16/32-bit and big-endian friendly.
- A new `sys/m68k/` machine-dependent layer of about 3-4k lines replaces
  `machdep.c`, `startup.S`, `exception.c` and `machparam.h`:
  - vector table, supervisor/user stacks, `rte` return paths
  - `trap #0` syscall path, signals and `sigreturn`
  - context switch and `copyin`/`copyout`
  - `spl*` mapped onto the 68k IPL mask
  - 68030 cache enable via `movec CACR`
- **No-MMU model:** keep the fixed user region and whole-process swapping
  (`USER_DATA_START`/`END`), as RetroBSD does today. There is no inter-process
  protection, and wild pointers do not fault. Accepted; the 68030 PMMU remains
  a possible later upgrade.
- Drop all PIC32 peripheral drivers (about 14k of the 17k lines in
  `sys/pic32/`). Reuse the kconfig mechanism with a new `sys/m68k/virt/`
  board (`Config`, plus entries in `files.kconf` and `devices.kconf`).

## 3. QEMU `virt` devices

- Machine: `qemu-system-m68k -M virt -cpu m68030` (fall back to `m68040` if the
  installed QEMU rejects the 030 on `virt`; to be verified).
- Drivers needed: Goldfish TTY (console), Goldfish PIC (6 levels), Goldfish
  timer/RTC (clock), `virtio-mmio` block (root filesystem and swap). Each is
  in the low hundreds of lines.
- Load with `-kernel` (ELF); debug with `-s -S` and `m68k-linux-gnu-gdb` or
  `gdb-multiarch`.

## 4. Userland and libc (low-medium risk)

- Most of `src/cmd` is portable C. About 20 asm files in `src/libc/mips/` need
  m68k versions: `crt0`, syscall stubs, `setjmp`/`longjmp`, `alloca`, signal
  trampoline.
- Stock m68k ABI (stack arguments, result in `d0`); ILP32 like MIPS o32.
- **Struct layout:** m68k aligns `int` to 2 bytes where MIPS uses 4. Audit
  on-disk and ABI-visible structs (`stat`, `utmp`, a.out header, directory
  entries).
- Exclude the native toolchain and debugger commands from the build for this
  machine by trimming `src/cmd/Makefile`.

## 5. Endianness and filesystem (medium-high risk)

MIPS here is little-endian (`mipsel`); m68k is big-endian. This is the subtlest
area and the most likely source of late-surfacing bugs.

- Audit `tools/fsutil` and the filesystem format for host-endian assumptions:
  superblock, inodes, directories, free lists.
- Check `exec` and the a.out header, `inet`/`htons` code, and `utmp`/`wtmp`.
- Build the image on the host with `fsutil` and verify with `fsutil --check`.
  A round trip (build, boot, `fsck`) is the proof.

## Risk summary

| Area                              | Risk                                          |
|-----------------------------------|-----------------------------------------------|
| Debian GCC (freestanding)         | Low                                           |
| Kernel MD layer (no MMU)          | Medium                                        |
| QEMU `virt` drivers               | Low-Medium (verify `-cpu m68030` on `virt`)   |
| libc asm and userland             | Low-Medium                                    |
| Endianness, filesystem, a.out     | **Medium-High**                               |
| Native compilers                  | Out of scope                                  |

## Open verification items

- Which GCC version Debian Testing ships for `m68k-linux-gnu` (resolved in M0: GCC 16.2.0; see D10) (a survey host
  showed GCC 12; any recent GCC should work).
- Whether QEMU `virt` accepts `-cpu m68030` (resolved in M0: yes; see D10).
- Whether the kernel needs any `libgcc` helper not provided by
  `src/libc/runtime/` (integer helpers only; FP is hardware, D12).
