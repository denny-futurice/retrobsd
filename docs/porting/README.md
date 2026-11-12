# RetroBSD 68030 port

This branch (`retrobsd-68k`) ports RetroBSD to the Motorola 68030, with first
bring-up on QEMU `virt`. The kernel runs without an MMU, as RetroBSD does on
PIC32. PIC32 support is abandoned in this fork (see D11), but its code and
documentation remain in the tree as reference while porting.

In scope: kernel, RetroBSD libc and userland, cross-built with vanilla GCC.
Out of scope: Amiga hardware and the native on-target compilers and tools.

## Documents

| Document                               | Contents                                  |
|----------------------------------------|-------------------------------------------|
| [01-assessment.md](01-assessment.md)   | Feasibility assessment and risks          |
| [02-backlog.md](02-backlog.md)         | Milestone task backlog (M0-M6)            |
| [03-decisions.md](03-decisions.md)     | Decision record (D1-D20)                  |
| [smoke/](smoke/)                       | Freestanding toolchain/QEMU smoke test    |

## Host setup (Debian Testing)

```shell
sudo apt install gcc-m68k-linux-gnu binutils-m68k-linux-gnu qemu-system-misc gdb-multiarch
```

Debian has no `m68k-elf` or `qemu-system-m68k` package: the compiler is the
`m68k-linux-gnu-` one, used freestanding, and `qemu-system-m68k` comes from
`qemu-system-misc`. `gdb-multiarch` is only needed for debugging. Verified versions are listed in
decision D10. Run `make check` to validate the setup.

## Editor setup (clangd)

`sys/`, `src/` and `include/` each have a `compile_flags.txt` that makes clangd
parse for m68k with `-nostdinc` against the tree's own headers, as the GCC build
does, so host (Linux/glibc) headers never shadow the BSD ones. `sys/` adds
`-DKERNEL`. The top-level `.clangd` pre-includes `<sys/param.h>` for header
files opened on their own. `tools/` are host programs and use clangd's defaults.
Restart clangd after changing these files. Known differences: clangd's m68k
target has an 8-byte `long double` and 8-byte `double` alignment (GCC: 12 and 2,
D17), so hover sizes and offsets of such structs differ from the real build.
Optional drivers (`LOG_ENABLED`, ...) and board defines from a `Config` are not
set, so their code shows as inactive.

## Smoke test

```shell
sh docs/porting/smoke/run.sh
```

Each test is a single freestanding source file in `smoke/` that checks one thing and prints an
expected string to the Goldfish TTY on `qemu-system-m68k -M virt -cpu m68030`, built with the flags
the port will use.
`startup` and `badvec` link with the kernel's `sys/m68k/startup.S` and
`sys/m68k/virt/kernel.ld` instead of `crt.S` and `l.ld`; `run.sh` pre-fills a test's
`smoke_bss` array through QEMU's loader device so a missing BSS clear is caught.
The machdep tests (`bootinfo` to `halt`) also link the kernel's `sys/m68k/machdep.c`, with
`ktest.c` standing in for the rest of the kernel; they stop QEMU through `boot(RB_HALT)`, and
`run.sh` checks that `halt` makes QEMU exit by itself with status 0.

| Test    | Checks                                            | Expected output |
|---------|---------------------------------------------------|-----------------|
| `hello` | toolchain, linker script and QEMU boot            | `hello m68030`  |
| `fpu`   | hardware 68881/68882 floating point (D12)         | `fpu ok`        |
| `startup` | kernel `startup.S`: BSS clear, stack, VBR       | `startup ok`    |
| `badvec`  | unexpected exception reaches `badtrap()`        | `badvec ok`     |
| `bootinfo` | `startup()` takes `physmem` from QEMU bootinfo | `bootinfo ok`   |
| `kstring`  | `ffs`, string and memory routines              | `kstring ok`    |
| `useraddr` | `baduaddr`/`badkaddr`, `copyin`/`copyout` range checks | `useraddr ok` |
| `udelay`   | `udelay()` timing against the Goldfish RTC     | `udelay ok`     |
| `halt`     | `boot(RB_HALT)` stops QEMU via `virt-ctrl`     | `halted`, exit status 0 |

`sh docs/porting/smoke/run.sh [test]` (or `make check`) runs all tests, or just the named one. Set `CROSS=` to use a
different toolchain prefix.

## Running a kernel image on QEMU

```shell
qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel <image.elf> -nographic
# to debug, add -s -S and attach gdb-multiarch:
#   gdb-multiarch -ex "set architecture m68k" -ex "target remote :1234" <image.elf>
```

Exit QEMU with `Ctrl-A X`.
