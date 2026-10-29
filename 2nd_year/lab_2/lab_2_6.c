#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

/* ---------- Контекст источника (поток или строка) ---------- */

struct src {
    FILE *stream;       /* != NULL для overscanf */
    const char *str;    /* != NULL для oversscanf */
    size_t pos;         /* позиция в строке для oversscanf */
};

/* Чтение одного символа из источника; возвращает -1 при EOF */
static int src_getc(struct src *s) {
    if (s->stream != NULL) {
        return fgetc(s->stream);
    }
    char c = s->str[s->pos];
    if (c == '\0') {
        return -1;
    }
    s->pos++;
    return (unsigned char)c;
}

/* Возврат одного символа в источник (только для строки).
   Для потока используем ungetc. */
static void src_ungetc(struct src *s, int c) {
    if (c == -1) {
        return;
    }
    if (s->stream != NULL) {
        ungetc(c, s->stream);
    } else {
        if (s->pos > 0) {
            s->pos--;
        }
    }
}

/* Пропуск пробельных символов */
static void src_skip_ws(struct src *s) {
    int c;
    while ((c = src_getc(s)) != -1) {
        if (!isspace((unsigned char)c)) {
            src_ungetc(s, c);
            return;
        }
    }
}

/* ---------- Считывание токена до разделителя ---------- */

/* Считывает последовательность символов, пока is_token_char(c) истинно.
   Возвращает длину (0, если ничего не прочитано). Буфер завершается '\0'. */
static size_t read_token(struct src *s, char *buf, size_t buf_size,
                         int (*is_token_char)(int)) {
    size_t len = 0;
    int c;
    while ((c = src_getc(s)) != -1) {
        if (!is_token_char(c)) {
            src_ungetc(s, c);
            break;
        }
        if (len + 1 < buf_size) {
            buf[len++] = (char)c;
        } else {
            /* переполнение: просто пропускаем дальше */
        }
    }
    buf[len] = '\0';
    return len;
}

/* Предикаты для токенов */
static int is_roman_char(int c) {
    return c == 'I' || c == 'V' || c == 'X' || c == 'L' || c == 'C' || c == 'D' || c == 'M';
}
static int is_bin_char(int c) {
    return c == '0' || c == '1';
}
static int is_alnum_char(int c) {
    return isalnum((unsigned char)c);
}

/* ---------- Парсеры ---------- */

/* Римские цифры -> int. Возвращает 0 при ошибке, 1 при успехе. */
static int parse_roman(const char *s, int *out) {
    if (s[0] == '\0') return 0;
    int total = 0;
    int prev = 0;
    size_t len = strlen(s);
    for (size_t i = len; i > 0; --i) {
        int v = 0;
        switch (s[i - 1]) {
        case 'I': v = 1; break;
        case 'V': v = 5; break;
        case 'X': v = 10; break;
        case 'L': v = 50; break;
        case 'C': v = 100; break;
        case 'D': v = 500; break;
        case 'M': v = 1000; break;
        default: return 0;
        }
        if (v < prev) {
            total -= v;
        } else {
            total += v;
            prev = v;
        }
    }
    *out = total;
    return 1;
}

/* Фибоначчиева система (0/1, младший -> старший, завершающая 1) -> unsigned int.
   Возвращает 0 при ошибке, 1 при успехе. */
static int parse_zeckendorf(const char *s, unsigned int *out) {
    size_t len = strlen(s);
    if (len == 0) return 0;
    if (s[len - 1] != '1') return 0;
    /* fib[0]=1, fib[1]=2, fib[2]=3, ... */
    unsigned int fib[64];
    int n = 0;
    fib[0] = 1;
    fib[1] = 2;
    n = 2;
    while (n < 63 && fib[n - 1] + fib[n - 2] > fib[n - 2]) {
        fib[n] = fib[n - 1] + fib[n - 2];
        ++n;
        if (n > 40) break; /* защита */
    }
    unsigned int result = 0;
    size_t idx = 0;
    /* s записана младший -> старший. Но fib[0]=1 — это коэффициент при 1,
       fib[1]=2 — при 2, и т.д. Значит s[0] соответствует fib[0], s[1] — fib[1].
       Завершающая '1' — маркер, не коэффициент. */
    size_t digits = len - 1; /* без завершающей 1 */
    for (size_t i = 0; i < digits; ++i) {
        if (s[i] == '1') {
            if ((int)i >= n) return 0;
            result += fib[i];
        } else if (s[i] != '0') {
            return 0;
        }
        (void)idx;
    }
    *out = result;
    return 1;
}

/* Строка в системе счисления base -> int. upper игнорируется (принимаем оба регистра). */
static int parse_base(const char *s, int base, int *out) {
    if (base < 2 || base > 36) base = 10;
    size_t len = strlen(s);
    if (len == 0) return 0;
    int neg = 0;
    size_t i = 0;
    if (s[0] == '-') { neg = 1; i = 1; }
    else if (s[0] == '+') { i = 1; }
    if (i >= len) return 0;
    long long result = 0;
    for (; i < len; ++i) {
        int c = (unsigned char)s[i];
        int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
        else return 0;
        if (d >= base) return 0;
        result = result * base + d;
    }
    *out = neg ? (int)(-result) : (int)result;
    return 1;
}

/* ---------- Основной обработчик ---------- */

/* Возвращает число успешно прочитанных полей. */
static int scan_impl(struct src *src, const char *fmt, va_list ap) {
    int assigned = 0;
    const char *p = fmt;

    while (*p) {
        if (isspace((unsigned char)*p)) {
            src_skip_ws(src);
            ++p;
            continue;
        }
        if (*p != '%') {
            /* литерал: должен совпасть с входным символом */
            int c = src_getc(src);
            if (c == -1) {
                return assigned;
            }
            if (c != (unsigned char)*p) {
                src_ungetc(src, c);
                return assigned;
            }
            ++p;
            continue;
        }

        /* %% */
        if (p[1] == '%') {
            int c = src_getc(src);
            if (c == -1) return assigned;
            if (c != '%') {
                src_ungetc(src, c);
                return assigned;
            }
            p += 2;
            continue;
        }

        /* Проверяем кастомные флаги */
        if (strncmp(p, "%Ro", 3) == 0) {
            src_skip_ws(src);
            char token[128];
            if (read_token(src, token, sizeof(token), is_roman_char) == 0) {
                return assigned;
            }
            int *out = va_arg(ap, int *);
            int v;
            if (!parse_roman(token, &v)) return assigned;
            if (out) *out = v;
            ++assigned;
            p += 3;
            continue;
        }
        if (strncmp(p, "%Zr", 3) == 0) {
            src_skip_ws(src);
            char token[128];
            if (read_token(src, token, sizeof(token), is_bin_char) == 0) {
                return assigned;
            }
            unsigned int *out = va_arg(ap, unsigned int *);
            unsigned int v;
            if (!parse_zeckendorf(token, &v)) return assigned;
            if (out) *out = v;
            ++assigned;
            p += 3;
            continue;
        }
        if (strncmp(p, "%Cv", 3) == 0 || strncmp(p, "%CV", 3) == 0) {
            src_skip_ws(src);
            char token[128];
            if (read_token(src, token, sizeof(token), is_alnum_char) == 0) {
                return assigned;
            }
            int *out = va_arg(ap, int *);
            int base = va_arg(ap, int);
            int v;
            if (!parse_base(token, base, &v)) return assigned;
            if (out) *out = v;
            ++assigned;
            p += 3;
            continue;
        }

        /* Стандартный спецификатор: считываем его целиком (до буквы-преобразователя)
           и формируем один спецификатор для fscanf/sscanf */
        size_t slen = 1;
        /* флаги */
        while (p[slen] && strchr("*", p[slen])) ++slen;
        /* ширина */
        while (p[slen] && isdigit((unsigned char)p[slen])) ++slen;
        /* длина */
        while (p[slen] && strchr("hlLqjzt", p[slen])) ++slen;
        /* преобразователь */
        if (p[slen]) ++slen;

        if (slen < 2 || slen > 32) {
            /* некорректный спецификатор — выходим */
            return assigned;
        }

        char specbuf[33];
        memcpy(specbuf, p, slen);
        specbuf[slen] = '\0';

        char conv = specbuf[slen - 1];
        int n = 0;

        if (conv == 'd' || conv == 'i' || conv == 'o' || conv == 'u' ||
            conv == 'x' || conv == 'X') {
            int *v = va_arg(ap, int *);
            if (src->stream != NULL) {
                n = fscanf(src->stream, specbuf, v);
            } else {
                n = sscanf(src->str + src->pos, specbuf, v);
                /* скорректировать позицию: sscanf не сообщает, сколько прочитал.
                   Приблизительно: пробелы + токен. */
                if (n == 1) {
                    while (src->str[src->pos] && isspace((unsigned char)src->str[src->pos])) ++src->pos;
                    while (src->str[src->pos] && !isspace((unsigned char)src->str[src->pos])) ++src->pos;
                }
            }
        } else if (conv == 'f' || conv == 'e' || conv == 'E' ||
                   conv == 'g' || conv == 'G' || conv == 'a' || conv == 'A') {
            float *v = va_arg(ap, float *);
            if (src->stream != NULL) {
                n = fscanf(src->stream, specbuf, v);
            } else {
                n = sscanf(src->str + src->pos, specbuf, v);
                if (n == 1) {
                    while (src->str[src->pos] && isspace((unsigned char)src->str[src->pos])) ++src->pos;
                    while (src->str[src->pos] && !isspace((unsigned char)src->str[src->pos])) ++src->pos;
                }
            }
        } else if (conv == 's') {
            char *v = va_arg(ap, char *);
            if (src->stream != NULL) {
                n = fscanf(src->stream, specbuf, v);
            } else {
                n = sscanf(src->str + src->pos, specbuf, v);
                if (n == 1) {
                    while (src->str[src->pos] && isspace((unsigned char)src->str[src->pos])) ++src->pos;
                    while (src->str[src->pos] && !isspace((unsigned char)src->str[src->pos])) ++src->pos;
                }
            }
        } else if (conv == 'c') {
            char *v = va_arg(ap, char *);
            if (src->stream != NULL) {
                n = fscanf(src->stream, specbuf, v);
            } else {
                n = sscanf(src->str + src->pos, specbuf, v);
                if (n == 1) src->pos += 1;
            }
        } else if (conv == 'p') {
            void **v = va_arg(ap, void **);
            if (src->stream != NULL) {
                n = fscanf(src->stream, specbuf, v);
            } else {
                n = sscanf(src->str + src->pos, specbuf, v);
                if (n == 1) {
                    while (src->str[src->pos] && isspace((unsigned char)src->str[src->pos])) ++src->pos;
                    while (src->str[src->pos] && !isspace((unsigned char)src->str[src->pos])) ++src->pos;
                }
            }
        } else {
            /* неизвестный преобразователь — останавливаемся */
            return assigned;
        }

        if (n != 1) {
            return assigned;
        }
        ++assigned;
        p += slen;
    }
    return assigned;
}

/* ---------- Публичные функции ---------- */

int overscanf(FILE *stream, const char *format, ...) {
    if (stream == NULL || format == NULL) return -1;
    struct src s;
    s.stream = stream;
    s.str = NULL;
    s.pos = 0;

    va_list ap;
    va_start(ap, format);
    int r = scan_impl(&s, format, ap);
    va_end(ap);
    return r;
}

int oversscanf(const char *str, const char *format, ...) {
    if (str == NULL || format == NULL) return -1;
    struct src s;
    s.stream = NULL;
    s.str = str;
    s.pos = 0;

    va_list ap;
    va_start(ap, format);
    int r = scan_impl(&s, format, ap);
    va_end(ap);
    return r;
}

/* ---------- Демонстрация ---------- */

int main(void) {
    /* --- oversscanf: стандартные флаги --- */
    {
        int a, b;
        char s[64];
        double d;
        char c;
        int n = oversscanf("42 100 hello 3.14 X",
                           "%d %d %s %lf %c", &a, &b, s, &d, &c);
        over_test: ;
        printf("Standard sscanf: n=%d a=%d b=%d s='%s' d=%.2f c='%c'\n",
               n, a, b, s, d, c);
    }

    /* --- oversscanf: %Ro --- */
    {
        int r1 = 0, r2 = 0, r3 = 0;
        int n = oversscanf("XIV MCMXCIV IV", "%Ro %Ro %Ro", &r1, &r2, &r3);
        printf("Ro sscanf: n=%d r1=%d r2=%d r3=%d\n", n, r1, r2, r3);
    }

    /* --- oversscanf: %Zr --- */
    {
        unsigned int z1 = 0, z2 = 0, z3 = 0;
        int n = oversscanf("11 101 1001", "%Zr %Zr %Zr", &z1, &z2, &z3);
        printf("Zr sscanf: n=%d z1=%u z2=%u z3=%u\n", n, z1, z2, z3);
    }

    /* --- oversscanf: %Cv / %CV --- */
    {
        int c1 = 0, c2 = 0, c3 = 0, c4 = 0;
        int n = oversscanf("ff 11111111 FF 73",
                           "%Cv %Cv %CV %CV", &c1, 16, &c2, 2, &c3, 16, &c4, 36);
        printf("Cv sscanf: n=%d c1=%d c2=%d c3=%d c4=%d\n", n, c1, c2, c3, c4);
    }

    /* --- overscanf через временный файл --- */
    {
        FILE *tmp = tmpfile();
        if (tmp) {
            fputs("42 XIV 1010 FF hello\n", tmp);
            rewind(tmp);

            int a = 0;
            int r = 0;
            unsigned int z = 0;
            int cv = 0;
            int cV = 0;
            char word[32];

            int n = overscanf(tmp, "%d %Ro %Zr %Cv %CV %s",
                              &a, &r, &z, &cv, 16, &cV, 16, word);
            printf("Standard fscanf: n=%d a=%d r=%d z=%u cv=%d cV=%d word='%s'\n",
                   n, a, r, z, cv, cV, word);
            fclose(tmp);
        }
    }

    return 0;
}