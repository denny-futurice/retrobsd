#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
# The sources are NetBSD's common/lib/libc/arch/m68k/string, unchanged (D42).
#
TOPSRC		= $(shell cd ../../../..; pwd)
include $(TOPSRC)/target.mk

# _LIBC: ffs.S names its entry ffs, not the kernel's __ffssi2.
ASFLAGS		+= ${DEFS} -D_LIBC

SRCS		= bcmp.S bcopy.S bzero.S ffs.S memcpy.S memmove.S memset.S \
                  strchr.S strcmp.S strlen.S strrchr.S
OBJS		= bcmp.o bcopy.o bzero.o ffs.o memcpy.o memmove.o memset.o \
                  strchr.o strcmp.o strlen.o strrchr.o

string.a:	${OBJS}
		@echo "buiding string.a"
		@$(AR) cru string.a ${OBJS}

tags:
		cwd=`pwd`; \
		for i in ${SRCS}; do \
			ctags -a -f ${TAGSFILE} $$cwd/$$i; \
		done

clean:
		rm -f string.a *.o *~ profiled/*.o tags Makefile.bak

depend:
		for i in ${SRCS}; do \
		    cc -M ${CFLAGS} $$i | awk ' { if ($$1 != prev) \
			{ if (rec != "") print rec; rec = $$0; prev = $$1; } \
			else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
			else rec = rec " " $$2 } } \
			END { print rec } ' >> makedep; done
		echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
		echo '$$r makedep' >>eddep
		echo 'w' >>eddep
		cp Makefile Makefile.bak
		ed - Makefile < eddep
		rm eddep makedep
		echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
		echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
		echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
