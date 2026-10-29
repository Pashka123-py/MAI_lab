#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

/* ---------- Вспомогательные буферы ---------- */

struct sbuf {
    char *data;
    size_t len;
    size_t cap;
    int error;
};

static void sbuf_init(struct sbuf *b) {
    b->cap = 128;
    b->len = 0;
    b->error = 0;
    b->data = malloc(b->cap);
    if (b->data == NULL) {
        b->error = 1;
        b->cap = 0;
    } else {
        b->data[0] = '\0';
    }
}

static void sbuf_free(struct sbuf *b) {
    free(b->data);
    b->data = NULL;
}

static void sbuf_reserve(struct sbuf *b, size_t need) {
    if (b->error) {
        return;
    }
    if (b->len + need + 1 > b->cap) {
        size_t new_cap = b->cap ? b->cap : 128;
        while (new_cap < b->len + need + 1) {
            new_cap *= 2;
        }
        char *tmp = realloc(b->data, new_cap);
        if (tmp == NULL) {
            b->error = 1;
            return;
        }
        b->data = tmp;
        b->cap = new_cap;
    }
}

static void sbuf_putc(struct sbuf *b, char c) {
    sbuf_reserve(b, 1);
    if (b->error) return;
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

static void sbuf_puts(struct sbuf *b, const char *s) {
    size_t n = strlen(s);
    sbuf_reserve(b, n);
    if (b->error) return;
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

/* ---------- Кастомные преобразования ---------- */

/* Римские цифры для int (0..3999), иначе "0" */
static void to_roman(int value, char *out, size_t out_size) {
    static const int vals[] = {1000,900,500,400,100,90,50,40,10,9,5,4,1};
    static const char *syms[] = {"M","CM","D","CD","C","XC","L","XL","X","IX","V","IV","I"};
    if (value <= 0 || value > 3999) {
        snprintf(out, out_size, "%d", value);
        return;
    }
    size_t pos = 0;
    for (int i = 0; i < 13 && value > 0; ++i) {
        while (value >= vals[i]) {
            size_t l = strlen(syms[i]);
            if (pos + l + 1 < out_size) {
                memcpy(out + pos, syms[i], l);
                pos += l;
            }
            value -= vals[i];
        }
    }
    out[pos] = '\0';
}

/* Число в фибоначчиевой системе: коэффициенты 0/1, младший -> старший, в конце 1 */
static void to_zeckendorf(unsigned int value, char *out, size_t out_size) {
    if (value == 0) {
        snprintf(out, out_size, "0");
        return;
    }
    unsigned int fib[64];
    int n = 0;
    fib[0] = 1;
    fib[1] = 2;
    n = 2;
    while (fib[n - 1] + fib[n - 2] <= value && n < 63) {
        fib[n] = fib[n - 1] + fib[n - 2];
        ++n;
    }
    unsigned int rem = value;
    char digits[64];
    int k = 0;
    for (int i = n - 1; i >= 0; --i) {
        if (fib[i] <= rem) {
            digits[k++] = '1';
            rem -= fib[i];
        } else {
            digits[k++] = '0';
        }
    }
    /* digits сейчас от старшего к младшему; переворачиваем */
    size_t pos = 0;
    for (int i = k - 1; i >= 0 && pos + 1 < out_size; --i) {
        out[pos++] = digits[i];
    }
    if (pos + 1 < out_size) {
        out[pos++] = '1'; /* завершающая единица */
    }
    out[pos] = '\0';
}

/* Перевод числа в систему счисления base (2..36), upper=1 -> заглавные */
static void to_base(long long value, int base, int upper, char *out, size_t out_size) {
    if (base < 2 || base > 36) base = 10;
    const char *digits = upper ? "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               : "0123456789abcdefghijklmnopqrstuvwxyz";
    unsigned long long uv;
    int neg = 0;
    if (value < 0) {
        neg = 1;
        uv = (unsigned long long)(-(value + 1)) + 1;
    } else {
        uv = (unsigned long long)value;
    }
    char tmp[128];
    int k = 0;
    if (uv == 0) tmp[k++] = '0';
    while (uv > 0 && k < (int)sizeof(tmp) - 1) {
        tmp[k++] = digits[uv % (unsigned)base];
        uv /= (unsigned)base;
    }
    size_t pos = 0;
    if (neg && pos + 1 < out_size) out[pos++] = '-';
    for (int i = k - 1; i >= 0 && pos + 1 < out_size; --i) {
        out[pos++] = tmp[i];
    }
    out[pos] = '\0';
}

/* Перевод строки из системы счисления base в десятичное (long long) */
static long long from_base(const char *s, int base, int upper) {
    if (base < 2 || base > 36) base = 10;
    long long result = 0;
    int neg = 0;
    const char *p = s;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (*p == '-') { neg = 1; ++p; }
    else if (*p == '+') { ++p; }
    for (; *p; ++p) {
        int c = (unsigned char)*p;
        int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'z') d = c - 'a' + 10;
        else if (c >= 'A' && c <= 'Z') d = c - 'A' + 10;
        else break;
        if (d >= base) break;
        result = result * base + d;
    }
    (void)upper;
    return neg ? -result : result;
}

/* Дамп памяти: байты значения в двоичном виде, слева направо, через пробел */
static void dump_bytes(const unsigned char *bytes, size_t n, char *out, size_t out_size) {
    size_t pos = 0;
    for (size_t i = 0; i < n; ++i) {
        for (int bit = 7; bit >= 0; --bit) {
            if (pos + 1 < out_size) {
                out[pos++] = ((bytes[i] >> bit) & 1) ? '1' : '0';
            }
        }
        if (i + 1 < n && pos + 1 < out_size) {
            out[pos++] = ' ';
        }
    }
    out[pos] = '\0';
}

/* ---------- Разбор спецификатора ---------- */

struct spec {
    int custom;           /* 1 если наш флаг */
    const char *ptr;      /* указатель на начало спецификатора в строке формата */
    size_t len;           /* длина спецификатора, включая '%' */
    char code[4];         /* буквенный код кастомного флага, напр. "Ro" */
};

/* Пытаемся распознать кастомный спецификатор, начиная с '%' */
static int match_custom(const char *fmt, struct spec *sp) {
    if (fmt[0] != '%') return 0;
    const char *codes[] = {"Ro","Zr","Cv","CV","to","TO","mi","mu","md","mf"};
    for (size_t i = 0; i < sizeof(codes)/sizeof(codes[0]); ++i) {
        size_t l = strlen(codes[i]);
        if (strncmp(fmt + 1, codes[i], l) == 0) {
            sp->custom = 1;
            sp->ptr = fmt;
            sp->len = 1 + l;
            memcpy(sp->code, codes[i], l);
            sp->code[l] = '\0';
            return 1;
        }
    }
    return 0;
}

/* Длина «стандартного» спецификатора (до буквы-преобразователя включительно) */
static size_t std_spec_len(const char *fmt) {
    size_t i = 1;
    if (fmt[i] == '\0') return 0;
    /* флаги */
    while (fmt[i] && strchr("-+ #0'", fmt[i])) ++i;
    /* ширина */
    if (fmt[i] == '*') ++i;
    else while (fmt[i] && isdigit((unsigned char)fmt[i])) ++i;
    /* точность */
    if (fmt[i] == '.') {
        ++i;
        if (fmt[i] == '*') ++i;
        else while (fmt[i] && isdigit((unsigned char)fmt[i])) ++i;
    }
    /* длина */
    while (fmt[i] && strchr("hlLqjzt", fmt[i])) ++i;
    /* преобразователь */
    if (fmt[i]) ++i;
    return i;
}

/* ---------- Основной обработчик ---------- */

static void format_impl(struct sbuf *out, const char *fmt, va_list ap) {
    const char *p = fmt;
    while (*p && !out->error) {
        if (*p != '%') {
            sbuf_putc(out, *p++);
            continue;
        }
        if (p[1] == '%') {
            sbuf_putc(out, '%');
            p += 2;
            continue;
        }

        struct spec sp;
        if (match_custom(p, &sp)) {
            char tmp[256];
            if (strcmp(sp.code, "Ro") == 0) {
                int v = va_arg(ap, int);
                to_roman(v, tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "Zr") == 0) {
                unsigned int v = va_arg(ap, unsigned int);
                to_zeckendorf(v, tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "Cv") == 0 || strcmp(sp.code, "CV") == 0) {
                int v = va_arg(ap, int);
                int base = va_arg(ap, int);
                int upper = (sp.code[1] == 'V');
                to_base(v, base, upper, tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "to") == 0 || strcmp(sp.code, "TO") == 0) {
                const char *s = va_arg(ap, const char *);
                int base = va_arg(ap, int);
                int upper = (sp.code[1] == 'O');
                long long v = from_base(s, base, upper);
                snprintf(tmp, sizeof(tmp), "%lld", v);
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "mi") == 0) {
                int v = va_arg(ap, int);
                unsigned char bytes[sizeof(int)];
                memcpy(bytes, &v, sizeof(int));
                dump_bytes(bytes, sizeof(int), tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "mu") == 0) {
                unsigned int v = va_arg(ap, unsigned int);
                unsigned char bytes[sizeof(unsigned int)];
                memcpy(bytes, &v, sizeof(unsigned int));
                dump_bytes(bytes, sizeof(unsigned int), tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "md") == 0) {
                double v = va_arg(ap, double);
                unsigned char bytes[sizeof(double)];
                memcpy(bytes, &v, sizeof(double));
                dump_bytes(bytes, sizeof(double), tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            } else if (strcmp(sp.code, "mf") == 0) {
                double v = va_arg(ap, double);
                float fv = (float)v;
                unsigned char bytes[sizeof(float)];
                memcpy(bytes, &fv, sizeof(float));
                dump_bytes(bytes, sizeof(float), tmp, sizeof(tmp));
                sbuf_puts(out, tmp);
            }
            p += sp.len;
            continue;
        }

        /* Стандартный спецификатор: определяем его длину и просим vsnprintf
           сформировать подстроку. Аргументы мы должны «вынуть» сами.
           Для этого используем отдельный va_list на основе копии. */
        size_t slen = std_spec_len(p);
        if (slen == 0) {
            sbuf_putc(out, '%');
            ++p;
            continue;
        }

        char specbuf[64];
        if (slen >= sizeof(specbuf)) slen = sizeof(specbuf) - 1;
        memcpy(specbuf, p, slen);
        specbuf[slen] = '\0';

        /* Определяем тип аргумента по последнему символу */
        char conv = specbuf[slen - 1];
        char tmp[512];
        int n = 0;

        if (conv == 'd' || conv == 'i') {
            int v = va_arg(ap, int);
            n = snprintf(tmp, sizeof(tmp), specbuf, v);
        } else if (conv == 'u' || conv == 'o' || conv == 'x' || conv == 'X') {
            unsigned int v = va_arg(ap, unsigned int);
            n = snprintf(tmp, sizeof(tmp), specbuf, v);
        } else if (conv == 'c') {
            int v = va_arg(ap, int);
            n = snprintf(tmp, sizeof(tmp), specbuf, v);
        } else if (conv == 's') {
            const char *v = va_arg(ap, const char *);
            n = snprintf(tmp, sizeof(tmp), specbuf, v ? v : "(null)");
        } else if (conv == 'f' || conv == 'F' || conv == 'e' || conv == 'E' ||
                   conv == 'g' || conv == 'G' || conv == 'a' || conv == 'A') {
            double v = va_arg(ap, double);
            n = snprintf(tmp, sizeof(tmp), specbuf, v);
        } else if (conv == 'p') {
            void *v = va_arg(ap, void *);
            n = snprintf(tmp, sizeof(tmp), specbuf, v);
        } else if (conv == 'n') {
            /* %n не поддерживаем — пропускаем аргумент */
            (void)va_arg(ap, int *);
            n = 0;
        } else {
            /* неизвестный — выведем как есть */
            n = snprintf(tmp, sizeof(tmp), "%s", specbuf);
        }
        if (n > 0) {
            sbuf_reserve(out, (size_t)n);
            if (!out->error) {
                size_t to_copy = (size_t)n;
                if (to_copy >= sizeof(tmp)) to_copy = sizeof(tmp) - 1;
                memcpy(out->data + out->len, tmp, to_copy);
                out->len += to_copy;
                out->data[out->len] = '\0';
            }
        }
        p += slen;
    }
}

/* ---------- Публичные функции ---------- */

int overprintf(const char *format, ...) {
    if (format == NULL) return -1;
    struct sbuf b;
    sbuf_init(&b);
    va_list ap;
    va_start(ap, format);
    format_impl(&b, format, ap);
    va_end(ap);
    if (b.error) {
        sbuf_free(&b);
        return -1;
    }
    int written = (int)fwrite(b.data, 1, b.len, stdout);
    int total = (int)b.len;
    sbuf_free(&b);
    if (written < 0) return -1;
    return total;
}

int oversprintf(char *str, const char *format, ...) {
    if (str == NULL || format == NULL) return -1;
    struct sbuf b;
    sbuf_init(&b);
    va_list ap;
    va_start(ap, format);
    format_impl(&b, format, ap);
    va_end(ap);
    if (b.error) {
        sbuf_free(&b);
        str[0] = '\0';
        return -1;
    }
    memcpy(str, b.data, b.len + 1);
    int len = (int)b.len;
    sbuf_free(&b);
    return len;
}

/* ---------- Демонстрация ---------- */

int main(void) {
    char buf[1024];

    /* Стандартные флаги */
    overprintf("Standard: d=%d s=%s x=%x f=%.2f c=%c\n", 42, "hello", 255, 3.14159, 'A');

    /* Римские цифры */
    overprintf("Ro: 1=%Ro 4=%Ro 9=%Ro 14=%Ro 1994=%Ro 3999=%Ro 0=%Ro\n",
               1, 4, 9, 14, 1994, 3999, 0);

    /* Фибоначчиева система */
    overprintf("Zr: 0=%Zr 1=%Zr 2=%Zr 3=%Zr 4=%Zr 5=%Zr 10=%Zr 100=%Zr\n",
               0u, 1u, 2u, 3u, 4u, 5u, 10u, 100u);

    /* Cv / CV */
    overprintf("Cv: 255 base16=%Cv 255 base2=%Cv 255 base36=%Cv\n", 255, 16, 255, 2, 255, 36);
    overprintf("CV: 255 base16=%CV 255 base2=%CV 255 base36=%CV\n", 255, 16, 255, 2, 255, 36);
    overprintf("Cv: 255 base99(->10)=%Cv -10 base8=%Cv\n", 255, 99, -10, 8);

    /* to / TO */
    overprintf("to: ff base16=%to 1010 base2=%to zz base36=%to\n",
               "ff", 16, "1010", 2, "zz", 36);
    overprintf("TO: FF base16=%TO 1010 base2=%TO ZZ base36=%TO\n",
               "FF", 16, "1010", 2, "ZZ", 36);

    /* Дампы памяти */
    overprintf("mi: %mi\n", 1);
    overprintf("mu: %mu\n", 1u);
    overprintf("md: %md\n", 1.0);
    overprintf("mf: %mf\n", 1.0f);

    /* oversprintf */
    int len = oversprintf(buf, "sprintf: %d + %d = %d, %Ro, %Cv", 2, 3, 5, 42, 255, 16);
    overprintf("oversprintf returned %d: [%s]\n", len, buf);

    return 0;
}