/*
 * Assembler macros for the m68k libc sources taken unchanged from NetBSD
 * (src/libc/m68k/string, D42): the subset of NetBSD's
 * sys/arch/m68k/include/asm.h they use, without profiling.
 */
#ifndef _MACHINE_ASM_H_
#define _MACHINE_ASM_H_

/*
 * GCC's m68k ELF convention returns pointers in a0 and callers read them
 * there; the NetBSD sources copy a pointer result from d0 to a0 only under
 * __SVR4_ABI__, which this GCC does not define.
 */
#ifndef __SVR4_ABI__
#define __SVR4_ABI__ 1
#endif

#define _C_LABEL(x)     x
#define _ASM_LABEL(x)   x

#define _ENTRY(name) \
        .text; .even; .globl name; .type name,@function; name:
#define ENTRY(name)             _ENTRY(_C_LABEL(name))
#define ASENTRY(name)           _ENTRY(_ASM_LABEL(name))
#define ENTRY_NOPROFILE(name)   ENTRY(name)
#define END(name)               .size name,.-name

#define RCSID(x)

#define WEAK_ALIAS(alias, sym) \
        .weak alias; alias = sym
#define STRONG_ALIAS(alias, sym) \
        .globl alias; alias = sym

/* The 68030 has extb.l; NetBSD spells it out for the 68010. */
#define EXTBL(reg)      extbl reg

#endif /* _MACHINE_ASM_H_ */
