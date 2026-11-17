/*
 * machdep.c baduaddr(), badkaddr(), the range checks, copyin(), copyout()
 * and copyinstr(): only the fixed user region USER_DATA_START..
 * USER_DATA_END is user memory, and a length cannot wrap around past it.
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
    char back[8], path[16];
    u_int len;

    check("baduaddr start", baduaddr(ustart) == 0);
    check("baduaddr last", baduaddr(uend - 1) == 0);
    check("baduaddr end", baduaddr(uend) != 0);
    check("baduaddr kernel", baduaddr(kbuf) != 0);
    check("badkaddr kernel", badkaddr(kbuf) == 0);
    check("badkaddr user", badkaddr(ustart) != 0);

    check("baduarange to end", baduarange(uend - 8, 8) == 0);
    check("baduarange past end", baduarange(uend - 8, 9) != 0);
    check("baduarange wrap", baduarange(uend - 0x100, 0xfffffff0) != 0);
    check("baduarange zero", baduarange(ustart, 0) == 0);
    check("badkrange kernel", badkrange(kbuf, 8) == 0);
    check("badkrange into user", badkrange(ustart - 4, 8) != 0);
    check("badkrange wrap", badkrange(kbuf, 0xfffffff0) != 0);

    check("copyout ok", copyout(kbuf, ustart, 8) == 0);
    check("copyout zero", copyout(kbuf, ustart, 0) == 0);
    check("copyout wrap", copyout(kbuf, uend - 0x100, 0xfffffff0) == EFAULT);
    check("copyin ok", copyin(ustart, back, 8) == 0 && bcmp(back, kbuf, 8) == 0);
    check("copyout to kernel", copyout(kbuf, back, 8) == EFAULT);
    check("copyout past end", copyout(kbuf, uend - 4, 8) == EFAULT);
    check("copyin from kernel", copyin(kbuf, back, 8) == EFAULT);

    check("copyinstr ok", copyinstr(ustart, path, sizeof path, &len) == 0 &&
        len == 8 && bcmp(path, kbuf, 8) == 0);
    check("copyinstr too long", copyinstr(ustart, path, 4, 0) == ENOENT);
    check("copyinstr from kernel", copyinstr(kbuf, path, sizeof path, 0) == EFAULT);
    bcopy("abc", uend - 3, 3);              /* no null byte before the end */
    check("copyinstr past end", copyinstr(uend - 3, path, sizeof path, 0) == EFAULT);
    bcopy("ab", uend - 3, 3);
    check("copyinstr at end", copyinstr(uend - 3, path, sizeof path, &len) == 0 && len == 3);

    report("useraddr");
    return 0;
}
