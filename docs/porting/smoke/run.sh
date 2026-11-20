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
#        cacr     startup.S: both caches cleared and off, whatever the
#                 bootloader left (D34)
#        bootinfo machdep.c startup(): physmem from the QEMU bootinfo
#        kstring  machdep.c ffs, string and memory routines
#        useraddr machdep.c address and range checks, copyin/copyout/copyinstr
#        udelay   machdep.c udelay() against the Goldfish RTC
#        halt     machdep.c boot(RB_HALT) stops QEMU (exit status 0)
#        swtch    locore.S setjmp/longjmp: switch to the process in u0 and
#                 back, u areas and p_addr exchanged
#        fpswtch  locore.S: FPU registers follow each process across the
#                 switch; savefpu() for fork() (D28)
#        ktrap    trap.c: a kernel-mode exception dumps registers and panics
#        uill     trap.c: user illegal instruction gives SIGILL, frame checked
#        upriv    trap.c: user privileged instruction gives SIGILL
#        utrace   trap.c: user trace gives SIGTRAP, T cleared
#        udivz    trap.c: user divide by zero gives SIGFPE
#        buserr   trap.c, locore.S: user bus error (hand-built format B
#                 frame) gives SIGBUS, frame trimmed to format 0 (D26)
#        ufpe     trap.c: user FPU exception (hand-built frame) gives SIGFPE
#        fpbiu    trap.c fpfault_clear() on hand-built 68881/2 state frames
#        tick     clock.c, trap.c: the RTC 1 alarm ticks at HZ (D27)
#        callout  kern_clock.c timeout() fires on the tick it was due
#        todr     clock.c inittodr(): time of day from RTC 2, or the
#                 file system time if the RTC is behind it
#        sysargs  trap.c: trap #0 copies the arguments from usp + 4, EFAULT
#                 past the user region (D23)
#        sysret   trap.c: result in d0, errno and carry on error,
#                 EJUSTRETURN, other registers kept (D23)
#        sysrestart trap.c: ERESTART runs trap #0 again (D23)
#        nosys    trap.c: an out-of-range number posts SIGSYS, EINVAL if
#                 SIGSYS is ignored
#        msec     clock.c sc_msec(): milliseconds from the tick count
#        execstack exec_subr.c: exec entry stack, sp at argc (D30)
#        execclear exec_subr.c: exec clears the registers, SR and FPU (D30)
#        pipe2    sys_pipe.c: pipe() from user mode returns d0 and d1
#        ptrace   sys_process.c: procxmt() register access and PT_STEP
#        sigframe signal.c sendsig(): the signal frame and handler entry (D31)
#        sigreturn signal.c: user-mode signal round trip through sigreturn
#        sigfpu   signal.c: FPU saved, reset for the handler, restored
#        sigfault signal.c: bad sigcontexts refused, stack overflow SIGILL
#        uentry   startup.S: main() returns into user mode at USER_DATA_START
#                 with a clean SR, stack and registers
#        icode    startup.S icode: execv /sbin/init -s, exit(errno) on failure
#        ttyrx    gftty.c: console input by receive interrupt, past the
#                 device's 128-byte buffer
#        ttyrxopen gftty.c: the receive interrupt is off until open
#        intrnest trap.c: a clock tick waits for the tty interrupt (D33)
#        consdev  sysctl.c, kern_sysctl.c: CPU_CONSDEV from user mode
#        errmsg   sysctl.c, kern_sysctl.c: CPU_ERRMSG from user mode
#        nlist    sysctl.c, kern_sysctl.c: CPU_NLIST from user mode, as an
#                 ordinary user (D35)
#        vdprobe  vd.c: both virtio disks found in command-line order, past
#                 another virtio device, with size and read-only flag (D36)
#        vdread   vd.c: reads finish by interrupt, queued, EOF at the end
#        vdwrite  vd.c: writes reach the disk; sector 0 and a read-only
#                 disk refuse them
#        vdpart   vd.c, disklabel.c: MBR partitions, offsets and bounds
#        vdbytes  vd.c: transfers of part of a sector, as swap does (D38)
#        fsutil   host test, no QEMU: tools/fsutil writes a big-endian file
#                 system (D37) that --check passes and the independent reader
#                 bsdread.c reads back, past block 65535 and through ".."
#        kernel   the whole kernel boots to the M2 root filesystem panic
#                 when it has no disk
#        rootfs   the kernel mounts an fsutil image as root on vd0a, runs
#                 its /sbin/init (rootinit.S, a hand-built a.out), which
#                 copies /etc/motd, past the direct blocks, to the console
#                 and halts; the image still passes fsutil --check after
#
# The tests from startup on link sys/m68k/startup.S and locore.S.  The
# machdep tests add sys/m68k/machdep.c and ktest.c, which stubs the rest of
# the kernel; the trap tests also add trap.c, clock.c, kern_clock.c and
# ttest.c; the exec, pipe, ptrace, signal and tty tests also link the one
# kernel file they test, and the tty tests get QEMU input on stdin.  The
# vd tests get two disk images (see vdstub.h).  The kernel test builds sys/m68k/virt/unix.elf with the board Makefile (in place, like a
# developer build; the products are gitignored) and boots it.
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
# Kernel headers and options for the machdep tests, as the virt board Makefile
# sets them up.
mkdir -p "$out/inc"
ln -sfn "$top/sys/include" "$out/inc/sys"
ln -sfn "$top/sys/m68k" "$out/inc/machine"
KCFLAGS="$CFLAGS -fno-builtin -DKERNEL -DFPU_NO_FSAVE -I$out/inc"

expected() {
    case "$1" in
    hello) echo "hello m68030" ;;
    fpu) echo "fpu ok" ;;
    startup|cacr) echo "$1 ok" ;;
    bootinfo|kstring|useraddr|udelay|swtch|fpswtch) echo "$1 ok" ;;
    ktrap) echo "panic: kernel trap" ;;
    uill|upriv|utrace|udivz|buserr|ufpe|fpbiu|tick|callout|todr|\
    sysargs|sysret|sysrestart|nosys|msec|execstack|execclear|pipe2|ptrace|\
    sigframe|sigreturn|sigfpu|sigfault|uentry|icode|ttyrx|ttyrxopen|intrnest|\
    consdev|errmsg|nlist|vdprobe|vdread|vdwrite|vdpart|vdbytes) echo "$1 ok" ;;
    halt) echo "halted" ;;
    kernel) echo "panic: No root filesystem found!" ;;
    rootfs) echo "rootfs ok: end of /etc/motd" ;;
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
# tty modes on stdin would stop it with SIGTTOU.  A test that needs console
# input gets the file $input instead.
run_qemu() {
    rc=0
    timeout 5 qemu-system-m68k -M virt -cpu m68030 -m 16M -kernel "$@" \
        -nographic -serial stdio -monitor none <"${input:-/dev/null}" \
        2>/dev/null || rc=$?
    echo "qemu exit status $rc"
}

# Check the output of run_qemu against the expected string.  Tests that
# halt must also have stopped QEMU by themselves, with exit status 0.
check_result() {
    name=$1 result=$2 halts=$3
    want=$(expected "$name")
    [ -n "$want" ] || { echo "FAIL $name: no expected output in run.sh" >&2; return 1; }
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

board=$top/sys/m68k/virt
build_kernel() {
    if ! make -C "$board" -j"$(nproc)" CROSS="$CROSS" unix.elf >"$out/kernel-build.log" 2>&1; then
        echo "FAIL $1: kernel build failed, see $out/kernel-build.log" >&2
        tail -20 "$out/kernel-build.log" >&2
        return 1
    fi
}

run_kernel() {
    build_kernel kernel || return 1
    check_result kernel "$(run_qemu "$board/unix.elf")" halts
}

# fsutil_clean name image: a clean --check prints the five phase headers
# and the summary line only (its exit status is 0 even with errors).
fsutil_clean() {
    timeout 60 "$top/tools/fsutil/fsutil" --check --partition=1 "$2" >"$2.check" 2>&1 || true
    if [ "$(grep -cv -e '^\*\* Phase [1-5] - ' \
            -e '^[0-9]* files [0-9]* blocks [0-9]* free$' "$2.check")" != 0 ] ||
       [ "$(wc -l <"$2.check")" != 6 ]; then
        echo "FAIL $1: fsutil --check found errors:" >&2
        head -20 "$2.check" >&2
        return 1
    fi
}

# rootfs: an image with an MBR, a 2 MB root and a 1 MB swap partition,
# extended to its full 1 + 3072 KB (fsutil writes only up to the root).  The
# root holds /dev/console, /sbin/init built from rootinit.S, and /etc/motd:
# 100 filler lines and the expected line last, 6.5 KB, so the kernel reads
# it through an indirect block.
run_rootfs() {
    d=$out/rootfs log=$out/rootfs-build.log
    rm -rf "$d" && mkdir -p "$d/root/etc" "$d/root/sbin"
    build_kernel rootfs || return 1
    user=$(awk '$1 == "USER_DATA_START" { sub(";", "", $3); print $3 }' "$top/src/elf32-m68k.ld")
    if ! { make -C "$top/tools/fsutil" &&
           ${CROSS}gcc $CFLAGS -c "$here/rootinit.S" -o "$d/rootinit.o" &&
           ${CROSS}ld -static -Ttext=$(printf 0x%x $((user - 32))) "$d/rootinit.o" -o "$d/rootinit.elf" &&
           ${CROSS}objcopy -O binary "$d/rootinit.elf" "$d/root/sbin/init"; } >"$log" 2>&1; then
        echo "FAIL rootfs: build failed, see $log" >&2
        tail -20 "$log" >&2
        return 1
    fi
    { awk 'BEGIN { for (i = 1; i <= 100; i++) printf "filler line %03d %48s\n", i, "" }'
      expected rootfs; } >"$d/root/etc/motd"
    printf '%s\n' default 'owner 0' 'group 0' 'dirmode 0775' 'filemode 0664' \
        'dir /dev' 'dir /etc' 'dir /sbin' 'cdev /dev/console' 'major 0' 'minor 0' \
        'file /etc/motd' 'file /sbin/init' 'mode 0700' >"$d/manifest"
    fsutil="timeout 60 $top/tools/fsutil/fsutil"
    { $fsutil --repartition=fs=2M:swap=1M "$d/disk.img" &&
      $fsutil --new --partition=1 --manifest="$d/manifest" "$d/disk.img" "$d/root" &&
      dd if=/dev/zero of="$d/disk.img" bs=1024 count=0 seek=3073 2>/dev/null; } >>"$log" 2>&1 ||
        { echo "FAIL rootfs: creating the image failed, see $log" >&2; return 1; }
    check_result rootfs "$(run_qemu "$board/unix.elf" \
        -drive file="$d/disk.img",if=none,format=raw,id=a -device virtio-blk-device,drive=a)" \
        halts || return 1
    fsutil_clean rootfs "$d/disk.img"
}

# The vd test disks (vdstub.h): every sector holds 64 lines "%07d\n" of
# its number.  Disk A (2048 sectors) has an MBR in sector 0 with two
# partitions, 64+512 and 576+256 sectors; disk B (1024 sectors) has none.
# blkdebug fails reads of disk A's sector 2000 with EIO.
pattern() {
    awk -v from="$1" -v to="$2" 'BEGIN {
        for (n = from; n < to; n++) for (i = 0; i < 64; i++) printf "%07d\n", n }'
}
# A 16-byte MBR entry: type, start, size (start and size < 65536).
mbr_entry() {
    printf '\0\0\0\0\'"$(printf %o "$1")"'\0\0\0'
    for v in "$2" "$3"; do
        printf '\'"$(printf %o $((v % 256)))"'\'"$(printf %o $((v / 256)))"'\0\0'
    done
}
make_disks() {
    [ -f "$out/diskb.img" ] && return
    { head -c 446 /dev/zero; mbr_entry 183 64 512; mbr_entry 184 576 256
      head -c 32 /dev/zero; printf '\125\252'; pattern 1 2048; } >"$out/diska.img"
    pattern 0 1024 >"$out/diskb.img"
    printf '[inject-error]\nevent = "read_aio"\nerrno = "5"\nsector = "2000"\n' \
        >"$out/diska.dbg"
}

# fsutil: a 100 MB root partition holding a directory of 60 files, a
# 3 MB file and a 66 MB file whose blocks go past 65535.  --add names the
# file in the image by the path it is given, so it runs in $d.
run_fsutil() {
    log=$out/fsutil.log d=$out/fsutil
    rm -rf "$d" && mkdir -p "$d/src/etc/sub"
    if ! make -C "$top/tools/fsutil" >"$log" 2>&1 ||
       ! ${HOSTCC:-cc} -O -Wall -Werror -o "$d/bsdread" "$here/bsdread.c" >>"$log" 2>&1; then
        echo "FAIL fsutil: build failed, see $log" >&2
        tail -20 "$log" >&2
        return 1
    fi
    # A byte-order bug can make fsutil loop, so every step has a time limit.
    fsutil="timeout 60 $top/tools/fsutil/fsutil"
    echo hello >"$d/src/etc/motd"
    for i in $(seq 1 60); do echo $i >"$d/src/etc/sub/file$i"; done
    head -c 3000000 /dev/urandom >"$d/src/big"
    head -c 66000000 /dev/urandom >"$d/huge"
    { $fsutil --repartition=fs=100M:swap=2M "$d/disk.img" &&
      $fsutil --new --partition=1 "$d/disk.img" "$d/src" &&
      (cd "$d" && $fsutil --add --partition=1 disk.img huge); } >>"$log" 2>&1 ||
        { echo "FAIL fsutil: creating the image failed, see $log" >&2; return 1; }
    fsutil_clean fsutil "$d/disk.img" || return 1
    r="timeout 60 $d/bsdread"
    $r "$d/disk.img" /huge >"$d/huge.out" 2>"$d/huge.err" &&
        cmp -s "$d/huge" "$d/huge.out" ||
        { echo "FAIL fsutil: /huge read back differs" >&2; return 1; }
    max=$(awk '/^maxblk/ { print $2 }' "$d/huge.err")
    [ "${max:-0}" -gt 65535 ] ||
        { echo "FAIL fsutil: /huge ends at block $max, not past 65535" >&2; return 1; }
    $r "$d/disk.img" /big 2>/dev/null | cmp -s "$d/src/big" - ||
        { echo "FAIL fsutil: /big read back differs" >&2; return 1; }
    got=$($r "$d/disk.img" /../lost+found/../etc/sub/file60 2>/dev/null) || true
    [ "$got" = 60 ] ||
        { echo "FAIL fsutil: /../lost+found/../etc/sub/file60 gave '$got'" >&2; return 1; }
    rm -rf "$d"
    echo "PASS fsutil"
}

# Run one build step, logging to $log.  run_test is called in an || list,
# where set -e does not apply, so failures are recorded in $buildfail.
build() {
    "$@" >>"$log" 2>&1 || buildfail=yes
}

run_test() {
    name=$1
    [ "$name" = kernel ] && { run_kernel; return; }
    [ "$name" = fsutil ] && { run_fsutil; return; }
    [ "$name" = rootfs ] && { run_rootfs; return; }
    log=$out/$name-build.log buildfail=
    rm -f "$log" "$out/$name.elf"
    # Tests from startup on use the kernel's own startup code and memory map.
    machdep="bootinfo kstring useraddr udelay halt swtch fpswtch"
    traps="ktrap uill upriv utrace udivz buserr ufpe fpbiu tick callout todr \
        sysargs sysret sysrestart nosys msec execstack execclear pipe2 ptrace \
        sigframe sigreturn sigfpu sigfault uentry icode ttyrx ttyrxopen intrnest \
        consdev errmsg nlist vdprobe vdread vdwrite vdpart vdbytes"
    class=plain
    case " startup cacr $machdep $traps " in *" $name "*) class=startup ;; esac
    case " $machdep " in *" $name "*) class=machdep ;; esac
    case " $traps " in *" $name "*) class=trap ;; esac
    cflags=$CFLAGS
    objs="$out/$name-crt.o $out/$name.o"
    if [ $class = plain ]; then
        crt=$here/crt.S; ld=$here/l.ld
    else
        crt=$top/sys/m68k/startup.S; ld=$top/sys/m68k/virt/kernel.ld
        build ${CROSS}gcc $KCFLAGS -c "$top/sys/m68k/locore.S" -o "$out/locore.o"
        objs="$objs $out/locore.o"
    fi
    if [ $class = machdep ] || [ $class = trap ]; then
        cflags="$KCFLAGS"
        case $name in ttyrx|ttyrxopen|intrnest)
            cflags="$cflags -DGFTTY_ENABLED -DCONS_MAJOR=UART_MAJOR -DCONS_MINOR=0" ;;
        consdev|errmsg|nlist)
            cflags="$cflags -DCONS_MAJOR=UART_MAJOR -DCONS_MINOR=0" ;;
        vd*)
            cflags="$cflags -DVD_ENABLED" ;;
        esac
        build ${CROSS}gcc $cflags -c "$top/sys/m68k/machdep.c" -o "$out/machdep.o"
        build ${CROSS}gcc $cflags -c "$here/ktest.c" -o "$out/ktest.o"
        objs="$objs $out/machdep.o $out/ktest.o"
    fi
    if [ $class = trap ]; then
        build ${CROSS}gcc $cflags -c "$top/sys/m68k/trap.c" -o "$out/trap.o"
        build ${CROSS}gcc $cflags -c "$top/sys/m68k/clock.c" -o "$out/clock.o"
        build ${CROSS}gcc $cflags -c "$top/sys/kernel/kern_clock.c" -o "$out/kern_clock.o"
        build ${CROSS}gcc $cflags -c "$here/ttest.c" -o "$out/ttest.o"
        objs="$objs $out/trap.o $out/clock.o $out/kern_clock.o $out/ttest.o"
    fi
    # The kernel files, under sys/, a test checks.
    case $name in
    execstack|execclear) extra=kernel/exec_subr ;;
    pipe2) extra=kernel/sys_pipe ;;
    ptrace) extra=kernel/sys_process ;;
    sigframe|sigreturn|sigfpu|sigfault) extra=m68k/signal ;;
    ttyrx|ttyrxopen|intrnest) extra=m68k/gftty ;;
    consdev|errmsg|nlist) extra="kernel/kern_sysctl m68k/sysctl" ;;
    vd*) extra="m68k/vd m68k/disklabel" ;;
    *) extra= ;;
    esac
    for f in $extra; do
        build ${CROSS}gcc $cflags -c "$top/sys/$f.c" -o "$out/${f##*/}.o"
        objs="$objs $out/${f##*/}.o"
    done
    build ${CROSS}gcc $cflags -c "$here/$name.c" -o "$out/$name.o"
    build ${CROSS}gcc $cflags -c "$crt" -o "$out/$name-crt.o"
    [ -z "$buildfail" ] && build ${CROSS}ld -T "$ld" -static $objs -o "$out/$name.elf"
    if [ -n "$buildfail" ]; then
        echo "FAIL $name: build failed, see $log" >&2
        tail -20 "$log" >&2
        return 1
    fi
    # Pre-fill smoke_bss, if the test has it, so a missing BSS clear shows.
    prefill=
    bss=$(${CROSS}nm "$out/$name.elf" | awk '$3 == "smoke_bss" { print $1 }')
    [ -n "$bss" ] && prefill="-device loader,addr=0x$bss,data=0xdeadbeef,data-len=4"
    input=
    case $name in
    ttyrx|ttyrxopen|intrnest)
        input=$out/tty-input
        awk 'BEGIN { for (i = 0; i < 300; i++) printf "%c", 97 + i % 26 }' >"$input" ;;
    esac
    disks=
    case $name in vd*)
        make_disks
        cp "$out/diska.img" "$out/$name-a.img"
        disks="-drive file=blkdebug:$out/diska.dbg:$out/$name-a.img,if=none,format=raw,id=a
            -device virtio-blk-device,drive=a -device virtio-rng-device
            -drive file=$out/diskb.img,if=none,format=raw,id=b,readonly=on
            -device virtio-blk-device,drive=b" ;;
    esac
    halts=
    [ "$name" = halt ] || [ $class = trap ] && halts=yes
    check_result "$name" "$(run_qemu "$out/$name.elf" $prefill $disks)" $halts
}

status=0
for t in ${1:-hello fpu startup cacr bootinfo kstring useraddr udelay halt swtch fpswtch \
        ktrap uill upriv utrace udivz buserr ufpe fpbiu tick callout todr \
        sysargs sysret sysrestart nosys msec execstack execclear pipe2 ptrace \
        sigframe sigreturn sigfpu sigfault uentry icode ttyrx ttyrxopen intrnest \
        consdev errmsg nlist vdprobe vdread vdwrite vdpart vdbytes fsutil kernel rootfs}; do
    run_test "$t" || status=1
done
exit $status
