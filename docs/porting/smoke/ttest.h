/*
 * Helpers from ttest.c for the trap smoke tests.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <machine/frame.h>
#include "ktest.h"

extern int ustack[];                    /* user stack top is ustack + USTACK */
#define USTACK  256

/* A hardware exception frame, as a test builds it. */
struct hwframe {
    u_short sr;
    u_int   pc;
    u_short fmtvec;                     /* format << 12 | vector * 4 */
    union {
        struct fmt2 f2;
        struct fmtB fB;
    } u;
};

extern u_int fake_ssp;                  /* kernel sp before fake_trap() */

void ttest_init(void);
void enter_user(void (*pc)(void), int sr);              /* does not return */
void fake_trap(const struct hwframe *f, int size);      /* does not return */

/* Each test defines got_signal(); psignal() calls it. */
void got_signal(int sig);
