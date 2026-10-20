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
| [03-decisions.md](03-decisions.md)     | Decision record (D1-D11)                  |
| [smoke/](smoke/)                       | Freestanding toolchain/QEMU smoke test    |

## Host setup (Debian Testing)

```shell
sudo apt install gcc-m68k-linux-gnu binutils-m68k-linux-gnu qemu-system-misc
```

Debian has no `m68k-elf` or `qemu-system-m68k` package: the compiler is the
`m68k-linux-gnu-` one, used freestanding, and `qemu-system-m68k` comes from
`qemu-system-misc`. Verified versions are listed in decision D10.

## Smoke test

```shell
sh docs/porting/smoke/run.sh
```

Builds a tiny freestanding program with the flags the port will use, boots it
on `qemu-system-m68k -M virt -cpu m68030`, and expects `hello m68030 9` on the
Goldfish TTY. Set `CROSS=` to use a different toolchain prefix.

## Running a kernel image on QEMU

```shell
qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel <image.elf> -nographic
# add -s -S and attach m68k-linux-gnu-gdb (or gdb-multiarch) to debug
```

Exit QEMU with `Ctrl-A X`.
