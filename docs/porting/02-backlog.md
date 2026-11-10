# Backlog: milestone tasks

Target: RetroBSD on 68030 (no MMU), QEMU `virt`. See
[01-assessment.md](01-assessment.md) and [03-decisions.md](03-decisions.md).

Status legend: `[ ]` open, `[x]` done.

## M0: Project setup
- [x] Create `retrobsd-68k` branch
- [x] Add `docs/porting/` (assessment, backlog, decision record)
- [x] Install `gcc-m68k-linux-gnu`, `binutils-m68k-linux-gnu`, `qemu-system-misc`
      (Debian has no `qemu-system-m68k` package; m68k is in `qemu-system-misc`)
- [x] Record toolchain and QEMU versions; verify `-M virt -cpu m68030`
      (see D10 in [03-decisions.md](03-decisions.md))
- [x] Freestanding smoke tests (`docs/porting/smoke/`): `hello` boots and prints
      on the Goldfish TTY, `fpu` checks hardware floating point (D12)
- [x] Add GDB (`gdb-multiarch`) and `make check` to validate the tooling setup
- [x] Source layout (D13): create the `sys/m68k/` and `sys/m68k/virt/` skeleton
      with a README, plus `src/libc/m68k/` and `src/startup-m68k/` directories.
      Add stubs only where a directory would otherwise be empty. Leave all
      reference code (`sys/pic32/`, `src/libc/mips/`, ...) untouched
- [x] Build-file approach (D14): convert `target.mk`, `sys/pic32/Makefile.kconf`
      and `clang-config.mk` in place, as the first step of M1 (the M1 tasks
      below are already worded for this); `clang-config.mk` is kept for now
- [x] Branch policy (D15): `retrobsd-68k` stays the long-lived branch on the fork
      and `master` an untouched upstream snapshot; nothing to implement beyond
      keeping to it (accepted)

## M1: Build system and sizing
These tasks assume the in-place approach proposed in M0 (D11): the existing build files
are converted for m68k, not duplicated, and the PIC32 build stops working.
- [x] Convert `target.mk` in place to GCC: `CROSS = m68k-linux-gnu-`,
      `-mcpu=68030 -m68881` and the freestanding flags from D10; remove the
      Clang/LLVM variables and `include clang-config.mk` (verified: `libc/gen`
      compiles to big-endian m68k ELF; `MACHINE` and the linker script are
      still MIPS, see tasks below)
- [x] Convert `sys/pic32/Makefile.kconf` (the kernel build template) the same
      way, then regenerate the board `Makefile`s via `kconfig` (verified:
      `kconfig` builds with `byacc`; `explorer16` and `olimex` regenerated
      with GCC and no Clang/ihex/pic32prog references)
- [ ] Delete `clang-config.mk` once nothing includes it (only the PIC32
      `sys/pic32/attic/*/Makefile` still do)
- [x] Set `MACHINE = m68k`, retarget the `include/machine` symlink to
      `../sys/m68k` and remove `include/pic32` (D13) (`target.mk`, `cross.mk`; `src/` now descends into
      `startup-m68k` and `libc/m68k`, which have no Makefiles yet)
- [ ] Create the minimal `sys/m68k/` headers needed to compile `sys/kernel/`:
      `machparam.h`, `cpu.h`, `float.h`, `limits.h`, `elf_machdep.h`,
      `debug.h` and `endian.h`; adapt from the pic32 versions with big-endian
      and m68k values
- [ ] Create `sys/m68k/files.kconf` (machine-independent section copied from
      pic32, `m68k/*` section written fresh, listing only existing files),
      `devices.kconf` and `Makefile.kconf`
- [ ] Replace `src/elf32-mips.ld` and the `elf32ltsmip` linker emulation with
      `-m m68kelf` and two ELF32 m68k linker scripts: `src/elf32-m68k.ld`
      (userland) and `sys/m68k/virt/kernel.ld` (kernel)
- [ ] Compile `sys/kernel/*.c` with GCC (no link); record the error/warning count
- [ ] Fix K&R/`-Werror` issues surfaced by GCC; pin the C dialect
- [ ] Trim `src/cmd/Makefile` for m68k: exclude native compilers and tools
      (`as`, `ld`, `aout`, `ccom`, `lcc`, `lccom`, `smlrc`, `adb`, `nm`, `size`,
      `strip`, `ar`, `ranlib`, ...)

## M2: Hello from the kernel
- [ ] Add the `sys/m68k/virt/` board `Config`; generate the board `Makefile`
      with `kconfig` (the `sys/m68k/` skeleton, headers and `files.kconf` /
      `devices.kconf` / `Makefile.kconf` come from M0 and M1)
- [ ] `startup.S`: vector table, stack setup, BSS clear, jump to `main`
- [ ] Goldfish TTY driver; console output of a banner on QEMU
- [ ] Document the QEMU invocation and a GDB recipe

## M3: Traps, syscalls, scheduling
- [ ] Exception and trap handling (`exception.c` equivalent)
- [ ] Goldfish PIC and timer; clock tick; `spl*` mapped to the IPL mask
- [ ] `trap #0` syscall path; signals and `sigreturn`
- [ ] Context switch, `copyin`/`copyout`
- [ ] FPU state save/restore (`fsave`/`frestore`, `fmovem`) in context switch
      and signal delivery; FPU exception and line-F trap handling (D12)
- [ ] 68030 cache enable via `CACR`
- [ ] Run a static `init` as a user process (fixed user region, no MMU)

## M4: Storage and filesystem (endianness work)
- [ ] virtio-mmio block driver
- [ ] Endianness audit of `tools/fsutil` and the on-disk format
- [ ] Big-endian ELF32 / EM_68K support in `tools/elf2aout`; m68k a.out magic
- [ ] Audit struct layout (2-byte `int` alignment): `stat`, `utmp`, a.out
      header, directory entries
- [ ] Build a root filesystem image; `fsutil --check` passes
- [ ] Mount the root filesystem on QEMU; configure swap

## M5: Libc and userland
- [ ] m68k `crt0`, syscall stubs, `setjmp`/`longjmp`, `alloca`, signal trampoline
- [ ] Select the 64-bit integer runtime helpers from `src/libc/runtime/`
      (`divdi3`, `udivmoddi4`, ...); soft-float code is not used (D12)
- [ ] Audit FP-dependent libc/libm code (`atof`, `ecvt`/`gcvt`, `printf` `%f`,
      `libm`) for MIPS or soft-float assumptions; confirm hardware FP works
- [ ] Build the reduced userland with `rootfs.manifest` adjusted
- [ ] Boot to a shell in QEMU; smoke test core commands

## M6: Optional / future
- [ ] 68030 PMMU for process protection
- [ ] Larger memory configuration and tuning beyond the 128 KB heritage limits
