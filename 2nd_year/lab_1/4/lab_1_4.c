#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 4096

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_OPEN_IN,
    ST_ERR_OPEN_OUT,
    ST_ERR_WRITE,
    ST_ERR_READ,
    ST_ERR_MEM,
    ST_ERR_UNKNOWN
} status_t;

/* флаги */
typedef enum {
    ACT_D = 0,
    ACT_I,
    ACT_S,
    ACT_A
} action_t;

/* разбор флага: возвращает ST_OK и пишет действие и нужно ли явное имя */
static status_t parse_flag(const char *arg, action_t *act, int *explicit_name) {
    if (!arg || !act || !explicit_name) return ST_ERR_FLAG;
    if (arg[0] != '-' && arg[0] != '/') return ST_ERR_FLAG;

    const char *body = arg + 1;
    *explicit_name = 0;

    if (body[0] == 'n' && body[1] != '\0') {
        *explicit_name = 1;
        ++body;
    }

    if (strlen(body) != 1) return ST_ERR_FLAG;

    switch (body[0]) {
        case 'd': *act = ACT_D; return ST_OK;
        case 'i': *act = ACT_I; return ST_OK;
        case 's': *act = ACT_S; return ST_OK;
        case 'a': *act = ACT_A; return ST_OK;
        default:  return ST_ERR_FLAG;
    }
}

/* генерация имени out_<in> */
static status_t make_out_name(const char *in, char **out) {
    if (!in || !out) return ST_ERR_UNKNOWN;
    size_t n = strlen(in);
    char *buf = (char *)malloc(n + 5);
    if (!buf) return ST_ERR_MEM;
    memcpy(buf, "out_", 4);
    memcpy(buf + 4, in, n + 1);
    *out = buf;
    return ST_OK;
}

/* счёт латинских букв */
static size_t count_latin(const char *s) {
    size_t k = 0;
    for (; *s; ++s) if (isalpha((unsigned char)*s) && ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z'))) ++k;
    return k;
}

/* счёт «прочих» (не буква, не цифра, не пробел) */
static size_t count_other(const char *s) {
    size_t k = 0;
    for (; *s; ++s) {
        unsigned char c = (unsigned char)*s;
        if (c == ' ') continue;
        if (c >= '0' && c <= '9') continue;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) continue;
        ++k;
    }
    return k;
}

/* -d: удалить цифры */
static status_t act_d(FILE *in, FILE *out) {
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') continue;
        if (fputc(c, out) == EOF) return ST_ERR_WRITE;
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* -i: количество латинских букв в каждой строке */
static status_t act_i(FILE *in, FILE *out) {
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), in)) {
        if (fprintf(out, "%zu\n", count_latin(line)) < 0) return ST_ERR_WRITE;
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* -s: количество «прочих» в каждой строке */
static status_t act_s(FILE *in, FILE *out) {
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), in)) {
        if (fprintf(out, "%zu\n", count_other(line)) < 0) return ST_ERR_WRITE;
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

/* -a: заменить не-цифры на ASCII-код в hex */
static status_t act_a(FILE *in, FILE *out) {
    int c;
    while ((c = fgetc(in)) != EOF) {
        if (c >= '0' && c <= '9') {
            if (fputc(c, out) == EOF) return ST_ERR_WRITE;
        } else {
            if (fprintf(out, "%X", (unsigned)c) < 0) return ST_ERR_WRITE;
        }
    }
    if (ferror(in)) return ST_ERR_READ;
    return ST_OK;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <flag> <input> [output]\n", argv[0]);
        return 1;
    }

    action_t act;
    int explicit_name = 0;
    status_t st = parse_flag(argv[1], &act, &explicit_name);
    if (st != ST_OK) {
        fprintf(stderr, "Error: unknown flag '%s'\n", argv[1]);
        return 2;
    }

    const char *in_name = argv[2];
    char *auto_out = NULL;
    const char *out_name = NULL;

    if (explicit_name) {
        if (argc != 4) {
            fprintf(stderr, "Error: explicit output file required\n");
            return 3;
        }
        out_name = argv[3];
    } else {
        if (argc != 3) {
            fprintf(stderr, "Error: too many arguments\n");
            return 3;
        }
        st = make_out_name(in_name, &auto_out);
        if (st != ST_OK) {
            fprintf(stderr, "Error: memory\n");
            return 4;
        }
        out_name = auto_out;
    }

    FILE *fin = fopen(in_name, "r");
    if (!fin) {
        fprintf(stderr, "Error: cannot open input '%s'\n", in_name);
        free(auto_out);
        return 5;
    }

    FILE *fout = fopen(out_name, "w");
    if (!fout) {
        fprintf(stderr, "Error: cannot open output '%s'\n", out_name);
        fclose(fin);
        free(auto_out);
        return 6;
    }

    switch (act) {
        case ACT_D: st = act_d(fin, fout); break;
        case ACT_I: st = act_i(fin, fout); break;
        case ACT_S: st = act_s(fin, fout); break;
        case ACT_A: st = act_a(fin, fout); break;
        default:    st = ST_ERR_UNKNOWN;   break;
    }

    fclose(fin);

    if (fclose(fout) != 0 && st == ST_OK) st = ST_ERR_WRITE;

    if (st != ST_OK) {
        switch (st) {
            case ST_ERR_WRITE: fprintf(stderr, "Error: write failed\n"); break;
            case ST_ERR_READ:  fprintf(stderr, "Error: read failed\n");  break;
            default:           fprintf(stderr, "Error: unknown\n");      break;
        }
        free(auto_out);
        return 7;
    }

    free(auto_out);
    return 0;
}
