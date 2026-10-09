#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <float.h>

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_CMD,
    ST_ERR_NUM,
    ST_ERR_EPS_RANGE,
    ST_ERR_RANGE,
    ST_ERR_NO_CONVERGENCE,
    ST_ERR_OVERFLOW,
    ST_ERR_UNKNOWN
} status_t;

/* разбор double */
static status_t parse_double(const char *s, double *out) {
    if (!s || !out || *s == '\0') return ST_ERR_NUM;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return ST_ERR_NUM;
    if (errno == ERANGE) return ST_ERR_NUM;
    if (!isfinite(v)) return ST_ERR_NUM;
    *out = v;
    return ST_OK;
}

/* разбор eps */
static status_t parse_eps(const char *s, double *out) {
    status_t st = parse_double(s, out);
    if (st != ST_OK) return ST_ERR_NUM;
    if (!(*out > 0.0)) return ST_ERR_EPS_RANGE;
    if (*out >= 1.0) return ST_ERR_EPS_RANGE;
    if (*out < 1e-15) return ST_ERR_EPS_RANGE;
    return ST_OK;
}

/* СУММЫ */

/* a: e^x = sum x^n / n! */
static status_t sum_a(double x, double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 1.0, term = 1.0;
    const long MAX_ITER = 1000000L;
    for (long n = 1; n < MAX_ITER; ++n) {
        term *= x / (double)n;
        sum += term;
        if (fabs(term) < eps) { *out = sum; return ST_OK; }
        if (!isfinite(sum)) return ST_ERR_OVERFLOW;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* b: cos(x) = sum (-1)^n x^(2n) / (2n)! */
static status_t sum_b(double x, double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 1.0, term = 1.0;
    double x2 = x * x;
    const long MAX_ITER = 1000000L;
    for (long n = 1; n < MAX_ITER; ++n) {
        term *= -x2 / ((double)(2 * n - 1) * (double)(2 * n));
        sum += term;
        if (fabs(term) < eps) { *out = sum; return ST_OK; }
        if (!isfinite(sum)) return ST_ERR_OVERFLOW;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* c: sum 3^(3n) (n!)^3 x^(2n) / (3n)! */
static status_t sum_c(double x, double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 1.0, term = 1.0;
    double x2 = x * x;
    const long MAX_ITER = 1000000L;
    for (long n = 1; n < MAX_ITER; ++n) {
        double dn = (double)n;
        double ratio = 27.0 * dn * dn * dn * x2 /
                       ((3.0 * dn - 2.0) * (3.0 * dn - 1.0) * (3.0 * dn));
        term *= ratio;
        sum += term;
        if (fabs(term) < eps) { *out = sum; return ST_OK; }
        if (!isfinite(sum)) return ST_ERR_OVERFLOW;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* d: sum_{n>=1} (-1)^n (2n-1)!! x^(2n) / (2n)!! */
static status_t sum_d(double x, double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double term = -x * x / 2.0;
    double sum = term;
    const long MAX_ITER = 1000000L;
    if (fabs(term) < eps) { *out = sum; return ST_OK; }
    for (long n = 2; n < MAX_ITER; ++n) {
        term *= -x * x * (2.0 * (double)n - 1.0) / (2.0 * (double)n);
        sum += term;
        if (fabs(term) < eps) { *out = sum; return ST_OK; }
        if (!isfinite(sum)) return ST_ERR_OVERFLOW;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* ИНТЕГРАЛЫ */

/*
 * Все интегралы берём методом Симпсона с автоматическим удвоением
 * числа разбиений до достижения заданной точности по правилу Рунге:
 *   |I_2n - I_n| / 15 <= eps
 */

/* подынтегральные функции с устранением особенностей */

/* a: ln(1+x)/x, при x->0 предел = 1 */
static double f_int_a(double x) {
    if (fabs(x) < 1e-12) return 1.0;
    return log(1.0 + x) / x;
}

/* b: e^(-x^2/2) */
static double f_int_b(double x) {
    return exp(-x * x * 0.5);
}


static double f_int_c(double u) {
    if (u <= 0.0) return 0.0;
    return -4.0 * u * log(u);
}

/* d: x^x, при x=0 -> 1 */
static double f_int_d(double x) {
    if (x <= 0.0) return 1.0;
    return exp(x * log(x));
}

typedef double (*func1_t)(double);

/* правило Симпсона на [a,b] с n отрезками (n чётное) */
static double simpson(func1_t f, double a, double b, long n) {
    double h = (b - a) / (double)n;
    double s = f(a) + f(b);
    for (long i = 1; i < n; i += 2) s += 4.0 * f(a + h * (double)i);
    for (long i = 2; i < n; i += 2) s += 2.0 * f(a + h * (double)i);
    return s * h / 3.0;
}

/* адаптивное вычисление интеграла с правилом Рунге */
static status_t integrate(func1_t f, double a, double b,
                          double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    if (a >= b) return ST_ERR_RANGE;

    long n = 4;
    double I_prev = simpson(f, a, b, n);
    const long MAX_N = 1L << 30;   /* ~33 млн отрезков максимум */
    for (;;) {
        n *= 2;
        if (n > MAX_N) return ST_ERR_NO_CONVERGENCE;
        double I_cur = simpson(f, a, b, n);
        double err = fabs(I_cur - I_prev) / 3.0;
        if (err <= eps) { *out = I_cur; return ST_OK; }
        I_prev = I_cur;
    }
}

/* ВЫВОД */

typedef status_t (*sum_fn)(double, double, double *);
typedef status_t (*int_fn)(double, double *);

static void print_sum(const char *name, status_t st, double v, double x) {
    switch (st) {
        case ST_OK:
            printf("sum_%s  x=%.6g  = %.15g\n", name, x, v);
            break;
        case ST_ERR_NO_CONVERGENCE:
            printf("sum_%s  x=%.6g  : no convergence\n", name, x);
            break;
        case ST_ERR_OVERFLOW:
            printf("sum_%s  x=%.6g  : overflow\n", name, x);
            break;
        default:
            printf("sum_%s  x=%.6g  : error\n", name, x);
            break;
    }
}

static void print_int(const char *name, status_t st, double v) {
    switch (st) {
        case ST_OK:
            printf("int_%s  = %.15g\n", name, v);
            break;
        case ST_ERR_NO_CONVERGENCE:
            printf("int_%s  : no convergence\n", name);
            break;
        default:
            printf("int_%s  : error\n", name);
            break;
    }
}

/* КОМАНДЫ */

static status_t run_sum(const char *which, int argc, char *argv[]) {
    if (argc != 4) return ST_ERR_ARGC;

    double eps, x;
    status_t st = parse_eps(argv[2], &eps);
    if (st != ST_OK) return st;
    st = parse_double(argv[3], &x);
    if (st != ST_OK) return st;

    struct { const char *name; sum_fn fn; } tab[4] = {
        {"a", sum_a}, {"b", sum_b}, {"c", sum_c}, {"d", sum_d}
    };

    for (int i = 0; i < 4; ++i) {
        if (strcmp(which, tab[i].name) == 0) {
            double v;
            st = tab[i].fn(x, eps, &v);
            print_sum(tab[i].name, st, v, x);
            return st;
        }
    }
    return ST_ERR_CMD;
}

static status_t run_int(const char *which, int argc, char *argv[]) {
    if (argc != 3) return ST_ERR_ARGC;

    double eps;
    status_t st = parse_eps(argv[2], &eps);
    if (st != ST_OK) return st;

    struct { const char *name; func1_t f; } tab[4] = {
        {"a", f_int_a}, {"b", f_int_b}, {"c", f_int_c}, {"d", f_int_d}
    };

    for (int i = 0; i < 4; ++i) {
        if (strcmp(which, tab[i].name) == 0) {
            double v;
            st = integrate(tab[i].f, 0.0, 1.0, eps, &v);
            print_int(tab[i].name, st, v);
            return st;
        }
    }
    return ST_ERR_CMD;
}

/* MAIN */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr,
                "Usage:\n"
                "  %s sum_a <eps> <x>\n"
                "  %s sum_b <eps> <x>\n"
                "  %s sum_c <eps> <x>\n"
                "  %s sum_d <eps> <x>\n"
                "  %s int_a <eps>\n"
                "  %s int_b <eps>\n"
                "  %s int_c <eps>\n"
                "  %s int_d <eps>\n",
                argv[0], argv[0], argv[0], argv[0],
                argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    const char *cmd = argv[1];
    status_t st;

    if (strncmp(cmd, "sum_", 4) == 0) {
        st = run_sum(cmd + 4, argc, argv);
    } else if (strncmp(cmd, "int_", 4) == 0) {
        st = run_int(cmd + 4, argc, argv);
    } else {
        fprintf(stderr, "Error: unknown command '%s'\n", cmd);
        return 2;
    }

    if (st != ST_OK) {
        switch (st) {
            case ST_ERR_ARGC:
                fprintf(stderr, "Error: wrong number of arguments\n");
                break;
            case ST_ERR_NUM:
                fprintf(stderr, "Error: invalid number\n");
                break;
            case ST_ERR_EPS_RANGE:
                fprintf(stderr, "Error: epsilon out of range\n");
                break;
            case ST_ERR_RANGE:
                fprintf(stderr, "Error: invalid range\n");
                break;
            case ST_ERR_CMD:
                fprintf(stderr, "Error: unknown sum/int name\n");
                break;
            case ST_ERR_NO_CONVERGENCE:
                fprintf(stderr, "Error: no convergence\n");
                break;
            default:
                fprintf(stderr, "Error: unknown\n");
                break;
        }
        return 3;
    }
    return 0;
}