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
      `struct devspec`) and `tty_pty.c` were not built (since fixed, see
      the K&R task).
- [x] Fix K&R/`-Werror` issues surfaced by GCC; pin the C dialect
      Done for the target libraries and the kernel. `-std=gnu99` is pinned.
      Survey: each libc subdirectory built with its own Makefile; all other
      target libraries were already clean. Fixed:
      - `src/libc/gen`: `abs.c`, `insque.c`, `remque.c` (no return type),
        `valloc.c` (undeclared `getpagesize`); not in the `gen` Makefile yet.
      - `signal.h` set macros (`sigemptyset`, ...) tripped `-Wunused-value`
        in `abort.c`, `popen.c`, `system.c`, `siginterrupt.c`, `signal.c`
        and `pause.c`: removed in favour of the `sigsetops.c` functions
        (D19), whose `sigismember` was also fixed (it tested the inverted
        mask). Checked on QEMU (68030).
      - `isinf.c`, `isnan.c`, `modf.c` (strict aliasing, swapped words in
        `modf`) and the PIC32-only `double`-to-`float` aliases in `isinff.c`,
        `isnanf.c`, `modff.c`: fixed by cherry-picking upstream `e7a6580`,
        which builds the real double versions. Checked on QEMU (68030).
      - `sys/kernel/subr_log.c` and `tty_pty.c`: removed the unreferenced
        `struct devspec` tables left behind by upstream `5cb608d`, declared
        the K&R `unused` parameters and dropped a dead `msg_bufc` check.
        With `LOG_ENABLED` and `PTY_ENABLED`, 48 of 51 `sys/kernel` files
        compile; the other 3 need the trap frame (M3).
      Userland programs are not covered; see M5.
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
Goal: the kernel links, boots on QEMU and stops in
`panic("No root filesystem found!")`; that `panic()` output is the proof of
life (no separate banner). There is no disk yet (D20).
- [x] Add the `sys/m68k/virt/` board `Config` with the root stubbed as
      `root on major 0 minor 0`, swap on `major 0 minor 1` (D20); generate
      the board `Makefile` with `kconfig`. Add a minimal m68k `devsw.c`
      whose block major 0 is a "no disk" entry that fails to open
      Done. kconfig crashed on a raw major/minor with an empty
      `devices.kconf` (`devtoname()` NULL dereference); it now names such
      devices `major N minor M`. `devsw.c` keeps the PIC32 major numbers;
      it compiles, and still references the console (`cn*`, step 4), the
      swap devices (`sw*`, `swc*`) and `mmrw`, which are ported from
      `sys/pic32/swap.c` and `mem.c` or stubbed by the `stubs.c` step
- [x] `startup.S`: vector table (every vector to a default handler that
      prints the vector number and halts), supervisor stack, BSS clear,
      jump to `main`
      Done (`sys/m68k/startup.S`). The default handler `badvec` calls
      `badtrap(vector, pc, sr)`, which `machdep.c` provides. If `main`
      returns, startup halts. Covered by the `startup` (BSS clear, stack,
      VBR) and `badvec` (`trap #15` reaches `badtrap` with vector 47, PC
      and SR) smoke tests in `make check`.
      M3 task 1 replaced `badvec`/`badtrap` and their test with the trap
      entry in `sys/m68k/locore.S`.
- [x] Minimal `machdep.c`: `startup`, `kconfig`, `boot`/halt (QEMU
      `virt-ctrl` power off), `spl*` via the status register IPL (D16),
      no-op `clkstart`
      Done (`sys/m68k/machdep.c`, device addresses and bootinfo records in
      `sys/m68k/virt.h`). `spl*` were already inline in `machparam.h`.
      `startup` reads `physmem` from the QEMU bootinfo memory chunk;
      `boot` writes `virt-ctrl` HALT (QEMU exits with status 0) or RESET;
      `idle` uses `stop #0x2000`; `udelay` busy-waits on the Goldfish RTC;
      `badtrap` prints and panics. Also ported unchanged from PIC32: the
      kernel tables, `ffs` (with `bfffo`), the string and memory routines
      and `copyin`/`copyout` (range checks against the fixed user region,
      no fault recovery). Covered by the `bootinfo`, `kstring`, `useraddr`,
      `udelay` and `halt` smoke tests in `make check`, which link
      `machdep.c` with kernel stubs (`smoke/ktest.c`)
- [x] Goldfish TTY console: polled `cnputc` only; the interrupt-driven tty
      side is M3
      Done. `sys/m68k/cons.c` is `sys/pic32/cons.c` unchanged; the driver
      `sys/m68k/gftty.c` (`device gftty0` in `Config`, char major 6 as
      `UART_MAJOR`, console `/dev/tty0`) writes output bytes as soon as
      they are queued and reads input by polling in `gfttygetc`. `startup`
      calls `gfttyinit` to keep the device interrupt off. Checked on QEMU
      with a throwaway harness: probe message, `printf` through `cnputc`,
      `cngetc` input. Not in `make check`: the kernel boot test (step 6)
      covers console output
- [x] `sys/m68k/stubs.c`: every remaining undefined symbol panics with "not
      implemented", so the kernel links (D20). The three trap-frame files
      (`exec_subr.c`, `sys_pipe.c`, `sys_process.c`) get placeholder
      `FRAME_*` definitions or stay out of the M2 link
      Done. The three trap-frame files stay out of the M2 link (commented
      out in `files.kconf`): placeholder `FRAME_*` values would compile
      silently wrong code. `stubs.c` stubs their symbols plus `setjmp`/
      `longjmp`, `sendsig`/`sigreturn`, `nosys`, `sc_msec`, `ucall`/
      `ufetch`/`ustore`, `cpu_sysctl` and `pipeops`; `icode` is an empty
      blob. `mem.c` and `swap.c` are copied unchanged from PIC32 instead of
      stubbed. `unix.elf` links (76 KB text) and on QEMU prints the banner,
      `gftty0` probe and `panic: No root filesystem found!`, then halts
      with exit status 0. Checked by hand; step 6 adds it to `make check`
- [x] Kernel boot test in `make check`: expect the `panic()` message
      Done: the `kernel` smoke test runs `make` in `sys/m68k/virt` and
      boots `unix.elf`, expecting `panic: No root filesystem found!` and
      QEMU exit status 0. Mutation-tested (build error, no halt, earlier
      panic). This found a kconfig bug: the generated `unix.elf` rule put
      the objects after `SYSTEM_DEP`'s order-only `|`, so editing a source
      rebuilt its object but never relinked the kernel. Fixed in
      `tools/kconfig/mkmakefile.c` (objects first)
- [x] Document the QEMU invocation and a GDB recipe
      Done in `docs/porting/README.md` ("Running a kernel image on QEMU",
      "Debugging the kernel with GDB"), checked against `unix.elf`: debug
      build with `make DEBUG=-g` (same as `kconfig -g`, no Makefile
      change), QEMU `-s -S`, `gdb-multiarch` breakpoints on `main` and
      `panic`, registers and the vector table

## M3: Traps, syscalls, scheduling
M3 ends when the machine-dependent kernel code exists, `stubs.c` is gone and
each part is proven by its own smoke test; the kernel boot test still ends at
the D20 root filesystem panic. Running process 1 is M4 (D21). Tasks are in
implementation order; each lists the tasks it depends on. The tasks were
renumbered after D24; D22 and D23 use the old numbers (old 4, 5, 6, 7, 8,
10, 11 are now 5, 6, 7, 8, 4, 11, 10).

- [x] 0. Record the M3 decisions: trap frame layout (D22: `struct
      trapframe` over the hardware frame, NetBSD style), syscall ABI (D23:
      `trap #0`, number in `d0`, arguments on the user stack, results in
      `d0`/`d1`, carry set and `errno` in `d0` on error) and the
      PIC32-only syscalls and `cpu_sysctl` (D24: `ustore`/`ufetch`/`ucall`
      removed, slots 152-154 reserved)
- [x] 1. Exception and trap handling (`exception.c` equivalent: `trap.S`,
      `trap.c`): `sys/m68k/frame.h` with `struct trapframe` (D22), and
      `u.u_frame` in `user.h` becomes `struct trapframe *`, set on traps
      from user mode. Common entry and exit through the trap frame, kernel
      faults print the registers and panic. Includes the FPU exception
      vectors and the line-F trap, posting `SIGFPE`/`SIGILL` (D12). Smoke
      tests: illegal instruction gives a register dump; user-mode faults
      post the right signal; bus error and FPU exception handling run on
      hand-built frames, since QEMU raises neither (D25)
      Done. `sys/m68k/frame.h` and the `PSL_*` bits in `machparam.h`;
      the entry and exit code is `sys/m68k/locore.S` (named so its object
      does not clash with `trap.o`): `trapentry` for every vector but the
      FPU exceptions, `fpfault` (`fsave`, `fpfault_clear()`, `frestore`)
      for vectors 48-54, and `trapret`, which applies `tf_stackadj`.
      `sys/m68k/trap.c` sorts by vector: interrupts are reported as stray
      until task 2, kernel faults dump the registers and panic, user
      faults post the signal and run the PIC32 return path (signals,
      `setpri`, `swtch`, `addupc`). User bus and address errors are
      trimmed to format 0 (D26). `trap #0` gives `SIGILL` until task 5.
      Smoke tests `ktrap`, `uill`, `upriv`, `utrace`, `udivz` on QEMU and
      `buserr`, `ufpe`, `fpbiu` on hand-built frames; `run.sh` now
      reports build failures instead of reusing stale objects
- [ ] 2. Goldfish PIC and RTC 1 alarm as the clock tick (D27; IPL 6, alarm re-armed
      each tick, `hardclock`); `spl*` stay two-level (D16). Time of day from
      RTC 2 at boot: a machine-dependent `inittodr(base)` called from
      `init_main.c` in place of `time.tv_sec = fs->fs_time`, falling back to
      the root filesystem time if the RTC reads earlier than it. Smoke tests,
      one each: ticks advance at `HZ`; a `timeout()` callback fires (linking
      the real `kern_clock.c`); `inittodr` reads RTC 2. Depends on 1
- [ ] 3. Context switch: `setjmp`/`longjmp` with the `u`/`u0` swap, including
      the user FPU state (`fsave`/`fmovem`, `fmovem`/`frestore`) kept in the
      `u` area (D12). First check that QEMU's 68030 supports `fsave`/
      `frestore`. Smoke test: two contexts switch, the `u` area and FP
      registers survive. Depends on 1
- [ ] 4. `copyin`/`copyout` exist (M2 `machdep.c`) but only range-check;
      review them against the user region before the syscall path relies
      on the same check for its arguments
- [ ] 5. `trap #0` syscall path, `nosys`, `sc_msec`. Smoke test from user
      mode: arguments, results in `d0`/`d1`, carry on error, restart.
      Depends on 1 and 4
- [ ] 6. m68k trap frame users (D22): `exec_subr.c` (`exec_setupstack`,
      `exec_clear`; the m68k ABI passes `argc`/`argv`/`envp` on the
      stack), `sys_process.c` and the pipe return value in `sys_pipe.c` use
      the `struct trapframe` fields. Re-enable the three files in
      `sys/m68k/files.kconf` and drop their stubs from `stubs.c`. Depends
      on 1 and 5
- [ ] 7. Signals: `sendsig`/`sigreturn` with an m68k `sigcontext` that also
      holds the FPU state (D12). Smoke test: signal frame round trip in user
      mode. Depends on 3 and 5
- [ ] 8. Process 1 entry: m68k `icode`, and `startup.S` enters user mode at
      `USER_DATA_START` when `main` returns in process 1, as PIC32's reset
      code does. Smoke test: user code runs and its `exit` syscall is
      handled. Running `init` itself is M4 (D21). Depends on 3 and 5
- [ ] 9. Goldfish TTY receive interrupt (PIC 1, source 32) feeding
      `ttyinput`, so `read()` on the console works. Depends on 2
- [ ] 10. 68030 cache enable via `CACR`; QEMU does not emulate the caches,
      so only a read-back is checked
- [ ] 11. Remaining stubs: `sys/m68k/stubs.c` is empty and removed (D20).
      Already done (D24): `ucall`/`ufetch`/`ustore` removed, `cpu_sysctl`
      in `sys/m68k/sysctl.c`; its smoke test (`sysctl` from user mode for
      each id) needs the syscall path of task 5. Depends on 3 and 5-8

## M4: Storage and filesystem (endianness work)
- [ ] virtio-mmio block driver; add it to `devices.kconf` and switch the
      board `Config` root and swap from the D20 stub to it
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
- [ ] Run a static `init` as a user process (fixed user region, no MMU),
      moved from M3 (D21): a freestanding test `/sbin/init` (smoke-test
      style, raw `trap #0` syscalls) on the root image, which opens the
      console and writes to it. The real `/sbin/init` follows with the M5
      libc

## M5: Libc and userland
- [ ] m68k `crt0`, syscall stubs, `setjmp`/`longjmp`, `alloca`, signal trampoline
- [ ] Select the 64-bit integer runtime helpers from `src/libc/runtime/`
      (`divdi3`, `udivmoddi4`, ...); soft-float code is not used (D12)
- [ ] Audit FP-dependent libc/libm code (`atof`, `ecvt`/`gcvt`, `printf` `%f`,
      `libm`) for MIPS or soft-float assumptions; confirm hardware FP works
- [ ] Build the reduced userland with `rootfs.manifest` adjusted
- [ ] Fix `-Werror` failures in `src/cmd` and `src/games` as they are built.
      Already seen: `src/libc/net/rcmd.c` (missing `sys/socket.h`; `net` is
      not built), `games/warp/sig.c` (`gtty`), `games/hunt/driver.c`
      (`#else` tokens), `cmd/mail.c` and `cmd/cron/popen.c`
      (`-Wparentheses`)
- [ ] Boot to a shell in QEMU; smoke test core commands
- [ ] Revisit `basic` PEEK/POKE (low priority): they are no-ops since the
      `ufetch`/`ustore` syscalls were removed (D24). Options: direct memory
      access, `/dev/mem`, or keep them disabled

## M6: Optional / future
- [ ] 68030 PMMU for process protection
- [ ] Real MMU and fault validation (D25): on WinUAE or hardware, recheck
      the code written against the manuals: bus and address error entry
      and frame trimming (format A/B), FPU exception entry, `fsave`/
      `frestore` in the context switch and signal frame, and the PMMU
      code. QEMU's 68030 model emulates none of these
- [ ] Prepare a real hardware board: the Amiga (D7, D25). Add WinUAE to the
      development flow, a `sys/m68k/<board>/` directory with its `Config`,
      console and timer drivers, and a boot path (second stage
      bootloader)
- [ ] Larger memory configuration and tuning beyond the 128 KB heritage limits
- [ ] Performance improvements, measured on QEMU before adopting any change.
      Candidates:
  - [ ] Alignment experiment: build everything with `-malign-int` (4-byte
        `int`/`long`/`double`) and compare against the GCC default (D17)
  - [ ] Signal set calls: `sigemptyset`/`sigaddset`/... are now function
        calls into `sigsetops.c` instead of header macros (D19). Measure
        whether the call overhead matters and, if so, inline them without
        reintroducing GCC-specific constructs
