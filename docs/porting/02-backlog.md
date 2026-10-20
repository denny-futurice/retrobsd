# Backlog: milestone tasks

Target: RetroBSD on 68030 (no MMU), QEMU `virt`. See
[01-assessment.md](01-assessment.md) and [03-decisions.md](03-decisions.md).

Status legend: `[ ]` open, `[x]` done.

## M0: Project setup
- [x] Create `retrobsd-68k` branch
- [x] Add `docs/porting/` (assessment, backlog, decision record)
- [ ] Install `gcc-m68k-linux-gnu`, `binutils-m68k-linux-gnu`, `qemu-system-m68k`
- [ ] Record toolchain and QEMU versions; verify `-M virt -cpu m68030`

## M1: Build system and sizing
- [ ] Add a `CROSS = m68k-linux-gnu-` variant of `target.mk` (no Clang/LLVM
      variables, no `clang-config.mk`)
- [ ] Add an equivalent variant of `sys/pic32/Makefile.kconf`
- [ ] Add an ELF32 m68k linker script (kernel and userland)
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
- [ ] Port or select soft-float and 64-bit runtime helpers
- [ ] Build the reduced userland with `rootfs.manifest` adjusted
- [ ] Boot to a shell in QEMU; smoke test core commands

## M6: Optional / future
- [ ] 68030 PMMU for process protection
- [ ] Larger memory configuration and tuning beyond the 128 KB heritage limits
