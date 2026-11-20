/*
 * sys/m68k/vd.c byte counts that are not whole sectors, as swap moves
 * (D38): a partial last sector is read without touching the memory past
 * the count, and written zero padded.  A zero count finishes at once.
 */
#include "ttest.h"
#include "vdstub.h"

static char data[2 * DEV_BSIZE], out[DEV_BSIZE];

void
got_signal(int sig)
{
    check("no signal", 0);
}

/* The first len bytes at p are the pattern line of a sector. */
static int
prefix(const char *p, const char *line, int len)
{
    int i;

    for (i = 0; i < len; i++)
        if (p[i] != line[i % 8])
            return 0;
    return 1;
}

static void
fill(void)
{
    int i;

    for (i = 0; i < sizeof(data); i++)
        data[i] = 0xa5;
}

/* data[from..to) all hold c. */
static int
all(int from, int to, int c)
{
    for (; from < to; from++)
        if ((u_char) data[from] != c)
            return 0;
    return 1;
}

int
main()
{
    int i;

    ttest_init();
    noproc = 1;
    probe_disks();
    check("open", vdopen(VD0, 0, 0) == 0);

    /* Block 900 is sectors 1800 and 1801. */
    fill();
    check("read 700", xfer(VD0, 900, data, 700, 1) == 0);
    check("read 700 data", sector_ok(data, 1800) &&
        prefix(data + 512, "0001801\n", 188));
    check("read 700 past count", all(700, sizeof(data), 0xa5));

    fill();
    check("read 100", xfer(VD0, 901, data, 100, 1) == 0);
    check("read 100 data", prefix(data, "0001802\n", 100));
    check("read 100 past count", all(100, sizeof(data), 0xa5));

    for (i = 0; i < DEV_BSIZE; i++)
        out[i] = i * 7 + 3;
    check("write 700", xfer(VD0, 950, out, 700, 0) == 0);
    fill();
    check("read back 700", xfer(VD0, 950, data, DEV_BSIZE, 1) == 0 &&
        bcmp(data, out, 700) == 0 && all(700, DEV_BSIZE, 0));
    check("write 100", xfer(VD0, 951, out, 100, 0) == 0);
    fill();
    check("read back 100", xfer(VD0, 951, data, DEV_BSIZE, 1) == 0 &&
        bcmp(data, out, 100) == 0 && all(100, 512, 0) &&
        sector_ok(data + 512, 1903));

    check("zero count", xfer(VD0, 960, data, 0, 1) == 0);
    report("vdbytes");
}
