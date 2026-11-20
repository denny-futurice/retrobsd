#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
TOPSRC		= $(shell cd ../../..; pwd)
include $(TOPSRC)/target.mk

CFLAGS          += ${DEFS}

# The 64-bit integer helpers GCC calls for m68k with -m68881 (D43):
# division and modulo, shifts by a variable count (inline or not, as -Os
# sees fit), and conversions between 64-bit integers and float or double,
# which the 68881 lacks.  Multiply is inline.  The soft-float sources are
# not built (D12); the conversions use their hardware FPU paths (GCC does
# not define __SOFT_FP__).  Sources are compiler-rt 12.0.0, unchanged.
OBJS            = divdi3.o \
                  moddi3.o \
                  udivdi3.o \
                  udivmoddi4.o \
                  umoddi3.o \
                  ashldi3.o \
                  ashrdi3.o \
                  lshrdi3.o \
                  fixdfdi.o \
                  fixsfdi.o \
                  fixunsdfdi.o \
                  fixunssfdi.o \
                  floatdidf.o \
                  floatdisf.o \
                  floatundidf.o \
                  floatundisf.o

runtime.a:      ${OBJS}
		@echo "buiding runtime.a"
		@$(AR) cru runtime.a ${OBJS}
clean:
		rm -f *.a *.o *~ profiled/*.o tags

cleandir:       clean
		rm -f .depend
