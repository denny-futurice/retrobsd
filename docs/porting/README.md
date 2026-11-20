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
| [03-decisions.md](03-decisions.md)     | Decision record (D1-D29)                  |
| [smoke/](smoke/)                       | Freestanding toolchain/QEMU smoke test    |

## Host setup (Debian Testing)

```shell
sudo apt install gcc-m68k-linux-gnu binutils-m68k-linux-gnu qemu-system-misc gdb-multiarch
```

Debian has no `m68k-elf` or `qemu-system-m68k` package: the compiler is the
`m68k-linux-gnu-` one, used freestanding, and `qemu-system-m68k` comes from
`qemu-system-misc`. `gdb-multiarch` is only needed for debugging; the `elf2aout`, `fsutil`, `rootfs`, `swap`, `mdswap` and `lib*` tests
also need a host `gcc` for the tools, and `fsutil` a host C compiler for its reader (`cc`, or set `HOSTCC`). Verified versions are listed in
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
The tests from `startup` on link with the kernel's `sys/m68k/startup.S`, `sys/m68k/locore.S`
and `sys/m68k/virt/kernel.ld` instead of `crt.S` and `l.ld`; `run.sh` pre-fills a test's
`smoke_bss` array through QEMU's loader device so a missing BSS clear is caught.
The machdep tests (`bootinfo` to `mdio`) also link the kernel's `sys/m68k/machdep.c`, with
`ktest.c` standing in for the rest of the kernel, and are built with the `virt` board's
`FPU_NO_FSAVE` (D28); they stop QEMU through `boot(RB_HALT)`, and
`run.sh` checks that `halt` makes QEMU exit by itself with status 0.
The trap tests (`ktrap` to `vdbytes`) add the kernel's `sys/m68k/trap.c`, `sys/m68k/clock.c`,
`sys/kernel/kern_clock.c` and `ttest.c`, which stubs the scheduler and signal calls
(`psignal()` calls the test's `got_signal()`), enters user mode with `rte`, and pushes
hand-built exception frames for what QEMU does not raise (D25). The system call tests define
their own small `sysent` table in place of `ttest.c`'s one-entry default, and run their user
code with the stack at `USER_DATA_END`, where `copyin` accepts the arguments. The exec, pipe,
ptrace and signal tests also link the one kernel file they check and stub its other callees.
`uentry` and `icode` return from `main()`, so `startup.S` runs their user code at
`USER_DATA_START` as it runs process 1; `startup` and `cacr` halt QEMU themselves, since `main()` must not
return into user mode. The tty tests link `sys/m68k/gftty.c` with `ttystub.h` in place of `tty.c`,
and `run.sh` feeds them 300 bytes of console input on QEMU's stdin. The sysctl tests link
`sys/kernel/kern_sysctl.c` and `sys/m68k/sysctl.c`; `sysctltest.h` defines the kernel symbols
they refer to and runs a list of `__sysctl()` calls from user mode, with the arguments and
buffers in the user region. The `vd` tests link `sys/m68k/vd.c` and `sys/m68k/disklabel.c`;
`vdstub.h` stands in for the buffer cache, with a `biowait()` that sleeps with `stop` until the
disk interrupt calls `biodone()`. `run.sh` builds two disk images, each sector holding its
own number in text: disk A, read-write and copied for each test, has an MBR with two
partitions, and QEMU's `blkdebug` fails reads of one of its sectors; disk B is read-only,
without an MBR. A `virtio-rng-device` sits in a slot between them, so the scan must skip it.
A test with no expected output in `run.sh` fails.
Build steps are logged to `$OUT/<test>-build.log`; a failed step fails the test.
The `kernel` test is not a source file: it builds `sys/m68k/virt/unix.elf` with the board
`Makefile` (in place and incremental, like a developer build; the build log goes to
`$OUT/kernel-build.log`) and boots it without a disk. Without a root filesystem (D20) the kernel panics, and
`panic()` halts QEMU, so the test expects the panic message and exit status 0.
The `rootfs` test boots the same kernel with a disk image that `tools/fsutil` builds (MBR, 2 MB
root, 1 MB swap); its `/sbin/init` halts QEMU through `reboot(RB_HALT)`. The board kernel swaps
on the memory disk `md0` (D41); the `swap` test relinks it with swap on `vd0b`.

| Test    | Checks                                            | Expected output |
|---------|---------------------------------------------------|-----------------|
| `hello` | toolchain, linker script and QEMU boot            | `hello m68030`  |
| `fpu`   | hardware 68881/68882 floating point (D12)         | `fpu ok`        |
| `startup` | kernel `startup.S`: BSS clear, stack, VBR       | `startup ok`    |
| `cacr`    | `startup.S` clears and disables both caches (D34) | `cacr ok`     |
| `bootinfo` | `startup()` takes `physmem` from QEMU bootinfo | `bootinfo ok`   |
| `kstring`  | `ffs`, string and memory routines              | `kstring ok`    |
| `useraddr` | `baduaddr`/`badkaddr`, range checks without wraparound, `copyin`/`copyout`/`copyinstr` | `useraddr ok` |
| `udelay`   | `udelay()` timing against the Goldfish RTC     | `udelay ok`     |
| `halt`     | `boot(RB_HALT)` stops QEMU via `virt-ctrl`     | `halted`, exit status 0 |
| `swtch`    | `setjmp`/`longjmp` switch to the process in `u0` and back: `u`/`u0` and `p_addr` exchanged | `swtch ok` |
| `fpswtch`  | FPU registers and FPCR follow each process across the switch; `savefpu()` (D28) | `fpswtch ok` |
| `mdio`    | the memory disk (`md.c`, D41) covers the RAM from `USER_DATA_END` to the bootinfo memory size, copies any byte count both ways at the right place and at once, and gives EOF, truncation and `EINVAL` at its end | `mdio ok` |
| `ktrap`    | kernel-mode exception dumps registers, panics  | `panic: kernel trap` |
| `uill`     | user illegal instruction: `SIGILL`, trap frame | `uill ok`       |
| `upriv`    | user privileged instruction: `SIGILL`          | `upriv ok`      |
| `utrace`   | user trace: `SIGTRAP`, T bit cleared           | `utrace ok`     |
| `udivz`    | user divide by zero: `SIGFPE`, format 2 frame  | `udivz ok`      |
| `buserr`   | hand-built format B bus error: `SIGBUS`, trimmed to format 0 (D26) | `buserr ok` |
| `ufpe`     | hand-built FPU exception: `SIGFPE`, FPSR in `u_code` | `ufpe ok` |
| `fpbiu`    | `fpfault_clear()` on hand-built 68881/2 state frames | `fpbiu ok` |
| `tick`     | RTC 1 alarm ticks at `HZ` through the PIC and `hardclock()` (D27) | `tick ok` |
| `callout`  | `timeout()` callback fires on the tick it was due | `callout ok` |
| `todr`     | `inittodr()`: time from RTC 2, or the file system time if the RTC is behind | `todr ok` |
| `sysargs`  | `trap #0` copies the arguments from `usp + 4`; `EFAULT` past the user region (D23) | `sysargs ok` |
| `sysret`   | result in `d0` and carry clear, errno and carry set, `EJUSTRETURN`, other registers kept (D23) | `sysret ok` |
| `sysrestart` | `ERESTART` backs the PC up over `trap #0` and the call runs again (D23) | `sysrestart ok` |
| `nosys`    | out-of-range number: `nosys()` posts `SIGSYS`, `EINVAL` if it is ignored | `nosys ok` |
| `msec`     | `sc_msec()`: milliseconds from the tick count | `msec ok` |
| `execstack` | exec entry stack: `sp` at `argc`, `argv` and `envp` vectors above it (D30) | `execstack ok` |
| `execclear` | exec zeroes the registers and SR, resets the FPU, clears the bss (D30) | `execclear ok` |
| `pipe2`    | `pipe()` from user mode returns the descriptors in `d0` and `d1` | `pipe2 ok` |
| `ptrace`   | `procxmt()`: register-only `PT_WRITE_U`, user SR bits, `PT_STEP` sets the trace bit | `ptrace ok` |
| `sigframe` | `sendsig`: handler arguments, trampoline return, saved registers, format 0 entry without tracing (D31) | `sigframe ok` |
| `sigreturn` | user-mode round trip: registers, PC, mask and user SR bits from the `sigcontext` | `sigreturn ok` |
| `sigfpu`   | FPU saved in the `sigcontext`, reset for the handler, restored by `sigreturn` | `sigfpu ok` |
| `sigfault` | bad contexts refused (`EFAULT`/`EINVAL`), signal frame running into the data gives `SIGILL` | `sigfault ok` |
| `uentry`   | `main()` returning enters user mode at `USER_DATA_START`: SR 0, `usp` at `USER_DATA_END`, registers cleared, kernel stack reset (D32) | `uentry ok` |
| `icode`    | process 1 code: `execv("/sbin/init", {"init", "-s", 0})`, then `exit` with the error | `icode ok` |
| `ttyrx`    | console input by receive interrupt reaches `ttyinput()` in order, more than the device's 128-byte buffer (D33) | `ttyrx ok` |
| `ttyrxopen` | the receive interrupt stays off until the first open | `ttyrxopen ok` |
| `intrnest` | a clock tick waits until the tty interrupt handler returns (D33) | `intrnest ok` |
| `consdev` | `sysctl` `CPU_CONSDEV` from user mode: the console device | `consdev ok` |
| `errmsg`  | `sysctl` `CPU_ERRMSG` from user mode: messages, range, short buffer | `errmsg ok` |
| `nlist`   | `sysctl` `CPU_NLIST` from user mode as an ordinary user (D35) | `nlist ok` |
| `vdprobe` | both virtio disks found in command-line order, past another virtio device, with size and read-only flag (D36) | `vdprobe ok` |
| `vdread`  | reads finish by interrupt, multi-block, queued, a device error gives `EIO`, EOF at the end of the disk | `vdread ok` |
| `vdwrite` | writes reach the disk through the raw disk and a partition; sector 0 and a read-only disk refuse them | `vdwrite ok` |
| `vdpart`  | MBR partitions (`disklabel.c`): entries, offsets, EOF and truncation at the end, missing partitions, a disk without an MBR | `vdpart ok` |
| `vdbytes` | transfers of part of a sector, as swap does: a partial last sector read without touching memory past the count, written zero padded (D38) | `vdbytes ok` |
| `elf2aout` | host only, no QEMU: `tools/elf2aout` turns an m68k ELF (`aoutprog.S`) into an a.out whose big-endian header matches the section headers and whose image matches `objcopy -O binary`; an object file and little-endian, non-m68k and shared-object ELF files are refused (D39) | `PASS elf2aout` |
| `fsutil`   | host only, no QEMU: `tools/fsutil` writes a big-endian filesystem (D37) that `--check` passes cleanly and `bsdread.c`, an independent reader, reads back past block 65535 and through `..` | `PASS fsutil` |
| `kernel`   | the whole kernel builds and boots to the M2 finish line, without a disk | `panic: No root filesystem found!`, exit status 0 |
| `rootfs`   | the kernel mounts an `fsutil` image as root on `vd0a`, swaps process 1 out and in on `vd0b`, and runs its `/sbin/init` (`rootinit.S`, converted by `elf2aout`), which copies `/etc/motd` (past the direct blocks) to the console and halts; the image then passes `fsutil --check` | `rootfs ok: end of /etc/motd`, exit status 0 |
| `swap`     | as `rootfs`, with `/sbin/init` from `swapinit.c`: a parent and a child take turns over two pipes, so each is swapped out and in at every turn while it sleeps, and both check patterns in their data, bss and stack; the kernel is relinked to swap on `vd0b`, and afterwards all six patterns are on that partition and the image passes `fsutil --check` (D40) | `swap ok`, exit status 0 |
| `mdswap`   | as `swap`, on the board kernel, which swaps on `md0` (D41); none of the patterns reach `vd0b` | `swap ok`, exit status 0 |
| `libcrt0`  | a C `/sbin/init` linked with `src/crt0.o` and `src/libc.a` (D42), as are the next four: `argc`, `argv`, the environment and `__progname` as process 1 and after an `execve`, and `main`'s result as the exit status | `libcrt0 ok`, exit status 0 |
| `libstring` | the NetBSD string routines: every alignment and length up to 70, overlapping moves both ways, pointer results in `a0` as well as `d0` | `libstring ok`, exit status 0 |
| `libsys`   | the system call stubs: `errno` and -1 (also as a pointer), numbers above 127, `pipe`, `sbrk`/`_brk`, `ptrace`'s cleared `errno`; the image then passes `fsutil --check` | `libsys ok`, exit status 0 |
| `libsetjmp` | `setjmp`/`_setjmp` and their `longjmp`s restore `d2`-`d7`, `a2`-`a6` and `fp2`-`fp7`, turn 0 into 1, and restore the signal mask or not; `sigsetjmp` both ways | `libsetjmp ok`, exit status 0 |
| `libendian` | byte order (D37): `<machine/endian.h>` says big-endian, as does a long's layout in memory; `htonl`/`ntohl` return their argument, `htons`/`ntohs` its low 16 bits | `libendian ok`, exit status 0 |
| `libwait`  | `<sys/wait.h>` (D37): a child's exit code, `SIGTERM`, `SIGQUIT` with a core dump and a stop, through the `W*` macros and the `union wait` fields, and fields set by hand at the bits the kernel uses | `libwait ok`, exit status 0 |
| `libint64` | the compiler-rt 64-bit helpers (D43): signed and unsigned division and modulo, shifts by a variable count, and 64-bit integer to and from `float`/`double` with ties to even and sticky bits | `libint64 ok`, exit status 0 |
| `libfp` | floating point in libc and libm (D44): `printf` `%e`/`%f`/`%g`, `scanf` `%f`/`%lf`, `atof`/`strtod`, `ecvt`/`gcvt`, `modf`, `frexp`/`ldexp`, `isnan`/`isinf`, `HUGE_VAL` as Inf, and 20 libm results to 1e-14, `fmod` included | `libfp ok`, exit status 0 |
| `userland` | the top-level `make` (D45): tools, kernel, userland and `sdcard.img`; no manifest file is missing and `fsutil --check` is clean | `PASS userland` |
| `libsignal` | `sigaction` and the trampoline (D31): handler arguments and mask, every register back after a handler that clobbers them, `oact`, `SIG_IGN`, invalid signals | `libsignal ok`, exit status 0 |

`sh docs/porting/smoke/run.sh [test]` (or `make check`) runs all tests, or just the named one. Set `CROSS=` to use a
different toolchain prefix. `run.sh -v [test]` (or `make check V=1`) also prints each test's
console output, with QEMU's exit status, before its result.

## Running a kernel image on QEMU

Build the kernel in its board directory and boot it:

```shell
make -C sys/m68k/virt -j$(nproc)
qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel sys/m68k/virt/unix.elf -nographic
```

To attach a disk, add `-drive file=disk.img,if=none,format=raw,id=d0 -device
virtio-blk-device,drive=d0`. Disks are `vd0`, `vd1` in command-line order (D36); root is the
first MBR partition of `vd0` (`vd0a`). Swap is the memory disk `md0`, the RAM above the user
window (D41), so its size follows `-m`; `swap on vd0b` in the board `Config` swaps to the second
partition instead.

QEMU loads the ELF and passes its bootinfo records (memory size, CPU) after `_end`. The
console is the Goldfish TTY on stdio. Without a disk holding a big-endian filesystem written by
the m68k `fsutil` (M4, D37), the kernel ends with `panic: No root filesystem found!` and `panic()` halts QEMU through `virt-ctrl`
(exit status 0). With one, it runs `/sbin/init` from it; the image of the `rootfs` test,
`$OUT/rootfs/disk.img`, boots to its test `init`. `fsutil --repartition` writes an image only up to
the end of the root, so extend it to cover the swap partition before booting. Otherwise exit QEMU with `Ctrl-A X`. The same command works for the smoke test
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

`break trap` catches every exception and interrupt; `p *tf` shows the trap frame
(`sys/m68k/frame.h`), with the vector offset in `tf_vector` (vector number times 4).
