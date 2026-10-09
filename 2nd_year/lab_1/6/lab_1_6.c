#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <errno.h>
#include <float.h>

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_NUM,
    ST_ERR_RANGE,
    ST_ERR_BASE,
    ST_ERR_DIGIT,
    ST_ERR_NO_ROOT,
    ST_ERR_OVERFLOW,
    ST_ERR_UNKNOWN
} status_t;

/*1. выпуклость*/

/* is_convex: n пар (x,y), проверка выпуклости */
static status_t is_convex(int n, int *out, ...) {
    if (!out) return ST_ERR_UNKNOWN;
    if (n < 3) return ST_ERR_RANGE;

    va_list ap;
    va_start(ap, out);

    double *xs = (double *)malloc((size_t)n * sizeof(double));
    double *ys = (double *)malloc((size_t)n * sizeof(double));
    if (!xs || !ys) {
        free(xs); free(ys);
        va_end(ap);
        return ST_ERR_OVERFLOW;
    }

    for (int i = 0; i < n; ++i) {
        xs[i] = va_arg(ap, double);
        ys[i] = va_arg(ap, double);
    }
    va_end(ap);

    int sign = 0;
    int ok = 1;

    for (int i = 0; i < n; ++i) {
        int j = (i + 1) % n;
        int k = (i + 2) % n;
        double cross = (xs[j] - xs[i]) * (ys[k] - ys[j])
                     - (ys[j] - ys[i]) * (xs[k] - xs[j]);
        if (fabs(cross) > 1e-12) {
            int s = cross > 0.0 ? 1 : -1;
            if (sign == 0) sign = s;
            else if (s != sign) { ok = 0; break; }
        }
    }

    free(xs);
    free(ys);
    *out = ok;
    return ST_OK;
}

/*2. многочлен*/

/* poly_value: значение многочлена в точке (схема Горнера) */
static status_t poly_value(double x, int n, double *out, ...) {
    if (!out) return ST_ERR_UNKNOWN;
    if (n < 0) return ST_ERR_RANGE;

    va_list ap;
    va_start(ap, out);

    double acc = 0.0;
    for (int i = 0; i <= n; ++i) {
        double c = va_arg(ap, double);
        acc = acc * x + c;
    }
    va_end(ap);

    if (!isfinite(acc)) return ST_ERR_OVERFLOW;
    *out = acc;
    return ST_OK;
}

/*3. числа Капрекара*/

/* digit_val: значение цифры */
static int digit_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

/* parse_in_base: разбор числа */
static status_t parse_in_base(const char *s, int base, unsigned long long *out) {
    if (!s || !out || *s == '\0') return ST_ERR_NUM;
    if (base < 2 || base > 36) return ST_ERR_BASE;

    unsigned long long v = 0;
    for (const char *p = s; *p; ++p) {
        int d = digit_val(*p);
        if (d < 0 || d >= base) return ST_ERR_DIGIT;
        if (v > (ULLONG_MAX - (unsigned)d) / (unsigned)base) return ST_ERR_OVERFLOW;
        v = v * (unsigned long long)base + (unsigned long long)d;
    }
    *out = v;
    return ST_OK;
}

/* to_base_str: запись числа */
static void to_base_str(unsigned long long v, int base, char *buf, size_t cap) {
    char tmp[70];
    int k = 0;
    if (v == 0) tmp[k++] = '0';
    while (v > 0) {
        int d = (int)(v % (unsigned long long)base);
        tmp[k++] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
        v /= (unsigned long long)base;
    }
    size_t j = 0;
    while (k > 0 && j + 1 < cap) buf[j++] = tmp[--k];
    buf[j] = '\0';
}

/* is_kaprekar: проверка числа Капрекара */
static int is_kaprekar(unsigned long long num, int base) {
    if (num == 0) return 0;
    unsigned long long sq = num * num;
    char buf[70];
    to_base_str(sq, base, buf, sizeof(buf));
    size_t len = strlen(buf);
    size_t right_len = len / 2;
    size_t left_len = len - right_len;

    unsigned long long left = 0, right = 0;
    for (size_t i = 0; i < left_len; ++i) {
        int d = digit_val(buf[i]);
        if (d < 0 || d >= base) return 0;
        left = left * (unsigned long long)base + (unsigned long long)d;
    }
    for (size_t i = left_len; i < len; ++i) {
        int d = digit_val(buf[i]);
        if (d < 0 || d >= base) return 0;
        right = right * (unsigned long long)base + (unsigned long long)d;
    }
    return (left + right) == num;
}

/* count_kaprekar: count строк, возвращает количество */
static status_t count_kaprekar(int base, int count, int *out, ...) {
    if (!out) return ST_ERR_UNKNOWN;
    if (base < 2 || base > 36) return ST_ERR_BASE;
    if (count < 0) return ST_ERR_RANGE;

    va_list ap;
    va_start(ap, out);

    int found = 0;
    for (int i = 0; i < count; ++i) {
        const char *s = va_arg(ap, const char *);
        unsigned long long v;
        status_t st = parse_in_base(s, base, &v);
        if (st != ST_OK) { va_end(ap); return st; }
        if (is_kaprekar(v, base)) {
            ++found;
            printf("  %s (base %d) = %llu - Kaprekar\n", s, base, v);
        }
    }
    va_end(ap);
    *out = found;
    return ST_OK;
}

/*4. среднее геометрическое*/

/* geom_mean: count чисел, среднее геометрическое */
static status_t geom_mean(int count, double *out, ...) {
    if (!out) return ST_ERR_UNKNOWN;
    if (count <= 0) return ST_ERR_RANGE;

    va_list ap;
    va_start(ap, out);

    double prod = 1.0;
    for (int i = 0; i < count; ++i) {
        double v = va_arg(ap, double);
        if (v < 0.0) { va_end(ap); return ST_ERR_RANGE; }
        prod *= v;
    }
    va_end(ap);

    if (!isfinite(prod)) return ST_ERR_OVERFLOW;
    *out = pow(prod, 1.0 / (double)count);
    return ST_OK;
}

/*5. быстрое возведение в степень*/

/* fast_pow: рекурсивное быстрое возведение */
static double fast_pow(double base, long exp) {
    if (exp == 0) return 1.0;
    if (exp < 0) return 1.0 / fast_pow(base, -exp);
    if (exp % 2 == 0) {
        double h = fast_pow(base, exp / 2);
        return h * h;
    }
    return base * fast_pow(base, exp - 1);
}

/*6. метод дихотомии*/

typedef double (*func1_t)(double);

/* bisection: корень f на [a,b] с точностью eps */
static status_t bisection(double a, double b, double eps,
                          func1_t f, double *out) {
    if (!out || !f) return ST_ERR_UNKNOWN;
    if (!(eps > 0.0)) return ST_ERR_RANGE;

    double fa = f(a), fb = f(b);
    if (!isfinite(fa) || !isfinite(fb)) return ST_ERR_OVERFLOW;
    if (fa == 0.0) { *out = a; return ST_OK; }
    if (fb == 0.0) { *out = b; return ST_OK; }
    if (fa * fb > 0.0) return ST_ERR_NO_ROOT;

    const int MAX_ITER = 10000;
    double mid = 0.0;
    for (int i = 0; i < MAX_ITER; ++i) {
        mid = 0.5 * (a + b);
        double fm = f(mid);
        if (!isfinite(fm)) return ST_ERR_OVERFLOW;
        if (fabs(fm) < eps || (b - a) * 0.5 < eps) {
            *out = mid;
            return ST_OK;
        }
        if (fa * fm < 0.0) {
            b = mid; fb = fm;
        } else {
            a = mid; fa = fm;
        }
    }
    *out = mid;
    return ST_OK;
}

/* уравнения */
static double f1(double x) { return x * x - 2.0; }
static double f2(double x) { return cos(x) - x; }
static double f3(double x) { return x * x * x - x - 2.0; }
static double f4(double x) { return exp(x) - 3.0 * x; }

/*демонстрации*/

static void demo_convex(void) {
    printf("--- 1. convex polygon ---\n");
    int ok = 0;
    status_t st;

    st = is_convex(4, &ok, 0.0,0.0, 1.0,0.0, 1.0,1.0, 0.0,1.0);
    printf("square   : st=%d convex=%d\n", (int)st, ok);

    st = is_convex(3, &ok, 0.0,0.0, 1.0,0.0, 0.5,1.0);
    printf("triangle : st=%d convex=%d\n", (int)st, ok);

    st = is_convex(5, &ok, 0.0,0.0, 2.0,0.0, 1.0,0.5, 2.0,2.0, 0.0,2.0);
    printf("concave  : st=%d convex=%d\n", (int)st, ok);

    st = is_convex(5, &ok, 0.0,0.0, 1.0,0.0, 1.5,1.0, 0.5,2.0, -0.5,1.0);
    printf("pentagon : st=%d convex=%d\n", (int)st, ok);

    /* коллинеарные вершины */
    st = is_convex(4, &ok, 0.0,0.0, 1.0,0.0, 2.0,0.0, 1.0,1.0);
    printf("flat     : st=%d convex=%d\n", (int)st, ok);

    st = is_convex(2, &ok, 0.0,0.0, 1.0,0.0);
    printf("bad n    : st=%d\n", (int)st);
    printf("\n");
}

static void demo_poly(void) {
    printf("--- 2. polynomial ---\n");
    double v;
    status_t st;

    /* 2x^3 - 3x^2 + 4x - 5, x=2 -> 7 */
    st = poly_value(2.0, 3, &v, 2.0, -3.0, 4.0, -5.0);
    printf("p(2)   = %g (st=%d)\n", v, (int)st);

    /* x^2 - 1, x=3 -> 8 */
    st = poly_value(3.0, 2, &v, 1.0, 0.0, -1.0);
    printf("p(3)   = %g (st=%d)\n", v, (int)st);

    /* const 42 */
    st = poly_value(100.0, 0, &v, 42.0);
    printf("p(100) = %g (st=%d)\n", v, (int)st);

    /* x^4 - 1, x=2 -> 15 */
    st = poly_value(2.0, 4, &v, 1.0, 0.0, 0.0, 0.0, -1.0);
    printf("p(2)   = %g (st=%d)\n", v, (int)st);

    st = poly_value(1.0, -1, &v, 1.0);
    printf("bad n  : st=%d\n", (int)st);
    printf("\n");
}

static void demo_kaprekar(void) {
    printf("--- 3. Kaprekar numbers ---\n");
    int cnt = 0;
    status_t st;

    st = count_kaprekar(10, 8, &cnt,
                        "9", "45", "55", "99", "297", "703", "999", "2223");
    printf("base 10: found %d (st=%d)\n", cnt, (int)st);

    cnt = 0;
    st = count_kaprekar(2, 5, &cnt, "1", "11", "101", "111", "1001");
    printf("base 2 : found %d (st=%d)\n", cnt, (int)st);

    cnt = 0;
    st = count_kaprekar(16, 4, &cnt, "1", "6", "A", "F");
    printf("base 16: found %d (st=%d)\n", cnt, (int)st);

    cnt = 0;
    st = count_kaprekar(10, 3, &cnt, "9", "XYZ", "45");
    printf("bad    : st=%d\n", (int)st);

    cnt = 0;
    st = count_kaprekar(1, 1, &cnt, "1");
    printf("bad base: st=%d\n", (int)st);
    printf("\n");
}

static void demo_geom(void) {
    printf("--- 4. geometric mean ---\n");
    double v;
    status_t st;

    st = geom_mean(3, &v, 1.0, 8.0, 27.0);
    printf("gm(1,8,27)   = %g (st=%d)\n", v, (int)st);

    st = geom_mean(4, &v, 2.0, 4.0, 8.0, 16.0);
    printf("gm(2,4,8,16) = %g (st=%d)\n", v, (int)st);

    st = geom_mean(1, &v, 5.0);
    printf("gm(5)        = %g (st=%d)\n", v, (int)st);

    st = geom_mean(3, &v, 0.0, 1.0, 2.0);
    printf("gm(0,1,2)    = %g (st=%d)\n", v, (int)st);

    st = geom_mean(2, &v, -1.0, 2.0);
    printf("gm(-1,2)     = st=%d\n", (int)st);

    st = geom_mean(0, &v);
    printf("gm()         = st=%d\n", (int)st);
    printf("\n");
}

static void demo_pow(void) {
    printf("--- 5. fast power ---\n");
    printf("2^10  = %g\n", fast_pow(2.0, 10));
    printf("3^-3  = %g\n", fast_pow(3.0, -3));
    printf("2^0   = %g\n", fast_pow(2.0, 0));
    printf("1.5^7 = %g\n", fast_pow(1.5, 7));
    printf("2^40  = %g\n", fast_pow(2.0, 40));
    printf("5^-2  = %g\n", fast_pow(5.0, -2));
    printf("\n");
}

static void demo_bisect(void) {
    printf("--- 6. bisection ---\n");
    double v;
    status_t st;

    st = bisection(0.0, 2.0, 1e-9, f1, &v);
    printf("x^2-2=0      : x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(0.0, 1.0, 1e-9, f2, &v);
    printf("cos x - x=0  : x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(1.0, 2.0, 1e-9, f3, &v);
    printf("x^3-x-2=0    : x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(0.0, 1.0, 1e-9, f4, &v);
    printf("e^x-3x=0 (1) : x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(1.0, 2.0, 1e-9, f4, &v);
    printf("e^x-3x=0 (2) : x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(0.0, 2.0, 1e-3, f1, &v);
    printf("x^2-2=0 eps=1e-3 : x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(0.0, 2.0, 1e-15, f1, &v);
    printf("x^2-2=0 eps=1e-15: x=%.12g (st=%d)\n", v, (int)st);

    st = bisection(2.0, 3.0, 1e-9, f1, &v);
    printf("no root case : st=%d\n", (int)st);

    st = bisection(0.0, 2.0, -1.0, f1, &v);
    printf("bad eps      : st=%d\n", (int)st);

    st = bisection(2.0, 0.0, 1e-9, f1, &v);
    printf("reversed     : st=%d\n", (int)st);

    printf("\n");
}

/*main*/

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <1..6>\n", argv[0]);
        return 1;
    }

    errno = 0;
    char *end = NULL;
    long n = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || errno == ERANGE) {
        fprintf(stderr, "Error: invalid number\n");
        return 2;
    }

    switch (n) {
        case 1: demo_convex();   return 0;
        case 2: demo_poly();     return 0;
        case 3: demo_kaprekar(); return 0;
        case 4: demo_geom();     return 0;
        case 5: demo_pow();      return 0;
        case 6: demo_bisect();   return 0;
        default:
            fprintf(stderr, "Error: number must be 1..6\n");
            return 3;
    }
}