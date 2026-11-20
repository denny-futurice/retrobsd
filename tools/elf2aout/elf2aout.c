/*
 * This program converts an elf executable to a BSD a.out executable.
 * The symbol table, the debugging symbols and other informational
 * sections are not copied.
 *
 * RetroBSD/m68k: the input is a big-endian 32-bit m68k ELF executable,
 * and the a.out header is written big-endian, whatever the host byte
 * order (D37).
 *
 * Copyright (c) 1995 Ted Lemon (hereinafter referred to as the author)
 * Copyright (c) 2011 Serge Vakulenko
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/types.h>

#ifndef _WIN32
#include <err.h>
#endif
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>

#ifdef _WIN32
#include <stdarg.h>

typedef unsigned char u_char;
typedef unsigned int u_int;


void verr(int eval, const char *fmt, va_list ap) {
    char* __progname = "elf2aout";
    int sverrno;

    sverrno = errno;
    (void)fprintf(stderr, "%s: ", __progname);
    if (fmt != NULL) {
        (void)vfprintf(stderr, fmt, ap);
        (void)fprintf(stderr, ": ");
    }
    (void)fprintf(stderr, "%s\n", strerror(sverrno));
    exit(eval);
}

void err(int eval, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verr(eval, fmt, ap);
    va_end(ap);
}

void verrx(int eval, const char *fmt, va_list ap) {
    char* __progname = "elf2aout";
    (void)fprintf(stderr, "%s: ", __progname);
    if (fmt != NULL)
        (void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr, "\n");
    exit(eval);
}

void errx(int eval, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    verrx(eval, fmt, ap);
    va_end(ap);
}

void vwarn(const char *fmt, va_list ap) {
    int sverrno = errno;
    char* __progname = "elf2aout";
    (void)fprintf(stderr, "%s: ", __progname);
    if (fmt != NULL) {
        (void)vfprintf(stderr, fmt, ap);
        (void)fprintf(stderr, ": ");
    }
    (void)fprintf(stderr, "%s\n", strerror(sverrno));
}

void warn(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vwarn(fmt, ap);
    va_end(ap);
}

void vwarnx(const char *fmt, va_list ap) {
    char* __progname = "elf2aout";
    (void)fprintf(stderr, "%s: ", __progname);
    if (fmt != NULL)
        (void)vfprintf(stderr, fmt, ap);
    (void)fprintf(stderr, "\n");
}

void warnx(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vwarnx(fmt, ap);
    va_end(ap);
}


#endif

/*
 * Header prepended to each a.out file.
 */
struct	exec {
	unsigned a_magic;	/* magic number */
#define OMAGIC      0407        /* old impure format */

        unsigned a_text;	/* size of text segment */
        unsigned a_data;	/* size of initialized data */
        unsigned a_bss;		/* size of uninitialized data */
        unsigned a_reltext;	/* size of text relocation info */
        unsigned a_reldata;	/* size of data relocation info */
        unsigned a_syms;	/* size of symbol table */
        unsigned a_entry; 	/* entry point */
};

struct sect {
	/* should be unsigned long, but assume no a.out binaries on LP64 */
	uint32_t vaddr;
	uint32_t len;
};

/*
 * Standard ELF types, structures, and macros.
 */

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct
{
  unsigned char	e_ident[EI_NIDENT];	/* Magic number and other info */
  uint16_t	e_type;			/* Object file type */
  uint16_t	e_machine;		/* Architecture */
  uint32_t	e_version;		/* Object file version */
  uint32_t	e_entry;		/* Entry point virtual address */
  uint32_t	e_phoff;		/* Program header table file offset */
  uint32_t	e_shoff;		/* Section header table file offset */
  uint32_t	e_flags;		/* Processor-specific flags */
  uint16_t	e_ehsize;		/* ELF header size in bytes */
  uint16_t	e_phentsize;		/* Program header table entry size */
  uint16_t	e_phnum;		/* Program header table entry count */
  uint16_t	e_shentsize;		/* Section header table entry size */
  uint16_t	e_shnum;		/* Section header table entry count */
  uint16_t	e_shstrndx;		/* Section header string table index */
} Elf32_Ehdr;

/* Program segment header.  */

typedef struct
{
  uint32_t	p_type;			/* Segment type */
  uint32_t	p_offset;		/* Segment file offset */
  uint32_t	p_vaddr;		/* Segment virtual address */
  uint32_t	p_paddr;		/* Segment physical address */
  uint32_t	p_filesz;		/* Segment size in file */
  uint32_t	p_memsz;		/* Segment size in memory */
  uint32_t	p_flags;		/* Segment flags */
  uint32_t	p_align;		/* Segment alignment */
} Elf32_Phdr;

/* Legal values for p_type (segment type).  */

#define	PT_NULL		0		/* Program header table entry unused */
#define PT_LOAD		1		/* Loadable program segment */
#define PT_DYNAMIC	2		/* Dynamic linking information */
#define PT_INTERP	3		/* Program interpreter */
#define PT_NOTE		4		/* Auxiliary information */
#define PT_SHLIB	5		/* Reserved */
#define PT_PHDR		6		/* Entry for header table itself */
#define PT_TLS		7		/* Thread-local storage segment */
#define	PT_NUM		8		/* Number of defined types */
#define PT_LOOS		0x60000000	/* Start of OS-specific */
#define PT_GNU_EH_FRAME	0x6474e550	/* GCC .eh_frame_hdr segment */
#define PT_GNU_STACK	0x6474e551	/* Indicates stack executability */
#define PT_GNU_RELRO	0x6474e552	/* Read-only after relocation */
#define PT_LOSUNW	0x6ffffffa
#define PT_SUNWBSS	0x6ffffffa	/* Sun Specific segment */
#define PT_SUNWSTACK	0x6ffffffb	/* Stack segment */
#define PT_HISUNW	0x6fffffff
#define PT_HIOS		0x6fffffff	/* End of OS-specific */
#define PT_LOPROC	0x70000000	/* Start of processor-specific */
#define PT_HIPROC	0x7fffffff	/* End of processor-specific */

/* Legal values for p_type field of Elf32_Phdr.  */

#define PT_MIPS_REGINFO	0x70000000	/* Register usage information */
#define PT_MIPS_RTPROC  0x70000001	/* Runtime procedure table. */
#define PT_MIPS_OPTIONS 0x70000002

/* Legal values for p_flags (segment flags).  */

#define PF_X		(1 << 0)	/* Segment is executable */
#define PF_W		(1 << 1)	/* Segment is writable */
#define PF_R		(1 << 2)	/* Segment is readable */
#define PF_MASKOS	0x0ff00000	/* OS-specific */
#define PF_MASKPROC	0xf0000000	/* Processor-specific */

/* e_ident indices and values, e_type and e_machine.  */

#define EI_CLASS	4		/* File class byte index */
#define ELFCLASS32	1		/* 32-bit objects */
#define EI_DATA		5		/* Data encoding byte index */
#define ELFDATA2MSB	2		/* 2's complement, big endian */
#define ET_EXEC		2		/* Executable file */
#define EM_68K		4		/* Motorola m68k family */

void	combine (struct sect *, struct sect *, int);
int	phcmp (const void *, const void *);
char   *save_read (int file, off_t offset, off_t len, const char *name);
void	copy (int, int, off_t, off_t);

/*
 * The ELF input and the a.out output are big-endian.
 */
static uint16_t
get16(const void *p)
{
	const unsigned char *b = p;

	return (uint16_t)(b[0] << 8 | b[1]);
}

static uint32_t
get32(const void *p)
{
	const unsigned char *b = p;

	return (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 |
	    (uint32_t)b[2] << 8 | b[3];
}

static void
put32(unsigned char *b, uint32_t v)
{
	b[0] = v >> 24;
	b[1] = v >> 16;
	b[2] = v >> 8;
	b[3] = v;
}

#define FIX16(f)	((f) = get16(&(f)))
#define FIX32(f)	((f) = get32(&(f)))

int
main(int argc, char **argv)
{
	Elf32_Ehdr ex;
	Elf32_Phdr *ph;
	int     i;
	struct sect text, data, bss;
	struct exec aex;
	unsigned char ahdr[sizeof aex];
	int     infile, outfile;
	uint32_t cur_vma = UINT32_MAX;
	int     verbose = 0;

	text.len = data.len = bss.len = 0;
	text.vaddr = data.vaddr = bss.vaddr = 0;

	/* Check args... */
	for (;;) {
		switch (getopt (argc, argv, "v")) {
		case EOF:
			break;
		case 'v':
			++verbose;
			continue;
		default:
usage:                  fprintf(stderr,
                            "usage: elf2aout [-v] <elf executable> <a.out executable>\n");
                        exit(1);
		}
		break;
	}
	argc -= optind;
	argv += optind;
	if (argc != 2)
		goto usage;

	/* Try the input file... */
	if ((infile = open(argv[0], O_RDONLY)) < 0) {
		fprintf(stderr, "Can't open %s for read: %s\n",
		    argv[0], strerror(errno));
		exit(1);
	}
	/* Read the header, which is at the beginning of the file... */
	i = read(infile, &ex, sizeof ex);
	if (i != sizeof ex) {
		fprintf(stderr, "ex: %s: %s.\n",
		    argv[0], i ? strerror(errno) : "End of file reached");
		exit(1);
	}
	if (memcmp(ex.e_ident, "\177ELF", 4) != 0 ||
	    ex.e_ident[EI_CLASS] != ELFCLASS32 ||
	    ex.e_ident[EI_DATA] != ELFDATA2MSB)
		errx(1, "%s: not a big-endian 32-bit ELF file", argv[0]);
	FIX16(ex.e_type);
	FIX16(ex.e_machine);
	FIX32(ex.e_version);
	FIX32(ex.e_entry);
	FIX32(ex.e_phoff);
	FIX32(ex.e_shoff);
	FIX32(ex.e_flags);
	FIX16(ex.e_ehsize);
	FIX16(ex.e_phentsize);
	FIX16(ex.e_phnum);
	FIX16(ex.e_shentsize);
	FIX16(ex.e_shnum);
	FIX16(ex.e_shstrndx);
	if (ex.e_machine != EM_68K)
		errx(1, "%s: not an m68k ELF file (machine %u)", argv[0],
		    ex.e_machine);
	if (ex.e_type != ET_EXEC)
		errx(1, "%s: not an ELF executable (type %u)", argv[0],
		    ex.e_type);
	if (ex.e_phnum == 0 || ex.e_phentsize != sizeof(Elf32_Phdr))
		errx(1, "%s: bad program headers", argv[0]);

	/* Read the program headers... */
	ph = (Elf32_Phdr *) save_read(infile, ex.e_phoff,
	    ex.e_phnum * sizeof(Elf32_Phdr), "ph");
	for (i = 0; i < ex.e_phnum; i++) {
		FIX32(ph[i].p_type);
		FIX32(ph[i].p_offset);
		FIX32(ph[i].p_vaddr);
		FIX32(ph[i].p_paddr);
		FIX32(ph[i].p_filesz);
		FIX32(ph[i].p_memsz);
		FIX32(ph[i].p_flags);
		FIX32(ph[i].p_align);
	}

	/* Figure out if we can cram the program header into an a.out
	 * header... Basically, we can't handle anything but loadable
	 * segments, but we can ignore some kinds of segments.   We can't
	 * handle holes in the address space, and we handle start addresses
	 * other than 0x1000 by hoping that the loader will know where to load
	 * - a.out doesn't have an explicit load address.   Segments may be
	 * out of order, so we sort them first. */
	qsort(ph, ex.e_phnum, sizeof(Elf32_Phdr), phcmp);
	for (i = 0; i < ex.e_phnum; i++) {
		/* Section types we can ignore... */
		if (ph[i].p_type == PT_NULL || ph[i].p_type == PT_NOTE ||
		    ph[i].p_type == PT_PHDR || ph[i].p_type == PT_MIPS_REGINFO ||
                    ph[i].p_type == PT_GNU_EH_FRAME)
			continue;

                /* Ignore zero sized segments. */
                if (ph[i].p_memsz == 0)
                        continue;

                if (verbose)
                        printf ("Section type=%x flags=%x vaddr=%x filesz=%x\n",
                            ph[i].p_type, ph[i].p_flags, ph[i].p_vaddr, ph[i].p_filesz);

		/* Section types we can't handle... */
		if (ph[i].p_type != PT_LOAD && ph[i].p_type != PT_GNU_EH_FRAME)
			errx(1, "Program header %d type %x can't be converted.", i, ph[i].p_type);

		/* Writable (data) segment? */
		if (ph[i].p_flags & PF_W) {
			struct sect ndata, nbss;

			ndata.vaddr = ph[i].p_vaddr;
			ndata.len = ph[i].p_filesz;
			nbss.vaddr = ph[i].p_vaddr + ph[i].p_filesz;
			nbss.len = ph[i].p_memsz - ph[i].p_filesz;

			combine(&data, &ndata, 0);
			combine(&bss, &nbss, 1);
		} else {
			struct sect ntxt;

			ntxt.vaddr = ph[i].p_vaddr;
			ntxt.len = ph[i].p_filesz;

			combine(&text, &ntxt, 0);
		}
		/* Remember the lowest segment start address. */
		if (ph[i].p_vaddr < cur_vma)
			cur_vma = ph[i].p_vaddr;
	}
	/* Sections must be in order to be converted... */
	if (text.vaddr > data.vaddr || data.vaddr > bss.vaddr ||
	    text.vaddr + text.len > data.vaddr || data.vaddr + data.len > bss.vaddr) {
		fprintf(stderr, "Sections ordering prevents a.out conversion.\n");
		exit(1);
	}

	/* If there's a data section but no text section, then the loader
	 * combined everything into one section.   That needs to be the text
	 * section, so just make the data section zero length following text. */
	if (data.len && text.len == 0) {
		text = data;
		data.vaddr = text.vaddr + text.len;
		data.len = 0;
	}
        if (verbose)
                printf ("Text vaddr = %x, data vaddr = %x\n", text.vaddr, data.vaddr);

	/* If there is a gap between text and data, we'll fill it when we copy
	 * the data, so update the length of the text segment as represented
	 * in a.out to reflect that, since a.out doesn't allow gaps in the
	 * program address space. */
	if (text.vaddr + text.len < data.vaddr) {
                if (verbose)
                        printf ("Update text len = %x (was %x)\n",
                                data.vaddr - text.vaddr, text.len);
		text.len = data.vaddr - text.vaddr;
        }

	/* We now have enough information to cons up an a.out header... */
	aex.a_magic = OMAGIC;
	aex.a_text = text.len;
	aex.a_data = data.len;
	aex.a_bss = bss.len;
	aex.a_entry = ex.e_entry;
	aex.a_syms = 0;
        aex.a_reltext = 0;
        aex.a_reldata = 0;
	if (verbose) {
                printf ("  magic: %#o\n", aex.a_magic);
                printf ("   text: %#x\n", aex.a_text);
                printf ("   data: %#x\n", aex.a_data);
                printf ("    bss: %#x\n", aex.a_bss);
                printf ("reltext: %#x\n", aex.a_reltext);
                printf ("reldata: %#x\n", aex.a_reldata);
                printf ("  entry: %#x\n", aex.a_entry);
                printf ("   syms: %u\n", aex.a_syms);
        }

	/* Make the output file... */
	if ((outfile = open(argv[1], O_WRONLY | O_CREAT, 0777)) < 0) {
		fprintf(stderr, "Unable to create %s: %s\n", argv[1], strerror(errno));
		exit(1);
	}
	/* Truncate file... */
	if (ftruncate(outfile, 0)) {
		warn("ftruncate %s", argv[1]);
	}
	/* Write the header... */
	put32(ahdr + 0, aex.a_magic);
	put32(ahdr + 4, aex.a_text);
	put32(ahdr + 8, aex.a_data);
	put32(ahdr + 12, aex.a_bss);
	put32(ahdr + 16, aex.a_reltext);
	put32(ahdr + 20, aex.a_reldata);
	put32(ahdr + 24, aex.a_syms);
	put32(ahdr + 28, aex.a_entry);
	i = write(outfile, ahdr, sizeof ahdr);
	if (i != sizeof ahdr) {
		perror("aex: write");
		exit(1);
	}
        /* Copy the loadable sections.   Zero-fill any gaps less than 64k;
         * complain about any zero-filling, and die if we're asked to
         * zero-fill more than 64k. */
        for (i = 0; i < ex.e_phnum; i++) {
                /* Unprocessable sections were handled above, so just verify
                 * that the section can be loaded before copying. */
                if (ph[i].p_type == PT_LOAD && ph[i].p_filesz) {
                        if (cur_vma != ph[i].p_vaddr) {
                                uint32_t gap = ph[i].p_vaddr - cur_vma;
                                char    obuf[1024];
                                if (gap > 65536)
                                        errx(1, "Intersegment gap (%ld bytes) too large.", (long) gap);
#ifdef DEBUG
                                warnx("Warning: %ld byte intersegment gap.",
                                    (long)gap);
#endif
                                memset(obuf, 0, sizeof obuf);
                                while (gap) {
                                        int     count = write(outfile, obuf, (gap > sizeof obuf
                                                ? sizeof obuf : gap));
                                        if (count < 0) {
                                                fprintf(stderr, "Error writing gap: %s\n",
                                                    strerror(errno));
                                                exit(1);
                                        }
                                        gap -= count;
                                }
                        }
                        copy(outfile, infile, ph[i].p_offset, ph[i].p_filesz);
                        cur_vma = ph[i].p_vaddr + ph[i].p_filesz;
                }
        }
	/* Looks like we won... */
	return 0;
}

void
copy(int out, int in, off_t offset, off_t size)
{
	char    ibuf[4096];
	int     remaining, cur, count;

	/* Go to the start of the ELF symbol table... */
	if (lseek(in, offset, SEEK_SET) < 0) {
		perror("copy: lseek");
		exit(1);
	}
	remaining = size;
	while (remaining) {
		cur = remaining;
		if (cur > (int)sizeof ibuf)
			cur = sizeof ibuf;
		remaining -= cur;
		if ((count = read(in, ibuf, cur)) != cur) {
			fprintf(stderr, "copy: read: %s\n",
			    count ? strerror(errno) : "premature end of file");
			exit(1);
		}
		if ((count = write(out, ibuf, cur)) != cur) {
			perror("copy: write");
			exit(1);
		}
	}
}

/*
 * Combine two segments, which must be contiguous.
 * If pad is true, it's okay for there to be padding between.
 */
void
combine(struct sect *base, struct sect *new, int pad)
{

	if (base->len == 0)
		*base = *new;
	else
		if (new->len) {
			if (base->vaddr + base->len != new->vaddr) {
				if (pad)
					base->len = new->vaddr - base->vaddr;
				else {
					fprintf(stderr,
					    "Non-contiguous data can't be converted.\n");
					exit(1);
				}
			}
			base->len += new->len;
		}
}

int
phcmp(const void *vh1, const void *vh2)
{
	const Elf32_Phdr *h1, *h2;

	h1 = (const Elf32_Phdr *)vh1;
	h2 = (const Elf32_Phdr *)vh2;

	if (h1->p_vaddr > h2->p_vaddr)
		return 1;
	else
		if (h1->p_vaddr < h2->p_vaddr)
			return -1;
		else
			return 0;
}

char *
save_read(int file, off_t offset, off_t len, const char *name)
{
	char   *tmp;
	int     count;
	off_t   off;
	if ((off = lseek(file, offset, SEEK_SET)) < 0) {
		fprintf(stderr, "%s: fseek: %s\n", name, strerror(errno));
		exit(1);
	}
	if ((tmp = malloc(len)) == NULL)
		errx(1, "%s: Can't allocate %ld bytes.", name, (long)len);
	count = read(file, tmp, len);
	if (count != len) {
		fprintf(stderr, "%s: read: %s.\n",
		    name, count ? strerror(errno) : "End of file reached");
		exit(1);
	}
	return tmp;
}
