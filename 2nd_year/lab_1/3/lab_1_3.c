#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <float.h>

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_NUM_INVALID,
    ST_ERR_EPS_RANGE,
    ST_ERR_ZERO,
    ST_ERR_NEGATIVE,
    ST_ERR_UNKNOWN
} status_t;

/* разбор вещественного */
static status_t parse_double(const char *s, double *out) {
    if (!s || !out || *s == '\0') return ST_ERR_NUM_INVALID;
    errno = 0;
    char *end = NULL;
    double v = strtod(s, &end);
    if (end == s || *end != '\0') return ST_ERR_NUM_INVALID;
    if (errno == ERANGE) return ST_ERR_NUM_INVALID;
    if (!isfinite(v)) return ST_ERR_NUM_INVALID;
    *out = v;
    return ST_OK;
}

/* разбор целого */
static status_t parse_long(const char *s, long *out) {
    if (!s || !out || *s == '\0') return ST_ERR_NUM_INVALID;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return ST_ERR_NUM_INVALID;
    if (errno == ERANGE) return ST_ERR_NUM_INVALID;
    *out = v;
    return ST_OK;
}

/* разбор epsilon */
static status_t parse_epsilon(const char *s, double *out) {
    status_t st = parse_double(s, out);
    if (st != ST_OK) return ST_ERR_NUM_INVALID;
    if (!(*out > 0.0)) return ST_ERR_EPS_RANGE;
    if (*out >= 1.0) return ST_ERR_EPS_RANGE;
    if (*out < 1e-14) return ST_ERR_EPS_RANGE;
    return ST_OK;
}

/* равенство вещественных с eps */
static int dbl_eq(double a, double b, double eps) {
    return fabs(a - b) <= eps;
}

/* проверка флага */
static int flag_is(const char *arg, const char *name) {
    if (!arg || !name) return 0;
    if (arg[0] != '-' && arg[0] != '/') return 0;
    return strcmp(arg + 1, name) == 0;
}

/*-q*/

/* решение одного квадратного уравнения */
static void solve_quadratic(double a, double b, double c, double eps) {
    printf("  a=%.6g b=%.6g c=%.6g : ", a, b, c);
    if (dbl_eq(a, 0.0, eps)) {
        if (dbl_eq(b, 0.0, eps)) {
            if (dbl_eq(c, 0.0, eps))
                printf("infinite solutions\n");
            else
                printf("no solutions\n");
        } else {
            printf("linear: x = %.10g\n", -c / b);
        }
        return;
    }
    double d = b * b - 4.0 * a * c;
    if (d < -eps) {
        printf("no real roots (D=%.6g)\n", d);
    } else if (fabs(d) <= eps) {
        printf("x1 = x2 = %.10g\n", -b / (2.0 * a));
    } else {
        double sq = sqrt(d);
        double x1 = (-b + sq) / (2.0 * a);
        double x2 = (-b - sq) / (2.0 * a);
        printf("x1 = %.10g, x2 = %.10g\n", x1, x2);
    }
}

/* все уникальные перестановки коэффициентов */
static status_t cmd_q(int argc, char *argv[]) {
    if (argc != 6) return ST_ERR_ARGC;

    double eps;
    status_t st = parse_epsilon(argv[2], &eps);
    if (st != ST_OK) return st;

    double coef[3];
    for (int i = 0; i < 3; ++i) {
        st = parse_double(argv[3 + i], &coef[i]);
        if (st != ST_OK) return st;
    }

    /* перестановки индексов 0,1,2 */
    const int perm[6][3] = {
        {0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}
    };

    /* исключаем дубликаты троек */
    double seen[6][3];
    int nseen = 0;

    for (int k = 0; k < 6; ++k) {
        double t[3];
        t[0] = coef[perm[k][0]];
        t[1] = coef[perm[k][1]];
        t[2] = coef[perm[k][2]];

        int dup = 0;
        for (int s = 0; s < nseen; ++s) {
            if (dbl_eq(seen[s][0], t[0], eps) &&
                dbl_eq(seen[s][1], t[1], eps) &&
                dbl_eq(seen[s][2], t[2], eps)) { dup = 1; break; }
        }
        if (dup) continue;

        for (int i = 0; i < 3; ++i) seen[nseen][i] = t[i];
        ++nseen;

        solve_quadratic(t[0], t[1], t[2], eps);
    }
    return ST_OK;
}

/*-m*/

static status_t cmd_m(int argc, char *argv[]) {
    if (argc != 4) return ST_ERR_ARGC;

    long a, b;
    status_t st = parse_long(argv[2], &a);
    if (st != ST_OK) return st;
    st = parse_long(argv[3], &b);
    if (st != ST_OK) return st;

    if (a == 0 || b == 0) return ST_ERR_ZERO;

    if (a % b == 0)
        printf("%ld is a multiple of %ld\n", a, b);
    else
        printf("%ld is NOT a multiple of %ld\n", a, b);
    return ST_OK;
}

/*-t*/

/* проверка: могут ли три числа быть сторонами прямоугольного треугольника */
static status_t cmd_t(int argc, char *argv[]) {
    if (argc != 6) return ST_ERR_ARGC;

    double eps;
    status_t st = parse_epsilon(argv[2], &eps);
    if (st != ST_OK) return st;

    double s[3];
    for (int i = 0; i < 3; ++i) {
        st = parse_double(argv[3 + i], &s[i]);
        if (st != ST_OK) return st;
    }

    for (int i = 0; i < 3; ++i) {
        if (s[i] <= eps) return ST_ERR_NEGATIVE;
    }

    /* сортируем три числа, чтобы найти гипотенузу (наибольшую сторону) */
    double x = s[0], y = s[1], z = s[2], tmp;
    if (x > y) { tmp = x; x = y; y = tmp; }
    if (y > z) { tmp = y; y = z; z = tmp; }
    if (x > y) { tmp = x; x = y; y = tmp; }

    /* z — наибольшая, x и y — катеты */
    double lhs = z * z;
    double rhs = x * x + y * y;

    if (fabs(lhs - rhs) <= eps * (lhs > 1.0 ? lhs : 1.0))
        printf("YES, right triangle (%.6g^2 = %.6g^2 + %.6g^2)\n", z, x, y);
    else
        printf("NO (%.6g^2 != %.6g^2 + %.6g^2)\n", z, x, y);
    return ST_OK;
}

/*main*/

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s -q eps a b c | -m a b | -t eps a b c\n", argv[0]);
        return 1;
    }

    const char *flag = argv[1];
    status_t st;

    if (flag_is(flag, "q")) {
        st = cmd_q(argc, argv);
    } else if (flag_is(flag, "m")) {
        st = cmd_m(argc, argv);
    } else if (flag_is(flag, "t")) {
        st = cmd_t(argc, argv);
    } else {
        fprintf(stderr, "Error: unknown flag '%s'\n", flag);
        return 2;
    }

    if (st != ST_OK) {
        switch (st) {
            case ST_ERR_ARGC:
                fprintf(stderr, "Error: wrong number of arguments\n");
                break;
            case ST_ERR_NUM_INVALID:
                fprintf(stderr, "Error: invalid number\n");
                break;
            case ST_ERR_EPS_RANGE:
                fprintf(stderr, "Error: epsilon out of range\n");
                break;
            case ST_ERR_ZERO:
                fprintf(stderr, "Error: zero is not allowed\n");
                break;
            case ST_ERR_NEGATIVE:
                fprintf(stderr, "Error: side must be positive\n");
                break;
            default:
                fprintf(stderr, "Error: unknown\n");
                break;
        }
        return 3;
    }
    return 0;
}