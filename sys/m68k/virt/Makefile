PARAM = -DQEMU_VIRT
PARAM += -DM68030
PARAM += -DGFTTY_ENABLED
PARAM += -DCONS_MINOR=0
PARAM += -DCONS_MAJOR=UART_MAJOR
LDSCRIPT = "virt/kernel.ld"
#
# Makefile for RetroBSD, m68k target (GCC, see docs/porting decision D14)
#
# DEBUG is set to -g by kconfig if debugging is requested (kconfig -g).
#
# Cross toolchain prefix.  Debian has no m68k-elf package, so the m68k-linux-gnu GCC
# is used freestanding.  Override on the command line or in Makefile.user if needed.
CROSS           ?= m68k-linux-gnu-

AS              = $(CROSS)gcc ${DEBUG}
CC              = $(CROSS)gcc -mcpu=68030 -m68881 -std=gnu99 -ffreestanding \
                  -fno-pic -fno-pie -fno-stack-protector \
                  ${DEBUG} -nostdinc -fno-builtin -Werror -Wall
CPP             = $(CROSS)cpp
LD              = $(CROSS)ld -m m68kelf
LDFLAGS         = -nostdlib -T ../${LDSCRIPT}
SIZE            = $(CROSS)size
OBJCOPY         = $(CROSS)objcopy
OBJDUMP         = $(CROSS)objdump
NM              = $(CROSS)nm

# sources are located via $S relative to the compilation directory
S               = ../..

DEPFLAGS	= -MT $@ -MP -MD -MF .deps/$*.dep
DEFS            = -I. ${PARAM} -DKERNEL $(DEPFLAGS)
CFLAGS          = -O ${DEFS}

# compile rules: rules are named COMPILE_${SUFFIX}
# SUFFIX is the file suffix, capitalized (e.g. C for a .c file).

COMPILE_C       = ${CC} -c ${CFLAGS} $<
COMPILE_S       = ${CC} -c ${DEFS} $<

OBJS = exec_aout.o exec_conf.o exec_elf.o exec_script.o init_main.o \
	init_sysent.o kern_clock.o kern_descrip.o kern_exec.o \
	kern_exit.o kern_fork.o kern_mman.o kern_proc.o kern_prot.o \
	kern_prot2.o kern_resource.o kern_sig.o kern_sig2.o kern_subr.o \
	kern_synch.o kern_sysctl.o kern_time.o subr_prf.o subr_rmap.o \
	sys_generic.o sys_inode.o syscalls.o tty.o tty_subr.o tty_tty.o \
	ufs_alloc.o ufs_bio.o ufs_bmap.o ufs_dsort.o ufs_fio.o \
	ufs_inode.o ufs_mount.o ufs_namei.o ufs_subr.o ufs_syscalls.o \
	ufs_syscalls2.o vfs_vnops.o vm_sched.o vm_swap.o vm_swp.o \
	clock.o cons.o devsw.o gftty.o machdep.o mem.o stubs.o swap.o \
	sysctl.o locore.o trap.o 

CFILES = $S/kernel/exec_aout.c $S/kernel/exec_conf.c $S/kernel/exec_elf.c \
	$S/kernel/exec_script.c $S/kernel/init_main.c \
	$S/kernel/init_sysent.c $S/kernel/kern_clock.c \
	$S/kernel/kern_descrip.c $S/kernel/kern_exec.c \
	$S/kernel/kern_exit.c $S/kernel/kern_fork.c \
	$S/kernel/kern_mman.c $S/kernel/kern_proc.c \
	$S/kernel/kern_prot.c $S/kernel/kern_prot2.c \
	$S/kernel/kern_resource.c $S/kernel/kern_sig.c \
	$S/kernel/kern_sig2.c $S/kernel/kern_subr.c \
	$S/kernel/kern_synch.c $S/kernel/kern_sysctl.c \
	$S/kernel/kern_time.c $S/kernel/subr_prf.c $S/kernel/subr_rmap.c \
	$S/kernel/sys_generic.c $S/kernel/sys_inode.c \
	$S/kernel/syscalls.c $S/kernel/tty.c $S/kernel/tty_subr.c \
	$S/kernel/tty_tty.c $S/kernel/ufs_alloc.c $S/kernel/ufs_bio.c \
	$S/kernel/ufs_bmap.c $S/kernel/ufs_dsort.c $S/kernel/ufs_fio.c \
	$S/kernel/ufs_inode.c $S/kernel/ufs_mount.c \
	$S/kernel/ufs_namei.c $S/kernel/ufs_subr.c \
	$S/kernel/ufs_syscalls.c $S/kernel/ufs_syscalls2.c \
	$S/kernel/vfs_vnops.c $S/kernel/vm_sched.c $S/kernel/vm_swap.c \
	$S/kernel/vm_swp.c $S/m68k/clock.c $S/m68k/cons.c \
	$S/m68k/devsw.c $S/m68k/gftty.c $S/m68k/machdep.c $S/m68k/mem.c \
	$S/m68k/stubs.c $S/m68k/swap.c $S/m68k/sysctl.c $S/m68k/trap.c \
	swapunix.c 

# load lines for config "xxx" will be emitted as:
# xxx: ${SYSTEM_OBJ} swapxxx.o ${SYSTEM_DEP}
#	${SYSTEM_LD_HEAD}
#	${SYSTEM_LD} swapxxx.o
#	${SYSTEM_LD_TAIL}
SYSTEM_OBJ      = startup.o ${OBJS} ioconf.o
SYSTEM_DEP      = Makefile ioconf.c | machine sys .deps
SYSTEM_LD_HEAD  = sh ../newvers.sh > vers.c; ${CC} $(CFLAGS) -c vers.c; rm -f $@
SYSTEM_LD       = -@echo ${LD} ${LDFLAGS} '$${SYSTEM_OBJ}' vers.o -o $@; \
                  ${LD} ${LDFLAGS} ${SYSTEM_OBJ} vers.o -o $@
SYSTEM_LD_TAIL  = ${SIZE} $@; \
                  $(OBJCOPY) -O binary $@ $(basename $@).bin; \
                  $(OBJDUMP) -d -S $@ > $(basename $@).dis; \
                  $(NM) -n $@ > $(basename $@).nm

unix: unix.elf

unix.elf: ${SYSTEM_OBJ} swapunix.o ${SYSTEM_DEP}
	${SYSTEM_LD_HEAD}
	${SYSTEM_LD} swapunix.o
	${SYSTEM_LD_TAIL}

swapunix.o: swapunix.c ${SYSTEM_DEP}
	${COMPILE_C}

all: unix

clean:
	rm -rf *.elf *.o *.nm *.dis *.bin

clean-all: clean
	rm -f *.h ioconf.c swap*.c vers.c machine sys .deps

.PHONY: reconfig

reconfig: ioconf.c

ioconf.c: Config ../../../tools/kconfig/kconfig | sys machine
	../../../tools/kconfig/kconfig Config
	$(MAKE) clean

# Written by the same kconfig run as ioconf.c above.  Keep it a plain
# dependency: two rules invoking kconfig would race under make -j.
swapunix.c: ioconf.c
	@:

machine:
	ln -s .. $@

sys:
	ln -s ../../include $@

.deps:
	mkdir .deps

startup.o: ../startup.S ${SYSTEM_DEP}
	${COMPILE_S}

ioconf.o: ioconf.c  ${SYSTEM_DEP}
	${COMPILE_C}

exec_aout.o: $S/kernel/exec_aout.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_conf.o: $S/kernel/exec_conf.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_elf.o: $S/kernel/exec_elf.c ${SYSTEM_DEP}
	${COMPILE_C}

exec_script.o: $S/kernel/exec_script.c ${SYSTEM_DEP}
	${COMPILE_C}

init_main.o: $S/kernel/init_main.c ${SYSTEM_DEP}
	${COMPILE_C}

init_sysent.o: $S/kernel/init_sysent.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_clock.o: $S/kernel/kern_clock.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_descrip.o: $S/kernel/kern_descrip.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_exec.o: $S/kernel/kern_exec.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_exit.o: $S/kernel/kern_exit.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_fork.o: $S/kernel/kern_fork.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_mman.o: $S/kernel/kern_mman.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_proc.o: $S/kernel/kern_proc.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_prot.o: $S/kernel/kern_prot.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_prot2.o: $S/kernel/kern_prot2.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_resource.o: $S/kernel/kern_resource.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_sig.o: $S/kernel/kern_sig.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_sig2.o: $S/kernel/kern_sig2.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_subr.o: $S/kernel/kern_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_synch.o: $S/kernel/kern_synch.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_sysctl.o: $S/kernel/kern_sysctl.c ${SYSTEM_DEP}
	${COMPILE_C}

kern_time.o: $S/kernel/kern_time.c ${SYSTEM_DEP}
	${COMPILE_C}

subr_prf.o: $S/kernel/subr_prf.c ${SYSTEM_DEP}
	${COMPILE_C}

subr_rmap.o: $S/kernel/subr_rmap.c ${SYSTEM_DEP}
	${COMPILE_C}

sys_generic.o: $S/kernel/sys_generic.c ${SYSTEM_DEP}
	${COMPILE_C}

sys_inode.o: $S/kernel/sys_inode.c ${SYSTEM_DEP}
	${COMPILE_C}

syscalls.o: $S/kernel/syscalls.c ${SYSTEM_DEP}
	${COMPILE_C}

tty.o: $S/kernel/tty.c ${SYSTEM_DEP}
	${COMPILE_C}

tty_subr.o: $S/kernel/tty_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

tty_tty.o: $S/kernel/tty_tty.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_alloc.o: $S/kernel/ufs_alloc.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_bio.o: $S/kernel/ufs_bio.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_bmap.o: $S/kernel/ufs_bmap.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_dsort.o: $S/kernel/ufs_dsort.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_fio.o: $S/kernel/ufs_fio.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_inode.o: $S/kernel/ufs_inode.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_mount.o: $S/kernel/ufs_mount.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_namei.o: $S/kernel/ufs_namei.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_subr.o: $S/kernel/ufs_subr.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_syscalls.o: $S/kernel/ufs_syscalls.c ${SYSTEM_DEP}
	${COMPILE_C}

ufs_syscalls2.o: $S/kernel/ufs_syscalls2.c ${SYSTEM_DEP}
	${COMPILE_C}

vfs_vnops.o: $S/kernel/vfs_vnops.c ${SYSTEM_DEP}
	${COMPILE_C}

vm_sched.o: $S/kernel/vm_sched.c ${SYSTEM_DEP}
	${COMPILE_C}

vm_swap.o: $S/kernel/vm_swap.c ${SYSTEM_DEP}
	${COMPILE_C}

vm_swp.o: $S/kernel/vm_swp.c ${SYSTEM_DEP}
	${COMPILE_C}

clock.o: $S/m68k/clock.c ${SYSTEM_DEP}
	${COMPILE_C}

cons.o: $S/m68k/cons.c ${SYSTEM_DEP}
	${COMPILE_C}

devsw.o: $S/m68k/devsw.c ${SYSTEM_DEP}
	${COMPILE_C}

gftty.o: $S/m68k/gftty.c ${SYSTEM_DEP}
	${COMPILE_C}

machdep.o: $S/m68k/machdep.c ${SYSTEM_DEP}
	${COMPILE_C}

mem.o: $S/m68k/mem.c ${SYSTEM_DEP}
	${COMPILE_C}

stubs.o: $S/m68k/stubs.c ${SYSTEM_DEP}
	${COMPILE_C}

swap.o: $S/m68k/swap.c ${SYSTEM_DEP}
	${COMPILE_C}

sysctl.o: $S/m68k/sysctl.c ${SYSTEM_DEP}
	${COMPILE_C}

locore.o: $S/m68k/locore.S ${SYSTEM_DEP}
	${COMPILE_S}

trap.o: $S/m68k/trap.c ${SYSTEM_DEP}
	${COMPILE_C}


-include .deps/*.dep
