#!/bin/sh
# Freestanding m68k smoke tests on QEMU virt (68030). Each test is one source file
# that checks a single thing and prints an expected string to the Goldfish TTY.
#
#   sh run.sh            run all tests
#   sh run.sh fpu        run one test
#
# Tests: hello  toolchain, linker script and QEMU boot
#        fpu    hardware floating point (D12)
set -e

CROSS=${CROSS:-m68k-linux-gnu-}
here=$(cd "$(dirname "$0")" && pwd)
out=${OUT:-${TMPDIR:-/tmp}/retrobsd-m68k-smoke}
mkdir -p "$out"

CFLAGS="-mcpu=68030 -m68881 -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie \
-fno-stack-protector -std=gnu99 -Os -Wall -Werror"

expected() {
    case "$1" in
    hello) echo "hello m68030" ;;
    fpu) echo "fpu ok" ;;
    esac
}

for tool in ${CROSS}gcc ${CROSS}ld qemu-system-m68k; do
    command -v "$tool" >/dev/null || { echo "missing tool: $tool" >&2; exit 1; }
done
command -v gdb-multiarch >/dev/null || echo "warning: gdb-multiarch not found (debugging only)" >&2
${CROSS}gcc --version | head -1
qemu-system-m68k --version | head -1

run_test() {
    name=$1
    want=$(expected "$name")
    ${CROSS}gcc $CFLAGS -c "$here/$name.c" -o "$out/$name.o"
    ${CROSS}gcc $CFLAGS -c "$here/crt.S" -o "$out/crt.o"
    ${CROSS}ld -T "$here/l.ld" -static "$out/crt.o" "$out/$name.o" -o "$out/$name.elf" 2>/dev/null
    # Tests spin forever after printing, so stop QEMU after a short timeout.
    result=$(timeout 5 qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel "$out/$name.elf" \
        -nographic -serial stdio -monitor none 2>/dev/null || true)
    case "$result" in
    *"$want"*) echo "PASS $name" ;;
    *) echo "FAIL $name: expected '$want', got '$result'" >&2; return 1 ;;
    esac
}

status=0
for t in ${1:-hello fpu}; do
    run_test "$t" || status=1
done
exit $status
