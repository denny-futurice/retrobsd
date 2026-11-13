/*
 * fpfault_clear() (sys/m68k/trap.c) on hand-built 68881/2 state frames:
 * it sets bit 27 of the BIU flags, the last long word of the frame, and
 * leaves a null frame alone.  QEMU's 68030 has no fsave (D25).
 */
#include "ttest.h"

void fpfault_clear(u_char *state);

void
got_signal(int sig)
{
}

/* A frame of version v and size n is unchanged but for byte n bit 3. */
static int
cleared(int v, int n)
{
    static u_char f[256];
    int i;

    bzero(f, sizeof(f));
    f[0] = v;
    f[1] = n;
    fpfault_clear(f);
    for (i = 0; i < (int) sizeof(f); i++)
        if (f[i] != (i == 0 ? v : i == 1 ? n : i == n ? 0x08 : 0))
            return 0;
    return 1;
}

int
main()
{
    static u_char null[4];

    fpfault_clear(null);
    check("null frame", null[0] == 0 && null[1] == 0 && null[2] == 0 && null[3] == 0);
    check("68881 idle", cleared(0x1f, 0x18));
    check("68882 idle", cleared(0x21, 0x38));
    check("68882 busy", cleared(0x21, 0xd4));
    report("fpbiu");
    return 0;
}
