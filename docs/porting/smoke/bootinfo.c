/*
 * machdep.c startup(): physmem comes from the QEMU bootinfo memory chunk.
 * run.sh starts QEMU with -m 16M.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include "ktest.h"

int main(void)
{
    startup();
    check("physmem is 16 MB", physmem == 16 * 1024 * 1024);
    report("bootinfo");
    return 0;
}
