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
These tasks assume the in-place approach proposed in M0 (D14): the existing build files
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
      Handover: blocked on D11 (PIC32 reference stays); leave open.
- [x] Set `MACHINE = m68k`, retarget the `include/machine` symlink to
      `../sys/m68k` and remove `include/pic32` (D13) (`target.mk`, `cross.mk`; `src/` now descends into
      `startup-m68k` and `libc/m68k`, which have no Makefiles yet)
- [x] Create the minimal `sys/m68k/` headers needed to compile `sys/kernel/`:
      `machparam.h`, `cpu.h`, `float.h`, `limits.h`, `elf_machdep.h` and
      `debug.h`, plus an m68k `label_t` in `sys/include/types.h` (D16).
      `float.h`/`limits.h` values are checked against GCC's predefined macros.
      `endian.h`, `types.h` and `net_mac.h` are not needed (only included under
      `__OpenBSD__`, `!_SYS_TYPES_H_` or `INET`). Trial compile: 47 of 51
      `sys/kernel/*.c` build; the rest need the trap frame (M3) or K&R fixes.
      GCC's default 2-byte `int`/`long`/pointer alignment is kept (D17).
- [x] Create `sys/m68k/files.kconf` (machine-independent section copied from
      pic32, `m68k/*` section written fresh, listing only existing files),
      `devices.kconf` and `Makefile.kconf`. Done: the `m68k/*` section and
      `devices.kconf` are still empty (no m68k sources or block drivers yet);
      `Makefile.kconf` moved from `sys/pic32/` (D14) without the PIC32
      `devcfg.c`/`*.hex` rules; `newvers.sh` copied. `tools/kconfig` accepts
      `architecture "m68k"`. A board `Config` needs a `config ... root on`
      device from `devices.kconf` before kconfig emits a link target.
- [x] Replace `src/elf32-mips.ld` and the `elf32ltsmip` linker emulation with
      `-m m68kelf` and two ELF32 m68k linker scripts: `src/elf32-m68k.ld`
      (userland) and `sys/m68k/virt/kernel.ld` (kernel)
      Done: `target.mk` and `src/Makefile` use `elf32-m68k.ld` (`elf32-mips.ld`
      kept, D13). Userland: linked at `USER_DATA_START`, text (RX) and
      data+bss (RW) as two `PT_LOAD` segments for `elf2aout`, `_end` for
      `sbrk`. Kernel: `.vectors` at 0, image from `0x400`, `u0`/`u`/`u_end`
      at the top of the region (D16), asserts the image ends below `u0`.
      Verified: the `hello` and `fpu` smoke tests boot on QEMU with
      `kernel.ld`, and the 47 compiling `sys/kernel` objects link to ~70 KiB.
      The a.out conversion is untested: `elf2aout` cannot read big-endian ELF
      yet (M4).
- [x] Compile `sys/kernel/*.c` with GCC (no link); record the error/warning count.
      Result via a kconfig-generated board Makefile (`-Wall -Werror`): 45 of
      48 standard files plus `ioconf.c` compile with no warnings. The other
      3 fail (37 errors) only on the MIPS trap frame: `exec_subr.c`,
      `sys_process.c` (`FRAME_*`, `ST_RP` from `sys/pic32/io.h`) and
      `sys_pipe.c` (`#error` for unknown architecture); these move with the
      trap frame to M3. Optional `subr_log.c` (K&R `-Wimplicit-int`, missing
      `struct devspec`) and `tty_pty.c` were not built.
- [ ] Fix K&R/`-Werror` issues surfaced by GCC; pin the C dialect
      `-std=gnu99` is pinned. Survey (each libc subdirectory built with its
      own Makefile, all other target libraries clean): the K&R failures
      `abs.c`, `insque.c`, `remque.c` (no return type) and `valloc.c`
      (undeclared `getpagesize`) in `src/libc/gen` are fixed; they are not
      in the `gen` Makefile yet. Open:
      - `signal.h` set macros (`sigemptyset`, ...) tripped `-Wunused-value`
        in `abort.c`, `popen.c`, `system.c`, `siginterrupt.c`, `signal.c`
        and `pause.c`: fixed by removing the macros in favour of the
        `sigsetops.c` functions (D19), whose `sigismember` was also fixed
        (it tested the inverted mask). Checked on QEMU (68030).
      - `isinf.c`, `isnan.c`, `modf.c` (strict aliasing, swapped words in
        `modf`) and the PIC32-only `double`-to-`float` aliases in `isinff.c`,
        `isnanf.c`, `modff.c`: fixed by cherry-picking upstream `e7a6580`,
        which builds the real double versions. Checked on QEMU (68030).
      - `sys/kernel/subr_log.c` (optional): K&R `int`, an always-false NULL
        check and missing `struct devspec`.
- [x] Trim `src/cmd/Makefile` for m68k: exclude native compilers and tools
      (`as`, `ld`, `aout`, `ccom`, `lcc`, `lccom`, `smlrc`, `adb`, `nm`, `size`,
      `strip`, `ar`, `ranlib`, ...)
      Done (D8, D18): `adb aout ar as cc cpp lcc lcpp ld nm ranlib smallc
      smlrc`, `size`/`strip` and the PIC32 hardware demos (`adc-demo`,
      `glcdtest`, `portio`, `pwm`, `wiznet`) are kept as commented-out
      `SUBDIR +=` / `STD +=` lines; uncommenting one re-enables that group
      (checked with `make -n` both ways). `ccom`/`lccom` were never built. A
      real build needs `crt0`, libc and `elf2aout` (M4/M5).
      `rootfs.manifest` still names the disabled programs (see M4).

## M2: Hello from the kernel
- [ ] Add the `sys/m68k/virt/` board `Config`; generate the board `Makefile`
      with `kconfig` (the `sys/m68k/` skeleton, headers and `files.kconf` /
      `devices.kconf` / `Makefile.kconf` come from M0 and M1). kconfig only
      emits the kernel link target for a `config unix root on <dev>` line,
      and `<dev>` must be listed in `devices.kconf`; the virtio block driver
      is M4, so decide whether to reserve its entry early
- [ ] `startup.S`: vector table, stack setup, BSS clear, jump to `main`
- [ ] Goldfish TTY driver; console output of a banner on QEMU
- [ ] Document the QEMU invocation and a GDB recipe

## M3: Traps, syscalls, scheduling
- [ ] Exception and trap handling (`exception.c` equivalent)
- [ ] m68k trap frame layout (`FRAME_*` offsets, replacing MIPS ones from
      `sys/pic32/io.h`) and its MI users: `exec_subr.c`, `sys_process.c`, and
      the pipe return value in `sys_pipe.c`
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
- [ ] Audit struct layout (2-byte `int` alignment, D17): `stat`, `utmp`, a.out
      header, directory entries. Already checked: `dinode`, `fs`, `direct` and
      the a.out `exec` header have the same field offsets with 2- and 4-byte
      alignment; comparing against the host build of `fsutil` is still open
- [ ] Build a root filesystem image; `fsutil --check` passes. Drop the
      entries for programs disabled in `src/cmd/Makefile` from
      `rootfs.manifest`: the native toolchain (D8) `/bin/{adb,aout,ar,as,cc,
      cpp,lcc,lcpp,ld,nm,ranlib,scc,size,strip}`, `/libexec/{smallc,smlrc}`,
      `/include/smallc` and the smallc examples, and the PIC32 hardware
      programs (D18) (`fsutil` only warns on missing files)
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
- [ ] Performance improvements, measured on QEMU before adopting any change.
      Candidates:
  - [ ] Alignment experiment: build everything with `-malign-int` (4-byte
        `int`/`long`/`double`) and compare against the GCC default (D17)
  - [ ] Signal set calls: `sigemptyset`/`sigaddset`/... are now function
        calls into `sigsetops.c` instead of header macros (D19). Measure
        whether the call overhead matters and, if so, inline them without
        reintroducing GCC-specific constructs
