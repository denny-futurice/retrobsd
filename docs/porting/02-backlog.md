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
- [ ] Decide source layout and record it as a decision. Proposal: kernel in
      `sys/m68k/` with the board in `sys/m68k/virt/`; userland asm in
      `src/libc/m68k/`. Open question: `git mv` the MIPS sources now, or leave
      `src/libc/mips/` and `sys/pic32/` untouched as reference until the m68k
      versions exist (proposal: leave them). Also decide what happens to the
      `include/machine` and `include/pic32` symlinks.
- [ ] Decide build-file approach and record it as a decision. Proposal: edit
      `target.mk`, `sys/pic32/Makefile.kconf` and `clang-config.mk` in place
      (D11), as the first step of M1; old versions stay in git history. If
      adopted, reword the M1 "variant" tasks accordingly.
- [ ] Decide branch policy and record it as a decision. Proposal:
      `retrobsd-68k` is the long-lived branch on the fork, and `master` stays
      an untouched snapshot of upstream (D11: no tracking or merging).

## M1: Build system and sizing
These tasks assume the in-place approach proposed in M0 (D11): the existing build files
are converted for m68k, not duplicated, and the PIC32 build stops working.
- [ ] Convert `target.mk` in place to GCC: `CROSS = m68k-linux-gnu-`,
      `-mcpu=68030 -m68881` and the freestanding flags from D10; remove the
      Clang/LLVM variables and `include clang-config.mk`
- [ ] Convert `sys/pic32/Makefile.kconf` (the kernel build template) the same
      way, then regenerate the board `Makefile`s via `kconfig`
- [ ] Delete `clang-config.mk` once nothing includes it
- [ ] Replace `src/elf32-mips.ld` and the `elf32ltsmip` linker emulation with an
      ELF32 m68k linker script and `-m m68kelf` (kernel and userland)
- [ ] Compile `sys/kernel/*.c` with GCC (no link); record the error/warning count
- [ ] Fix K&R/`-Werror` issues surfaced by GCC; pin the C dialect
- [ ] Trim `src/cmd/Makefile` for m68k: exclude native compilers and tools
      (`as`, `ld`, `aout`, `ccom`, `lcc`, `lccom`, `smlrc`, `adb`, `nm`, `size`,
      `strip`, `ar`, `ranlib`, ...)

## M2: Hello from the kernel
- [ ] Add `sys/m68k/` and a `sys/m68k/virt/` board (`Config`; hook into
      `files.kconf` / `devices.kconf`; regenerate the board `Makefile`)
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
