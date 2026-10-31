#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

typedef enum {
    SUCCESS = 0,
    ERROR_NULL_POINTER,
    ERROR_INVALID_ARGUMENT,
    ERROR_OUT_OF_MEMORY
} status;

static status shift_polynomial(double epsilon, double a, double **out_coeffs, int n, ...) {
    if (!out_coeffs || n < 0 || epsilon <= 0.0) {
        return ERROR_INVALID_ARGUMENT;
    }
    *out_coeffs = NULL;

    double *g = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (!g) {
        return ERROR_OUT_OF_MEMORY;
    }

    double *f = (double *)malloc((size_t)(n + 1) * sizeof(double));
    if (!f) {
        free(g);
        return ERROR_OUT_OF_MEMORY;
    }

    va_list ap;
    va_start(ap, n);
    for (int i = 0; i <= n; ++i) {
        f[i] = va_arg(ap, double);
    }
    va_end(ap);

    /* Вычисляем g_k = f^(k)(a) / k! */
    for (int k = 0; k <= n; ++k) {
        double val = 0.0;
        for (int i = k; i <= n; ++i) {
            /* Вычисляем биномиальный коэффициент C(i, k) */
            double c = 1.0;
            for (int j = 1; j <= k; ++j) {
                c = c * (i - k + j) / j;
            }
            val += c * f[i] * pow(a, i - k);
        }
        if (fabs(val) < epsilon) {
            val = 0.0;
        }
        g[k] = val;
    }

    free(f);
    *out_coeffs = g;
    return SUCCESS;
}

static void print_shifted_poly(const double *g, int n, double a) {
    printf("g(x-a) = ");
    int printed = 0;

    char sign = (a < 0) ? '+' : '-';
    double abs_a = fabs(a);

    for (int i = 0; i <= n; ++i) {
        if (g[i] == 0.0) {
            continue;
        }

        if (printed > 0 && g[i] > 0.0) {
            printf("+ ");
        }

        if (i == 0) {
            printf("%g ", g[i]);
        } else {
            if (abs_a == 0.0) {
                printf("%g*x^%d ", g[i], i);
            } else {
                printf("%g*(x %c %g)^%d ", g[i], sign, abs_a, i);
            }
        }
        printed++;
    }
    if (printed == 0) {
        printf("0");
    }
    printf("\n");
}

int main(void) {
    double epsilon = 1e-9;
    double a = -1.0;
    int n = 2;
    double *coeffs = NULL;

    printf("Initial polynomial: f(x) = 1 + 2*x + 1*x^2\n");
    printf("Shift value a = %g\n", a);

    status st = shift_polynomial(epsilon, a, &coeffs, n, 1.0, 2.0, 1.0);

    if (st == SUCCESS) {
        print_shifted_poly(coeffs, n, a);
        free(coeffs);
    } else {
        printf("Error: status code %d\n", (int)st);
    }

    printf("\nInitial polynomial: f(x) = -1 + 5*x - 3*x^2 + 2*x^3\n");
    a = 1.0;
    n = 3;
    st = shift_polynomial(epsilon, a, &coeffs, n, -1.0, 5.0, -3.0, 2.0);
    if (st == SUCCESS) {
        print_shifted_poly(coeffs, n, a);
        free(coeffs);
    } else {
        printf("Error: status code %d\n", (int)st);
    }

        /* Тест 3: a = 0 — многочлен не должен измениться */
    printf("\nInitial polynomial: f(x) = 7 - 2*x + 4*x^2\n");
    a = 0.0;
    n = 2;
    st = shift_polynomial(epsilon, a, &coeffs, n, 7.0, -2.0, 4.0);
    if (st == SUCCESS) {
        print_shifted_poly(coeffs, n, a);
        free(coeffs);
    } else {
        printf("Error: status code %d\n", (int)st);
    }

    /* Тест 4: дробное a = 0.5 */
    printf("\nInitial polynomial: f(x) = 2*x^2 - 3*x + 1\n");
    a = 0.5;
    n = 2;
    st = shift_polynomial(epsilon, a, &coeffs, n, 1.0, -3.0, 2.0);
    if (st == SUCCESS) {
        print_shifted_poly(coeffs, n, a);
        free(coeffs);
    } else {
        printf("Error: status code %d\n", (int)st);
    }

    return 0;
}