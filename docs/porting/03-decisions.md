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
  deletes `stubs.c`; M4 replaces the stubbed root and swap with the virtio
  block device in `Config` and `devices.kconf`.

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
