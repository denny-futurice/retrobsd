/*      $NetBSD: elf_machdep.h,v 1.7 2000/04/02 15:35:50 minoura Exp $  */

#define ELF_MACHDEP_ID_CASES                                            \
                case EM_68K:                                            \
                        break;

#define ARCH_ELFSIZE            32      /* MD native binary size */

/* m68k relocs.  */

#define R_68K_NONE              0       /* no reloc */
#define R_68K_32                1       /* direct 32 bit */
#define R_68K_16                2       /* direct 16 bit */
#define R_68K_8                 3       /* direct 8 bit */
#define R_68K_PC32              4       /* PC relative 32 bit */
#define R_68K_PC16              5       /* PC relative 16 bit */
#define R_68K_PC8               6       /* PC relative 8 bit */
#define R_68K_GOT32             7       /* 32 bit PC relative GOT entry */
#define R_68K_GOT16             8       /* 16 bit PC relative GOT entry */
#define R_68K_GOT8              9       /* 8 bit PC relative GOT entry */
#define R_68K_GOT32O            10      /* 32 bit GOT offset */
#define R_68K_GOT16O            11      /* 16 bit GOT offset */
#define R_68K_GOT8O             12      /* 8 bit GOT offset */
#define R_68K_PLT32             13      /* 32 bit PC relative PLT address */
#define R_68K_PLT16             14      /* 16 bit PC relative PLT address */
#define R_68K_PLT8              15      /* 8 bit PC relative PLT address */
#define R_68K_PLT32O            16      /* 32 bit PLT offset */
#define R_68K_PLT16O            17      /* 16 bit PLT offset */
#define R_68K_PLT8O             18      /* 8 bit PLT offset */
#define R_68K_COPY              19      /* copy symbol at runtime */
#define R_68K_GLOB_DAT          20      /* create GOT entry */
#define R_68K_JMP_SLOT          21      /* create PLT entry */
#define R_68K_RELATIVE          22      /* adjust by program base */

#define R_68K_max               23
#define R_TYPE(name)            __CONCAT(R_68K_,name)
