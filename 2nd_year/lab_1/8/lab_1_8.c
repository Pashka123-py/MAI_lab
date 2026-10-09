#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LEX 4096

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_OPEN_IN,
    ST_ERR_OPEN_OUT,
    ST_ERR_READ,
    ST_ERR_WRITE,
    ST_ERR_BAD_DIGIT,
    ST_ERR_LEX_TOO_LONG,
    ST_ERR_OVERFLOW,
    ST_ERR_UNKNOWN
} status_t;

/* значение цифры */
static int digit_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
    if (c >= 'a' && c <= 'z') return c - 'a' + 10;
    return -1;
}

/* чтение лексемы: 1 - ok, 0 - eof, -1 - error, -2 - too long */
static int read_lexeme(FILE *f, char *buf, size_t cap) {
    int c;
    do {
        c = fgetc(f);
        if (c == EOF) {
            if (ferror(f)) return -1;
            return 0;
        }
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    size_t i = 0;
    while (c != EOF && c != ' ' && c != '\t' && c != '\n' && c != '\r') {
        if (i + 1 >= cap) {
            while (c != EOF && c != ' ' && c != '\t' && c != '\n' && c != '\r')
                c = fgetc(f);
            return -2;
        }
        buf[i++] = (char)c;
        c = fgetc(f);
    }
    buf[i] = '\0';
    return 1;
}

/* минимальное основание для строки s (уже без ведущих нулей) */
static status_t min_base(const char *s, int *base_out, int *max_digit_out) {
    if (!s || !base_out || !max_digit_out) return ST_ERR_UNKNOWN;
    if (*s == '\0') return ST_ERR_BAD_DIGIT;

    int maxd = 0;
    for (const char *p = s; *p; ++p) {
        int d = digit_val(*p);
        if (d < 0) return ST_ERR_BAD_DIGIT;
        if (d > maxd) maxd = d;
    }
    int b = maxd + 1;
    if (b < 2) b = 2;
    *base_out = b;
    *max_digit_out = maxd;
    return ST_OK;
}

/* убрать ведущие нули, оставить хотя бы один '0' */
static void strip_leading_zeros(const char *in, char *out, size_t cap) {
    const char *p = in;
    while (*p == '0') ++p;
    if (*p == '\0') {
        if (cap >= 2) { out[0] = '0'; out[1] = '\0'; }
        else if (cap >= 1) out[0] = '\0';
        return;
    }
    size_t n = strlen(p);
    if (n + 1 > cap) n = cap - 1;
    memcpy(out, p, n);
    out[n] = '\0';
}

/* перевод строки s в base b в 10-чную */
static status_t to_decimal(const char *s, int base, unsigned long long *out) {
    if (!s || !out) return ST_ERR_UNKNOWN;
    unsigned long long v = 0;
    for (const char *p = s; *p; ++p) {
        int d = digit_val(*p);
        if (d < 0 || d >= base) return ST_ERR_BAD_DIGIT;
        if (v > (ULLONG_MAX - (unsigned)d) / (unsigned)base) return ST_ERR_OVERFLOW;
        v = v * (unsigned long long)base + (unsigned long long)d;
    }
    *out = v;
    return ST_OK;
}

/* обработка одного токена */
static status_t process_token(FILE *fo, const char *tok) {
    char clean[MAX_LEX];
    strip_leading_zeros(tok, clean, sizeof(clean));

    int base = 0, maxd = 0;
    status_t st = min_base(clean, &base, &maxd);
    if (st != ST_OK) return st;

    unsigned long long dec = 0;
    st = to_decimal(clean, base, &dec);
    if (st != ST_OK) return st;

    if (fprintf(fo, "%s %d %llu\n", clean, base, dec) < 0)
        return ST_ERR_WRITE;
    return ST_OK;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input> <output>\n", argv[0]);
        return 1;
    }

    FILE *fi = fopen(argv[1], "r");
    if (!fi) {
        fprintf(stderr, "Error: cannot open input '%s'\n", argv[1]);
        return 2;
    }
    FILE *fo = fopen(argv[2], "w");
    if (!fo) {
        fprintf(stderr, "Error: cannot open output '%s'\n", argv[2]);
        fclose(fi);
        return 3;
    }

    char buf[MAX_LEX];
    status_t st = ST_OK;

    for (;;) {
        int r = read_lexeme(fi, buf, sizeof(buf));
        if (r == -1) { st = ST_ERR_READ; break; }
        if (r == -2) { st = ST_ERR_LEX_TOO_LONG; break; }
        if (r == 0) break;

        st = process_token(fo, buf);
        if (st != ST_OK) break;
    }

    if (fclose(fi) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(fo) != 0 && st == ST_OK) st = ST_ERR_WRITE;

    if (st != ST_OK) {
        switch (st) {
            case ST_ERR_READ:          fprintf(stderr, "Error: read failed\n"); break;
            case ST_ERR_WRITE:         fprintf(stderr, "Error: write failed\n"); break;
            case ST_ERR_BAD_DIGIT:     fprintf(stderr, "Error: bad digit\n"); break;
            case ST_ERR_LEX_TOO_LONG:  fprintf(stderr, "Error: lexeme too long\n"); break;
            case ST_ERR_OVERFLOW:      fprintf(stderr, "Error: overflow\n"); break;
            default:                   fprintf(stderr, "Error: unknown\n"); break;
        }
        return 4;
    }
    return 0;
}