# RetroBSD → 68030 on QEMU `virt`: feasibility assessment

**Scope:** vanilla GCC on Debian, a freestanding kernel, RetroBSD's own libc for
userland, no-MMU operation (on a 68030), QEMU `virt` bring-up only.

Out of scope: Amiga hardware, and the native on-target compilers and tools
(`as`, `ld`, `cc`, `adb`, `aout`, PCC/`ccom`, `lcc`, `lccom`, `smlrc`, ...).
Everything is cross-built on the host.

This is a desk assessment based on a survey of the repository (file sizes and
structure), written before any code was built. Progress is tracked in
[02-backlog.md](02-backlog.md) and later choices in
[03-decisions.md](03-decisions.md); notes marked *Update* record where
they changed this text.

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
  *Update (M1):* the build files were converted in place rather than
  duplicated (D14); the kernel template now lives in `sys/m68k/`. 45 of the
  48 standard `sys/kernel` files compile with `-Werror`.
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
  - 68030 caches via `movec CACR`: off until M6 (D34)
- **No-MMU model:** keep the fixed user region and whole-process swapping
  (`USER_DATA_START`/`END`), as RetroBSD does today. There is no inter-process
  protection, and wild pointers do not fault. Accepted; the 68030 PMMU remains
  a possible later upgrade (section 6).
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
  entries). *Update (M1):* GCC's default is kept (D17); `dinode`, `fs`,
  `direct` and the a.out header lay out identically with either alignment.
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

## 6. Multiple processes in memory (future, undecided)

Today one user process is in core: every process runs in the fixed window
`USER_DATA_START`-`USER_DATA_END` and a switch swaps whole processes (D5).
Swap to RAM (D41, M4) makes those switches cheap without changing the model.
Two ways to go further are recorded here; the choice is an M6 evaluation task.

### 6a. Process slots without an MMU

- **How:** split user memory into slots and load each program at its slot's
  address. Programs must be relocated at load: link with `--emit-relocs`,
  keep the relocations in the a.out (`a_reltext`/`a_reldata`, which
  `elf2aout` now drops) and apply them in `exec_aout.c`. GCC's
  `-msep-data` (data through a table at `a5`, as uClinux does) shrinks but
  does not remove the fixups.
- **A process cannot move once it runs:** pointers on its stack and heap are
  absolute, so it must swap back in to the same slot.
- **`fork`:** the child holds pointers into the parent's slot, so it must
  share that slot, and parent and child swap against each other as today
  (uClinux has only `vfork` for this reason). Workable rule: `fork` keeps
  the child in the parent's slot; `exec` takes a free slot. Fork-then-exec
  then works well.
- **Limits:** fixed slot sizes cap `sbrk` and stack growth and fragment
  memory; no protection between processes or for the kernel.
- **Kernel work:** 11 kernel files assume the fixed window (`exec`, signals,
  user address checks, `swapin`, ...) and need a per-process base; the
  2.11BSD core allocator (`coremap`), removed in RetroBSD, comes back.
- **Pro:** works on every 68030, including the MMU-less 68EC030 (A4000/030,
  many accelerator cards).
- **Risk:** medium. Toolchain and `elf2aout` changes, a new exec path, and
  `fork` semantics that differ from 2.11BSD.

### 6b. PMMU virtualization

- **How:** every process is linked at the same virtual address, as today;
  each gets its own physical memory and translation tables, and a switch
  loads the 68030 CPU root pointer and flushes the ATC. Whole-process
  swapping can stay; early-termination descriptors keep the tables small.
- **Pro:** no relocation, `fork` is a plain copy, protection between
  processes and for the kernel, room for growing segments or copy-on-write
  later. User programs and `elf2aout` do not change.
- **Risks:**
  - QEMU's 68030 has no PMMU (D25): the code can only be tested on WinUAE
    (or real hardware), or on QEMU's 68040, whose MMU differs (table
    format, `pflush`/`ptest`).
  - The 68EC030 has no MMU, so the no-MMU kernel (D5) must stay as an
    option.
  - User access from the kernel needs `moves` and fault recovery (D25
    notes).
- **Risk:** medium-high, mostly from testing outside QEMU.

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
