#!/bin/sh
# Build the RetroBSD m68k kernel, userland and sdcard.img, then boot it in
# QEMU (virt, 68030) on the terminal for interactive testing.
#
# Usage: boot-retrobsd.sh [-n] [-m MEM] [repo]
#   -n      skip the build, boot the existing kernel and image
#   -m MEM  guest RAM (default 16M)
#   repo    RetroBSD tree (default: the directory of this script)
#
# Login as root with an empty password. Quit QEMU with Ctrl-A x.
# The image is booted from a scratch copy, so sdcard.img is not modified.
set -e

build=1 mem=16M
while getopts nm: opt; do
    case $opt in
    n) build=0 ;;
    m) mem=$OPTARG ;;
    *) sed -n '2,11s/^# \{0,1\}//p' "$0"; exit 2 ;;
    esac
done
shift $((OPTIND - 1))
top=${1:-$(cd "$(dirname "$0")" && pwd)}

for t in m68k-linux-gnu-gcc qemu-system-m68k make; do
    command -v $t >/dev/null || { echo "missing: $t" >&2; exit 1; }
done

if [ $build = 1 ]; then
    echo "== building tools, kernel, userland and sdcard.img in $top"
    make -C "$top" -j"$(nproc 2>/dev/null || echo 2)"
fi

kernel=$top/sys/m68k/virt/unix.elf
image=$top/sdcard.img
[ -f "$kernel" ] || { echo "no kernel: $kernel" >&2; exit 1; }
[ -f "$image" ] || { echo "no image: $image" >&2; exit 1; }

echo "== checking the root filesystem"
"$top/tools/fsutil/fsutil" --check --partition=1 "$image"

# fsutil writes a sparse image shorter than its partition table; QEMU
# needs the whole disk, so boot a copy grown to cover all partitions.
disk=$(mktemp /tmp/retrobsd-boot.XXXXXX.img)
trap 'rm -f "$disk"' EXIT INT TERM
cp --sparse=always "$image" "$disk"
truncate -s 220M "$disk"

echo "== booting; login: root (no password); quit: Ctrl-A x"
qemu-system-m68k -M virt -cpu m68030 -m "$mem" \
    -kernel "$kernel" -nographic -serial mon:stdio \
    -drive file="$disk",if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0
