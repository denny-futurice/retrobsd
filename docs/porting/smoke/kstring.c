/*
 * machdep.c library routines: ffs (bfffo), string and memory functions.
 * The memory functions take a word-at-a-time path for aligned buffers of
 * 16 bytes or more, so both aligned and unaligned cases are checked.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include "ktest.h"

/* machdep.c defines memcpy for GCC; no kernel header declares it. */
void *memcpy(void *dst, const void *src, size_t nbytes);

static unsigned a[10], b[10];

int main(void)
{
    char *ca = (char *) a, *cb = (char *) b;
    char str[8];
    u_int len;
    int i;

    check("ffs 0", ffs(0) == 0);
    check("ffs lsb", ffs(1) == 1);
    check("ffs msb", ffs(0x80000000) == 32);
    check("ffs mixed", ffs(0x50) == 5);

    for (i = 0; i < 40; i++)
        ca[i] = i + 1;
    bzero(cb + 1, 38);
    check("bzero", cb[1] == 0 && cb[38] == 0);

    bcopy(ca + 1, cb + 3, 33);
    check("bcopy unaligned", cb[2] == 0 && cb[3] == 2 && cb[35] == 34 && cb[36] == 0);
    bcopy(ca, cb, 40);
    check("bcopy aligned", a[0] == b[0] && a[9] == b[9]);
    check("memcpy", memcpy(cb, ca, 4) == cb);

    check("bcmp equal", bcmp(ca, cb, 40) == 0);
    cb[37] = 0;
    check("bcmp order", bcmp(ca, cb, 40) > 0 && bcmp(cb, ca, 40) < 0);

    check("strlen", strlen("hello") == 5 && strlen("") == 0);
    check("strncmp", strncmp("abc", "abd", 2) == 0 && strncmp("abc", "abd", 3) < 0);
    check("copystr", copystr("abc", str, sizeof str, &len) == 0 && len == 4 &&
        strncmp(str, "abc", 4) == 0);
    check("copystr overflow", copystr("abcdef", str, 3, &len) == ENOENT && len == 3);

    report("kstring");
    return 0;
}
