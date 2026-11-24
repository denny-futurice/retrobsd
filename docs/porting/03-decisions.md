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
- **Decision:** No external libc. 64-bit integer helpers (`__udivdi3` etc.)
  come from `src/libc/runtime/`, not the Linux-built `libgcc`. Floating point
  is hardware, not software (see D12; originally this decision also assigned
  soft-float helpers to `runtime/`).

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
- **Superseded in part by D25:** the Amiga becomes the first real hardware
  board, as M6 work; it stays out of scope until then.

## D8: Native on-target compilers and tools are out of scope
- **Decision:** `as`, `ld`, `cc`, `adb`, `aout`, PCC/`ccom`, `lcc`, `lccom`,
  `smlrc` and similar are not ported. They are excluded from the m68k build.
  All code is cross-built on the host.

## D9: Keep the 2.11BSD filesystem format; big-endian on disk
- **Status:** Proposed; to be confirmed during the M4 endianness audit.
- **Decision:** Retain the existing block and inode structures, with host tools
  (`fsutil`, `elf2aout`) made endian-correct.

## D10: Verified toolchain baseline (M0)
- **Versions:** `m68k-linux-gnu-gcc` 16.2.0 (Debian 16.2.0-3), GNU ld/binutils
  2.47, QEMU 11.1.2 (`qemu-system-misc` 1:11.1.2+ds-3; there is no
  `qemu-system-m68k` package).
- **Verified:** `qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel x.elf
  -nographic` boots a freestanding ELF32 big-endian m68k binary, and a write to
  the Goldfish TTY at `0xff008000` reaches the console. `-cpu m68030` is
  accepted on `virt`, so no `m68040` fallback is needed.
- **Flags that work:** `-mcpu=68030 -ffreestanding -nostdlib -nostdinc
  -fno-pic -fno-pie -fno-stack-protector -std=gnu99 -Os -Wall -Werror`; link
  with `m68k-linux-gnu-ld -T script.ld -static`. (Originally recorded with
  `-msoft-float`; superseded by D12.)
- **Note:** the toolchain emits a reference to `__udivdi3` for runtime 64-bit
  division in freestanding code, so 64-bit integer helpers must come from
  `src/libc/runtime/` as planned in D3. The Linux `libgcc` is not used.
  Floating point needs no helper functions (D12).
- **Note:** QEMU `virt` loads an ELF kernel at its entry point with no stack set
  up; `crt0` must set `%sp` itself.

## D11: PIC32 support is abandoned in this fork
- **Context:** This fork exists to run RetroBSD on the 68030. Keeping both
  targets working would force abstraction and refactoring that has no value for
  this goal.
- **Decision:** PIC32 support may be broken or removed. The m68k port is not
  required to coexist with PIC32, and no refactoring is done to support both
  (no `ARCH=` selector, no dual-target build files).
- **Consequences:**
  - `target.mk`, `Makefile.kconf`, `clang-config.mk` and the `libc/mips`
    sources can be converted or replaced in place for m68k.
  - PIC32 code and documentation stay in the tree for now as reference
    material while porting; delete them later, once nothing needs consulting.
  - No upstream merging or tracking is expected. Any rebase onto
    `RetroBSD/retrobsd` is done manually, per patch, and only where relevant.

## D12: Soft float is rejected; a 68881/68882 FPU is mandatory
- **Context:** The initial plan (D3, D10) reused the compiler-rt-style soft-float
  code in `src/libc/runtime/`. It is clean, but validating it for correctness
  on m68k would be too much effort. All hardware platforms of interest come with
  a 68881 or 68882 FPU.
- **Decision:** Floating point uses the hardware FPU. Code is built with
  hardware float (`-mcpu=68030 -m68881`, which is GCC's default for this
  target); `-msoft-float` is not used. Systems without an FPU are unsupported.
- **Verified (M0):** a freestanding program using `double` arithmetic builds
  with no undefined symbols (no `libgcc` FP helpers) and runs correctly on
  QEMU `virt` with both `-cpu m68030` and `-cpu m68040`.
- **Consequences:**
  - The FP files in `src/libc/runtime/` (`adddf3.c`, `muldf3.c`, `fp_*`, ...)
    are not used on m68k. Leave them in place (PIC32 reference, D11). The 64-bit
    integer helpers there (`divdi3.c`, `udivmoddi4.c`, ...) are still needed.
  - The kernel must save and restore FPU state across context switches and
    signals (`fsave`/`frestore`, `fmovem`), and handle FPU exceptions and the
    line-F trap. This is new machine-dependent work (backlog M3).
  - The kernel itself should not use floating point; check the kernel
    objects for FPU instructions.
  - Userland FP code (libc `atof`, `ecvt`/`gcvt`, `printf` `%f`, `libm`) uses
    the FPU directly. Their MIPS-specific or soft-float assumptions need
    auditing (M5).
  - QEMU's m68k CPUs provide an FPU, so bring-up on `virt` needs no emulation
    of one.

## D13: Source layout
- **Decision:**
  - **Kernel:** machine-dependent code goes in `sys/m68k/`. It is flat, and
    board-specific drivers use name prefixes (`gf_tty.c`, `virtio_blk.c`).
    - `sys/m68k/` holds `files.kconf`, `devices.kconf`, `Makefile.kconf` and
      `Config.generic`, because kconfig reads them from `../` relative to a
      board directory.
    - `files.kconf` is built by copying the machine-independent `kernel/*`
      section from `sys/pic32/files.kconf` and writing the `m68k/*` section
      fresh. It lists only files that exist.
    - The QEMU board is `sys/m68k/virt/`, containing `Config`, the generated
      `Makefile` and `kernel.ld`.
  - **Userland:** assembly goes in `src/libc/m68k/`, the C runtime start-up in
    `src/startup-m68k/`, and the linker script in `src/elf32-m68k.ld`. These
    names follow from `MACHINE = m68k` in `target.mk`, which `src/Makefile` and
    `src/libc/Makefile` use to select `startup-$(MACHINE)` and `$(MACHINE)`.
  - **Reference code:** `sys/pic32/`, `src/libc/mips/`, `src/startup-mips/` and
    `src/elf32-mips.ld` are not moved or renamed. They stay untouched until
    their m68k counterparts exist (D11), and are deleted later.
  - **Headers:** `include/machine` is retargeted to `../sys/m68k` in M1,
    together with `MACHINE = m68k`. `include/pic32` is removed and no
    `include/m68k` link is added. Nothing includes `<pic32/...>`; everything
    goes through `<machine/...>`.
- **Consequences:** the tree holds PIC32 and m68k code side by side for a while.
  Nothing builds both. The sizing compile of `sys/kernel` in M1 needs real
  machine headers, so minimal `sys/m68k/` headers are created in M1.
- **History:** amended after review. The original text put the symlink change
  in M2 and added an `include/m68k` link; both were dropped.

## D14: Build files are converted in place
- **Decision:** `target.mk`, `sys/pic32/Makefile.kconf` and `clang-config.mk`
  are edited in place for GCC and m68k, rather than duplicated as variants or
  selected by an `ARCH=` switch. This is the first step of M1. The old versions
  remain in git history. `clang-config.mk` is deleted once nothing includes it.
- **Consequences:** the PIC32 build breaks at that point, as allowed by D11.
  `sys/pic32/Makefile.kconf` keeps its name until the m68k board layout exists
  (D13), and is then moved.

## D15: Branch policy
- **Status:** Accepted.
- **Decision:** `retrobsd-68k` is the long-lived working branch, kept on the fork
  `denny-futurice/retrobsd` (`origin`). `master` stays an untouched snapshot of
  upstream and is not rebased or merged into the working branch (D11). The
  `upstream` remote (`RetroBSD/retrobsd`) is kept for reference and for
  cherry-picking individual patches by hand where relevant. Pushing is done
  manually by the owner.

## D16: Machine constants and memory map for QEMU virt
- **Status:** Proposed; values to be revisited once the kernel links (M2) and
  the first processes run (M5).
- **Decision:** `sys/m68k/machparam.h` keeps the PIC32 table sizes (`NBUF`,
  `NPROC`, ...), `HZ` and `DEV_BSIZE`, and sets:
  - `ENDIAN = BIG`.
  - A fixed memory map in low RAM: kernel (vector table, image, stack) at
    `0x00000000`-`0x0007ffff`, the single user region at
    `0x00080000`-`0x000bffff` (256 KiB, `USER_DATA_START`/`USER_DATA_END`).
  - `USIZE = 4096` (PIC32: 3072), to leave room for 68030 exception frames on
    the kernel stack.
  - Inside the kernel region (`sys/m68k/virt/kernel.ld`): vector table at 0
    (VBR stays 0), kernel image from `0x400`, then `u0` and `u` as the top two
    `USIZE` blocks, with the kernel stack ending at `u_end` =
    `KERNEL_DATA_END`, as on PIC32. User programs are linked at
    `USER_DATA_START` by `src/elf32-m68k.ld`.
  - Two interrupt levels, as on PIC32: `spl*()` either masks all interrupts
    (IPL 7) or none, via the status register. Real Goldfish PIC levels come
    later if needed.
  - `long double` is the 96-bit 68881 extended type (`float.h`), unlike PIC32,
    where `double` was 32 bits.
- **Consequences:** the 256 KiB user region and 2 MiB `SWAP_MBYTES` hold few
  swapped processes; swap size is revisited with the user region.

## D17: Keep GCC's default m68k data alignment
- **Decision:** use GCC's default m68k ABI, which aligns `int`, `long`,
  pointers and floating types to 2 bytes. `-malign-int` is not used.
- **Rationale:** performance is not critical yet. `-malign-int` would save bus
  cycles on misaligned 32-bit and `double` accesses, but it must be applied to
  every object, is a rarely tested GCC mode, and changes no on-disk layout:
  `dinode`, `fs`, `direct` and the a.out `exec` header have identical field
  offsets either way. Compatibility with existing binaries is not required.
- **Consequences:** an `-malign-int` build is kept as a measured experiment in
  the M6 performance item.

## D18: PIC32 hardware programs are not built; disabled programs stay listed
- **Context:** `src/cmd` has demos and tools for PIC32 peripherals: `adc-demo`
  (ADC), `glcdtest` (graphic LCD), `portio` (GPIO), `pwm` and `wiznet`
  (WIZnet Ethernet). QEMU virt has no equivalent hardware.
- **Decision:** they are not built for m68k. Programs left out of the build,
  these and the native toolchain (D8), are kept as commented-out
  `SUBDIR +=` / `STD +=` lines in `src/cmd/Makefile`, so a group is re-enabled
  by uncommenting one line. Their sources stay in the tree (D11).
- **Consequences:** their `rootfs.manifest` entries are removed with the M4
  root filesystem task.

## D19: Signal set operations are functions, not header macros
- **Context:** `sys/include/signal.h` defined `sigemptyset`, `sigfillset`,
  `sigaddset`, `sigdelset` and `sigismember` as comma-expression macros. Used
  as statements, GCC reports `-Wunused-value`, which `-Werror` makes fatal in
  six libc files. A GCC statement-expression rewrite would also work.
- **Decision:** remove the five macros (keep `sigmask`) and declare the
  functions, which `src/libc/gen/sigsetops.c` already provides. This is plain
  C and avoids a GCC-specific construct. `sigismember` in `sigsetops.c` is
  fixed at the same time: it tested `~sigmask(signo)` (also wrong upstream,
  hidden by the macro).
- **Consequences:** each use is a function call; acceptable for now, and an
  M6 performance item re-evaluates it.

## D20: M2 boots without storage: stubbed root device and M3 stubs
- **Context:** kconfig only emits the kernel link target for a
  `config unix root on <dev>` line, and `main()` always mounts the root.
  The virtio block driver is M4, and many machine-dependent routines
  (`copyin`, `sendsig`, `setjmp`, the trap frame users, ...) are M3. Linking
  the `sys/kernel` objects leaves 74 undefined symbols.
- **Decision:**
  - The virt board `Config` uses kconfig's raw device-number form,
    `root on major 0 minor 0` and `swap on major 0 minor 1`, so no
    `devices.kconf` entry is needed. Block major 0 in the m68k `devsw.c` is
    a "no disk" entry whose open fails.
  - `sys/m68k/stubs.c` defines every routine M2 does not implement; each
    panics with "not implemented".
  - M2 is done when the kernel boots on QEMU and prints
    `panic: No root filesystem found!` from `main()`. That output replaces
    a separate banner as the M2 proof.
- **Consequences:** the M2 boot test exercises startup, the console,
  `printf`, table setup and `panic()` without a disk. M3 empties and
  deletes `stubs.c` (done, D32); M4 replaces the stubbed root and swap with the virtio
  block device in `Config` and `devices.kconf` (done, D36).

## D21: Running the first user process moves from M3 to M4
- **Context:** M3 listed "run a static `init` as a user process". That
  needs a root filesystem (`main()` panics at `mountfs` before it creates
  process 1, D20), storage is M4, and a real `init` needs the M5 libc
  (`crt0`, syscall stubs) and a `/dev/console` node on the image.
- **Decision:** M3 ends when the machine-dependent kernel code (traps,
  clock, context switch, syscalls, signals, FPU, process 1 entry) exists,
  `stubs.c` is gone, and each part is proven by its own smoke test. The
  kernel boot test still ends at the D20 root filesystem panic. Running
  process 1 moves to the end of M4, with a freestanding test `init` on the
  root image; the real `/sbin/init` comes with M5.
- **Consequences:** M3 smoke tests drive user mode, `trap #0` and context
  switches directly, without `main()`. The first end-to-end run of process
  1 happens only once the root filesystem mounts.

## D22: The trap frame is a C struct over the hardware exception frame
- **Context:** PIC32 trap entry saves the user registers as an array of
  32-bit words; `u.u_frame` (`int *`) points at it and the kernel indexes it
  with `FRAME_*` (`exec_subr.c`, `sys_process.c`, `sys_pipe.c`, the trap and
  signal code). The 68030 pushes its own frame: a 16-bit SR, a 32-bit PC at
  byte offset 2, a format/vector word, then format-dependent words (bus and
  address errors push 16- or 46-word frames). The PC cannot be an `int[]`
  index. The alternative, copying `usp`, `sr` and `pc` into a word array on
  entry and back on exit, was rejected for code clarity.
- **Decision:**
  - `sys/m68k/frame.h` defines `struct trapframe` after NetBSD's
    `m68k/include/frame.h`: `tf_regs[16]` (`d0`-`d7`, `a0`-`a7`, where `a7`
    is the user stack pointer), `tf_pad`, `tf_stackadj`, then the hardware
    frame `tf_sr`, `tf_pc` and `tf_format`/`tf_vector`. With GCC's 2-byte
    `int` alignment (D17) `tf_pc` follows `tf_sr` without `__packed`.
  - Trap entry leaves the hardware frame in place and pushes the registers
    and `usp` below it; exit restores them and `rte`s through it. No
    `sr`/`pc` copies.
  - `u.u_frame` becomes `struct trapframe *` (`sys/include/user.h`); the
    `FRAME_*` indexes go away. `exec_subr.c`, `sys_process.c` and
    `sys_pipe.c` use the struct fields (M3 task 5). FP registers are not in
    the frame: they are in the `u` area (task 3) and the `sigcontext`
    (task 6).
  - `u.u_frame` is set only for traps taken from user mode, as on PIC32.
  - Writes to `tf_sr` from ptrace or `sigreturn` keep only the condition
    codes and the trace bit; the supervisor bit and the interrupt mask are
    forced to user values.
  - A user bus or address error is never retried (no MMU, nothing to fix):
    the process gets a signal, and the exit path replaces the long fault
    frame with a format 0 frame (NetBSD's `tf_stackadj`) so execution goes
    on at `tf_pc`.
  - ptrace single step (`PT_STEP`, PIC32 `ST_RP`) sets the trace bit `T1`
    (`0x8000`) in `tf_sr`.
  - NetBSD's m68k trap, exception return and signal code (`trap.c`, the
    `locore`/`trap_subr` exit path, `sig_machdep.c`) is the reference for
    tasks 1, 4 and 6; its BSD licence is compatible and copied code keeps
    the NetBSD copyright notices.
- **Consequences:** about 80 lines change in machine-independent kernel
  files (`user.h`, `exec_subr.c`, `sys_process.c`, `sys_pipe.c`), which is
  acceptable since MIPS no longer has to build (D11). `adb`, the only
  userland `FRAME_*` user, is out of scope (D8). Register order for ptrace
  and GDB stays `d0`-`a7`, `sr`, `pc`.

## D23: Syscall ABI: `trap #0`, number in `d0`, arguments on the user stack
- **Context:** PIC32 encodes the syscall number in the `syscall`
  instruction, passes arguments in `a0`-`a3` (5 and 6 on the stack), and on
  error returns to a different instruction in the stub with `errno` in
  `t0`. The 68030 has only 16 trap vectors, so the number cannot be in the
  instruction. The kernel needs at most 6 arguments, all 32-bit (`off_t` is
  `long`); only `pipe` returns a second value; `fork` returns 0 in the
  child and the pid in the parent as its normal result; `sigaction` takes
  the libc signal trampoline address as a hidden 4th argument. The ABI is
  fixed now because the kernel syscall path (M3 task 4), the signal code
  (task 6) and the M5 libc stubs depend on it.
- **Decision:** the BSD m68k convention (NetBSD, 4.4BSD):
  - System calls are `trap #0` with the syscall number in `d0`. Numbers
    out of range go to `sysent[0]` (`nosys`).
  - Arguments stay on the user stack where GCC's calling convention puts
    them: the kernel reads `sy_narg` longs from `usp + 4` (above the stub's
    return address) after a `baduaddr` check, and fails with `EFAULT` if
    they are not in the user region. A stub is
    `moveq #SYS_x,%d0; trap #0; bcs cerror; rts` (`move.l` instead of
    `moveq` for numbers above 127). Passing arguments in
    registers (`d1`-`d5`, `a0`, as Linux m68k does) was rejected: every
    stub would load and save registers, and the only gain is not reading
    user memory, which without an MMU is a plain load.
  - Results: `d0`, and `d1` for the second value (`pipe`).
  - Errors: the carry flag is set and `d0` holds `errno`; the stub branches
    with `bcs cerror`. Success clears the carry, so any `d0` value is a
    valid result.
  - The kernel preserves every register except `d0`, `d1` and the
    condition codes; GCC treats `d0`, `d1`, `a0` and `a1` as clobbered by
    calls anyway.
  - `ERESTART` sets `tf_pc` back by 2 (the size of `trap #0`) and leaves
    `d0`, which still holds the syscall number. `EJUSTRETURN`
    (`sigreturn`) leaves the trap frame (D22) untouched.
  - The libc `sigaction` stub passes the trampoline address as a real 4th
    stack argument. The trampoline calls `sigreturn`, then `exit` if that
    fails, as on PIC32; its frame layout is M3 task 6.
  - `trap #1` to `trap #15` from user mode raise `SIGILL`; none is reserved
    while debuggers are out of scope (D8).
- **Consequences:** syscall stubs are four instructions. The convention
  also works with an MMU, as NetBSD on the 68030 shows: only the kernel's
  argument fetch changes, from a range check and direct load to a `copyin`
  (with `moves` and fault recovery, which an MMU kernel needs for
  `copyin`/`copyout` anyway); libc stubs, results, error flag and restart
  rule stay the same.

## D24: PIC32-only syscalls and machine-dependent sysctl
- **Context:** `sys/pic32/sysctl.c` holds three 2BSD-style syscalls and
  `cpu_sysctl`. `ucall` (154, superuser only) calls any kernel function at
  a given `spl`; it comes from 2.11BSD on the PDP-11, where PIC32 used it
  for driver probe and attach code. `ustore` (152) and `ufetch` (153,
  superuser only) write and read one word at any address; they are
  RetroBSD additions that reused 2.11BSD's PDP-11 slots `nostk` and
  `fetchi`, so flash and I/O registers could be reached from user mode.
  4.4BSD and NetBSD have none of the three. Users in `src/`: `basic`
  PEEK/POKE (`ufetch`/`ustore`); `adb`'s opcode names (out of scope, D8);
  the `autoconfig(8)` man page mentions `ucall`. `cpu_sysctl` mixes
  machine-independent entries with PIC32 hardware ones.
- **Decision:**
  - `ustore`, `ufetch` and `ucall` are removed. Slots 152-154 in
    `init_sysent.c` become `{ 0, nosys }` (`"#152"`-`"#154"` in
    `syscalls.c`) and stay reserved, so later syscall numbers do not move.
    `SYS_ustore`/`SYS_ufetch`/`SYS_ucall`, the `systm.h` prototypes, the
    stubs, the `ucall(2)` man page and its `rootfs.manifest` entry go; the
    libc wrappers are not carried over to `src/libc/m68k`. The PIC32 and
    MIPS sources stay as reference (D11).
  - `basic` PEEK/POKE become no-ops (POKE does nothing, PEEK returns 0) so
    the userland builds; they are revisited, at low priority, in M5.
  - `cpu_sysctl` moves to `sys/m68k/sysctl.c` with the three ids that
    `sys/m68k/cpu.h` already keeps: `CPU_CONSDEV` (console device),
    `CPU_ERRMSG` (the `errlist` table, for libc `strerror`) and `CPU_NLIST`
    (the kernel symbol table, for `knlist` in `ps`, `pstat`, `vmstat`,
    `iostat`, `fstat` and `devupdate`). The PIC32 SD card timeouts, CPU and
    bus clock rates and TMSCP are dropped. `CPU_NLIST` copies the symbol
    name in with `copyin` (at most 16 bytes) instead of reading the user
    pointer directly, so a bad address gives `EFAULT`.
- **Consequences:** a program using syscalls 152-154 gets `SIGSYS`/`ENOSYS`
  like any unused slot. User code can no longer call kernel functions;
  that would also be wrong under a future MMU.

## D25: Code QEMU cannot exercise is written against the manuals
- **Context:** A probe on QEMU 11.1.2 (`-M virt -cpu m68030`) showed that
  QEMU raises illegal instruction, line A/F, privilege violation, trace,
  divide by zero, `CHK`, `TRAPV` and `trap #n` correctly, but does not
  emulate: bus errors (unassigned reads return data, writes are dropped;
  only `-cpu m68040` faults, with a 68040 frame), address errors (an odd
  jump target gives an illegal instruction), FPU exceptions (enabled
  exceptions only set `FPSR` bits), `fsave`/`frestore` (illegal on the
  68030 model), `rte` format errors and the 68030 PMMU (`pmove` is a line F
  trap). Musashi, checked as an alternative, has no 68030 bus or address
  error frames and no FPU exceptions either. The affected code is small and
  isolated: the bus/address error entry and frame trimming, the FPU
  exception entry, and the FPU state save in the context switch and signal
  frame.
- **Decision:** that code is written against the MC68030 and MC68881/68882
  user's manuals (and NetBSD as reference, D22) without emulator
  validation for now. Its smoke tests on QEMU hand-build the exception
  frames and FPU state frames from the manuals and enter the handlers
  directly, so the handler logic is tested even though the CPU behaviour
  is not. Real validation waits for the first real hardware board, the
  Amiga, which brings WinUAE into the flow (M6). No other emulator core is
  integrated now.
- **Consequences:** a misreading of the manuals shows up only on WinUAE or
  hardware; the M6 validation task lists the code to recheck. The QEMU
  limits are listed in `docs/porting/README.md`. `fsave`/`frestore` being
  illegal on QEMU's 68030 must be handled by the context switch (M3 task
  3), which decides how.

## D26: User bus and address errors are not restarted
- **Context:** a bus or address error leaves a format A or B frame (32 or
  92 bytes) whose `rte` reruns the faulting bus cycle. NetBSD keeps that
  frame and, when it delivers a signal, copies the extra state into the
  signal frame so that `sigreturn` can restore it and rerun the cycle. That
  serves demand paging, where the fault is fixed and the cycle retried.
  Without an MMU (D4) nothing can fix the fault: a rerun faults again.
- **Decision:** `trap()` (`sys/m68k/trap.c`) posts `SIGBUS` with the fault
  address in `u.u_code` and trims the frame to format 0 straight away:
  `tf_stackadj` is set to the dropped size, and the exit path in
  `sys/m68k/locore.S` moves SR and PC up over the dropped part (NetBSD's
  `faultstkadj`). The process resumes at the frame's PC, if it resumes at
  all. Kernel-mode bus and address errors panic.
- **Consequences:** the signal frame (M3 task 7) never holds bus fault
  state. A `SIGBUS` handler that returns resumes at the frame's PC, which
  on the 68030 is not exactly the faulting instruction; handlers are
  expected to exit or `longjmp`, and an ignored `SIGBUS` may fault again.
  Demand paging with an MMU (D23 notes) would need the NetBSD scheme
  instead.

## D27: First Goldfish RTC is the clock tick, second is the time of day
- **Context:** QEMU `virt` has two identical Goldfish RTCs: RTC 1 at
  `0xff006000` (PIC 6 source 1) and RTC 2 at `0xff007000` (PIC 6 source 2),
  both at IPL 6. Each counts nanoseconds since the epoch and has one
  alarm. Writing an RTC's time moves its alarm with it, and a `TIME_LOW`
  read latches the `TIME_HIGH` value, so two users doing 64-bit reads of
  the same RTC can corrupt each other. Linux (`arch/m68k/virt`) and NetBSD
  (`sys/arch/virt68k/dev/mainbus.c`) both use RTC 1 as the timer and RTC 2
  as the time-of-day clock.
- **Decision:** the same layout. RTC 1 is the clock tick: its alarm is
  re-armed each tick at the previous deadline plus `1e9 / HZ` ns (5 ms at
  `HZ` 200), never from the current time. The tick handler does not read
  the time; if ticks were missed the new deadline is already past, the
  alarm fires again at once and the missed ticks are delivered in a burst,
  as in NetBSD. `udelay` keeps reading RTC 1's `TIME_LOW`. RTC 2 is the
  time of day: read once at boot, written only if the kernel ever sets
  the hardware clock, and its interrupt stays disabled. Both RTCs have
  their interrupt, alarm and pending interrupt cleared at startup.
- **Consequences:** only PIC 6 source 1 is enabled for the clock. Setting
  the hardware time cannot disturb the tick. `HZ` stays 200 (an exact
  number of nanoseconds per tick).

## D28: User FPU state saved with or without `fsave`, chosen per board
- **Context:** the user FPU state must follow each process across the
  `u`/`u0` exchange in `longjmp` (D12). On a 68881/2 the full state is the
  `fsave` frame (up to 216 bytes for a busy 68882) plus `fp0`-`fp7` and the
  control registers; `fsave` also waits for a running FPU operation and
  keeps a pending FPU exception with the process. QEMU implements `fsave`/
  `frestore` only for its 68040 model; on `-cpu m68030` both are illegal
  instructions (`docs/qemu`, `target/m68k/translate.c`). QEMU's
  `fmovem.l` of several control registers also stores and loads them in
  reverse order (FPIAR at the lowest address, where the 68881/2 puts FPCR).
- **Decision:** `struct user` gets a `u_fpstate` area (frame, registers,
  FPCR/FPSR/FPIAR, 324 bytes) under `__mc68000__`. `longjmp` saves the FPU
  into `u` before the exchange and restores it from the new `u` after, only
  when the process changes; `fork()` calls the machine-dependent
  `savefpu()` before `swapout()` copies the live `u` for the child. By
  default the save is NetBSD's: `fsave`, and `fmovem` of the registers
  unless the frame is null; the restore mirrors it and ends in `frestore`.
  A board whose CPU model lacks `fsave` sets the kconfig option
  `FPU_NO_FSAVE` and saves only the registers; `virt` does. The control
  registers are always moved one at a time, so the layout is the same on
  QEMU and hardware. Option B (probe `fsave` at boot) and C (registers
  only everywhere) were rejected: B adds a check per switch, and C on a
  real 68881/2 would let a pending exception surface in the wrong context.
- **Consequences:** the `fsave` path is assembled but untested until a
  hardware board runs (WinUAE, M6; as D25). The kernel stack in `u` shrinks
  by 324 bytes (`sizeof(struct user)` 1330 of `USIZE` 4096). `label_t`
  gains the PC (13 words). The kernel must not use FPU registers itself
  outside these paths (checked in the disassembly: only `fpfault` and
  `fpfault_clear`).

## D29: QEMU source as a documentation submodule
- **Context:** several decisions (D25, D27, D28) depend on what QEMU's
  `virt` machine and 68030 model actually do, and checking that against
  fetched web copies is slow and unpinned.
- **Decision:** `docs/qemu` is a shallow git submodule of
  `https://gitlab.com/qemu-project/qemu.git` at tag `v11.1.2`, the QEMU
  version the smoke tests run on. It is reference material only; nothing
  builds from it. `git submodule update --init` fetches it.
- **Consequences:** when the QEMU used for `make check` changes version,
  the submodule moves to the matching tag in the same commit.

## D30: Exec entry stack: `sp` at `argc`, `argv` and `envp` vectors above it
- **Context:** PIC32 `exec_setupstack` passes `argc`, `argv` and `envp` in
  `$a0`-`$a2` and leaves four empty argument slots below `argv`. The m68k
  has no argument registers in its calling convention, so `crt0` (M5) and
  the kernel must agree on where a new program finds them. Option A was a
  ready-made C call frame (`sp` at an empty return address, then `argc`
  and pointers to the two vectors), so `crt0` could stay in C; option B
  the traditional Unix layout.
- **Decision:** option B, the layout of V7, 2.11BSD on the PDP-11, 4.3BSD
  on the VAX, and NetBSD and Linux on m68k. On entry `sp` points at
  `argc`; `argv[0]` .. `argv[argc-1]`, a null pointer, `envp[0]` ..
  `envp[envc-1]` and a null pointer follow at increasing addresses, then
  the argument and environment strings. The top word of the stack points
  at `argv[0]` for `ps`, as on PIC32. All other registers are zero and SR
  is 0 (user mode, IPL 0). `exec` also resets the FPU (D28: `u_fpstate`
  zeroed and loaded; with `fsave` that is a null frame `frestore`).
- **Consequences:** `crt0` needs a few instructions of assembly to compute
  `argv` (`sp + 4`) and `envp` (`argv + 4 * (argc + 1)`) before calling C.
  The PIC32 code reserved four times too many words for `argv` (pointer
  arithmetic on `char **`), leaving a gap before `envp`; the m68k code
  packs the vectors, which `stack.len` in `exec_aout.c`/`exec_elf.c`
  (`argc + envc + 4` words) covers exactly. Changing the layout later means
  changing the kernel and `crt0` together.

## D31: Signal context: every register in a public, extensible `sigcontext`
- **Context:** M3 task 7 needs an m68k `struct sigcontext` for `sendsig`
  and `sigreturn`. Three layouts were weighed. A: every user register in
  the public `sigcontext` (PIC32, Linux m68k). B: 4.4BSD hp300 and NetBSD
  m68k, a small public `sigcontext` (mask, `sp`, `fp`, PC, SR) followed by
  a private kernel area with the other registers, extra exception frame
  words and FPU state. C: 2.11BSD on the PDP-11, where the kernel saves only
  the scratch registers and relies on the C calling convention to keep
  the rest across the handler. C saves about 48 bytes a signal, but fails
  for handlers that do not follow GCC's convention and hides registers a
  handler may need to read or change (debuggers, emulation, `SIGFPE` fix-up).
  Without an MMU, user register values grant no privilege; only SR and PC
  need checks, in every option.
- **Decision:** option A. `sigcontext` holds the old signal mask and
  alternate stack state, `d0`-`a7`, PC and SR, so the trampoline only
  calls `sigreturn`. Two additions keep it open for an MMU:
  - A flags/size word says which optional parts follow, so later state
    (bus fault state for restartable faults with demand paging, which D26
    rules out today) can be added behind it as a private tail, as in B.
  - The FPU state is the last member, so such a tail never moves a public
    field.
  `sendsig` builds the context in a kernel buffer and `copyout`s it, and
  `sigreturn` `copyin`s and checks it, instead of using user pointers
  directly. Without an MMU these are range checks and copies; with one they
  become the real `copyin`/`copyout`, and the layout does not change.
  - Signal frame, lowest address first: the trampoline address as the
    handler's return address, `sig`, `code`, `scp`, then the
    `sigcontext`. The handler returns with `sp` at `sig`; the trampoline
    does `pea 12(%sp)`, pushes a dummy return address and calls
    `sigreturn(scp)` (D23), then `exit` if that fails. It does not use the
    `scp` slot, which a C handler may overwrite.
  - FPU: `sigcontext` ends with the whole `struct fpstate` (D28, 324
    bytes), now in `<machine/fpu.h>` so userland sees it. The handler
    starts with a reset FPU, as after exec (D30), so a pending FPU
    exception cannot fire in it; `sigreturn` loads the saved state.
  - The handler is entered from a format 0 frame without tracing:
    `sendsig` trims any other format (`trimframe`, D26), so `rte` cannot
    resume a half-done FPU instruction (format 9), and clears `T1`/`T0`;
    the saved SR keeps them for `sigreturn`.
  - `sigreturn` refuses (`EINVAL`) unknown flags, an odd PC, and an FPU
    frame `frestore` would reject with a kernel-mode format error: only a
    null frame, or one with this FPU's version byte (from the last
    non-null frame `sendsig` saved) and a 68881/2 idle or busy size. SR
    keeps only the user bits (D22), and the mask drops `SIGKILL`/`SIGSTOP`.
- **Consequences:** the layout is user-visible ABI from M5 on (libc, the
  trampoline, any handler using `scp`); a later change needs the flags word
  or a new layout. The kernel copy is 424 bytes of kernel stack, mostly
  the FPU area, which fits in `u` (D28). The version check and the `fsave`
  frame sizes cannot run on QEMU (D25) and are rechecked in M6.

## D32: Process 1 entry and `icode` in `startup.S`
- **Context:** `main()` returns in process 1 after copying `icode` to
  `USER_DATA_START` (`init_main.c`), and the reset code must then run it in
  user mode, as PIC32's does. `icode` was an empty array in `stubs.c`.
- **Decision:**
  - `startup.S` handles the return from `main()`: IPL 7, kernel stack back
    at `u_end`, `usp` at `USER_DATA_END`, every data and address register
    cleared, then `rte` through a format 0 frame with SR 0 (user mode,
    IPL 0) to `USER_DATA_START`.
  - `icode`, `initflags` and `icodeend` are assembly in `startup.S`, and
    `stubs.c` is deleted. The code is position independent (PC-relative
    addresses) and builds `argv` on the user stack, so it needs no
    relocation after the copy; system calls follow D23, and a failed
    `execv` ends in `exit(errno)`. `initflags` is `"-"` plus spare bytes,
    as on PIC32, for `main()`'s `"-s"` patch.
  - `startup.S` includes no C headers, so `sys/m68k/virt/kernel.ld` defines
    `USER_DATA_START`/`USER_DATA_END` next to its copy of
    `KERNEL_DATA_END`; they must match `machparam.h`, which the `uentry`
    test checks.
- **Consequences:** a `main()` that returns now runs whatever is at
  `USER_DATA_START`; smoke tests that do not want that must halt
  (`startup` writes the virt controller's halt command). The kernel boot
  test still ends at the D20 panic, before process 1 exists (D21).

## D33: Console receive interrupt; interrupt handlers do not nest
- **Context:** the Goldfish TTY raises PIC 1 source 32 (IPL 1) while its
  128-byte input buffer holds data; `READ_BUFFER` copies input to memory.
  The 68030 lets a higher interrupt level preempt a lower one, so the
  clock (IPL 6) could interrupt a tty handler halfway through `ttyinput()`
  and run callouts on the same queues. PIC32 interrupts did not nest, and
  D16 has only two `spl` levels.
- **Decision:**
  - `gfttyintr()` reads all buffered input in 32-byte batches and passes
    each byte to `ttyinput()`, as PIC32's `uartintr()` does. The first
    open enables the interrupt at the PIC and the device; close leaves it
    on, as on PIC32. Before that the console reads input by polling
    (`gfttygetc`).
  - `intr()` dispatches by PIC level and source with fixed cases, like the
    clock; a handler table can come with virtio in M4.
  - `intr()` raises the IPL to 7 before calling any handler, so handlers
    never nest; the exception return restores the interrupted level.
- **QEMU notes** (affect tests only):
  - `goldfish_tty` does not tell the character backend when its buffer has
    room again, so input past the first 128 bytes waits until QEMU's main
    loop wakes for something else; idle, that can take a second. The clock
    tick timer wakes it every tick, so the tests run the clock.
  - While an interrupt is raised but masked, the TCG vCPU takes and drops
    QEMU's global lock on every translated block, starving the main loop
    and its timers; a clock alarm does not fire then. The `intrnest` test
    turns the tty interrupt off while it stays in the handler.
- **Consequences:** interrupt latency is the longest handler. Separate
  interrupt levels can come later with real `spl` levels (D16).

## D34: 68030 caches stay off until M6
- **Context:** the 68030 has a 256-byte instruction cache and a 256-byte
  data cache, controlled by `CACR`. Without an MMU, every process runs at
  the same fixed addresses, so the instruction cache goes stale whenever
  the kernel writes code there: swap-in (the `u`/`u0` exchange in
  `longjmp`), `exec` loading a program, and `ptrace` writes. The data
  cache goes stale when a device writes memory (the Goldfish TTY's
  `READ_BUFFER`, virtio in M4, Amiga chip DMA), and without an MMU only
  the board's hardware (`CIIN`) can keep device registers out of it.
  QEMU emulates neither cache: it only stores the `CACR` bits, so no cache
  handling can be tested before M6 (D25).
- **Decision:** the kernel turns both caches off early in `_start`, with one
  `movec` that also clears them (`CI|CD`), instead of relying on the reset
  value: a bootloader such as Kickstart may hand over with the caches on.
  There is no cache flushing anywhere in the kernel.
- **Consequences:** correct on any 68030 regardless of caching hardware,
  at the cost of speed on real hardware. Cache management is an M6 task,
  on WinUAE, where it can be tested: enable the instruction cache (with
  burst fill), clear it after a process switch, at the end of
  `exec_clear()` and after `ptrace` writes, and decide on the data cache
  together with `CIIN` and device input. Supersedes the "cache enable"
  item of the assessment (`01-assessment.md`, section 2).

## D35: Machine-dependent sysctl requests skip the root check
- **Context:** `__sysctl()` in `sys/kernel/kern_sysctl.c` refuses any call
  that passes new data (`new != NULL`) unless the caller is root, before it
  looks at the name. `CPU_NLIST` (D24) is read-only but passes the symbol
  name as new data, so libc's `knlist` fails with `EPERM` for everyone but
  root. `ps`, `pstat`, `vmstat`, `iostat` and `fstat` are setgid, not
  setuid (`rootfs.manifest`), so they do not work for ordinary users. The
  PIC32 kernel has the same problem; it went unnoticed because RetroBSD on
  PIC32 is normally used as root. A port where `ps` works only for root is
  not acceptable.
- **Decision:** `__sysctl()` skips the root check for `CTL_MACHDEP` and
  leaves write permission to `cpu_sysctl()`. None of its three requests
  writes anything: `CPU_CONSDEV` refuses new data with `EPERM`
  (`sysctl_rdstruct`), `CPU_ERRMSG` ignores it and `CPU_NLIST` only reads
  the name. A writable machine-dependent request added later must call
  `suser()` itself. Every other top level keeps the root check.
- **Consequences:** `ps` and the other `knlist` users work for ordinary
  users, as far as the kernel is concerned. The check now runs after the
  name is copied in, so a bad name pointer gives `EFAULT` before `EPERM`.

## D36: virtio block driver `vd`, interrupt driven, MBR partitions
- **Context:** QEMU's m68k `virt` board has 128 virtio-mmio slots
  (`0xff010000`, `0x200` apart, interrupts on Goldfish PICs 2-5, source
  slot % 32 + 1) and forces the modern (virtio 1.0) interface: the
  registers, the rings and the configuration space are little-endian.
  QEMU fills the slots from 127 down, in command-line order. PIC32's
  `sd.c` does each transfer synchronously in `strategy` and copies the MBR
  partition entries straight into `struct diskpart`, whose `u_int` fields
  are little-endian on disk. Polling for completion with interrupts masked
  is the state in which QEMU's main loop, which completes disk requests,
  is starved by a pending masked interrupt (D33).
- **Decision:**
  - `sys/m68k/vd.c` ("virtio disk", block major 0, `/dev/vd0`, `vd0a`...)
    scans all 128 slots, from 127 down, when the first unit is probed, so
    `vd0` is the first `-device virtio-blk-device` on the QEMU command
    line. It accepts only `VIRTIO_F_VERSION_1` and reads the device's
    read-only flag; every value exchanged with the device is byte-swapped.
    Registers are read and written with `move.l` in inline assembly: GCC
    turns a masked `volatile` read into a byte `btst`, and virtio-mmio
    ignores accesses that are not 32 bits wide.
  - I/O is interrupt driven: `vdstrategy()` queues the buffer and starts
    the request; the interrupt, dispatched by `intr()` (D33), finishes it
    with `biodone()` and starts the next. One request is in flight per
    disk, on a static queue of 4 descriptors. The caches are off (D34), so
    the device writes straight into kernel or user buffers.
  - Partitions follow PIC32: an MBR in sector 0, minor = unit * 8 +
    partition, partition 0 is the whole disk, 1-4 the MBR entries. Writes
    to sector 0 are refused. The MBR is parsed by `sys/m68k/disklabel.c`,
    separate from the driver, so later disk drivers (Amiga IDE or SCSI)
    share it and an Amiga RDB parser can be added next to it (M6). Three
    partition bits allow 7 partitions per disk, enough for an Amiga disk
    with AmigaOS and BSD partitions.
  - The board `Config` puts root on `vd0a` and swap on `vd0b`. The `/dev`
    names in `rootfs.manifest` and `etc/fstab` change from `sd` to `vd`
    with the root filesystem task.
- **Consequences:** the driver needs a working `sleep()` in `biowait()`,
  including at boot when the root is opened. Without a disk, or without a
  filesystem on it, the kernel still stops at the D20 root panic until the
  mount task.

## D37: Big-endian on-disk and data formats, converted by the host tools
- **Context:** the endianness audit (M4) found that every file the build
  writes for the target assumes little-endian MIPS. `tools/fsutil` writes
  the superblock, inodes, directories, indirect and free-list blocks
  little-endian, through explicit byte helpers, open-coded shifts and raw
  host `unsigned` arrays. `tools/elf2aout` reads the ELF headers and
  writes the a.out header and symbol table in host order. The kernel and
  userland read all of these natively. The MBR is little-endian by
  definition, and the virtio interface is little-endian (D36). Some data
  files are byte streams or already big-endian (zoneinfo, `bogdict`,
  text), others are raw `struct` dumps committed as little-endian
  binaries (`adventure.dat`, `fortunes.dat`, `cards.pck`, `retroImage`).
  The structs in the dumps contain pointers and `long`s, so with the m68k
  2-byte alignment (D17) their layout also changes, not only their byte
  order.
- **Decision:**
  - Everything the kernel or userland reads natively is big-endian on
    the target: the filesystem, a.out files and binary data files. The
    kernel and userland keep reading native values; no byte swapping is
    added on the target, except where a format is little-endian by
    definition (MBR, virtio).
  - The host tools convert. `fsutil` and `elf2aout` read and write target
    values through explicit big-endian helpers, whatever the host's byte
    order; the MBR keeps explicit little-endian helpers. The tools are
    converted in place for m68k (D11, D14); they no longer produce PIC32
    images.
  - Raw struct dumps are not byte-swapped by hand. They are regenerated
    for the target ABI in M6. Until then the games and programs that read
    them (`adventure`, `fortune`, `monop`, `retroforth`) are left out of
    the m68k build and the root image.
  - Userland gets a `machine/endian.h` (`BYTE_ORDER == BIG_ENDIAN`), and
    `htonl`/`htons`/`ntohl`/`ntohs` are the identity.
  - `<sys/wait.h>` decodes the status with shifts and masks instead of
    LSB-first bitfields. The kernel builds the status numerically
    (`W_EXITCODE`, `W_STOPCODE`), and on big-endian GCC the `union wait`
    bitfields start at the most significant bit, so every `W*` macro
    would read the wrong bits.
- **Consequences:** images built by older `fsutil` versions or for PIC32
  are unreadable by the m68k kernel, and the reverse is also true. The
  `fsutil` magic numbers stay numeric (`FSMAGIC1/2`); the bytes on disk
  read `<<SF` instead of `FS<<`. The host-side `fsutil` keeps assuming a
  32-bit `time_t` (`gcc -m32`), which is unrelated to byte order.

## D38: Swap I/O sleeps: `vd` takes any byte count, `swtch` tests the u area
- **Context:** the first boot with an `fsutil` root (M4) reached the
  boot-time `newproc()`, which swaps out a copy of process 0 as process 1,
  and failed twice. PIC32's `sd` was polled and accepted any byte count;
  `vd` (D36) is interrupt driven and refused counts that were not whole
  sectors. Swap moves the u area and the data and stack segments with
  their byte sizes (`icode` is a few dozen bytes), and `swap()` now sleeps
  until the interrupt. During that swap-out `newproc()` points
  `u.u_procp` at the child, so `swtch()` took process 0 for a user
  process and `longjmp`ed to its `u_qsave`, which had never been set: the
  kernel ran from address 0 through the zeroed vector table into `_start`
  and booted again.
- **Decision:**
  - `vd` accepts any byte count, as `sd` did. The whole sectors go to
    the caller's buffer; a partial last sector goes through a 512-byte
    bounce buffer per disk, zero padded on write, and only the requested
    bytes are copied out on read. A zero count finishes at once.
  - `swtch()` decides whether it runs in process 0 by whose u area is in
    `u` (`proc[0].p_addr == &u`), not by `u.u_procp`. In every other case
    the two tests agree.
- **Consequences:** (superseded in part by D40: `swap()` no longer sleeps)
  swap I/O may sleep anywhere it is done, which PIC32
  never exercised; later swap tests (M4) must cover a user process being
  swapped while it waits. The `rootfs` smoke test checks the boot-time
  swap; `vdbytes` checks partial sectors.

## D39: User programs stay a.out; `elf2aout` reads only m68k ELF
- **Context:** the backlog asked for "the m68k a.out magic" in
  `elf2aout`, for a choice between converting and dropping the kernel's
  ELF loader, and for a choice on the `-s` symbol-file mode. `exec_aout.c`
  accepts only machine ID 0 (`MID_ZERO`) with OMAGIC; the BSD m68k
  machine IDs (`MID_M68K`, `MID_M68K4K`) describe 8 KB and 4 KB page
  layouts that do not apply without an MMU. `exec_elf.c` loads only one
  read-write-execute `PT_LOAD` segment, so it refuses the two segments
  `src/elf32-m68k.ld` makes, refuses programs without bss, and checks for
  little-endian MIPS. `-s` writes 4.4BSD `nlist` records that libc
  `nlist()` cannot parse, and no Makefile uses it.
- **Decision:**
  - User programs are OMAGIC a.out with machine ID 0, as on PIC32; only
    the header's byte order changes (D37).
  - `elf2aout` reads every ELF field through big-endian helpers, so it
    gives the same output on any host, and refuses anything but a
    32-bit big-endian `EM_68K` executable with program headers. `-s` and
    the symbol translation are removed; the section headers are not
    read.
  - `exec_elf.c` stays in the tree but is out of the m68k kernel: it is
    `optional exec_elf` in `files.kconf`, and its `execsw` entry is
    under `#ifdef EXEC_ELF`. Fixing it is the lowest priority M6 task.
- **Consequences:** a.out files carry no symbols (`a_syms` is 0), as
  before. Kernel and userland ELF files cannot be run directly; QEMU loads
  the kernel ELF with `-kernel`, which does not involve `elf2aout`.

## D40: `swap()` waits for its I/O without a context switch
- **Context:** the `swap` smoke test (M4) forks a user process and swaps
  parent and child out and in at every turn. The kernel restarted itself
  after a few turns. `swap()` slept until the `vd` interrupt (D38), and
  the sleep runs `swtch()`, which may `longjmp` to process 0 and so
  exchange the contents of `u` and `u0`. Swap I/O moves the u area from
  `p_addr` (`&u` in `newproc()`) or into `u0` (`swapin()`), and virtio
  reads and writes that memory while the request is in flight, so the swap
  image or the incoming u area could hold the other process's u. In
  `newproc()` the sleep also runs with `u.u_procp` pointing at the child.
  PIC32's `sd` finished every request inside the strategy routine, so
  `swap()` never actually slept there. The bug depends on timing: with
  gdb breakpoints in `longjmp` the test passed.
- **Decision:** `swap()` waits for `B_DONE` by calling `idle()` (`stop`
  at IPL 0) instead of `sleep()`, then sets `noproc` back to 0. No other
  process runs during swap I/O, so `u` and `u0` stay put. Nothing is
  lost: only one user process is in core, and swap I/O runs in it or in
  process 0 (`sched()`). Other block I/O still sleeps. D38's `swtch()`
  test of `proc[0].p_addr` stays.
- **Consequences:** supersedes the D38 consequence that swap I/O may
  sleep. Interrupts are still taken while waiting, but the time is
  counted as idle. The `swap` smoke test checks a forked process's data,
  bss and stack across swaps while it sleeps on a pipe.

## D41: Swap to a memory disk before M6
- **Context:** only one user process is in core (D5), so every switch
  between processes swaps whole processes. On disk that is slow, above
  all on the Amiga under WinUAE (M6). The `virt` board has 16 MB, of
  which the kernel and the user window use 768 KB. Several processes in
  core (slots without an MMU, or the PMMU) are a larger change and still
  undecided (assessment section 6). PIC32 RetroBSD already swapped to RAM
  on boards that had it (`sdramp.c`, `sramc.c`, `spirams.c`).
- **Decision:** add a memory-backed block device in M4, `md`, that the
  board `Config` can name as the swap device (`swap on md0`). Its storage
  is the physical memory above `USER_DATA_END` up to the bootinfo memory
  size, and its strategy routine copies with `bcopy` and completes the
  request at once. The process model, `swap()` and `sched()` stay as they
  are. `vd0b` remains a valid swap device.
- **Consequences:** a process switch costs a copy of the process's size
  instead of disk I/O. Swap I/O is synchronous, as on PIC32, so D40's
  wait loop never runs for `md`. The size of the swap area depends on the
  machine's memory; on QEMU it is set with `-m`.

## D42: m68k crt0 and libc machine layer
- **Context:** M5 needs the machine-dependent part of libc: `crt0`, the
  system call stubs, `setjmp`/`longjmp` and the signal trampoline. The
  generic libc has no C versions of `bcmp`, `bcopy`, `bzero`, `ffs`,
  `index`/`strchr`, `rindex`/`strrchr`, `strcmp`, `strlen` or `memmove`;
  MIPS supplies them in assembly. GCC for m68k-linux returns pointers in
  `a0` (and copies them to `d0`), and callers read `a0`.
- **Decision:**
  - The string routines are NetBSD's `common/lib/libc/arch/m68k/string/*.S`
    (commit `f7ee448f3b88`), copied unchanged into `src/libc/m68k/string/`.
    `sys/m68k/asm.h` is a subset of NetBSD's `<machine/asm.h>` and defines
    `__SVR4_ABI__`, so pointer results are set in `a0` as well. `ffs.S` is
    built with `-D_LIBC`, otherwise it is named `__ffssi2`. The `m68k`
    archive is extracted last in `src/libc/Makefile`, so these objects
    replace the `compat` C versions of the same name.
  - System call stubs (`SYS.h`): the number goes in `d0` with `moveq` up
    to 127 and `movel` above, then `trap #0` (D23); on carry they branch
    to `cerror`, which stores `errno` and returns -1 in `d0` and `a0`.
    `ptrace` clears `errno` first. `sigaction` pushes its three arguments
    again with the trampoline as the 4th and a dummy return address below.
    The trampoline calls `sigreturn(&sigcontext)` at `sp + 12` after the
    handler returns (D31), and exits with the errno if that fails.
    `vfork` stays a plain stub: libc's `SYS_vfork` is 2, which is `fork`.
  - `jmp_buf` is 33 longs: `d2`-`d7`, `a2`-`a6`, `sp`, return address,
    mask-saved flag, signal mask, `fp2`-`fp7` (12 bytes each). `longjmp`
    turns 0 into 1. `setjmp`/`longjmp` save and restore the mask with
    `sigprocmask`; `_setjmp`/`_longjmp` do not; `sigsetjmp` chooses.
  - `crt0`'s `_start` passes `argc`, `argv` and `envp` (D30) to the C
    `__start` as arguments, with a zero return address. `alloca` is
    `<alloca.h>`'s `__builtin_alloca`; nothing is needed in libc.
- **Consequences:** `src/crt0.o` and `src/libc.a` build for m68k. The
  smoke tests `libcrt0`, `libstring`, `libsys`, `libsetjmp` and
  `libsignal` run a C `/sbin/init` linked with them. The signal `code`
  argument is only meaningful for hardware traps; after `kill` it is
  stale, as in 2.11BSD.

## D43: 64-bit integer helpers from compiler-rt 12.0.0
- **Context:** `src/libc/runtime/` holds compiler-rt 12.0.0 sources (plus
  a `__mips__` alias edit in `int_lib.h` that m68k does not need, since
  GCC defines `__ELF__`). The MIPS build used them for soft float and
  64-bit division. With `-mcpu=68030 -m68881` GCC inlines 64-bit
  multiply and most shifts and all 32-bit and FP arithmetic, but calls
  `__divdi3`, `__moddi3`, `__udivdi3`, `__umoddi3`, the shifts
  `__ashldi3`/`__ashrdi3`/`__lshrdi3` (by a variable count, at `-Os`
  depending on context), and the conversions between 64-bit integers and
  `float`/`double` (`__fix[uns]{s,d}fdi`, `__float[un]di{s,d}f`), which the
  68881 does not have. Checked with `nm -u` on a probe at `-O0`, `-Os`
  and `-O2`. `long double` (`xf`) conversions are also called, but no
  built program uses `long double`, and compiler-rt's `xf` code assumes
  the x86 80-bit layout. `__ctzdi2`, `__ffsdi2`, `__popcount*`,
  `__paritysi2` come only from explicit builtins, which nothing uses.
- **Decision:** `src/libc/runtime/Makefile` builds only those 16 helpers.
  The ones missing from the tree (the shifts and the eight conversions)
  are added from the `llvmorg-12.0.0` tag, unchanged, to match the other
  files. The soft-float objects are no longer built (D12); their sources
  stay as reference. The conversions use compiler-rt's hardware-FPU paths
  (`__SOFT_FP__` is not defined).
  - Licence: the compiler-rt files are Apache-2.0 WITH LLVM-exception
    (their SPDX headers), as were the ones already in the tree.
    `src/libc/runtime/LICENSE.txt` held only the older NCSA/MIT text, so
    it is replaced by compiler-rt 12.0.0's `LICENSE.TXT` (Apache 2.0, the
    LLVM exceptions and the legacy NCSA/MIT licence), as Apache 2.0
    section 4(a) requires for redistributed sources. Apache 2.0 files may
    sit in the BSD-licensed tree, each file keeping its licence. The LLVM
    exception lifts 4(a), 4(b) and 4(d) for the helpers compiled into
    programs, so binaries need no notice. Of the programs linked with
    libc, `sre` is GPLv3 or later, which is compatible with Apache 2.0.
- **Consequences:** the `libint64` smoke test checks every helper,
  including ties to even and sticky bits in the conversions; the
  68881's extended precision does not change the results, because the
  helpers' intermediate sums are exact in extended precision. A program
  using `long double` with 64-bit integers would fail to link (missing
  `__floatdixf` etc.) until helpers written for the 68881 format are
  added.

## D44: floating point in libc and libm
- **Context:** the M5 audit of the FP code in `src/libc` (`atof`,
  `strtod`, `ecvt`/`fcvt`/`gcvt`, `doprnt` `%e`/`%f`/`%g`, `doscan`,
  `frexp`/`ldexp`/`modf`, `isnan`/`isinf`) and `src/libm`. Almost all of
  it is portable V7/4.3BSD C that does its arithmetic through `double`
  and needs no change on the 68881. Four places assume the PIC32's
  32-bit `double` or little-endian words, and `sys/m68k/float.h` already
  gives the IEEE double values:
  - `include/math.h` defined `HUGE_VAL` as `FLT_MAX` (3.4e38), so
    `ldexp`, `exp`, `log`, `tan`, `j0`/`j1`/`jn` returned a finite 3.4e38
    for overflow and poles. `printf("%f", exp(1000.))` printed 39 digits.
  - `src/libm/fmod.c` unpacked a double with the low word first, so
    every `fmod` was wrong on big-endian.
  - `doscan.c` kept the `%f`/`%lf`/`%e` stores under `#if HAVE_FLOAT`,
    which was never defined (a size saving on PIC32): `scanf` counted
    the match but stored nothing.
  - `ldexp` returned `HUGE_VAL` for exponents `>= DBL_MAX_EXP`, but
    `frexp` mantissas are in [0.5, 1), so 1024 is still finite:
    `ldexp(1., 1023)` overflowed.
- **Decision:**
  - `HUGE_VAL` is `__builtin_huge_val()` (+Inf), as ISO C allows and
    GCC's own headers do.
  - `fmod.c`'s union orders its words by `__BYTE_ORDER__`, as `wait.h`
    does (D37).
  - The `HAVE_FLOAT` guard is removed and `doscan.c` includes
    `<stdlib.h>` for `atof`; the FPU is mandatory (D12), so scanf can
    always convert.
  - `ldexp` overflows only above `DBL_MAX_EXP`.
  - `frexp(±Inf)` returns the infinity with exponent 0. It used to loop
    forever (`Inf/2` is `Inf`), which the Inf `HUGE_VAL` would otherwise
    make reachable through `log(exp(1000.))`.
  - The libm algorithms (Newton `sqrt`, Cody–Waite polynomials) stay;
    they are within 1e-14 of the correctly rounded results tested.
- **Consequences:**
  - `-std=gnu99` implies `-fexcess-precision=fast`: a `double` result
    can stay in a 68881 register at 64-bit precision, so comparing it
    exactly with a constant can fail. The tests store results to
    `volatile double` first; programs that compare floats exactly can
    behave differently from MIPS. Setting the FPCR rounding precision
    to double is possible later, but is not done.
  - Smoke test `libfp` (printf, scanf, atof/strtod, ecvt/gcvt, modf,
    frexp/ldexp, isnan/isinf, HUGE_VAL and 20 libm results). Libc tests
    now link `-lm -lc`, and `run.sh` builds `src/libm`. Each of the fixes
    above, and fewer `sqrt` iterations, is caught when reverted.
  - Remaining known limits, not port-specific: `ldexp` flushes results
    below `DBL_MIN_EXP - 2` to 0 (no denormals), `sqrt(Inf)` is NaN, and
    there is no `long double` libm.

## D45: building the userland and the root image

- **Context:** the M5 task builds the reduced userland with GCC and packs it
  into `sdcard.img`. The first full build showed:
  - `m68k-linux-gnu-gcc` predefines `linux`, `__linux`, `__linux__` and
    `__gnu_linux__`; `virus` took its Linux branch and wanted `termios.h`.
  - With `-ffreestanding`, GCC knows nothing about `exit`/`abort` being
    noreturn, so `basic`, `viless`, and flex scanners (`awk`, `atc`) fail
    `-Werror=return-type`; and `abs`/`labs`/`fabs` are not built-ins, so
    the calls need libc, which had no `abs`, `labs` or `fabs` objects.
  - `tip`'s `zzhack` union read `short`/`char` members out of a
    `char *` set to `(char *)TRUE`; on a big-endian CPU those are the
    high bytes, i.e. 0. Every boolean and character variable was off.
  - A few new GCC warnings (`-Wmisleading-indentation` in `scm`,
    `-Wint-in-bool-context` in `backgammon`).
  - `rootfs.manifest` still listed PIC32 devices, the native toolchain
    (D8), the PIC32 hardware programs (D18) and PIC32 machine headers.
- **Decision:**
  - `target.mk` undefines the four `linux` macros for userland, and the
    clangd `compile_flags.txt` files follow. The kernel does not test them.
  - `exit`, `_exit` and `abort` are declared
    `__attribute__((__noreturn__))`; libc gains `abs.o`, `labs.o` and
    `fabs.o`.
  - `tip`'s `zz_boolean`/`zz_character` become `int`, the size of the
    pointer they alias, and `tip` builds with `-fno-strict-aliasing`.
  - `scm` and `backgammon` are fixed in place.
  - `retroforth`, `adventure`, `fortune` and `monop` stay out of the build
    and the manifest (little-endian data files, D37); they come back in M6.
  - `src install` installs `termcap.small` as `/etc/termcap` (avoiding the
    host `ex` run that rebuilds `termcap.full`) and the `libvmf` and
    `libgpanel` man pages.
  - The manifest's devices follow `sys/m68k/devsw.c`: `vd0`/`vd1` with
    partitions a-d, `md0`, `swap*`, the character devices of majors 0-6
    with one `tty0` (`NGFTTY` is 1) and the ptys. `/include/machine` lists
    the m68k headers. `etc/fstab` mounts `vd0`; `etc/ttys` has `tty0` only.
  - The top-level `make` builds the kernel in `sys/m68k/virt`.
- **Consequences:**
  - `make` at the top builds tools, kernel, userland and `sdcard.img`;
    `fsutil --check` is clean, and the image boots to multi-user in QEMU
    (login as root, `ls`, `ps`, `df`, `bc` work by hand).
  - Smoke test `userland`: runs the top-level make, fails on any manifest
    file that was not installed and checks the image with `fsutil`.
    Reverting each fix above (or adding a bogus manifest entry) is caught.
  - Still PIC32-flavoured: `/etc/rc` sets hostname `pic32`, and
    `etc/MAKEDEV` knows `sd*` only.

## D46: the per-process memory limit is the user window

- **Context:** `MAXMEM` in `sys/include/param.h` was 96 KB, sized for
  PIC32's 128 KB of RAM. It caps `exec` (`exec_estab`) and `brk`
  (`kern_mman.c`) and is printed as "user mem". The m68k user window is
  256 KB (`USER_DATA_SIZE`, D16), so programs between 96 and 256 KB were
  refused. Testing the new limit found three more bugs:
  - `exec_estab` summed text, data, heap and stack but not bss, so an
    a.out whose bss reached into the stack was loaded and run (in this
    case jumping to address 0, which hung the machine).
  - `execve` tried every format handler until one returned 0, so the
    a.out handler's `ENOMEM` was overwritten by the script handler's
    `ENOEXEC`.
  - libc `sbrk` (from RetroBSD's MIPS libc) printed "Out of memory" and
    returned the *old* break on failure. 2.11BSD returns -1, and
    `malloc`, `sh` and `csh` test for -1; so they took a failed `sbrk`
    as success and used memory beyond the break, which here can be the
    stack.
- **Decision:**
  - `MAXMEM` moves to `sys/m68k/machparam.h` as `USER_DATA_SIZE`: text,
    data, bss, heap and stack together may fill the window. The `brk`
    check then stops the heap at the current stack.
  - `exec_estab` counts `bss.len` in the `MAXMEM` sum.
  - `execve` stops at the first handler that returns anything other
    than `ENOEXEC` ("not my format"), as NetBSD's `check_exec` does.
  - `src/libc/m68k/sys/sbrk.c` returns -1 on failure, with `errno` set by
    the `_brk` stub, and no longer prints a message. The MIPS copy is
    left as it is (D11).
- **Consequences:**
  - The banner says "user mem = 256 kbytes"; a process can grow to
    256 KB, and whole-process swapping moves up to that much.
  - Smoke test `libmem`: an init with 160 KB of bss runs; `sbrk` in 4 KB
    steps fails with `ENOMEM` within 16 KB of the stack; a `fork` with the
    window full swaps both processes and the patterns survive; an a.out
    with 255 KB of bss is refused with `ENOMEM`. Reverting `MAXMEM`, the
    `brk` check, the bss term, the `execve` loop or `sbrk` is caught.
  - The stack can still grow down into a heap that has been extended
    after it; `growstack` catches it only at the next trap (as before).

## D47: end-to-end userland smoke test and awk fixes

- **Context:** the last M5 task boots `sdcard.img` to a shell and checks
  the core commands. The first run showed that `awk` printed `$0` for
  any program. That bug is not m68k-specific: RetroBSD's switch of the
  V7 awk lexer from lex to flex broke it on PIC32 too, as a native host
  build of the same sources shows. The hostname in `etc/rc` was still
  `pic32`.
- **Decision:**
  - Smoke test `userland-e2e` (`run.sh`; named for the range of
    userland it covers, not just the shell) boots the real image through `init`,
    `rc`, `getty` and `login`, and drives the console through a FIFO.
    It logs in as root and types each line of `userland-e2e.cmds` followed by
    `echo @@N@`, waiting for that marker before sending the next line.
    It then sends `sync; halt` and requires QEMU to exit with status 0.
    Every line of `userland-e2e.expect` must appear as a whole console line;
    `@PASSWD_LINES@` stands for the line count of `etc/passwd`. Expected
    values are computed independently (for example `2^100`, the MD5 of
    `abc`, and the big-endian `od -x` words).
  - `etc/rc` sets the hostname to `m68k`.
  - The `awk` fixes:
    - `awk.lx.l`: flex ignores the custom `input()` and reads `yyin`,
      so awk read its input data as the program. A `YY_INPUT` now calls
      `input()`, which reads the program string `lexprog` up to its NUL
      (otherwise `yyin`, for `-f`).
    - `awk.lx.l`: the start-state switch (`INITIAL` to `A`, and `sc` to
      `A` returning `}`) now uses flex's `YY_START`.
    - `parse.c`: `ALLOC` sizes nodes as `sizeof(node) + n` pointers,
      because `BOTCH` became 0.
    - `b.c`: `cfoll` shares equal follow sets, so `freetr` clears the
      duplicates before freeing them; `cgotofn` no longer frees
      `state[0]`, which is `foll[0]`. Both were double frees, found
      with ASan.
  - Known limits of this V7 awk that remain: `/=/` lexes as the `/=`
    operator (write `/[=]/`), and a program with only `BEGIN` still
    reads its standard input.
- **Consequences:** `make check` boots the full system and exercises
  the shell, text tools, `awk`, `bc`, files, pipes, background jobs,
  `ps`, `df` and `cal`. Each run takes a few minutes, mostly for the
  build. The hostname, a bogus expected line and reverting the `awk`
  fixes are each caught.

## D48: BASIC INP/OUT through /dev/mem

- **Context:** `basic`'s `OUT addr,value` and `INP(addr)` (PEEK/POKE)
  used the RetroBSD `ustore`/`ufetch` syscalls, removed in D24, and have
  been no-ops since. They should work again, at the least effort, and
  only for root. The kernel already has `/dev/mem` (`sys/m68k/mem.c`),
  which reads and writes kernel RAM and the user window.
- **Decision:**
  - `basic` reads and writes the 32-bit word at `addr & ~3` (as
    `ufetch`/`ustore` did) through `/dev/mem`: read-only for `INP`,
    read-write for `OUT`. If the open, seek or transfer fails it prints
    the `perror` message and stops the program with a new "Memory
    access" error. No syscalls are re-added.
  - `/dev/mem` and `/dev/kmem` become mode 0640 (root, group wheel; `ps`
    and `pstat` are setgid wheel). They were 0664, so anyone could read
    them.
  - The 4.4BSD securelevel policy is kept: `init` raises the securelevel
    to 1 in multi-user mode, where opening `/dev/mem` for writing gives
    `EPERM` even for root. So `OUT` works in single-user mode only, which
    is acceptable: BASIC programs that poke memory are not expected to
    run in multi-user mode. (Without an MMU this protects little, since
    any program can write kernel memory through a pointer.)
  - `rootfs.manifest` gave `/dev/null`, `/dev/zero` and `/dev/tty` mode
    `666`, which `fsutil` reads with `strtoul(..., 0)` as decimal (octal
    1232). Non-root users could not read them. They are now `0666`.
- **Consequences:**
  - Smoke test `basic` uses the same console driver as `userland-e2e`
    (`run_console`), so it fails separately. Two new line types in the
    `.cmds` files, `@send` (type without a marker) and `@wait` (wait for
    a console line), let it drop to single-user with `kill -TERM 1`.
    Making `/dev/mem` readable again, opening it read-write for `INP`,
    dropping the error, or dropping the alignment is caught; so is the
    old `/dev/null` mode, by a new `userland-e2e` line.
  - Only RAM is reachable; device registers are not. On the Amiga,
    reaching the custom chips and CIAs is an M6 task.
