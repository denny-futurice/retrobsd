#ifndef _MACHINE_FLOAT_H_
#define _MACHINE_FLOAT_H_

/*
 * Characteristics of the floating types with a 68881/68882 FPU (D12):
 * IEEE single and double, and the 96-bit extended format for long double.
 * Values match GCC's predefined macros for -mcpu=68030 -m68881.
 */

#define FLT_RADIX       2
#define FLT_ROUNDS      1               /* round to nearest */
#define FLT_EVAL_METHOD 2               /* evaluate in long double (FPU registers) */

/*
 * These values provide information pertaining to the float type.
 */
#define FLT_EPSILON     1.19209290E-07F /* b**(1-p) */
#define FLT_MAX         3.40282347E+38F /* (1-b**(-p))*b**emax */
#define FLT_MIN         1.17549435E-38F /* b**(emin-1) */
#define FLT_DIG         6               /* floor((p-1)*log10(b))+(b == 10) */
#define FLT_MANT_DIG    24              /* p */
#define FLT_MAX_10_EXP  38              /* floor(log10((1-b**(-p))*b**emax)) */
#define FLT_MAX_EXP     128             /* emax */
#define FLT_MIN_10_EXP  (-37)           /* ceil(log10(b**(emin-1))) */
#define FLT_MIN_EXP     (-125)          /* emin */

/*
 * These values provide information pertaining to the double type.
 */
#define DBL_EPSILON     2.2204460492503131E-16
#define DBL_MAX         1.7976931348623157E+308
#define DBL_MIN         2.2250738585072014E-308
#define DBL_DIG         15
#define DBL_MANT_DIG    53
#define DBL_MAX_10_EXP  308
#define DBL_MAX_EXP     1024
#define DBL_MIN_10_EXP  (-307)
#define DBL_MIN_EXP     (-1021)

/*
 * These values provide information pertaining to the long double type
 * (68881 extended precision, 12 bytes in memory).
 */
#define LDBL_EPSILON    1.08420217248550443401E-19L
#define LDBL_MAX        1.18973149535723176502E+4932L
#define LDBL_MIN        1.68105157155604675313E-4932L
#define LDBL_DIG        18
#define LDBL_MANT_DIG   64
#define LDBL_MAX_10_EXP 4932
#define LDBL_MAX_EXP    16384
#define LDBL_MIN_10_EXP (-4931)
#define LDBL_MIN_EXP    (-16382)

#endif /* _MACHINE_FLOAT_H_ */
