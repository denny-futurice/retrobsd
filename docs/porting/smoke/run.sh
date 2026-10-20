#!/bin/sh
# Build the freestanding m68k smoke test and boot it on QEMU virt (68030).
# Expected output: "hello m68030 9"
set -e

CROSS=${CROSS:-m68k-linux-gnu-}
here=$(cd "$(dirname "$0")" && pwd)
out=${OUT:-${TMPDIR:-/tmp}/retrobsd-m68k-smoke}
mkdir -p "$out"

CFLAGS="-mcpu=68030 -msoft-float -ffreestanding -nostdlib -nostdinc -fno-pic -fno-pie \
-fno-stack-protector -std=gnu99 -Os -Wall -Werror"

for tool in ${CROSS}gcc ${CROSS}ld qemu-system-m68k; do
    command -v "$tool" >/dev/null || { echo "missing tool: $tool" >&2; exit 1; }
done
${CROSS}gcc --version | head -1
qemu-system-m68k --version | head -1

${CROSS}gcc $CFLAGS -c "$here/t.c" -o "$out/t.o"
${CROSS}gcc $CFLAGS -c "$here/crt.S" -o "$out/crt.o"
${CROSS}ld -T "$here/l.ld" -static "$out/crt.o" "$out/t.o" -o "$out/t.elf"

# The test spins forever after printing, so stop it after a short timeout.
result=$(timeout 5 qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel "$out/t.elf" \
    -nographic -serial stdio -monitor none 2>&1 || true)
echo "$result"
case "$result" in
*"hello m68030 9"*) echo "PASS" ;;
*) echo "FAIL" >&2; exit 1 ;;
esac
