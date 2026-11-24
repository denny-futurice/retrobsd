/*
 * Per-process memory limit (D46): MAXMEM is the whole 256 KB user window.
 * This /sbin/init has 160 KB of bss, over PIC32's 96 KB, so it only runs
 * if exec allows it.  Then sbrk grows the heap up to the stack and must
 * fail there with ENOMEM; a fork swaps the full window out and back in
 * with both patterns intact; and an a.out whose bss fits the window but
 * leaves no room for the stack is refused with ENOMEM.
 */
#include "libtest.h"
#include <a.out.h>
#include <sys/wait.h>

#define BIG     (160 * 1024)
#define STEP    4096

static unsigned char big[BIG];

static void
fill(unsigned char *p, unsigned n, unsigned seed)
{
    while (n--)
        *p++ = seed++ * 7;
}

static int
same(const unsigned char *p, unsigned n, unsigned seed)
{
    while (n--)
        if (*p++ != (unsigned char)(seed++ * 7))
            return 0;
    return 1;
}

int
main(void)
{
    char here, *start, *brk0, *top;
    unsigned heap;
    struct exec hdr;
    int fd, pid, st = -1;
    char *argv[] = { "toobig", 0 };

    console();
    fill(big, BIG, 1);
    check(same(big, BIG, 1));

    start = brk0 = sbrk(0);
    check(brk0 >= (char *)big + BIG);
    while ((top = sbrk(STEP)) != (char *)-1) {
        if (top + STEP > &here) {
            check(!"sbrk went past the stack");
            break;
        }
        brk0 = top + STEP;
    }
    check(top == (char *)-1 && errno == ENOMEM);
    check(sbrk(0) == brk0);
    heap = brk0 - start;
    check(BIG + heap > 200 * 1024);
    check(&here - brk0 < 16 * 1024);
    fill((unsigned char *)start, heap, 3);

    pid = fork();
    if (pid == 0) {
        if (!same(big, BIG, 1) || !same((unsigned char *)start, heap, 3))
            _exit(1);
        fill(big, BIG, 5);
        fill((unsigned char *)start, heap, 9);
        _exit(42);
    }
    check(pid > 0);
    check(wait(&st) == pid);
    check(WIFEXITED(st) && WEXITSTATUS(st) == 42);
    check(same(big, BIG, 1));
    check(same((unsigned char *)start, heap, 3));

    memset(&hdr, 0, sizeof hdr);
    hdr.a_midmag = OMAGIC;
    hdr.a_text = 4;
    hdr.a_bss = 256 * 1024 - 1024;
    hdr.a_entry = 0;
    fd = creat("/toobig", 0755);
    check(fd >= 0);
    check(write(fd, &hdr, sizeof hdr) == sizeof hdr);
    check(write(fd, "\116\161\116\161", 4) == 4);   /* nop; nop */
    close(fd);
    errno = 0;
    execv("/toobig", argv);
    check(errno == ENOMEM);
    unlink("/toobig");

    finish("libmem");
    return 0;
}
