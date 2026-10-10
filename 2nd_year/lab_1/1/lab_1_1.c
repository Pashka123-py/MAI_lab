#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stddef.h>

#define H_LIMIT       100ULL
#define H_BUF_SIZE    100
#define TABLE_SIZE    10
#define E_MAX_X       10ULL
#define HEX_BUF_SIZE  32

typedef enum {
    OK = 0,
    ERR_ARGC,
    ERR_FLAG,
    ERR_NUMBER,
    ERR_OVERFLOW,
    ERR_RANGE,
    ERR_BUFFER,
    ERR_NULLPTR
} status_t;

typedef enum {
    CLASS_NEITHER,
    CLASS_PRIME,
    CLASS_COMPOSITE
} number_class_t;

static const char *status_message(status_t st)
{
    switch (st) {
    case OK:           return "OK";
    case ERR_ARGC:     return "wrong number of arguments";
    case ERR_FLAG:     return "unknown or invalid flag";
    case ERR_NUMBER:   return "x must be a natural number (integer >= 1)";
    case ERR_OVERFLOW: return "overflow: result does not fit into unsigned long long";
    case ERR_RANGE:    return "x is out of range for this flag (for -e: x <= 10)";
    case ERR_BUFFER:   return "internal error: buffer is too small";
    case ERR_NULLPTR:  return "internal error: null pointer";
    }
    return "unknown error";
}

/*argument parsing*/

status_t parse_natural(const char *str, unsigned long long *out)
{
    char *end = NULL;
    unsigned long long value;

    if (str == NULL || out == NULL) return ERR_NULLPTR;
    if (!isdigit((unsigned char)str[0])) return ERR_NUMBER;

    value = strtoull(str, &end, 10);
    if (end == str || *end != '\0') return ERR_NUMBER;
    if (value == ULLONG_MAX) return ERR_OVERFLOW;   /* переполнение (errno не используется) */
    if (value == 0) return ERR_NUMBER;

    *out = value;
    return OK;
}

status_t parse_flag(const char *str, char *out)
{
    if (str == NULL || out == NULL) return ERR_NULLPTR;
    if (strlen(str) != 2) return ERR_FLAG;
    if (str[0] != '-' && str[0] != '/') return ERR_FLAG;
    if (strchr("hpseaf", str[1]) == NULL) return ERR_FLAG;

    *out = str[1];
    return OK;
}

/*computing functions*/

/* -h: multiples of x in the range [1; limit] */
status_t find_multiples(unsigned long long x, unsigned long long limit,
                        unsigned long long *buf, size_t cap, size_t *count)
{
    size_t n = 0;
    unsigned long long m;

    if (buf == NULL || count == NULL) return ERR_NULLPTR;
    if (x == 0) return ERR_NUMBER;

    for (m = x; m <= limit; m += x) {   /* m <= limit, x <= limit: no overflow */
        if (n >= cap) return ERR_BUFFER;
        buf[n++] = m;
    }
    *count = n;
    return OK;
}

/* -p: prime / composite */
status_t classify_number(unsigned long long x, number_class_t *cls)
{
    unsigned long long i;

    if (cls == NULL) return ERR_NULLPTR;
    if (x == 0) return ERR_NUMBER;

    if (x == 1) { *cls = CLASS_NEITHER; return OK; }
    if (x < 4)  { *cls = CLASS_PRIME;   return OK; }
    if (x % 2 == 0 || x % 3 == 0) { *cls = CLASS_COMPOSITE; return OK; }

    for (i = 5; i <= x / i; i += 6) {
        if (x % i == 0 || x % (i + 2) == 0) {
            *cls = CLASS_COMPOSITE;
            return OK;
        }
    }
    *cls = CLASS_PRIME;
    return OK;
}

/* -s: digits of x in base 16, from most to least significant, into buf */
status_t to_hex_digits(unsigned long long x, char *buf, size_t cap, size_t *len)
{
    const char *digits = "0123456789ABCDEF";
    unsigned long long tmp = x;
    size_t n = 0, i;

    if (buf == NULL || len == NULL) return ERR_NULLPTR;
    if (x == 0) return ERR_NUMBER;

    while (tmp > 0) { n++; tmp /= 16; }
    if (n + 1 > cap) return ERR_BUFFER;

    for (i = n; i > 0; i--) {
        buf[i - 1] = digits[x % 16];
        x /= 16;
    }
    buf[n] = '\0';
    *len = n;
    return OK;
}

/* -e: table[b-1][e-1] = b^e, b = 1..10, e = 1..x */
status_t build_power_table(unsigned long long x,
                           unsigned long long table[][TABLE_SIZE])
{
    unsigned long long b, e, p;

    if (table == NULL) return ERR_NULLPTR;
    if (x < 1 || x > E_MAX_X) return ERR_RANGE;

    for (b = 1; b <= TABLE_SIZE; b++) {
        p = 1;
        for (e = 1; e <= x; e++) {
            p *= b;
            table[b - 1][e - 1] = p;
        }
    }
    return OK;
}

/* -a: 1 + 2 + ... + x = x(x+1)/2 */
status_t sum_natural(unsigned long long x, unsigned long long *result)
{
    unsigned long long a, b;

    if (result == NULL) return ERR_NULLPTR;
    if (x == 0) return ERR_NUMBER;
    if (x == ULLONG_MAX) return ERR_OVERFLOW;

    a = x;
    b = x + 1;
    if (a % 2 == 0) a /= 2; else b /= 2;

    if (a > ULLONG_MAX / b) return ERR_OVERFLOW;
    *result = a * b;
    return OK;
}

/* -f: x! */
status_t factorial(unsigned long long x, unsigned long long *result)
{
    unsigned long long r = 1, i;

    if (result == NULL) return ERR_NULLPTR;
    if (x == 0) return ERR_NUMBER;

    for (i = 2; i <= x; i++) {
        if (r > ULLONG_MAX / i) return ERR_OVERFLOW;
        r *= i;
    }
    *result = r;
    return OK;
}

/*output*/

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <x> <flag>\n"
                    "Flags (start with '-' or '/'): h p s e a f\n", prog);
}

static void print_multiples(const unsigned long long *buf, size_t count)
{
    size_t i;
    if (count == 0) {
        printf("There are no natural numbers up to 100 that are multiples of x\n");
        return;
    }
    for (i = 0; i < count; i++)
        printf("%llu%c", buf[i], (i + 1 < count) ? ' ' : '\n');
}

static void print_class(number_class_t cls)
{
    switch (cls) {
    case CLASS_PRIME:     printf("The number is prime\n"); break;
    case CLASS_COMPOSITE: printf("The number is composite\n"); break;
    case CLASS_NEITHER:   printf("The number is neither prime nor composite\n"); break;
    }
}

static void print_hex_digits(const char *digits, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++)
        printf("%c%c", digits[i], (i + 1 < len) ? ' ' : '\n');
}

static void print_power_table(unsigned long long x,
                              unsigned long long table[][TABLE_SIZE])
{
    unsigned long long b, e;

    printf("%6s", "base");
    for (e = 1; e <= x; e++) printf(" %12llu", e);
    printf("\n");
    for (b = 1; b <= TABLE_SIZE; b++) {
        printf("%6llu", b);
        for (e = 1; e <= x; e++) printf(" %12llu", table[b - 1][e - 1]);
        printf("\n");
    }
}

/*main*/

int main(int argc, char *argv[])
{
    unsigned long long x = 0, value = 0;
    unsigned long long multiples[H_BUF_SIZE];
    unsigned long long table[TABLE_SIZE][TABLE_SIZE];
    char hex[HEX_BUF_SIZE];
    size_t count = 0, len = 0;
    number_class_t cls;
    char flag = '\0';
    status_t st;

    if (argc != 3) {
        print_usage(argv[0]);
        fprintf(stderr, "Error: %s\n", status_message(ERR_ARGC));
        return 1;
    }

    st = parse_natural(argv[1], &x);
    if (st != OK) {
        fprintf(stderr, "Error: %s\n", status_message(st));
        return 1;
    }
    st = parse_flag(argv[2], &flag);
    if (st != OK) {
        fprintf(stderr, "Error: %s\n", status_message(st));
        return 1;
    }

    switch (flag) {
    case 'h':
        st = find_multiples(x, H_LIMIT, multiples, H_BUF_SIZE, &count);
        if (st == OK) print_multiples(multiples, count);
        break;
    case 'p':
        st = classify_number(x, &cls);
        if (st == OK) print_class(cls);
        break;
    case 's':
        st = to_hex_digits(x, hex, sizeof hex, &len);
        if (st == OK) print_hex_digits(hex, len);
        break;
    case 'e':
        st = build_power_table(x, table);
        if (st == OK) print_power_table(x, table);
        break;
    case 'a':
        st = sum_natural(x, &value);
        if (st == OK) printf("%llu\n", value);
        break;
    case 'f':
        st = factorial(x, &value);
        if (st == OK) printf("%llu\n", value);
        break;
    default:
        st = ERR_FLAG;
        break;
    }

    if (st != OK) {
        fprintf(stderr, "Error: %s\n", status_message(st));
        return 1;
    }
    return 0;
}
