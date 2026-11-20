/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *  @(#)wait.h  7.2.1 (2.11BSD GTE) 1995/06/23
 */

/*
 * This file holds definitions relevent to the wait system call.
 * Some of the options here are available only through the ``wait3''
 * entry point; the old entry point with one argument has more fixed
 * semantics, never returning status of unstopped children, hanging until
 * a process terminates if any are outstanding, and never returns
 * detailed information about process resource utilization (<vtimes.h>).
 */

/*
 * Structure of the information in the first word returned by both
 * wait and wait3.  If w_stopval==WSTOPPED, then the second structure
 * describes the information returned, else the first.  See WUNTRACED below.
 */
union wait  {
    int     w_status;           /* used in syscall */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    /*
     * Big-endian GCC allocates bit-fields from the most significant bit,
     * so the fields come in the reverse order, after 16 bits of padding,
     * to sit at the same bits of w_status as on little-endian (D37).
     */
    struct {
        unsigned :16;
        unsigned w_Retcode :8;  /* exit code if w_termsig==0 */
        unsigned w_Coredump :1; /* core dump indicator */
        unsigned w_Termsig :7;  /* termination signal */
    } w_T;
    struct {
        unsigned :16;
        unsigned w_Stopsig :8;  /* signal that stopped us */
        unsigned w_Stopval :8;  /* == W_STOPPED if stopped */
    } w_S;
#else
    /*
     * Terminated process status.
     */
    struct {
        unsigned w_Termsig :7;  /* termination signal */
        unsigned w_Coredump :1; /* core dump indicator */
        unsigned w_Retcode :8;  /* exit code if w_termsig==0 */
    } w_T;
    /*
     * Stopped process status.  Returned
     * only for traced children unless requested
     * with the WUNTRACED option bit.
     */
    struct {
        unsigned w_Stopval :8;  /* == W_STOPPED if stopped */
        unsigned w_Stopsig :8;  /* signal that stopped us */
    } w_S;
#endif
};
#define w_termsig   w_T.w_Termsig
#define w_coredump  w_T.w_Coredump
#define w_retcode   w_T.w_Retcode
#define w_stopval   w_S.w_Stopval
#define w_stopsig   w_S.w_Stopsig

#define WSTOPPED    0177        /* value of s.stopval if process is stopped */

/*
 * Option bits for the second argument of wait3.  WNOHANG causes the
 * wait to not hang if there are no stopped or terminated processes, rather
 * returning an error indication in this case (pid==0).  WUNTRACED
 * indicates that the caller should receive status about untraced children
 * which stop due to signals.  If children are stopped and a wait without
 * this option is done, it is as though they were still running... nothing
 * about them is returned.
 */
#define WNOHANG     1           /* dont hang in wait */
#define WUNTRACED   2           /* tell about stopped, untraced children */

/*
 * The status macros take an int or a union wait and use shifts and masks,
 * so they do not depend on the byte order (D37).
 */
#define _W_INT(x)       (*(int*)&(x))
#define WIFSTOPPED(x)   ((_W_INT(x) & 0377) == WSTOPPED)
#define WIFSIGNALED(x)  ((_W_INT(x) & 0377) != WSTOPPED && \
                        (_W_INT(x) & 0177) != 0)
#define WIFEXITED(x)    ((_W_INT(x) & 0377) != WSTOPPED && \
                        (_W_INT(x) & 0177) == 0)
#define WEXITSTATUS(x)  ((_W_INT(x) >> 8) & 0377)
#define WTERMSIG(x)     (_W_INT(x) & 0177)
#define WCOREDUMP(x)    ((_W_INT(x) >> 7) & 1)
#define WSTOPSIG(x)     ((_W_INT(x) >> 8) & 0377)

#define W_STOPCODE(sig)     ((sig << 8) | WSTOPPED)
#define W_EXITCODE(ret,sig) ((ret << 8) | (sig))

#define WAIT_ANY    (-1)
#define WAIT_MYPGRP 0

#ifndef KERNEL
#include <sys/time.h>
#include <sys/resource.h>

int wait (int *istat);
int wait3 (int *istat, int options, struct rusage *rup);
int waitpid (int pid, int *istat, int options);
int wait4 (int pid, int *istat, int options, struct rusage *rup);
#endif
