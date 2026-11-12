/*
 * Helpers from ktest.c for the smoke tests that link machdep.c.
 */
void kputs(const char *s);
void check(const char *what, int ok);
void report(const char *name);     /* does not return */
