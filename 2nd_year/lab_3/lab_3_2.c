#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Статус-коды функций */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARG,
    STATUS_ERR_ALLOC,
    STATUS_ERR_INPUT,
    STATUS_ERR_TIME_FORMAT,
    STATUS_ERR_TIME_ORDER,
    STATUS_ERR_DUPLICATE_ID,
    STATUS_ERR_RANGE
} Status;

/* Описание заявки */
typedef struct {
    long long id;
    int start;
    int finish;
} Talk;

/* Парсинг времени HH:MM -> минуты */
static Status parse_time(const char *s, int *out_minutes)
{
    int hh, mm;
    if (s == NULL || out_minutes == NULL) return STATUS_ERR_ARG;
    if (strlen(s) != 5) return STATUS_ERR_TIME_FORMAT;
    if (s[2] != ':') return STATUS_ERR_TIME_FORMAT;
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') return STATUS_ERR_TIME_FORMAT;
    if (s[3] < '0' || s[3] > '9' || s[4] < '0' || s[4] > '9') return STATUS_ERR_TIME_FORMAT;
    hh = (s[0] - '0') * 10 + (s[1] - '0');
    mm = (s[3] - '0') * 10 + (s[4] - '0');
    if (hh > 23 || mm > 59) return STATUS_ERR_RANGE;
    *out_minutes = hh * 60 + mm;
    return STATUS_OK;
}

/* Сравнение для qsort: по времени окончания */
static int cmp_talk_finish(const void *a, const void *b)
{
    const Talk *ta = (const Talk *)a;
    const Talk *tb = (const Talk *)b;
    if (ta->finish < tb->finish) return -1;
    if (ta->finish > tb->finish) return 1;
    return 0;
}

/* Сравнение для qsort: по id (для проверки уникальности) */
static int cmp_ll(const void *a, const void *b)
{
    long long x = *(const long long *)a;
    long long y = *(const long long *)b;
    if (x < y) return -1;
    if (x > y) return 1;
    return 0;
}

/* Проверка уникальности id */
static Status check_unique_ids(const Talk *talks, size_t n)
{
    long long *ids;
    size_t i;
    if (talks == NULL || n == 0) return STATUS_ERR_ARG;
    ids = (long long *)malloc(n * sizeof(long long));
    if (ids == NULL) return STATUS_ERR_ALLOC;
    for (i = 0; i < n; i++) ids[i] = talks[i].id;
    qsort(ids, n, sizeof(long long), cmp_ll);
    for (i = 1; i < n; i++) {
        if (ids[i] == ids[i - 1]) {
            free(ids);
            return STATUS_ERR_DUPLICATE_ID;
        }
    }
    free(ids);
    return STATUS_OK;
}

/* Поиск минимального элемента среди первых k (куча не нужна — k может быть большим) */
static int find_min_index(const int *rooms, int k)
{
    int i, idx = 0;
    for (i = 1; i < k; i++) {
        if (rooms[i] < rooms[idx]) idx = i;
    }
    return idx;
}

/* Основная функция: максимум докладов в k залах */
static Status max_talks(Talk *talks, size_t n, int k, int *out_count)
{
    int *rooms;
    size_t i;
    int count = 0;
    if (talks == NULL || out_count == NULL) return STATUS_ERR_ARG;
    if (n == 0) { *out_count = 0; return STATUS_OK; }
    if (k <= 0) return STATUS_ERR_ARG;

    /* Проверка корректности времен */
    for (i = 0; i < n; i++) {
        if (talks[i].start < 0 || talks[i].finish < 0) return STATUS_ERR_RANGE;
        if (talks[i].start >= talks[i].finish) return STATUS_ERR_TIME_ORDER;
    }

    /* Сортировка по времени окончания */
    qsort(talks, n, sizeof(Talk), cmp_talk_finish);

    /* Массив времени освобождения залов */
    rooms = (int *)malloc((size_t)k * sizeof(int));
    if (rooms == NULL) return STATUS_ERR_ALLOC;
    for (i = 0; i < (size_t)k; i++) rooms[i] = -1;

    /* Жадный выбор */
    for (i = 0; i < n; i++) {
        int idx = find_min_index(rooms, k);
        if (rooms[idx] <= talks[i].start) {
            rooms[idx] = talks[i].finish;
            count++;
        }
    }

    free(rooms);
    *out_count = count;
    return STATUS_OK;
}

/* Чтение одной строки, возврат 0 при EOF */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) return 0;
    return 1;
}

/* Удаление хвостового \n */
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[--len] = '\0';
    }
}

/* Чтение и валидация положительного int из строки */
static Status parse_positive_int(const char *s, int *out)
{
    char *end = NULL;
    long v;
    if (s == NULL || out == NULL) return STATUS_ERR_ARG;
    v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return STATUS_ERR_INPUT;
    if (v <= 0 || v > 1000000) return STATUS_ERR_RANGE;
    *out = (int)v;
    return STATUS_OK;
}

/* Чтение и валидация положительного long long */
static Status parse_positive_ll(const char *s, long long *out)
{
    char *end = NULL;
    long long v;
    if (s == NULL || out == NULL) return STATUS_ERR_ARG;
    v = strtoll(s, &end, 10);
    if (end == s || *end != '\0') return STATUS_ERR_INPUT;
    if (v <= 0) return STATUS_ERR_RANGE;
    *out = v;
    return STATUS_OK;
}

int main(void)
{
    char line[256];
    int n = 0, k = 0;
    int i;
    Talk *talks = NULL;
    Status st;
    int result = 0;

    if (!read_line(line, sizeof(line))) { printf("Input error\n"); return 1; }
    strip_newline(line);

    {
        char *tok1 = strtok(line, " \t");
        char *tok2 = strtok(NULL, " \t");
        char *extra = strtok(NULL, " \t");
        if (tok1 == NULL || tok2 == NULL || extra != NULL) {
            printf("Input error: expected two integers\n");
            return 1;
        }
        st = parse_positive_int(tok1, &n);
        if (st != STATUS_OK) { printf("Input error: invalid n\n"); return 1; }
        st = parse_positive_int(tok2, &k);
        if (st != STATUS_OK) { printf("Input error: invalid k\n"); return 1; }
    }

    talks = (Talk *)malloc((size_t)n * sizeof(Talk));
    if (talks == NULL) { printf("Memory allocation error\n"); return 1; }

    for (i = 0; i < n; i++) {
        char idbuf[64], stbuf[16], finbuf[16];
        long long id;
        int stm, finm;
        int got;
        if (!read_line(line, sizeof(line))) {
            printf("Input error: unexpected EOF\n");
            free(talks);
            return 1;
        }
        strip_newline(line);
        got = sscanf(line, "%63s %15s %15s", idbuf, stbuf, finbuf);
        if (got != 3) {
            printf("Input error: line %d format\n", i + 2);
            free(talks);
            return 1;
        }
        st = parse_positive_ll(idbuf, &id);
        if (st != STATUS_OK) { printf("Input error: invalid id\n"); free(talks); return 1; }
        st = parse_time(stbuf, &stm);
        if (st != STATUS_OK) { printf("Input error: invalid start_time\n"); free(talks); return 1; }
        st = parse_time(finbuf, &finm);
        if (st != STATUS_OK) { printf("Input error: invalid finish_time\n"); free(talks); return 1; }
        if (stm >= finm) { printf("Input error: start must be < finish\n"); free(talks); return 1; }
        talks[i].id = id;
        talks[i].start = stm;
        talks[i].finish = finm;
    }

    st = check_unique_ids(talks, (size_t)n);
    if (st != STATUS_OK) {
        printf("Input error: duplicate id\n");
        free(talks);
        return 1;
    }

    st = max_talks(talks, (size_t)n, k, &result);
    if (st != STATUS_OK) {
        printf("Runtime error: status %d\n", (int)st);
        free(talks);
        return 1;
    }

    printf("%d\n", result);
    free(talks);
    return 0;
}