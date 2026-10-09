/*
 * lab_1_2.c
 * Вычисление констант e, pi, ln2, sqrt(2), gamma с заданной точностью.
 *
 * Компиляция: gcc -std=c99 -Wall -Wextra -pedantic -o lab_1_2 lab_1_2.c -lm
 * Запуск:     ./lab_1_2 <epsilon>
 */

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
    ST_ERR_EPS_INVALID,
    ST_ERR_EPS_RANGE,
    ST_ERR_NO_CONVERGENCE,
    ST_ERR_OVERFLOW,
    ST_ERR_UNKNOWN
} status_t;

/* разбор epsilon */
static status_t parse_epsilon(const char *s, double *out) {
    if (!s || !out || *s == '\0') return ST_ERR_EPS_INVALID;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return ST_ERR_EPS_INVALID;
    if (errno == ERANGE) return ST_ERR_EPS_RANGE;
    if (!(v > 0.0) || v >= 1.0) return ST_ERR_EPS_RANGE;
    if (v < 1e-14) return ST_ERR_EPS_RANGE;
    *out = v;
    return ST_OK;
}

/* e через ряд */
static status_t e_series(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 1.0, term = 1.0;
    const long MAX_ITER = 1000000L;
    for (long n = 1; n < MAX_ITER; ++n) {
        term /= (double)n;
        sum += term;
        if (term < eps) { *out = sum; return ST_OK; }
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* e через предел */
static status_t e_limit(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double prev = 0.0, cur = 2.0;
    const long MAX_ITER = 100000000L;
    for (long n = 2; n < MAX_ITER; ++n) {
        prev = cur;
        cur = pow(1.0 + 1.0 / (double)n, (double)n);
        if (fabs(cur - prev) < eps) { *out = cur; return ST_OK; }
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* e через уравнение ln(x) = 1 */
static status_t e_equation(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double x = 2.0;
    const long MAX_ITER = 1000000L;
    for (long i = 0; i < MAX_ITER; ++i) {
        double xn = x * (2.0 - log(x));
        if (!isfinite(xn)) return ST_ERR_OVERFLOW;
        if (fabs(xn - x) < eps) { *out = xn; return ST_OK; }
        x = xn;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* pi через ряд */
static status_t pi_series(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 0.0, sign = 1.0, term;
    const long MAX_ITER = 100000000L;
    long n = 1;
    do {
        term = 1.0 / (double)(2 * n - 1);
        sum += sign * term;
        sign = -sign;
        ++n;
        if (n > MAX_ITER) return ST_ERR_NO_CONVERGENCE;
    } while (term >= eps * 0.25);
    *out = 4.0 * sum;
    return ST_OK;
}

/* pi через уравнение cos(x) = -1 */
static status_t pi_equation(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double x = 3.0;
    const long MAX_ITER = 1000000L;
    for (long i = 0; i < MAX_ITER; ++i) {
        double s = sin(x);
        if (fabs(s) < DBL_EPSILON) { *out = x; return ST_OK; }
        double xn = x + (cos(x) + 1.0) / s;
        if (!isfinite(xn)) return ST_ERR_OVERFLOW;
        if (fabs(xn - x) < eps) { *out = xn; return ST_OK; }
        x = xn;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* pi через предел (Валлис) */
static status_t pi_limit(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double prod = 1.0, prev = 0.0;
    const long MAX_ITER = 100000000L;
    for (long n = 1; n < MAX_ITER; ++n) {
        prev = prod;
        double a = (2.0 * (double)n) / (2.0 * (double)n - 1.0);
        double b = (2.0 * (double)n) / (2.0 * (double)n + 1.0);
        prod *= a * b;
        if (fabs(prod - prev) < eps) { *out = 2.0 * prod; return ST_OK; }
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* ln2 через ряд */
static status_t ln2_series(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 0.0, sign = 1.0, term;
    const long MAX_ITER = 1000000000L;
    long n = 1;
    do {
        term = 1.0 / (double)n;
        sum += sign * term;
        sign = -sign;
        ++n;
        if (n > MAX_ITER) return ST_ERR_NO_CONVERGENCE;
    } while (term >= eps);
    *out = sum;
    return ST_OK;
}

/* ln2 через уравнение e^x = 2 */
static status_t ln2_equation(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double x = 0.5;
    const long MAX_ITER = 1000000L;
    for (long i = 0; i < MAX_ITER; ++i) {
        double ex = exp(x);
        if (!isfinite(ex)) return ST_ERR_OVERFLOW;
        double xn = x - (ex - 2.0) / ex;
        if (fabs(xn - x) < eps) { *out = xn; return ST_OK; }
        x = xn;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* ln2 через предел n*(2^(1/n) - 1) */
static status_t ln2_limit(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double prev = 0.0, cur = 0.0;
    const long MAX_ITER = 100000000L;
    for (long n = 1; n < MAX_ITER; ++n) {
        prev = cur;
        cur = (double)n * (pow(2.0, 1.0 / (double)n) - 1.0);
        if (n > 1 && fabs(cur - prev) < eps) { *out = cur; return ST_OK; }
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* sqrt(2) через биномиальный ряд */
static status_t sqrt2_series(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 1.0;
    double coef = 1.0;
    const long MAX_ITER = 100000000L;
    for (long k = 1; k < MAX_ITER; ++k) {
        coef *= (0.5 - (double)(k - 1)) / (double)k;
        double term = coef;
        sum += term;
        if (fabs(term) < eps) { *out = sum; return ST_OK; }
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* sqrt(2) через уравнение x^2 = 2 */
static status_t sqrt2_equation(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double x = 1.5;
    const long MAX_ITER = 1000000L;
    for (long i = 0; i < MAX_ITER; ++i) {
        double xn = 0.5 * (x + 2.0 / x);
        if (!isfinite(xn)) return ST_ERR_OVERFLOW;
        if (fabs(xn - x) < eps) { *out = xn; return ST_OK; }
        x = xn;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* sqrt(2) через итерационный предел */
static status_t sqrt2_limit(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double x = -0.5;
    const long MAX_ITER = 1000000L;
    for (long i = 0; i < MAX_ITER; ++i) {
        double xn = x - x * x * 0.5 + 1.0;
        if (!isfinite(xn)) return ST_ERR_OVERFLOW;
        if (fabs(xn - x) < eps) { *out = xn; return ST_OK; }
        x = xn;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* gamma через предел H_n - ln n */
static status_t gamma_series(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double sum = 0.0, prev = 0.0;
    const long MAX_ITER = 100000000L;
    for (long m = 1; m < MAX_ITER; ++m) {
        sum += 1.0 / (double)m;
        double g = sum - log((double)m);
        if (m > 1 && fabs(g - prev) < eps) { *out = g; return ST_OK; }
        prev = g;
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* gamma через решение H_n - ln n = x при большом n */
static status_t gamma_equation(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    (void)eps;
    const long n = 1000000L;
    double Hn = 0.0;
    for (long k = 1; k <= n; ++k) Hn += 1.0 / (double)k;
    *out = Hn - log((double)n);
    return ST_OK;
}

/* проверка простоты */
static int is_prime(long n) {
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (long d = 3; d * d <= n; d += 2)
        if (n % d == 0) return 0;
    return 1;
}

/* gamma через предел произведения по простым */
static status_t gamma_limit(double eps, double *out) {
    if (!out) return ST_ERR_UNKNOWN;
    double prod = 1.0;
    double prev = 0.0;
    const long MAX_T = 10000000L;
    for (long t = 2; t < MAX_T; ++t) {
        if (is_prime(t)) {
            prod *= ((double)t - 1.0) / (double)t;
        }
        if (t % 1000 == 0 || t < 100) {
            double g = log((double)t) * prod;
            if (t > 100 && fabs(g - prev) < eps) { *out = g; return ST_OK; }
            prev = g;
        }
    }
    return ST_ERR_NO_CONVERGENCE;
}

/* печать результата */
static void print_result(const char *name, const char *method, status_t st, double val) {
    switch (st) {
        case ST_OK:
            printf("%-8s %-10s = %.15f\n", name, method, val);
            break;
        case ST_ERR_NO_CONVERGENCE:
            printf("%-8s %-10s : error: no convergence\n", name, method);
            break;
        case ST_ERR_OVERFLOW:
            printf("%-8s %-10s : error: overflow\n", name, method);
            break;
        default:
            printf("%-8s %-10s : error: unknown\n", name, method);
            break;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <epsilon>\n", argv[0]);
        return 1;
    }

    double eps = 0.0;
    status_t st = parse_epsilon(argv[1], &eps);
    if (st != ST_OK) {
        fprintf(stderr, "Error: invalid epsilon (must be 1e-14 <= eps < 1)\n");
        return 2;
    }

    printf("epsilon = %.15g\n\n", eps);

    double v;

    printf("--- e ---\n");
    st = e_series(eps, &v);   print_result("e", "series",   st, v);
    st = e_equation(eps, &v); print_result("e", "equation", st, v);
    st = e_limit(eps, &v);    print_result("e", "limit",    st, v);
    printf("\n");

    printf("--- pi ---\n");
    st = pi_series(eps, &v);   print_result("pi", "series",   st, v);
    st = pi_equation(eps, &v); print_result("pi", "equation", st, v);
    st = pi_limit(eps, &v);    print_result("pi", "limit",    st, v);
    printf("\n");

    printf("--- ln 2 ---\n");
    st = ln2_series(eps, &v);   print_result("ln2", "series",   st, v);
    st = ln2_equation(eps, &v); print_result("ln2", "equation", st, v);
    st = ln2_limit(eps, &v);    print_result("ln2", "limit",    st, v);
    printf("\n");

    printf("--- sqrt(2) ---\n");
    st = sqrt2_series(eps, &v);   print_result("sqrt2", "series",   st, v);
    st = sqrt2_equation(eps, &v); print_result("sqrt2", "equation", st, v);
    st = sqrt2_limit(eps, &v);    print_result("sqrt2", "limit",    st, v);
    printf("\n");

    printf("--- gamma ---\n");
    st = gamma_series(eps, &v);   print_result("gamma", "series",   st, v);
    st = gamma_equation(eps, &v); print_result("gamma", "equation", st, v);
    st = gamma_limit(eps, &v);    print_result("gamma", "limit",    st, v);

    return 0;
}