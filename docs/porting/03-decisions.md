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
  - Kernel machine-dependent code goes in `sys/m68k/`, with the QEMU board in
    `sys/m68k/virt/` (`Config` and board `Makefile`, generated by `kconfig`).
  - Userland assembly goes in `src/libc/m68k/`.
  - `sys/pic32/` and `src/libc/mips/` are not moved or renamed. They stay
    untouched as reference until their m68k counterparts exist (D11), and are
    deleted later.
  - The `include/machine` symlink (currently `-> pic32`) is retargeted to `m68k`,
    and `include/pic32` is replaced by `include/m68k -> ../sys/m68k`, when
    `sys/m68k/` is created (M2).
- **Consequences:** the tree holds PIC32 and m68k code side by side for a while.
  Nothing builds both.

## D14: Build files are converted in place
- **Decision:** `target.mk`, `sys/pic32/Makefile.kconf` and `clang-config.mk`
  are edited in place for GCC and m68k, rather than duplicated as variants or
  selected by an `ARCH=` switch. This is the first step of M1. The old versions
  remain in git history. `clang-config.mk` is deleted once nothing includes it.
- **Consequences:** the PIC32 build breaks at that point, as allowed by D11.
  `sys/pic32/Makefile.kconf` keeps its name until the m68k board layout exists
  (D13), and is then moved.

## D15: Branch policy
- **Decision:** `retrobsd-68k` is the long-lived working branch, kept on the fork
  `denny-futurice/retrobsd` (`origin`). `master` stays an untouched snapshot of
  upstream and is not rebased or merged into the working branch (D11). The
  `upstream` remote (`RetroBSD/retrobsd`) is kept for reference and for
  cherry-picking individual patches by hand where relevant. Pushing is done
  manually by the owner.
