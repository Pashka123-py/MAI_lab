#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

/* Статус-коды */

typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_ARG,
    ST_ERR_MEM,
    ST_ERR_LEN,
    ST_ERR_NULL
} status_t;

/* Вспомогательные функции (без string.h) */

static int my_strlen(const char *s)
{
    int n = 0;
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

static void my_strcpy(char *dst, const char *src)
{
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int my_isdigit_c(char c)
{
    return (c >= '0' && c <= '9');
}

static int my_isalpha_c(char c)
{
    return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

static char my_toupper_c(char c)
{
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return c;
}

static status_t parse_unsigned(const char *s, unsigned int *out)
{
    unsigned long long acc = 0ULL;
    int i = 0;

    if (s == NULL || out == NULL) {
        return ST_ERR_NULL;
    }
    if (s[0] == '\0') {
        return ST_ERR_ARG;
    }
    while (s[i] != '\0') {
        if (!my_isdigit_c(s[i])) {
            return ST_ERR_ARG;
        }
        acc = acc * 10ULL + (unsigned long long)(s[i] - '0');
        if (acc > (unsigned long long)UINT_MAX) {
            return ST_ERR_ARG;
        }
        ++i;
    }
    *out = (unsigned int)acc;
    return ST_OK;
}

/* Функция длины */

static status_t calc_length(const char *s, int *out_len)
{
    int len;

    if (s == NULL || out_len == NULL) {
        return ST_ERR_NULL;
    }
    len = my_strlen(s);
    *out_len = len;
    return ST_OK;
}

/* Функция reverse */

static status_t make_reversed(const char *s, char **out)
{
    int len, i;
    char *buf;

    if (s == NULL || out == NULL) {
        return ST_ERR_NULL;
    }
    len = my_strlen(s);
    buf = (char *)malloc((size_t)len + 1U);
    if (buf == NULL) {
        return ST_ERR_MEM;
    }
    for (i = 0; i < len; ++i) {
        buf[i] = s[len - 1 - i];
    }
    buf[len] = '\0';
    *out = buf;
    return ST_OK;
}

/* Функция upper на нечётных позициях */

static status_t make_upper_odd(const char *s, char **out)
{
    int len, i;
    char *buf;

    if (s == NULL || out == NULL) {
        return ST_ERR_NULL;
    }
    len = my_strlen(s);
    buf = (char *)malloc((size_t)len + 1U);
    if (buf == NULL) {
        return ST_ERR_MEM;
    }
    for (i = 0; i < len; ++i) {
        char c = s[i];
        if ((i % 2) != 0) {
            c = my_toupper_c(c);
        }
        buf[i] = c;
    }
    buf[len] = '\0';
    *out = buf;
    return ST_OK;
}

/* Функция -n (digits, letters, others) */

static status_t make_grouped(const char *s, char **out)
{
    int len, i, pos;
    char *buf;

    if (s == NULL || out == NULL) {
        return ST_ERR_NULL;
    }
    len = my_strlen(s);
    buf = (char *)malloc((size_t)len + 1U);
    if (buf == NULL) {
        return ST_ERR_MEM;
    }
    pos = 0;

    /* цифры */
    for (i = 0; i < len; ++i) {
        if (my_isdigit_c(s[i])) {
            buf[pos++] = s[i];
        }
    }
    /* буквы */
    for (i = 0; i < len; ++i) {
        if (my_isalpha_c(s[i])) {
            buf[pos++] = s[i];
        }
    }
    /* остальное */
    for (i = 0; i < len; ++i) {
        if (!my_isdigit_c(s[i]) && !my_isalpha_c(s[i])) {
            buf[pos++] = s[i];
        }
    }
    buf[pos] = '\0';
    *out = buf;
    return ST_OK;
}

/* Функция -с (конкатенация чётных по порядку) */

static status_t make_concat_random(int start_index, int argc, char *const *argv,
                                   unsigned int seed, char **out)
{
    int count, i, k, total;
    int *order;
    char *buf;
    int pos;

    if (argv == NULL || out == NULL) {
        return ST_ERR_NULL;
    }
    count = argc - start_index;
    if (count <= 0) {
        return ST_ERR_ARG;
    }

    total = 0;
    for (i = start_index; i < argc; ++i) {
        total += my_strlen(argv[i]);
    }

    buf = (char *)malloc((size_t)total + 1U);
    if (buf == NULL) {
        return ST_ERR_MEM;
    }

    order = (int *)malloc((size_t)count * sizeof(int));
    if (order == NULL) {
        free(buf);
        return ST_ERR_MEM;
    }

    for (i = 0; i < count; ++i) {
        order[i] = start_index + i;
    }

    srand(seed);
    for (i = count - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int tmp = order[i];
        order[i] = order[j];
        order[j] = tmp;
    }

    pos = 0;
    for (k = 0; k < count; ++k) {
        const char *src = argv[order[k]];
        int slen = my_strlen(src);
        for (i = 0; i < slen; ++i) {
            buf[pos++] = src[i];
        }
    }
    buf[pos] = '\0';

    free(order);
    *out = buf;
    return ST_OK;
}

/* Вывод */

static void print_result(const char *label, const char *s)
{
    printf("%s%s\n", label, s);
}

/* main */

int main(int argc, char **argv)
{
    status_t rc;
    const char *flag;

    if (argc < 3) {
        printf("Error: not enough arguments\n");
        printf("Usage: prog <-l|-r|-u|-n> <string>\n");
        printf("       prog -c <seed> <str1> [str2 ...]\n");
        return EXIT_FAILURE;
    }

    flag = argv[1];

    /* ---- -l ---- */
    if (flag[0] == '-' && flag[1] == 'l' && flag[2] == '\0') {
        int len = 0;
        rc = calc_length(argv[2], &len);
        switch (rc) {
        case ST_OK:
            printf("Length: %d\n", len);
            break;
        case ST_ERR_NULL:
            printf("Error: null argument\n");
            return EXIT_FAILURE;
        default:
            printf("Error: unexpected status %d\n", (int)rc);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* ---- -r ---- */
    if (flag[0] == '-' && flag[1] == 'r' && flag[2] == '\0') {
        char *res = NULL;
        rc = make_reversed(argv[2], &res);
        switch (rc) {
        case ST_OK:
            print_result("Reversed: ", res);
            free(res);
            break;
        case ST_ERR_MEM:
            printf("Error: memory allocation failed\n");
            return EXIT_FAILURE;
        case ST_ERR_NULL:
            printf("Error: null argument\n");
            return EXIT_FAILURE;
        default:
            printf("Error: unexpected status %d\n", (int)rc);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* ---- -u ---- */
    if (flag[0] == '-' && flag[1] == 'u' && flag[2] == '\0') {
        char *res = NULL;
        rc = make_upper_odd(argv[2], &res);
        switch (rc) {
        case ST_OK:
            print_result("Upper odd: ", res);
            free(res);
            break;
        case ST_ERR_MEM:
            printf("Error: memory allocation failed\n");
            return EXIT_FAILURE;
        case ST_ERR_NULL:
            printf("Error: null argument\n");
            return EXIT_FAILURE;
        default:
            printf("Error: unexpected status %d\n", (int)rc);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* ---- -n ---- */
    if (flag[0] == '-' && flag[1] == 'n' && flag[2] == '\0') {
        char *res = NULL;
        rc = make_grouped(argv[2], &res);
        switch (rc) {
        case ST_OK:
            print_result("Grouped: ", res);
            free(res);
            break;
        case ST_ERR_MEM:
            printf("Error: memory allocation failed\n");
            return EXIT_FAILURE;
        case ST_ERR_NULL:
            printf("Error: null argument\n");
            return EXIT_FAILURE;
        default:
            printf("Error: unexpected status %d\n", (int)rc);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    /* ---- -c ---- */
    if (flag[0] == '-' && flag[1] == 'c' && flag[2] == '\0') {
        unsigned int seed = 0U;
        char *res = NULL;
        int start;

        if (argc < 4) {
            printf("Error: -c requires <seed> and at least one string\n");
            return EXIT_FAILURE;
        }

        rc = parse_unsigned(argv[2], &seed);
        switch (rc) {
        case ST_OK:
            break;
        case ST_ERR_ARG:
            printf("Error: seed must be unsigned int\n");
            return EXIT_FAILURE;
        case ST_ERR_NULL:
            printf("Error: null argument\n");
            return EXIT_FAILURE;
        default:
            printf("Error: unexpected status %d\n", (int)rc);
            return EXIT_FAILURE;
        }

        start = 3;
        rc = make_concat_random(start, argc, argv, seed, &res);
        switch (rc) {
        case ST_OK:
            print_result("Concatenated: ", res);
            free(res);
            break;
        case ST_ERR_MEM:
            printf("Error: memory allocation failed\n");
            return EXIT_FAILURE;
        case ST_ERR_ARG:
            printf("Error: no strings to concatenate\n");
            return EXIT_FAILURE;
        case ST_ERR_NULL:
            printf("Error: null argument\n");
            return EXIT_FAILURE;
        default:
            printf("Error: unexpected status %d\n", (int)rc);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    printf("Error: unknown flag '%s'\n", flag);
    printf("Usage: prog <-l|-r|-u|-n> <string>\n");
    printf("       prog -c <seed> <str1> [str2 ...]\n");
    return EXIT_FAILURE;
}