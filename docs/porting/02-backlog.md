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
      trimmed to format 0 (D26). `trap #0` gave `SIGILL` until task 5.
      Smoke tests `ktrap`, `uill`, `upriv`, `utrace`, `udivz` on QEMU and
      `buserr`, `ufpe`, `fpbiu` on hand-built frames; `run.sh` now
      reports build failures instead of reusing stale objects
- [x] 2. Goldfish PIC and RTC 1 alarm as the clock tick (D27; IPL 6, alarm re-armed
      each tick, `hardclock`); `spl*` stay two-level (D16). Time of day from
      RTC 2 at boot: a machine-dependent `inittodr(base)` called from
      `init_main.c` in place of `time.tv_sec = fs->fs_time`, falling back to
      the root filesystem time if the RTC reads earlier than it. Smoke tests,
      one each: ticks advance at `HZ`; a `timeout()` callback fires (linking
      the real `kern_clock.c`); `inittodr` reads RTC 2. Depends on 1.
      Done: `sys/m68k/clock.c` (`clkstart`, `clockintr`, `inittodr`), PIC
      dispatch in `intr()` (`trap.c`; unhandled sources are disabled),
      `startup()` masks all PIC sources; smoke tests `tick`, `callout`, `todr`
- [x] 3. Context switch: `setjmp`/`longjmp` with the `u`/`u0` swap, including
      the user FPU state (`fsave`/`fmovem`, `fmovem`/`frestore`) kept in the
      `u` area (D12). First check that QEMU's 68030 supports `fsave`/
      `frestore`. Smoke test: two contexts switch, the `u` area and FP
      registers survive. Depends on 1
      Done: it does not (D28). `setjmp`/`longjmp`/`savefpu` in
      `sys/m68k/locore.S`; `label_t` gains the PC, `struct user` the
      `u_fpstate` save area; `fork()` calls `savefpu()` before `swapout()`;
      the `virt` board sets `FPU_NO_FSAVE`; smoke tests `swtch`, `fpswtch`.
      FPU reset on `exec` is left to task 6, FPU state in the signal
      frame to task 7
- [x] 4. `copyin`/`copyout` exist (M2 `machdep.c`) but only range-check;
      review them against the user region before the syscall path relies
      on the same check for its arguments
      Done: `baduarange`/`badkrange` compare the length with the room left
      in the region (the old `a + n - 1` check let a huge length wrap
      around, and rejected zero-length copies at the region start); used
      by `copyin`/`copyout`, `ioctl`, `physio` and `/dev/mem`. `rwuio`
      now checks each user `iovec`, and `namei` copies user names with
      the new `copyinstr`, which stops at the region end. Smoke test
      `useraddr` extended. There is no hardware protection without an
      MMU: these checks catch bad arguments, not hostile code
- [x] 5. `trap #0` syscall path, `nosys`, `sc_msec`. Smoke test from user
      mode: arguments, results in `d0`/`d1`, carry on error, restart.
      Depends on 1 and 4
      Done: `syscall()` in `sys/m68k/trap.c` per D23 (arguments by
      `copyin` from `usp + 4`, `EFAULT` past the user region; the handler
      runs under `setjmp(&u.u_qsave)`); `trap #1`-`#15` still give
      `SIGILL`. `d1` is left alone: only `pipe()` sets it, through
      `u_frame` (task 6). `nosys()` in `trap.c`, `sc_msec()` in
      `clock.c` from a tick count (5 ms resolution); both stubs removed.
      Smoke tests `sysargs`, `sysret`, `sysrestart`, `nosys`, `msec`
- [x] 6. m68k trap frame users (D22): `exec_subr.c` (`exec_setupstack`,
      `exec_clear`; the m68k ABI passes `argc`/`argv`/`envp` on the
      stack), `sys_process.c` and the pipe return value in `sys_pipe.c` use
      the `struct trapframe` fields. Re-enable the three files in
      `sys/m68k/files.kconf` and drop their stubs from `stubs.c`. Depends
      on 1 and 5
      Done: exec enters user mode with `sp` at `argc` and the `argv`/`envp`
      vectors above it (D30); this also fixes PIC32's `argv` reservation,
      which was four times too large. `exec_clear` zeroes the registers
      and SR and resets the FPU through a zeroed `u_fpstate` and the new
      `restorefpu`. `pipe()` returns the write descriptor in `d1`.
      `procxmt` reads `u` at any even offset (the PC is 2-byte aligned),
      writes only `d0`-`a7`, PC and the user SR bits, and `PT_STEP` sets
      `PSL_T`. Smoke tests `execstack`, `execclear`, `pipe2`, `ptrace`
- [x] 7. Signals: `sendsig`/`sigreturn` with an m68k `sigcontext` that also
      holds the FPU state (D12). Smoke test: signal frame round trip in user
      mode. Depends on 3 and 5
      Done: `sys/m68k/signal.c` per D31. `sigcontext` holds every
      register, a flags word and `struct fpstate` (moved to
      `<machine/fpu.h>`); the frame is built in the kernel and copied out.
      The handler gets a reset FPU and a format 0 frame without tracing;
      `sigreturn` refuses unknown flags, an odd PC and FPU frames
      `frestore` would reject. Stubs removed; `stubs.c` holds only
      `icode`. Smoke tests `sigframe`, `sigreturn`, `sigfpu`, `sigfault`
- [x] 8. Process 1 entry: m68k `icode`, and `startup.S` enters user mode at
      `USER_DATA_START` when `main` returns in process 1, as PIC32's reset
      code does. Smoke test: user code runs and its `exit` syscall is
      handled. Running `init` itself is M4 (D21). Depends on 3 and 5.
      Done (D32): `startup.S` resets the kernel stack, sets `usp` to
      `USER_DATA_END`, clears the registers and `rte`s to user mode at
      IPL 0; `icode` is position-independent code in `startup.S` that
      builds `argv` on the user stack, execs `/sbin/init` and exits with
      the error. `stubs.c` is deleted. Smoke tests `uentry`, `icode`
- [x] 9. Goldfish TTY receive interrupt (PIC 1, source 32) feeding
      `ttyinput`, so `read()` on the console works. Depends on 2.
      Done (D33): `gfttyintr` passes all buffered input to `ttyinput` in
      32-byte batches; the first open enables the interrupt, as PIC32's
      `uartopen` does; `intr()` dispatches it and runs every handler at
      IPL 7, so handlers do not nest. Smoke tests `ttyrx`, `ttyrxopen`,
      `intrnest`; the path through `tty.c` to `read()` is checked when
      `init` runs (M4)
- [x] 10. 68030 caches off (D34): `_start` clears and disables both
      caches through `CACR`, whatever the bootloader left; enabling them
      is an M6 task. Smoke test `cacr`: with every cache bit set before
      `_start` runs, the enable bits read 0 after boot and the clear bits
      were written
- [x] 11. Remaining stubs: `sys/m68k/stubs.c` is empty and removed (D20);
      done with task 8 (D32). Already done (D24): `ucall`/`ufetch`/
      `ustore` removed, `cpu_sysctl`
      in `sys/m68k/sysctl.c`; its smoke test (`sysctl` from user mode for
      each id) needs the syscall path of task 5. Depends on 3 and 5-8.
      Done (D35): `__sysctl()` no longer requires root for
      `CTL_MACHDEP`, so `CPU_NLIST` (`knlist` in `ps` and friends) works
      for ordinary users. Smoke tests `consdev`, `errmsg`, `nlist`

## M4: Storage and filesystem (endianness work)
- [x] virtio-mmio block driver `vd` (D36), MBR parser in
      `sys/m68k/disklabel.c`; add it to `devices.kconf` and switch the
      board `Config` root and swap from the D20 stub to it. Smoke tests
      `vdprobe`, `vdread`, `vdwrite`, `vdpart`
- [x] Endianness audit (D37). Findings, each covered by a task below:
      `fsutil` writes the filesystem little-endian; `check.c` and
      `fsutil.c` read indirect blocks as 16-bit block numbers; the MBR is
      read and written with host-order casts; `elf2aout` reads and writes
      in host order; `sys/kernel/exec_elf.c` accepts only little-endian
      MIPS; there is no `machine/endian.h`, and `htonl`/`htons` exist
      only as MIPS assembly; `<sys/wait.h>` decodes the status through
      LSB-first bitfields; `adventure.dat`, `fortunes.dat`, `cards.pck` and
      `retroImage` are committed little-endian struct dumps. Checked and
      unaffected: the kernel (apart from `exec_elf.c`), the `FSMAGIC`
      constants, `dev_t` in device inodes, zoneinfo (`zic` and `ctime.c`
      use explicit big-endian), `bogdict` and the text files, the libc
      and libm IEEE unions (`isinf`, `isnan`, `modf`, `fmod` split a
      64-bit integer with shifts), and `tar`, `compress` and `swab`
- [x] Audit struct layout (2-byte `int` alignment, D17). Offsets taken
      from `m68k-linux-gnu-gcc` match what the host tools write, field by
      field: `struct fs` (all 32-bit, `fs_magic2` at 1020, 1024 bytes),
      `fblk` (count then 200 block numbers), `dinode` (two 16-bit fields,
      then 32-bit, 64 bytes), `direct` (32/16/16 then name) and the a.out
      `exec` header (eight 32-bit words, same in `elf2aout`). `stat` and
      `utmp` have no padding and are only written on the target. Only the
      byte order changes. Found, not used by the build: `elf2aout -s`
      writes 4.4BSD `nlist` records, while libc `nlist()` parses
      RetroBSD's compact records (length, type, little-endian value,
      name) and `include/nlist.h` declares a third layout; executables
      carry no symbols (`a_syms` is 0)
- [x] `fsutil` writes big-endian (D37), converted in place:
      - `superblock.c` `fs_read16/32`, `fs_write16/32` assemble big-endian;
      - open-coded little-endian shifts in `block.c` (indirect free),
        `create.c` (root and `lost+found` directories) and `inode.c`
        (`fs_dirent_pack`/`unpack`) use the helpers;
      - raw host `unsigned` arrays for free-list blocks (`block.c`
        `fs_block_free`, `fs_block_alloc`) and indirect blocks (`inode.c`
        bmap) are converted on read and write;
      - fix the latent bug: `check.c` `scan_indirect_block` and
        `fsutil.c` `print_*indirect_block` read indirect entries as 16-bit
        numbers; make them 32-bit;
      - the MBR (`superblock.c` partition table, `fsutil.c`
        `--repartition`) uses explicit little-endian helpers.
      Host tests: `--new`, `--add`, `--extract` round trip and `--check`;
      the superblock magic reads `<<SF` on disk.
      Done: helpers `fs_get32/16`, `fs_put32/16`, `mbr_get32/put32` in
      `bsdfs.h`; `fs_read_block32`/`fs_write_block32` convert free-list
      and indirect blocks; `fs_dirhdr_read`/`write` replace the raw
      directory header structs. Also fixed `print_triple_indirect_block`,
      which called the single-indirect printer. Smoke test `fsutil`
      (host only): `--check` must print no errors, and the independent
      big-endian reader `smoke/bsdread.c` reads a 66 MB file whose blocks
      pass 65535 and follows `..` of `/` and `lost+found`. Mutation-checked:
      each helper reversed, raw block and header I/O, the 16-bit indirect
      scan and the single-byte `..` inode all fail it
- [x] Kernel mounts an `fsutil` image (smoke test): a small image with an
      MBR, a root partition and a file; the kernel mounts `vd0a` as root
      and reads the file back. This test is the one that proves the
      `fsutil` and kernel formats match. Depends on the `fsutil` task;
      replaces the D20 root panic.
      Done: smoke test `rootfs`. The image holds `/dev/console`, a 6.5 KB
      `/etc/motd` (read through an indirect block) and `/sbin/init` from
      `smoke/rootinit.S`, which copies the file to the console and halts;
      the image passes `fsutil --check` afterwards. The `kernel` test still
      boots without a disk and expects the root panic. Fixed on the way
      (D38): `vd` takes byte counts that are not whole sectors (test
      `vdbytes`), and `swtch()` no longer mistakes process 0 for a user
      process while the boot-time `newproc()` swap-out sleeps
- [x] `elf2aout` for m68k (D37, D39): read big-endian ELF32 with `EM_68K`;
      write the a.out header and symbol table big-endian; set the m68k
      a.out magic. In the kernel, `exec_elf.c` checks `ELFDATA2LSB` and
      `EM_MIPS`: change it to `ELFDATA2MSB`/`EM_68K`, or drop the ELF exec
      path (decide in the task). The `-s` symbol-file mode is not used by
      the build and its format does not match libc `nlist()`: leave it
      unconverted or remove it. Host test: convert a smoke-test ELF and
      check the header fields. Then build `smoke/rootinit.S` through
      `elf2aout` instead of its hand-written a.out header
      Done (D39). `elf2aout` reads every ELF field and writes the header
      through big-endian helpers and refuses anything but a 32-bit
      big-endian `EM_68K` executable; the header stays OMAGIC with
      machine ID 0; `-s` and the symbol translation are removed, and the
      section headers are no longer read. `exec_elf.c` stays in the tree
      but builds only with `options "EXEC_ELF"`. The `elf2aout` smoke
      test converts `aoutprog.S` (entry past the start, a gap before the
      data, odd sizes) and checks the header and the image; 7 mutations
      of `elf2aout.c` are caught. `rootinit.S` now has text, data and bss
      and goes through `elf2aout`
- [x] Run a static `init` as a user process (fixed user region, no MMU),
      moved from M3 (D21): a freestanding test `/sbin/init` (smoke-test
      style, raw `trap #0` syscalls) on the root image, which opens the
      console and writes to it. The real `/sbin/init` follows with the M5
      libc. Depends on the mount task and `elf2aout`.
      Done with the mount task: `smoke/rootinit.S` carried its own OMAGIC
      header at first; it now goes through `elf2aout`
- [x] Configure swap on `vd0b`; test that a process is swapped out and
      back in. Depends on `init`. Swap on `vd0b` is configured and the
      boot-time swap-out and swap-in of process 1's u area work (`rootfs`);
      still to test: a user process with data and stack segments swapped
      out and back in, including while it sleeps (D38)
      Done: smoke test `swap`. `/sbin/init` from `smoke/swapinit.c` (C,
      raw `trap #0` syscalls) forks; parent and child take 8 turns over two
      pipes, so each is swapped out and in while it sleeps, and both check
      patterns in data, bss and stack; the six patterns are then found on
      the swap partition. Fixed on the way (D40): `swap()` waits without a
      context switch, as the `u`/`u0` exchange corrupted u areas in
      flight. 5 mutations (the fix reverted, stack address, no stack
      swap-out, the `vd` tail copy, the data address) are caught
- [x] Swap to RAM (D41): a memory-backed block device `md` for swap, in
      `sys/m68k/` with entries in `files.kconf`/`devices.kconf`; storage
      is physical memory from `USER_DATA_END` to the bootinfo memory size,
      the strategy routine copies and completes at once, and `psize`
      reports the size. The `virt` `Config` swaps on `md0`. Smoke test:
      `swapinit.c` passes with swap on `md0` (a second test next to
      `swap`, which keeps `vd0b` covered through its own board config or
      a kernel built for it), and the banner shows the `md0` swap size.
      Depends on the swap task
      Done: `sys/m68k/md.c` on block major 1 (PIC32's `sramc` slot); the
      `virt` `Config` swaps on `md0` (15616 KB with `-m 16M`). Tests: `mdio`
      (driver: placement, byte counts, bounds), `mdswap` (`swapinit.c` on
      the board kernel; nothing reaches `vd0b`), and `swap` now relinks
      the board objects with swap on `vd0b`. 8 mutations of `md.c` and
      the swap device are caught; `>=` in the bounds test is equivalent

## M5: Libc and userland
- [x] m68k `crt0`, syscall stubs, `setjmp`/`longjmp`, `alloca`, signal trampoline
      Done (D42): `src/startup-m68k/crt0.c`, `src/libc/m68k/{sys,gen}` and
      the NetBSD string routines in `src/libc/m68k/string` (with the
      `sys/m68k/asm.h` shim); `alloca` is `__builtin_alloca`. Smoke tests
      `libcrt0`, `libstring`, `libsys`, `libsetjmp` and `libsignal`; every
      mutant of the stubs, `jmp_buf` saves, mask handling, trampoline and
      `crt0` offsets is caught
- [x] Byte order for userland (D37): `machine/endian.h` (`BYTE_ORDER ==
      BIG_ENDIAN`) in `sys/m68k`; `htonl`/`htons`/`ntohl`/`ntohs` as the
      identity in `src/libc/m68k` (MIPS has `gen/htonl.S`, `gen/htons.S`)
      Done: `sys/m68k/endian.h` (4.4BSD values) and
      `src/libc/m68k/gen/{htonl,htons}.c`; `htons`/`ntohs` keep only the
      low 16 bits of their `unsigned` argument. Nothing in the tree
      includes `<machine/endian.h>` yet; picoc tests the byte order at run
      time. Smoke test `libendian`; swapped bytes, a missing mask and a
      wrong `BYTE_ORDER` are caught
- [x] `<sys/wait.h>` (D37): rewrite `WIFEXITED`, `WEXITSTATUS`,
      `WTERMSIG`, `WIFSTOPPED`, ... with shifts and masks, and give
      `union wait` its big-endian field order; users are `sh`, `csh`,
      `cron`, `popen`, `system`. Test: a child's exit code and signal as
      seen by the parent
      Done: the macros read the status through `_W_INT` (an `int` or a
      `union wait`) with shifts and masks; under `__BYTE_ORDER__ ==
      __ORDER_BIG_ENDIAN__` the `union wait` fields come in reverse order
      after 16 bits of padding, so they sit at the same bits of `w_status`.
      Smoke test `libwait`: exit code, `SIGTERM`, `SIGQUIT` with a core
      dump, and a stop and continue, through the macros and the fields,
      and fields set by hand as `system()` does. All non-equivalent
      mutants are caught (masking `WIFSTOPPED` with 0177 differs only for
      signal 127)
- [x] Select the 64-bit integer runtime helpers from `src/libc/runtime/`
      (`divdi3`, `udivmoddi4`, ...); soft-float code is not used (D12)
      Done (D43): the 16 helpers GCC calls (division and modulo, shifts,
      64-bit integer to and from `float`/`double`); the shifts and
      conversions are added from compiler-rt 12.0.0, the version already
      in the tree. Soft-float objects are no longer built. Smoke test
      `libint64`; removing any helper, the big-endian word order, a
      conversion's sticky bit or its low word is caught
- [ ] Audit FP-dependent libc/libm code (`atof`, `ecvt`/`gcvt`, `printf` `%f`,
      `libm`) for MIPS or soft-float assumptions; confirm hardware FP works.
      Word order is already checked (D37): the IEEE unions split a 64-bit
      integer with shifts and are byte-order neutral
- [ ] Build the reduced userland and the full root filesystem image;
      `fsutil --check` passes. Rename the disk nodes in `rootfs.manifest`
      and `etc/fstab` from `sd` to `vd` (D36). Drop the entries for
      programs disabled in `src/cmd/Makefile` from `rootfs.manifest`: the
      native toolchain (D8) `/bin/{adb,aout,ar,as,cc,cpp,lcc,lcpp,ld,nm,
      ranlib,scc,size,strip}`, `/libexec/{smallc,smlrc}`, `/include/smallc`
      and the smallc examples, and the PIC32 hardware programs (D18)
      (`fsutil` only warns on missing files). Leave out `adventure`,
      `fortune`, `monop` (`src/games/Makefile` `SUBDIR`) and `retroforth`
      (`src/cmd/Makefile` `SUBDIR`) with their manifest entries, data
      files and man pages: their data files are little-endian (D37) and
      come back in M6
- [ ] Fix `-Werror` failures in `src/cmd` and `src/games` as they are built.
      Already seen: `src/libc/net/rcmd.c` (missing `sys/socket.h`; `net` is
      not built), `games/warp/sig.c` (`gtty`), `games/hunt/driver.c`
      (`#else` tokens), `cmd/mail.c` and `cmd/cron/popen.c`
      (`-Wparentheses`)
- [ ] Per-process memory limit: `MAXMEM` in `sys/include/param.h` is
      96 KB (PIC32 heritage), but the m68k user window is 256 KB
      (`USER_DATA_START`-`USER_DATA_END`). `exec_subr.c` refuses programs
      whose text, data, heap and stack exceed it and `kern_mman.c` caps
      `sbrk`; the banner prints it as "user mem". Derive it from the
      window (`USER_DATA_END - USER_DATA_START`, in `machparam.h`) and
      test that a program over 96 KB runs and that `sbrk` past the window
      fails
- [ ] Boot to a shell in QEMU; smoke test core commands
- [ ] Revisit `basic` PEEK/POKE (low priority): they are no-ops since the
      `ufetch`/`ustore` syscalls were removed (D24). Options: direct memory
      access, `/dev/mem`, or keep them disabled

## M6: Optional / future
- [ ] Evaluate keeping several user processes in memory (assessment
      section 6): process slots without an MMU (relocation at load,
      `fork` shares the parent's slot) or PMMU virtualization (one
      address space per process, with protection; testable only on
      WinUAE or QEMU's 68040). Decide, record the choice, then add
      implementation tasks
- [ ] Real MMU and fault validation (D25): on WinUAE or hardware, recheck
      the code written against the manuals: bus and address error entry
      and frame trimming (format A/B), FPU exception entry, `fsave`/
      `frestore` in the context switch and signal frame, and the PMMU
      code. QEMU's 68030 model emulates none of these
- [ ] 68030 cache management (D34), on WinUAE: enable the instruction cache
      with burst fill and clear it after a process switch (`longjmp`), at
      the end of `exec_clear()` and after `ptrace` writes; decide on the
      data cache together with `CIIN` for device registers and clears
      after device input (TTY `READ_BUFFER`, virtio, chip DMA)
- [ ] Amiga Rigid Disk Block (RDB) partitions: a parser next to the MBR
      one in `sys/m68k/disklabel.c` (D36). Partitioning and image tools
      for the Amiga are existing ones, not `fsutil`
- [ ] Prepare a real hardware board: the Amiga (D7, D25). Add WinUAE to the
      development flow, a `sys/m68k/<board>/` directory with its `Config`,
      console and timer drivers, and a boot path (second stage
      bootloader)
- [ ] Data files in target format (D37), then put `adventure`, `fortune`,
      `monop` and `retroforth` back in the build and `rootfs.manifest`:
      `adventure.dat` (`adventure glorkz`), `fortunes.dat` (`strfile`) and
      `cards.pck` (`initdeck`) are raw struct dumps with pointers and
      `long`s, and `retroImage` is raw 32-bit cells; all are committed
      little-endian. Options: generate them on the target, or with host
      generators that write the m68k layout explicitly; swap the
      `retroImage` words at build time
- [ ] Larger memory configuration and tuning beyond the 128 KB heritage limits
- [ ] Performance improvements, measured on QEMU before adopting any change.
      Candidates:
  - [ ] Alignment experiment: build everything with `-malign-int` (4-byte
        `int`/`long`/`double`) and compare against the GCC default (D17)
  - [ ] Signal set calls: `sigemptyset`/`sigaddset`/... are now function
        calls into `sigsetops.c` instead of header macros (D19). Measure
        whether the call overhead matters and, if so, inline them without
        reintroducing GCC-specific constructs
- [ ] Run ELF executables directly (lowest priority; userland stays
      a.out through `elf2aout`). `sys/kernel/exec_elf.c` loads only a
      single read-write-execute `PT_LOAD` segment, so the two segments
      from `src/elf32-m68k.ld` (RX and RW) give ENOEXEC; it rejects a
      segment whose file size is not below its memory size (a program
      without bss); and it checks `ELFDATA2LSB`/`EM_MIPS`. Fix: accept
      `ELFDATA2MSB`/`EM_68K`, load each `PT_LOAD` (text and data/bss
      inside the fixed user region), and allow `p_filesz == p_memsz`.
      It builds only with `options "EXEC_ELF"` (D39).
      Cost: ELF headers and section tables make files on the root image
      larger
