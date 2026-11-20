/*
 * The string routines from NetBSD (D42): results against byte-at-a-time
 * references for every alignment and length up to 70, overlapping moves
 * both ways, the bytes around each destination untouched, and pointer
 * results in a0 (where the compiler reads them) as well as d0 (read here
 * through a cast to a function returning long).
 */
#include "libtest.h"
#include <strings.h>

#define N 80

static unsigned char src[N + 8], dst[N + 8], ref[N + 8];

static void
fill(unsigned char *p, int seed)
{
    int i;

    for (i = 0; i < N + 8; i++)
        p[i] = (unsigned char) (i * 7 + seed);
}

static void
refmove(unsigned char *d, const unsigned char *s, int n)
{
    unsigned char t[N + 8];
    int i;

    for (i = 0; i < n; i++)
        t[i] = s[i];
    for (i = 0; i < n; i++)
        d[i] = t[i];
}

static void
copies(void)
{
    int sa, da, n;

    for (sa = 0; sa < 4; sa++)
        for (da = 0; da < 4; da++)
            for (n = 0; n <= 70; n++) {
                fill(src, 1);
                fill(dst, 2); fill(ref, 2);
                refmove(ref + da, src + sa, n);
                check(memcpy(dst + da, src + sa, n) == dst + da);
                check(memcmp(dst, ref, sizeof dst) == 0);
                fill(dst, 2);
                bcopy(src + sa, dst + da, n);
                check(memcmp(dst, ref, sizeof dst) == 0);
                fill(dst, 2);
                check(memmove(dst + da, src + sa, n) == dst + da);
                check(memcmp(dst, ref, sizeof dst) == 0);
            }
}

static void
overlaps(void)
{
    int off, n;

    for (off = 1; off < 6; off++)
        for (n = 0; n <= 70; n++) {
            fill(dst, 3); fill(ref, 3);
            refmove(ref + off, ref, n);
            check(memmove(dst + off, dst, n) == dst + off);
            check(memcmp(dst, ref, sizeof dst) == 0);
            fill(dst, 3); fill(ref, 3);
            refmove(ref, ref + off, n);
            bcopy(dst + off, dst, n);
            check(memcmp(dst, ref, sizeof dst) == 0);
        }
}

static void
fills(void)
{
    int a, n, i;

    for (a = 0; a < 4; a++)
        for (n = 0; n <= 70; n++) {
            fill(dst, 4); fill(ref, 4);
            for (i = 0; i < n; i++)
                ref[a + i] = 0xa5;
            check(memset(dst + a, 0x1a5, n) == dst + a);
            check(memcmp(dst, ref, sizeof dst) == 0);
            fill(dst, 4);
            for (i = 0; i < n; i++)
                ref[a + i] = 0;
            bzero(dst + a, n);
            check(memcmp(dst, ref, sizeof dst) == 0);
        }
}

static void
compares(void)
{
    int a, n;

    for (a = 0; a < 4; a++)
        for (n = 1; n <= 70; n++) {
            fill(dst, 5); fill(ref, 5);
            check(bcmp(dst + a, ref + a, n) == 0);
            ref[a + n - 1]++;
            check(bcmp(dst + a, ref + a, n) != 0);
            check(bcmp(dst + a, ref + a, n - 1) == 0);
        }
    check(strcmp("abc", "abc") == 0);
    check(strcmp("abc", "abd") < 0);
    check(strcmp("abd", "abc") > 0);
    check(strcmp("ab", "abc") < 0);
    check(strcmp("abc", "ab") > 0);
    check(strcmp("", "") == 0);
    check(strcmp("\x80", "a") > 0);
}

static void
searches(void)
{
    static const char s[] = "hello, world";
    static const char long37[] = "0123456789012345678901234567890123456";

    check(strlen("") == 0);
    check(strlen(long37) == 37);
    check(strlen(long37 + 3) == 34);
    check(strchr(s, 'o') == s + 4);
    check(index(s, 'o') == s + 4);
    check(strrchr(s, 'o') == s + 8);
    check(rindex(s, 'o') == s + 8);
    check(strchr(s, 'x') == 0);
    check(strrchr(s, 'x') == 0);
    check(strchr(s, '\0') == s + 12);
    check(strrchr(s, '\0') == s + 12);
    check(strchr(s, 'h') == s);
    check(ffs(0) == 0);
    check(ffs(1) == 1);
    check(ffs(0x18) == 4);
    check(ffs(0x80000000) == 32);
}

/* The same calls with the result read from d0. */
static void
d0results(void)
{
    static const char s[] = "abcabc";
    long (*cp)(void *, const void *, size_t) = (void *) memcpy;
    long (*mv)(void *, const void *, size_t) = (void *) memmove;
    long (*st)(void *, int, size_t) = (void *) memset;
    long (*ch)(const char *, int) = (void *) strchr;
    long (*rch)(const char *, int) = (void *) strrchr;

    check(cp(dst + 1, src, 9) == (long) (dst + 1));
    check(mv(dst + 1, dst, 9) == (long) (dst + 1));
    check(mv(dst, dst + 1, 9) == (long) dst);
    check(st(dst + 3, 0, 40) == (long) (dst + 3));
    check(ch(s, 'b') == (long) (s + 1));
    check(ch(s, 'x') == 0);
    check(rch(s, 'b') == (long) (s + 4));
    check(rch(s, 'x') == 0);
}

int
main(void)
{
    console();
    copies();
    overlaps();
    fills();
    compares();
    searches();
    d0results();
    finish("libstring");
    return 0;
}
