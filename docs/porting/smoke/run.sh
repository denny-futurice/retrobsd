#!/bin/sh
# Freestanding m68k smoke tests on QEMU virt (68030). Each test is one source file
# that checks a single thing and prints an expected string to the Goldfish TTY.
#
#   sh run.sh            run all tests
#   sh run.sh fpu        run one test
#   sh run.sh -v [test]  also print each test's console output to stdout
#
# Tests: hello    toolchain, linker script and QEMU boot
#        fpu      hardware floating point (D12)
#        startup  kernel startup.S: BSS clear, stack, VBR
#        badvec   kernel startup.S: unexpected exception reaches badtrap()
#        bootinfo machdep.c startup(): physmem from the QEMU bootinfo
#        kstring  machdep.c ffs, string and memory routines
#        useraddr machdep.c baduaddr/badkaddr, copyin/copyout range checks
#        udelay   machdep.c udelay() against the Goldfish RTC
#        halt     machdep.c boot(RB_HALT) stops QEMU (exit status 0)
#        kernel   the whole kernel boots to the M2 root filesystem panic
#
# The machdep tests link sys/m68k/machdep.c with ktest.c, which stubs the
# rest of the kernel.  The kernel test builds sys/m68k/virt/unix.elf with
# the board Makefile (in place, like a developer build; the products are
# gitignored) and boots it.
set -e

verbose=
if [ "$1" = -v ]; then
    verbose=yes
    shift
fi

CROSS=${CROSS:-m68k-linux-gnu-}
here=$(cd "$(dirname "$0")" && pwd)
top=$(cd "$here/../../.." && pwd)
out=${OUT:-${TMPDIR:-/tmp}/retrobsd-m68k-smoke}
mkdir -p "$out"

CFLAGS="-mcpu=68030 -m68881 -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie \
-fno-stack-protector -std=gnu99 -Os -Wall -Werror"
# Kernel headers for the machdep tests, as the board Makefile sets them up.
mkdir -p "$out/inc"
ln -sfn "$top/sys/include" "$out/inc/sys"
ln -sfn "$top/sys/m68k" "$out/inc/machine"
KCFLAGS="$CFLAGS -fno-builtin -DKERNEL -I$out/inc"

expected() {
    case "$1" in
    hello) echo "hello m68030" ;;
    fpu) echo "fpu ok" ;;
    startup) echo "startup ok" ;;
    badvec) echo "badvec ok" ;;
    bootinfo|kstring|useraddr|udelay) echo "$1 ok" ;;
    halt) echo "halted" ;;
    kernel) echo "panic: No root filesystem found!" ;;
    esac
}

for tool in ${CROSS}gcc ${CROSS}ld qemu-system-m68k; do
    command -v "$tool" >/dev/null || { echo "missing tool: $tool" >&2; exit 1; }
done
command -v gdb-multiarch >/dev/null || echo "warning: gdb-multiarch not found (debugging only)" >&2
${CROSS}gcc --version | head -1
qemu-system-m68k --version | head -1

# Run an ELF on QEMU; prints its console output and "qemu exit status N".
# Most tests spin forever after printing, so stop QEMU after a short timeout;
# the machdep tests and the kernel halt QEMU themselves.  stdin is /dev/null:
# timeout runs QEMU outside the terminal's foreground group, and QEMU setting
# tty modes on stdin would stop it with SIGTTOU.
run_qemu() {
    rc=0
    timeout 5 qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel "$@" \
        -nographic -serial stdio -monitor none </dev/null 2>/dev/null || rc=$?
    echo "qemu exit status $rc"
}

# Check the output of run_qemu against the expected string.  Tests that
# halt must also have stopped QEMU by themselves, with exit status 0.
check_result() {
    name=$1 result=$2 halts=$3
    want=$(expected "$name")
    if [ -n "$verbose" ]; then
        echo "--- $name output"
        printf '%s\n' "$result" | tr -d '\r' | sed 's/^/  /'
    fi
    if [ -n "$halts" ]; then
        case "$result" in
        *"qemu exit status 0") ;;
        *) echo "FAIL $name: QEMU did not exit by itself: '$result'" >&2; return 1 ;;
        esac
    fi
    case "$result" in
    *"$want"*) echo "PASS $name" ;;
    *) echo "FAIL $name: expected '$want', got '$result'" >&2; return 1 ;;
    esac
}

run_kernel() {
    board=$top/sys/m68k/virt
    if ! make -C "$board" -j"$(nproc)" CROSS="$CROSS" unix.elf >"$out/kernel-build.log" 2>&1; then
        echo "FAIL kernel: build failed, see $out/kernel-build.log" >&2
        tail -20 "$out/kernel-build.log" >&2
        return 1
    fi
    check_result kernel "$(run_qemu "$board/unix.elf")" halts
}

run_test() {
    name=$1
    [ "$name" = kernel ] && { run_kernel; return; }
    # Tests from startup on use the kernel's own startup code and memory map.
    case "$name" in
    startup|badvec|bootinfo|kstring|useraddr|udelay|halt)
        crt=$top/sys/m68k/startup.S; ld=$top/sys/m68k/virt/kernel.ld ;;
    *) crt=$here/crt.S; ld=$here/l.ld ;;
    esac
    cflags=$CFLAGS
    objs="$out/$name-crt.o $out/$name.o"
    case "$name" in
    bootinfo|kstring|useraddr|udelay|halt)
        cflags="$KCFLAGS"
        ${CROSS}gcc $cflags -c "$top/sys/m68k/machdep.c" -o "$out/machdep.o"
        ${CROSS}gcc $cflags -c "$here/ktest.c" -o "$out/ktest.o"
        objs="$objs $out/machdep.o $out/ktest.o" ;;
    esac
    ${CROSS}gcc $cflags -c "$here/$name.c" -o "$out/$name.o"
    ${CROSS}gcc $cflags -c "$crt" -o "$out/$name-crt.o"
    ${CROSS}ld -T "$ld" -static $objs -o "$out/$name.elf" 2>/dev/null
    # Pre-fill smoke_bss, if the test has it, so a missing BSS clear shows.
    prefill=
    bss=$(${CROSS}nm "$out/$name.elf" | awk '$3 == "smoke_bss" { print $1 }')
    [ -n "$bss" ] && prefill="-device loader,addr=0x$bss,data=0xdeadbeef,data-len=4"
    halts=
    [ "$name" = halt ] && halts=yes
    check_result "$name" "$(run_qemu "$out/$name.elf" $prefill)" $halts
}

status=0
for t in ${1:-hello fpu startup badvec bootinfo kstring useraddr udelay halt kernel}; do
    run_test "$t" || status=1
done
exit $status
