#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEX 4096

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_OPEN,
    ST_ERR_READ,
    ST_ERR_WRITE,
    ST_ERR_LEX_TOO_LONG,
    ST_ERR_UNKNOWN
} status_t;

/* разбор флага */
static status_t parse_flag(const char *arg, char *act) {
    if (!arg || !act) return ST_ERR_FLAG;
    if (arg[0] != '-' && arg[0] != '/') return ST_ERR_FLAG;
    if (arg[1] == '\0' || arg[2] != '\0') return ST_ERR_FLAG;
    if (arg[1] != 'r' && arg[1] != 'a') return ST_ERR_FLAG;
    *act = arg[1];
    return ST_OK;
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

/* запись строки */
static status_t write_str(FILE *f, const char *s) {
    if (fputs(s, f) == EOF) return ST_ERR_WRITE;
    return ST_OK;
}

/* запись символа */
static status_t write_ch(FILE *f, int c) {
    if (fputc(c, f) == EOF) return ST_ERR_WRITE;
    return ST_OK;
}

/* -r: чередование лексем */
static status_t cmd_r(const char *in1, const char *in2, const char *out) {
    FILE *f1 = fopen(in1, "r");
    if (!f1) return ST_ERR_OPEN;
    FILE *f2 = fopen(in2, "r");
    if (!f2) { fclose(f1); return ST_ERR_OPEN; }
    FILE *fo = fopen(out, "w");
    if (!fo) { fclose(f1); fclose(f2); return ST_ERR_OPEN; }

    char buf[MAX_LEX];
    int pos = 0;
    int e1 = 0, e2 = 0;
    int first = 1;
    status_t st = ST_OK;

    while (!e1 || !e2) {
        FILE *src = pos == 0 ? f1 : f2;
        int *ended = pos == 0 ? &e1 : &e2;

        if (*ended) {
            pos = 1 - pos;
            continue;
        }

        int r = read_lexeme(src, buf, sizeof(buf));
        if (r == -1) { st = ST_ERR_READ; break; }
        if (r == -2) { st = ST_ERR_LEX_TOO_LONG; break; }
        if (r == 0) {
            *ended = 1;
            pos = 1 - pos;
            continue;
        }

        if (!first) {
            st = write_ch(fo, ' ');
            if (st != ST_OK) break;
        }
        first = 0;
        st = write_str(fo, buf);
        if (st != ST_OK) break;

        pos = 1 - pos;
    }

    if (fclose(f1) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(f2) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(fo) != 0 && st == ST_OK) st = ST_ERR_WRITE;
    return st;
}

/* перевод числа в base 4 или 8 */
static void to_base(unsigned v, int base, char *out) {
    char tmp[64];
    int k = 0;
    if (v == 0) tmp[k++] = '0';
    while (v > 0) {
        tmp[k++] = (char)('0' + (v % (unsigned)base));
        v /= (unsigned)base;
    }
    for (int i = 0; i < k; ++i) out[i] = tmp[k - 1 - i];
    out[k] = '\0';
}

/* вывод лексемы с преобразованием */
static status_t print_lex(FILE *fo, const char *lex, int base_code, int to_lower) {
    char tmp[MAX_LEX];
    size_t n = strlen(lex);
    if (n >= sizeof(tmp)) n = sizeof(tmp) - 1;

    for (size_t i = 0; i < n; ++i) {
        char c = lex[i];
        if (to_lower && c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        tmp[i] = c;
    }
    tmp[n] = '\0';

    if (base_code == 0) {
        return write_str(fo, tmp);
    }

    for (size_t i = 0; i < n; ++i) {
        unsigned code = (unsigned char)tmp[i];
        char b[64];
        to_base(code, base_code, b);
        if (i > 0) {
            status_t st = write_ch(fo, ' ');
            if (st != ST_OK) return st;
        }
        status_t st = write_str(fo, b);
        if (st != ST_OK) return st;
    }
    return ST_OK;
}

/* -a: преобразование лексем */
static status_t cmd_a(const char *in, const char *out) {
    FILE *fi = fopen(in, "r");
    if (!fi) return ST_ERR_OPEN;
    FILE *fo = fopen(out, "w");
    if (!fo) { fclose(fi); return ST_ERR_OPEN; }

    char buf[MAX_LEX];
    long k = 0;
    status_t st = ST_OK;

    for (;;) {
        int r = read_lexeme(fi, buf, sizeof(buf));
        if (r == -1) { st = ST_ERR_READ; break; }
        if (r == -2) { st = ST_ERR_LEX_TOO_LONG; break; }
        if (r == 0) break;

        ++k;
        if (k > 1) {
            st = write_ch(fo, ' ');
            if (st != ST_OK) break;
        }

        if (k % 10 == 0) {
            st = print_lex(fo, buf, 4, 1);
        } else if (k % 2 == 0) {
            st = print_lex(fo, buf, 0, 1);
        } else if (k % 5 == 0) {
            st = print_lex(fo, buf, 8, 0);
        } else {
            st = print_lex(fo, buf, 0, 0);
        }
        if (st != ST_OK) break;
    }

    if (fclose(fi) != 0 && st == ST_OK) st = ST_ERR_READ;
    if (fclose(fo) != 0 && st == ST_OK) st = ST_ERR_WRITE;
    return st;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s -r <f1> <f2> <out> | -a <in> <out>\n", argv[0]);
        return 1;
    }

    char act;
    status_t st = parse_flag(argv[1], &act);
    if (st != ST_OK) {
        fprintf(stderr, "Error: unknown flag '%s'\n", argv[1]);
        return 2;
    }

    if (act == 'r') {
        if (argc != 5) {
            fprintf(stderr, "Error: -r needs 3 paths\n");
            return 3;
        }
        st = cmd_r(argv[2], argv[3], argv[4]);
    } else {
        if (argc != 4) {
            fprintf(stderr, "Error: -a needs 2 paths\n");
            return 3;
        }
        st = cmd_a(argv[2], argv[3]);
    }

    if (st != ST_OK) {
        switch (st) {
            case ST_ERR_OPEN:         fprintf(stderr, "Error: cannot open file\n"); break;
            case ST_ERR_READ:         fprintf(stderr, "Error: read failed\n"); break;
            case ST_ERR_WRITE:        fprintf(stderr, "Error: write failed\n"); break;
            case ST_ERR_LEX_TOO_LONG: fprintf(stderr, "Error: lexeme too long\n"); break;
            default:                  fprintf(stderr, "Error: unknown\n"); break;
        }
        return 4;
    }
    return 0;
}
