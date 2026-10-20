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
| [03-decisions.md](03-decisions.md)     | Decision record (D1-D15)                  |
| [smoke/](smoke/)                       | Freestanding toolchain/QEMU smoke test    |

## Host setup (Debian Testing)

```shell
sudo apt install gcc-m68k-linux-gnu binutils-m68k-linux-gnu qemu-system-misc gdb-multiarch
```

Debian has no `m68k-elf` or `qemu-system-m68k` package: the compiler is the
`m68k-linux-gnu-` one, used freestanding, and `qemu-system-m68k` comes from
`qemu-system-misc`. `gdb-multiarch` is only needed for debugging. Verified versions are listed in
decision D10. Run `make check` to validate the setup.

## Smoke test

```shell
sh docs/porting/smoke/run.sh
```

Each test is a single freestanding source file in `smoke/` that checks one thing and prints an
expected string to the Goldfish TTY on `qemu-system-m68k -M virt -cpu m68030`, built with the flags
the port will use.

| Test    | Checks                                            | Expected output |
|---------|---------------------------------------------------|-----------------|
| `hello` | toolchain, linker script and QEMU boot            | `hello m68030`  |
| `fpu`   | hardware 68881/68882 floating point (D12)         | `fpu ok`        |

`sh docs/porting/smoke/run.sh [test]` (or `make check`) runs all tests, or just the named one. Set `CROSS=` to use a
different toolchain prefix.

## Running a kernel image on QEMU

```shell
qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel <image.elf> -nographic
# to debug, add -s -S and attach gdb-multiarch:
#   gdb-multiarch -ex "set architecture m68k" -ex "target remote :1234" <image.elf>
```

Exit QEMU with `Ctrl-A X`.
