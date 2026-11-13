# RetroBSD 68030 port

This branch (`retrobsd-68k`) ports RetroBSD to the Motorola 68030, with first
bring-up on QEMU `virt`. The kernel runs without an MMU, as RetroBSD does on
PIC32. PIC32 support is abandoned in this fork (see D11), but its code and
documentation remain in the tree as reference while porting.

In scope: kernel, RetroBSD libc and userland, cross-built with vanilla GCC.
Out of scope: Amiga hardware (until M6, D25) and the native on-target compilers
and tools.

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
The `kernel` test is not a source file: it builds `sys/m68k/virt/unix.elf` with the board
`Makefile` (in place and incremental, like a developer build; the build log goes to
`$OUT/kernel-build.log`) and boots it. Without a root filesystem (D20) the kernel panics, and
`panic()` halts QEMU, so the test expects the panic message and exit status 0.

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
| `kernel`   | the whole kernel builds and boots to the M2 finish line | `panic: No root filesystem found!`, exit status 0 |

`sh docs/porting/smoke/run.sh [test]` (or `make check`) runs all tests, or just the named one. Set `CROSS=` to use a
different toolchain prefix. `run.sh -v [test]` (or `make check V=1`) also prints each test's
console output, with QEMU's exit status, before its result.

## Running a kernel image on QEMU

Build the kernel in its board directory and boot it:

```shell
make -C sys/m68k/virt -j$(nproc)
qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel sys/m68k/virt/unix.elf -nographic
```

QEMU loads the ELF and passes its bootinfo records (memory size, CPU) after `_end`. The
console is the Goldfish TTY on stdio. Until M4 there is no root filesystem, so the kernel ends
with `panic: No root filesystem found!` and `panic()` halts QEMU through `virt-ctrl`
(exit status 0). Otherwise exit QEMU with `Ctrl-A X`. The same command works for the smoke test
images in `$OUT` (default `/tmp/retrobsd-m68k-smoke`).

### QEMU limitations

QEMU's 68030 model (checked on QEMU 11.1.2) does not emulate bus errors
(unassigned memory reads return data, writes are dropped), address errors
(an odd jump target raises an illegal instruction), FPU exceptions
(enabled exceptions only set `FPSR` bits), `fsave`/`frestore` (illegal
instructions), `rte` format errors or the 68030 PMMU (`pmove` raises a
line F trap). Code for these is written against the Motorola manuals and
tested with hand-built frames; real validation is M6, on WinUAE (D25).

## Debugging the kernel with GDB

Build with debug info. `DEBUG` on the command line does the same as regenerating the board
Makefile with `kconfig -g`, without changing the committed Makefile; `make clean` first, since
objects built without `-g` are not rebuilt otherwise (and again afterwards to drop `-g`):

```shell
make -C sys/m68k/virt clean
make -C sys/m68k/virt -j$(nproc) DEBUG=-g
```

Start QEMU stopped at the first instruction (`-S`) with a GDB server on port 1234 (`-s`), and
attach `gdb-multiarch` from a second terminal. GDB takes the architecture from the ELF:

```shell
qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel sys/m68k/virt/unix.elf -nographic -s -S
gdb-multiarch -ex "target remote :1234" sys/m68k/virt/unix.elf
```

```
(gdb) break main
(gdb) break panic
(gdb) continue                  # stops in main, then continue again to stop in panic
(gdb) bt                        # panic called from main () at init_main.c:159
(gdb) info registers pc ps sp   # GDB names the status register "ps", not "sr"
(gdb) x/2i $pc
(gdb) x/4xw 0                   # vector table: initial SSP, reset PC, bus and address error
(gdb) kill                      # also ends QEMU
```

`break badtrap` catches unexpected exceptions; its arguments are the vector number, the PC and
the SR of the faulting instruction.
