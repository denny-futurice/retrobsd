/*
 * The 64-bit integer helpers from compiler-rt (D43): signed and unsigned
 * division and modulo, shifts by a variable count (through the operators
 * and the helpers themselves, since GCC inlines some), and conversions
 * between 64-bit integers and float or double, with ties rounded to even
 * and the low bits kept as sticky bits.  Operands are volatile so the
 * helpers are called at run time; results are stored to volatile float
 * and double to round away the 68881's extended precision.
 */
#include "libtest.h"

long long __ashldi3(long long, int);
long long __ashrdi3(long long, int);
long long __lshrdi3(long long, int);

#define NELEM(a) (sizeof(a) / sizeof((a)[0]))

/* Generated with Python's arbitrary precision integers. */
static const struct { long long a, b, q, r; } sdiv[] = {
    { 1311768467463790320LL, 74565LL, 17592281465349LL, 42135LL },
    { -1311768467463790320LL, 74565LL, -17592281465349LL, -42135LL },
    { 9223372036854775807LL, -3LL, -3074457345618258602LL, 1LL },
    { (-9223372036854775807LL - 1), 4294967297LL, -2147483647LL, -2147483649LL },
    { 1000000000000LL, 7LL, 142857142857LL, 1LL },
    { -7LL, 2LL, -3LL, -1LL },
    { 1311768467463790320LL, 1147797409030816545LL, 1LL, 163971058432973775LL },
};
static const struct { unsigned long long a, b, q, r; } udiv[] = {
    { 0xffffffffffffffffULL, 0x3ULL, 0x5555555555555555ULL, 0x0ULL },
    { 0xfedcba9876543210ULL, 0x100000000ULL, 0xfedcba98ULL, 0x76543210ULL },
    { 0xfedcba9876543210ULL, 0x123456789ULL, 0xe0000000ULL, 0x96543210ULL },
    { 0x8000000000000000ULL, 0xffffffffffffffffULL, 0x0ULL, 0x8000000000000000ULL },
    { 0x3039ULL, 0x100000000000ULL, 0x0ULL, 0x3039ULL },
};
/* x = 0xf123456789abcdef shifted by n: left, arithmetic right, logical right */
static const struct { int n; unsigned long long l, a, r; } shift[] = {
    { 0, 0xf123456789abcdefULL, 0xf123456789abcdefULL, 0xf123456789abcdefULL },
    { 1, 0xe2468acf13579bdeULL, 0xf891a2b3c4d5e6f7ULL, 0x7891a2b3c4d5e6f7ULL },
    { 4, 0x123456789abcdef0ULL, 0xff123456789abcdeULL, 0xf123456789abcdeULL },
    { 31, 0xc4d5e6f780000000ULL, 0xffffffffe2468acfULL, 0x1e2468acfULL },
    { 32, 0x89abcdef00000000ULL, 0xfffffffff1234567ULL, 0xf1234567ULL },
    { 33, 0x13579bde00000000ULL, 0xfffffffff891a2b3ULL, 0x7891a2b3ULL },
    { 40, 0xabcdef0000000000ULL, 0xfffffffffff12345ULL, 0xf12345ULL },
    { 63, 0x8000000000000000ULL, 0xffffffffffffffffULL, 0x1ULL },
};

int
main(void)
{
    volatile long long sa, sb;
    volatile unsigned long long ua, ub;
    volatile double d;
    volatile float f;
    volatile int n;
    unsigned i;

    console();

    for (i = 0; i < NELEM(sdiv); i++) {
        sa = sdiv[i].a;
        sb = sdiv[i].b;
        check(sa / sb == sdiv[i].q);
        check(sa % sb == sdiv[i].r);
    }
    for (i = 0; i < NELEM(udiv); i++) {
        ua = udiv[i].a;
        ub = udiv[i].b;
        check(ua / ub == udiv[i].q);
        check(ua % ub == udiv[i].r);
    }
    for (i = 0; i < NELEM(shift); i++) {
        ua = 0xf123456789abcdefULL;
        sa = (long long)ua;
        n = shift[i].n;
        check(ua << n == shift[i].l);
        check((unsigned long long)(sa >> n) == shift[i].a);
        check(ua >> n == shift[i].r);
        check((unsigned long long)__ashldi3(sa, n) == shift[i].l);
        check((unsigned long long)__ashrdi3(sa, n) == shift[i].a);
        check((unsigned long long)__lshrdi3(sa, n) == shift[i].r);
    }

    /* __floatdidf, __floatundidf */
    sa = 0x0020000000000001LL; d = sa; check(d == 9007199254740992.0);
    sa = 0x0020000000000003LL; d = sa; check(d == 9007199254740996.0);
    sa = 0x7fffffffffffffffLL; d = sa; check(d == 9223372036854775808.0);
    sa = -9223372036854775807LL - 1; d = sa; check(d == -9223372036854775808.0);
    sa = -12345; d = sa; check(d == -12345.0);
    ua = 0xffffffffffffffffULL; d = ua; check(d == 18446744073709551616.0);
    ua = 0x8000000000000401ULL; d = ua; check(d == 9223372036854777856.0);

    /* __floatdisf, __floatundisf */
    sa = 0x1000001; f = sa; check(f == 16777216.0f);
    sa = 0x1000003; f = sa; check(f == 16777220.0f);
    sa = -0x1000003; f = sa; check(f == -16777220.0f);
    sa = 0x7fffffffffffffffLL; f = sa; check(f == 9223372036854775808.0f);
    sa = 0x4000004000000001LL; f = sa; check(f == 4611686568183201792.0f);
    sa = -0x4000004000000001LL; f = sa; check(f == -4611686568183201792.0f);
    ua = 0xffffffffffffffffULL; f = ua; check(f == 18446744073709551616.0f);
    ua = 0x8000008000000001ULL; f = ua; check(f == 9223373136366403584.0f);

    /* __fixdfdi, __fixunsdfdi */
    d = -1.5; check((long long)d == -1);
    d = 1e18; check((long long)d == 1000000000000000000LL);
    d = -9.2e18; check((long long)d == -9200000000000000000LL);
    d = 9223372036854774784.0; check((long long)d == 0x7ffffffffffffc00LL);
    d = 1.8e19; check((unsigned long long)d == 18000000000000000000ULL);
    d = 18446744073709549568.0;
    check((unsigned long long)d == 0xfffffffffffff800ULL);

    /* __fixsfdi, __fixunssfdi */
    f = -1099511627776.0f; check((long long)f == -1099511627776LL);
    f = -2.75f; check((long long)f == -2);
    f = 9223372036854775808.0f;
    check((unsigned long long)f == 0x8000000000000000ULL);
    f = 1250999402496.0f;                       /* 0x123456 << 20 */
    check((unsigned long long)f == 0x12345600000ULL);

    finish("libint64");
    return 0;
}
