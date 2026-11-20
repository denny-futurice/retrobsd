/*
 * Independent reader for the fsutil test, built with the host compiler:
 * prints a file from partition 1 of a RetroBSD disk image, decoding the
 * filesystem as big-endian (D37) and the MBR as little-endian.  Shares no
 * code with tools/fsutil.  Prints the highest block it used to stderr.
 *
 *   bsdread disk.img /path
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BSIZE 1024

static FILE *img;
static long base;
static unsigned long maxblk;

static void fail(const char *msg, const char *arg)
{
    fprintf(stderr, "%s%s\n", msg, arg ? arg : "");
    exit(1);
}

static unsigned long be32(const unsigned char *p)
{
    return (unsigned long)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static unsigned be16(const unsigned char *p)
{
    return p[0] << 8 | p[1];
}

static void blk(unsigned long n, unsigned char *buf)
{
    if (fseek(img, base + (long)n * BSIZE, SEEK_SET) != 0 ||
        fread(buf, 1, BSIZE, img) != BSIZE)
        fail("short read", NULL);
}

/* Inode n: size and the 7 block addresses (4 direct, 3 indirect). */
static unsigned long inode(unsigned long n, unsigned long addr[7])
{
    unsigned char buf[BSIZE];
    unsigned long off = (n + 15) * 64;  /* inode 1 starts block 1, 16 per block */
    const unsigned char *d;
    int i;

    blk(off / BSIZE, buf);
    d = buf + off % BSIZE;
    for (i = 0; i < 7; i++)
        addr[i] = be32(d + 16 + 4 * i);
    return be32(d + 12);
}

static unsigned long bmap(const unsigned long addr[7], unsigned long lbn)
{
    unsigned char buf[BSIZE];
    unsigned long span = 1, nb;
    int lev, sh;

    if (lbn < 4)
        return addr[lbn];
    lbn -= 4;
    for (lev = 1; lev <= 3; lev++) {
        span *= 256;
        if (lbn < span) {
            nb = addr[3 + lev];
            for (sh = lev - 1; sh >= 0; sh--) {
                blk(nb, buf);
                nb = be32(buf + 4 * ((lbn >> (8 * sh)) & 255));
            }
            return nb;
        }
        lbn -= span;
    }
    fail("file too large", NULL);
    return 0;
}

/* Read a whole file into a malloc'd buffer. */
static unsigned char *readfile(unsigned long ino, unsigned long *sizep)
{
    unsigned long addr[7], size = inode(ino, addr), i, nb;
    unsigned char *data = malloc(size + BSIZE), buf[BSIZE];

    if (!data)
        fail("out of memory", NULL);
    for (i = 0; i * BSIZE < size; i++) {
        nb = bmap(addr, i);
        if (nb > maxblk)
            maxblk = nb;
        blk(nb, buf);
        memcpy(data + i * BSIZE, buf, BSIZE);
    }
    *sizep = size;
    return data;
}

static unsigned long lookup(unsigned long dir, const char *name, size_t len)
{
    unsigned long size, o = 0, ino;
    unsigned char *d = readfile(dir, &size);
    unsigned reclen, namlen;

    while (o + 8 <= size) {
        ino = be32(d + o);
        reclen = be16(d + o + 4);
        namlen = be16(d + o + 6);
        if (ino && namlen == len && memcmp(d + o + 8, name, len) == 0) {
            free(d);
            return ino;
        }
        if (reclen == 0)
            fail("zero reclen", NULL);
        o += reclen;
    }
    fail("not found: ", name);
    return 0;
}

int main(int argc, char **argv)
{
    unsigned char buf[BSIZE], *data;
    unsigned long ino = 2, size;
    const char *p, *e;

    if (argc != 3)
        fail("usage: bsdread disk.img /path", NULL);
    img = fopen(argv[1], "rb");
    if (!img)
        fail("cannot open ", argv[1]);
    if (fread(buf, 1, 512, img) != 512 || buf[510] != 0x55 || buf[511] != 0xaa)
        fail("no MBR signature", NULL);
    p = (const char *)buf + 446 + 8;
    base = (long)((unsigned long)(unsigned char)p[0] | (unsigned char)p[1] << 8 |
                  (unsigned char)p[2] << 16 | (unsigned long)(unsigned char)p[3] << 24) * 512;

    blk(0, buf);
    if (memcmp(buf, "<<SF", 4) != 0)
        fail("bad magic1", NULL);
    if (memcmp(buf + 1020, "SF>>", 4) != 0)
        fail("bad magic2", NULL);

    for (p = argv[2]; *p; p = e) {
        while (*p == '/')
            p++;
        if (!*p)
            break;
        for (e = p; *e && *e != '/'; e++)
            continue;
        ino = lookup(ino, p, e - p);
    }
    data = readfile(ino, &size);
    fwrite(data, 1, size, stdout);
    fprintf(stderr, "maxblk %lu\n", maxblk);
    return 0;
}
