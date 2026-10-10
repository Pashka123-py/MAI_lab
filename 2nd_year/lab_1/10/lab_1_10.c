#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_LINE 4096

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_BASE,
    ST_ERR_NUM,
    ST_ERR_DIGIT,
    ST_ERR_OVERFLOW,
    ST_ERR_MEM,
    ST_ERR_UNKNOWN
} status_t;

/* значение цифры: 0-9A-Z, только прописные */
static int digit_val_strict(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    return -1;
}

/* разбор числа в base со знаком; используется long long */
static status_t parse_signed(const char *s, int base, long long *out) {
    if (!s || !out || *s == '\0') return ST_ERR_NUM;
    if (base < 2 || base > 36) return ST_ERR_BASE;

    int neg = 0;
    const char *p = s;
    if (*p == '-') { neg = 1; ++p; }
    else if (*p == '+') { ++p; }
    if (*p == '\0') return ST_ERR_NUM;

    unsigned long long v = 0;
    unsigned long long limit = neg
        ? (unsigned long long)LLONG_MAX + 1ULL
        : (unsigned long long)LLONG_MAX;

    for (; *p; ++p) {
        int d = digit_val_strict(*p);
        if (d < 0) return ST_ERR_DIGIT;
        if (d >= base) return ST_ERR_DIGIT;
        if (v > (limit - (unsigned long long)d) / (unsigned long long)base)
            return ST_ERR_OVERFLOW;
        v = v * (unsigned long long)base + (unsigned long long)d;
    }

    if (neg) {
        if (v == (unsigned long long)LLONG_MAX + 1ULL) *out = LLONG_MIN;
        else *out = -(long long)v;
    } else {
        *out = (long long)v;
    }
    return ST_OK;
}

/* запись числа (со знаком) в base */
static void to_base_str(long long v, int base, char *buf, size_t cap) {
    char tmp[80];
    int k = 0;
    int neg = v < 0;
    unsigned long long u;
    if (neg) u = (unsigned long long)(-(v + 1)) + 1ULL;
    else     u = (unsigned long long)v;

    if (u == 0) tmp[k++] = '0';
    while (u > 0) {
        int d = (int)(u % (unsigned long long)base);
        tmp[k++] = (char)(d < 10 ? '0' + d : 'A' + d - 10);
        u /= (unsigned long long)base;
    }
    if (neg) tmp[k++] = '-';

    size_t j = 0;
    while (k > 0 && j + 1 < cap) buf[j++] = tmp[--k];
    buf[j] = '\0';
}

/* проверка переполнения при сложении */
static status_t add_ll(long long a, long long b, long long *out) {
    if (b > 0 && a > LLONG_MAX - b) return ST_ERR_OVERFLOW;
    if (b < 0 && a < LLONG_MIN - b) return ST_ERR_OVERFLOW;
    *out = a + b;
    return ST_OK;
}

/* модуль числа (для LLONG_MIN — особая аккуратность) */
static unsigned long long abs_ll(long long v) {
    if (v < 0) return (unsigned long long)(-(v + 1)) + 1ULL;
    return (unsigned long long)v;
}

/* убрать ведущие нули, знак сохраняем */
static void strip_zeros(const char *s, char *out, size_t cap) {
    int neg = 0;
    if (*s == '-') { neg = 1; ++s; }
    while (*s == '0') ++s;
    if (*s == '\0') s = "0";

    size_t need = strlen(s) + (size_t)neg + 1;
    if (need > cap) need = cap;
    size_t j = 0;
    if (neg && j + 1 < cap) out[j++] = '-';
    while (*s && j + 1 < cap) out[j++] = *s++;
    out[j] = '\0';
}

int main(void) {
    char line[MAX_LINE];

    /* ввод базы */
    printf("Enter base (2..36): ");
    if (!fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "Error: no input\n");
        return 1;
    }
    size_t L = strlen(line);
    while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) line[--L] = '\0';

    char *end = NULL;
    long base = strtol(line, &end, 10);
    if (end == line || *end != '\0' || base == LONG_MAX || base == LONG_MIN) {
        fprintf(stderr, "Error: invalid base\n");
        return 2;
    }
    if (base < 2 || base > 36) {
        fprintf(stderr, "Error: base must be in [2..36]\n");
        return 2;
    }

    long long sum = 0;
    long long best = 0;
    unsigned long long best_abs = 0;
    int have_best = 0;
    long count = 0;

    printf("Enter numbers in base %ld (\"Stop\" to finish):\n", base);

    while (fgets(line, sizeof(line), stdin)) {
        L = strlen(line);
        while (L > 0 && (line[L-1] == '\n' || line[L-1] == '\r')) line[--L] = '\0';

        if (strcmp(line, "Stop") == 0) break;
        if (L == 0) continue;  /* пустая строка — пропускаем */

        long long v;
        status_t st = parse_signed(line, (int)base, &v);
        if (st != ST_OK) {
            switch (st) {
                case ST_ERR_DIGIT:
                    fprintf(stderr, "Error: invalid digit in '%s'\n", line);
                    break;
                case ST_ERR_OVERFLOW:
                    fprintf(stderr, "Error: number too large: '%s'\n", line);
                    break;
                default:
                    fprintf(stderr, "Error: invalid number '%s'\n", line);
                    break;
            }
            continue;   /* пропускаем плохой ввод, не выходим */
        }

        status_t as = add_ll(sum, v, &sum);
        if (as != ST_OK) {
            fprintf(stderr, "Error: sum overflow\n");
            return 3;
        }

        unsigned long long av = abs_ll(v);
        if (!have_best || av > best_abs) {
            best_abs = av;
            best = v;
            have_best = 1;
        }
        ++count;
    }

    if (!have_best) {
        printf("No numbers entered.\n");
        return 0;
    }

    printf("\nEntered %ld number(s).\n", count);

    /* максимум по модулю */
    char buf[80];
    to_base_str(best, (int)base, buf, sizeof(buf));
    char clean[80];
    strip_zeros(buf, clean, sizeof(clean));

    printf("\nMax by abs value:\n");
    printf("  base %2ld : %s\n", base, clean);
    printf("  base 10 : %lld\n", best);
    printf("  base  9 : ", 0L);
    to_base_str(best, 9, buf, sizeof(buf));  printf("%s\n", buf);
    printf("  base 18 : ");
    to_base_str(best, 18, buf, sizeof(buf)); printf("%s\n", buf);
    printf("  base 27 : ");
    to_base_str(best, 27, buf, sizeof(buf)); printf("%s\n", buf);
    printf("  base 36 : ");
    to_base_str(best, 36, buf, sizeof(buf)); printf("%s\n", buf);

    /* сумма */
    printf("\nSum:\n");
    to_base_str(sum, (int)base, buf, sizeof(buf));
    strip_zeros(buf, clean, sizeof(clean));
    printf("  base %2ld : %s\n", base, clean);
    printf("  base 10 : %lld\n", sum);
    printf("  base  9 : ");
    to_base_str(sum, 9, buf, sizeof(buf));  printf("%s\n", buf);
    printf("  base 18 : ");
    to_base_str(sum, 18, buf, sizeof(buf)); printf("%s\n", buf);
    printf("  base 27 : ");
    to_base_str(sum, 27, buf, sizeof(buf)); printf("%s\n", buf);
    printf("  base 36 : ");
    to_base_str(sum, 36, buf, sizeof(buf)); printf("%s\n", buf);

    return 0;
}
