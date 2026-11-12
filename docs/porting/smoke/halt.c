/*
 * machdep.c boot(RB_HALT): prints "halted" and stops QEMU through
 * virt-ctrl.  run.sh checks that QEMU exits by itself with status 0.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/reboot.h>
#include "ktest.h"

int main(void)
{
    boot(0, RB_HALT);
    kputs("boot returned\n");
    return 0;
}
