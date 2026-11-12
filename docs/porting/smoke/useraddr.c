/*
 * machdep.c baduaddr(), badkaddr(), copyin() and copyout(): only the
 * fixed user region USER_DATA_START..USER_DATA_END is user memory.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include "ktest.h"

static char kbuf[8] = "1234567";

int main(void)
{
    caddr_t ustart = (caddr_t) USER_DATA_START;
    caddr_t uend = (caddr_t) USER_DATA_END;
    char back[8];

    check("baduaddr start", baduaddr(ustart) == 0);
    check("baduaddr last", baduaddr(uend - 1) == 0);
    check("baduaddr end", baduaddr(uend) != 0);
    check("baduaddr kernel", baduaddr(kbuf) != 0);
    check("badkaddr kernel", badkaddr(kbuf) == 0);
    check("badkaddr user", badkaddr(ustart) != 0);

    check("copyout ok", copyout(kbuf, ustart, 8) == 0);
    check("copyin ok", copyin(ustart, back, 8) == 0 && bcmp(back, kbuf, 8) == 0);
    check("copyout to kernel", copyout(kbuf, back, 8) == EFAULT);
    check("copyout past end", copyout(kbuf, uend - 4, 8) == EFAULT);
    check("copyin from kernel", copyin(kbuf, back, 8) == EFAULT);

    report("useraddr");
    return 0;
}
