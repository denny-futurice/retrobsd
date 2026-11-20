/*
 * Floating point in libc and libm on the 68881 (D44): printf and scanf
 * conversions, atof and strtod, ecvt and gcvt, modf, frexp and ldexp,
 * isnan and isinf, HUGE_VAL as infinity, and the libm functions to
 * double precision, fmod's word order included.  Results are stored to
 * volatile double to round away the 68881's extended precision.
 */
#include "libtest.h"
#include <math.h>
#include <stdlib.h>

char *ecvt(double, int, int *, int *);
char *gcvt(double, int, char *);

static volatile double zero = 0.0, one = 1.0;

/* Within 1e-14 of the expected value, relative unless it is 0. */
static int
near(double got, double want)
{
    double d = got - want, m = want < 0 ? -want : want;

    if (d < 0)
        d = -d;
    return d <= (m > 1 ? m : 1) * 1e-14;
}

static int
fmt(const char *want, const char *f, double x)
{
    char buf[64];

    sprintf(buf, f, x);
    if (strcmp(buf, want) == 0)
        return 1;
    printf("got \"%s\" for %s\n", buf, want);
    return 0;
}

int
main(void)
{
    volatile double d, ip;
    double s;
    float f;
    char *end, *p, buf[32];
    int e, sign;

    console();

    /* printf, with more digits than a float holds. */
    check(fmt("3.141593", "%f", 3.14159265358979));
    check(fmt("0.3333333333", "%.10f", 1.0 / 3));
    check(fmt("1.234568e+04", "%e", 12345.678));
    check(fmt("0.0001", "%g", 0.0001));
    check(fmt("1e+20", "%g", 1e20));
    check(fmt("-2.50", "%.2f", -2.5));
    check(fmt("123456789012.000000", "%f", 123456789012.0));
    check(fmt("Inf", "%f", HUGE_VAL));

    /* atof, strtod and sscanf. */
    check(atof("3.25") == 3.25);
    check(atof("-1.5e300") == -1.5e300);
    check(atof("1e-300") == 1e-300);
    d = strtod("2.5e-3xyz", &end);
    check(d == 2.5e-3 && *end == 'x');
    d = strtod("123456789012345", 0);
    check(d == 123456789012345.0);
    check(sscanf("0.1 2.75", "%lf %lf", &s, &d) == 2 && s == 0.1 && d == 2.75);
    check(sscanf("-0.375", "%f", &f) == 1 && f == -0.375f);

    /* ecvt and gcvt. */
    p = ecvt(3.14159, 4, &e, &sign);
    check(strcmp(p, "3142") == 0 && e == 1 && sign == 0);
    p = ecvt(-0.00125, 3, &e, &sign);
    check(strcmp(p, "125") == 0 && e == -2 && sign != 0);
    check(strcmp(gcvt(1234.5, 6, buf), "1234.5") == 0);

    /* modf, frexp and ldexp. */
    d = modf(-3.75, (double *)&ip);
    check(d == -0.75 && ip == -3.0);
    d = frexp(48.0, &e);
    check(d == 0.75 && e == 6);
    d = frexp(1e-300, &e);
    check(d >= 0.5 && d < 1 && e == -996);
    d = frexp(-HUGE_VAL, &e);
    check(isinf(d) && d < 0 && e == 0);
    check(ldexp(0.75, 6) == 48.0);
    check(ldexp(1.0, 1023) == 8.98846567431158e307);
    check(isinf(ldexp(1.0, 1024)) && ldexp(-1.0, 1024) < 0);

    /* isnan, isinf and HUGE_VAL. */
    d = zero / zero;
    check(isnan(d) && !isinf(d));
    d = one / zero;
    check(isinf(d) && !isnan(d) && HUGE_VAL == d);
    check(!isnan(1e308) && !isinf(1e308));

    /* libm, to double precision. */
    check(near(sqrt(2.0), 1.4142135623730951));
    check(near(sqrt(1e300), 1e150));
    check(near(sin(M_PI / 6), 0.5));
    check(near(cos(M_PI / 3), 0.5));
    check(near(tan(M_PI_4), 1.0));
    check(near(atan(1.0), M_PI_4));
    check(near(atan2(-1.0, -1.0), -3 * M_PI_4));
    check(near(asin(0.5), M_PI / 6));
    check(near(exp(1.0), M_E));
    check(near(log(10.0), M_LN10));
    check(near(log10(1e-7), -7.0));
    check(near(pow(2.0, 0.5), M_SQRT2));
    check(near(pow(10.0, 15.0), 1e15));
    check(near(sinh(1.0), 1.1752011936438014));
    check(near(tanh(0.5), 0.46211715726000974));
    check(near(hypot(3e200, 4e200), 5e200));
    check(isinf(exp(1000.0)));
    check(floor(-2.5) == -3.0 && ceil(-2.5) == -2.0);
    check(floor(4503599627370495.5) == 4503599627370495.0);
    check(fmod(5.5, 2.0) == 1.5);
    check(fmod(-7.25, 2.0) == -1.25);
    check(fmod(1e15 + 3, 7.0) == 2.0);
    check(fmod(0.3, 0.1) == 0.09999999999999998);
    check(isnan(fmod(1.0, 0.0)));

    finish("libfp");
    return 0;
}
