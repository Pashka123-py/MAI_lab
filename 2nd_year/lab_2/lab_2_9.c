#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <stddef.h>

/* ---------- Статусы выполнения ---------- */
enum status {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_BASE
};

/* Результат проверки одной дроби */
struct fraction_result {
    double fraction;
    int base;
    int has_finite; /* 1 — конечное представление, 0 — бесконечное */
};

/* ---------- Вспомогательные функции ---------- */

/* Проверка основания системы счисления */
static int is_valid_base(int base) {
    return base >= 2 && base <= 36;
}

/* Проверка, что значение лежит в диапазоне (0; 1) с учётом эпсилон */
static int is_valid_fraction(double x, double eps) {
    if (isnan(x) || isinf(x)) {
        return 0;
    }
    return x > eps && (1.0 - x) > eps;
}

/* 
 * Представление дроби x (0 < x < 1) в системе счисления base 
 * имеет конечное представление тогда и только тогда, 
 * когда знаменатель несократимой дроби является делителем 
 * некоторой степени основания. */
static int fraction_has_finite_representation(double x, int base, double eps) {
    if (!is_valid_base(base) || !is_valid_fraction(x, eps)) {
        return 0;
    }

    /* 
     * Рациональное приближение через цепные дроби.
     * Ограничиваем знаменатель 1e9 — этого достаточно для double 
     * (точность ~1e-15) и для типичных тестовых значений. */
    const long long MAX_DEN = 1000000000LL;

    long long p0 = 0, q0 = 1;
    long long p1 = 1, q1 = 0;
    double v = x;

    for (int i = 0; i < 64; ++i) {
        double a = floor(v);
        long long ai = (long long)a;

        long long p2 = ai * p1 + p0;
        long long q2 = ai * q1 + q0;

        if (q2 > MAX_DEN || q2 < 0) {
            break;
        }

        p0 = p1; q0 = q1;
        p1 = p2; q1 = q2;

        double approx = (double)p1 / (double)q1;
        if (fabs(approx - x) <= eps) {
            break;
        }

        double frac = v - a;
        if (frac <= eps) {
            break;
        }
        v = 1.0 / frac;
    }

    long long num = p1;
    long long den = q1;

    if (den <= 0 || num <= 0) {
        return 0;
    }

    /* Сокращаем дробь */
    long long a = num, b = den;
    while (b != 0) {
        long long t = a % b;
        a = b;
        b = t;
    }
    long long g = a;
    if (g > 1) {
        num /= g;
        den /= g;
    }

    /* 
     * den должно состоять только из простых множителей base.
     * Убираем из den все множители, входящие в base.
     */
    long long d = den;
    long long bbase = base;

    while (d > 1) {
        long long g2 = 1;
        long long aa = d, bb = bbase;
        while (bb != 0) {
            long long t = aa % bb;
            aa = bb;
            bb = t;
        }
        g2 = aa;

        if (g2 == 1) {
            /* оставшийся множитель не входит в base — бесконечно */
            return 0;
        }

        while (d % g2 == 0) {
            d /= g2;
        }
    }

    return 1;
}

/* ---------- Основная функция ---------- */

/*
 * Принимает основание системы счисления, эпсилон и количество дробей,
 * затем сами дроби (double).
 * Результат записывается в массив results (размер count),
 * возвращается статус выполнения.
 */
static enum status check_fractions(int base, double eps, int count,
                                   struct fraction_result *results, ...) {
    if (results == NULL || count < 0) {
        return STATUS_ERR_ARGS;
    }
    if (!is_valid_base(base)) {
        return STATUS_ERR_BASE;
    }
    if (eps <= 0.0 || isnan(eps) || isinf(eps)) {
        return STATUS_ERR_ARGS;
    }

    va_list ap;
    va_start(ap, results);

    enum status result = STATUS_OK;

    for (int i = 0; i < count; ++i) {
        double x = va_arg(ap, double);
        results[i].fraction = x;
        results[i].base = base;
        results[i].has_finite = fraction_has_finite_representation(x, base, eps);
    }

    va_end(ap);
    return result;
}

/* ---------- Демонстрация ---------- */

int main(void) {
    const double EPS = 1e-12;

    /* --- Тест 1: основание 10, несколько дробей --- */
    {
        struct fraction_result res[5];
        enum status st = check_fractions(10, EPS, 5, res,
                                         0.5, 0.25, 0.3, 0.125, 0.1);
        if (st == STATUS_OK) {
            printf("Base 10:\n");
            for (int i = 0; i < 5; ++i) {
                printf("  %.10f -> %s\n", res[i].fraction,
                       res[i].has_finite ? "finite" : "infinite");
            }
        } else {
            printf("Base 10 test failed: %d\n", (int)st);
        }
    }

    /* --- Тест 2: основание 2 --- */
    {
        struct fraction_result res[4];
        enum status st = check_fractions(2, EPS, 4, res,
                                         0.5, 0.25, 0.1, 0.75);
        if (st == STATUS_OK) {
            printf("Base 2:\n");
            for (int i = 0; i < 4; ++i) {
                printf("  %.10f -> %s\n", res[i].fraction,
                       res[i].has_finite ? "finite" : "infinite");
            }
        } else {
            printf("Base 2 test failed: %d\n", (int)st);
        }
    }

    /* --- Тест 3: основание 3 --- */
    {
        struct fraction_result res[3];
        enum status st = check_fractions(3, EPS, 3, res,
                                         0.5, 1.0 / 3.0, 1.0 / 9.0);
        if (st == STATUS_OK) {
            printf("Base 3:\n");
            for (int i = 0; i < 3; ++i) {
                printf("  %.10f -> %s\n", res[i].fraction,
                       res[i].has_finite ? "finite" : "infinite");
            }
        } else {
            printf("Base 3 test failed: %d\n", (int)st);
        }
    }

    /* --- Тест 4: основание 16 --- */
    {
        struct fraction_result res[3];
        enum status st = check_fractions(16, EPS, 3, res,
                                         0.5, 1.0 / 16.0, 0.1);
        if (st == STATUS_OK) {
            printf("Base 16:\n");
            for (int i = 0; i < 3; ++i) {
                printf("  %.10f -> %s\n", res[i].fraction,
                       res[i].has_finite ? "finite" : "infinite");
            }
        } else {
            printf("Base 16 test failed: %d\n", (int)st);
        }
    }

    /* --- Тест 5: count = 0 --- */
    {
        struct fraction_result res[1];
        enum status st = check_fractions(10, EPS, 0, res);
        printf("Count=0: status=%d\n", (int)st);
    }

    /* --- Тест 6: неверное основание --- */
    {
        struct fraction_result res[1];
        enum status st = check_fractions(1, EPS, 1, res, 0.5);
        printf("Bad base 1: status=%d\n", (int)st);
    }

    /* --- Тест 7: неверное основание (37) --- */
    {
        struct fraction_result res[1];
        enum status st = check_fractions(37, EPS, 1, res, 0.5);
        printf("Bad base 37: status=%d\n", (int)st);
    }

    /* --- Тест 8: неверный эпсилон --- */
    {
        struct fraction_result res[1];
        enum status st = check_fractions(10, -1e-12, 1, res, 0.5);
        printf("Bad eps: status=%d\n", (int)st);
    }

    return 0;
}