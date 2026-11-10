MACHINE     = m68k
DESTDIR     ?= $(TOPSRC)
RELEASE     = 0.0
BUILD       = $(shell git rev-list HEAD --count)
VERSION     = $(RELEASE)-$(BUILD)

# Cross toolchain prefix.  Debian has no m68k-elf package, so the m68k-linux-gnu GCC
# is used freestanding.  Override on the command line or in Makefile.user if needed.
CROSS       ?= m68k-linux-gnu-

CC		= $(CROSS)gcc -mcpu=68030 -m68881 -std=gnu99 -ffreestanding \
                  -fno-pic -fno-pie -fno-stack-protector \
                  -fomit-frame-pointer -I$(TOPSRC)/include
LD		= $(CROSS)ld -m m68kelf
AR		= $(CROSS)ar
RANLIB          = $(CROSS)ranlib
SIZE            = $(CROSS)size
NM              = $(CROSS)nm
OBJDUMP         = $(CROSS)objdump
AS		= $(CC) -x assembler-with-cpp -c
YACC            = byacc
LEX             = flex
INSTALL		= install -m 644
INSTALLDIR	= install -m 755 -d
TAGSFILE	= tags
MANROFF		= nroff -man -h
DOCROFF		= nroff -mdoc -h
ELF2AOUT	= $(TOPSRC)/tools/elf2aout/elf2aout

CFLAGS		= -Os -nostdinc

LDFLAGS		= --nmagic -T$(TOPSRC)/src/elf32-mips.ld $(TOPSRC)/src/crt0.o -L$(TOPSRC)/src
LIBS		= -lc

# Enable mips16e instruction set by default
#CFLAGS		+= -mips16

# Catch all warnings
CC              += -Werror
