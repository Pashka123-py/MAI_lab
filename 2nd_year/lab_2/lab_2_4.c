#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>

/* Статусы выполнения */
enum status {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_MEMORY,
    STATUS_ERR_INPUT,
    STATUS_ERR_CAPACITY
};

/* Описание одного файла */
struct file_info {
    char *filename;
    unsigned int size;
    unsigned int importance;
};

/* Результат выбора: массив выбранных файлов, суммарный размер и важность */
struct selection {
    struct file_info *files;
    size_t count;
    unsigned long long total_size;
    unsigned long long total_importance;
};

/* Освобождение массива описаний файлов */
static void free_files(struct file_info *files, size_t n) {
    if (files == NULL) {
        return;
    }
    for (size_t i = 0; i < n; ++i) {
        free(files[i].filename);
    }
    free(files);
}

/* Чтение одной строки произвольной длины до перевода строки или EOF.
   Возвращает STATUS_OK и указатель на строку (без \n) в *out_line. */
static enum status read_line(char **out_line) {
    size_t cap = 128;
    size_t len = 0;
    char *buf = malloc(cap);
    if (buf == NULL) {
        return STATUS_ERR_MEMORY;
    }

    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        if (len + 1 >= cap) {
            size_t new_cap = cap * 2;
            char *tmp = realloc(buf, new_cap);
            if (tmp == NULL) {
                free(buf);
                return STATUS_ERR_MEMORY;
            }
            buf = tmp;
            cap = new_cap;
        }
        buf[len++] = (char)c;
    }

    if (c == EOF && len == 0) {
        free(buf);
        return STATUS_ERR_INPUT;
    }

    buf[len] = '\0';
    *out_line = buf;
    return STATUS_OK;
}

/* Пропуск пробельных символов в начале строки, возврат указателя на первый непробельный */
static const char *skip_ws(const char *s) {
    while (*s == ' ' || *s == '\t' || *s == '\r') {
        ++s;
    }
    return s;
}

/* Чтение неотрицательного целого (unsigned int) из строки; *ok=1 при успехе */
static int parse_uint(const char **ps, unsigned int *out) {
    const char *s = skip_ws(*ps);
    if (*s < '0' || *s > '9') {
        return 0;
    }
    unsigned long long val = 0;
    while (*s >= '0' && *s <= '9') {
        val = val * 10 + (unsigned long long)(*s - '0');
        if (val > UINT_MAX) {
            return 0;
        }
        ++s;
    }
    *ps = s;
    *out = (unsigned int)val;
    return 1;
}

/* Чтение имени файла: непробельная последовательность; *ok=1 при успехе */
static char *parse_name(const char **ps) {
    const char *s = skip_ws(*ps);
    const char *start = s;
    while (*s != '\0' && *s != ' ' && *s != '\t' && *s != '\r') {
        ++s;
    }
    size_t len = (size_t)(s - start);
    if (len == 0) {
        return NULL;
    }
    char *name = malloc(len + 1);
    if (name == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < len; ++i) {
        name[i] = start[i];
    }
    name[len] = '\0';
    *ps = s;
    return name;
}

/* Проверка, что после разбора в строке остались только пробелы */
static int only_ws_left(const char *s) {
    s = skip_ws(s);
    return *s == '\0';
}

/* Задача 0/1 рюкзака: максимизация важности при ограничении размера.
   Возвращает статус; результат — через параметр result.
   Все массивы освобождаются. */
static enum status solve_knapsack(const struct file_info *files, size_t n,
                                  unsigned int capacity, struct selection *result) {
    result->files = NULL;
    result->count = 0;
    result->total_size = 0;
    result->total_importance = 0;

    if (files == NULL || result == NULL) {
        return STATUS_ERR_ARGS;
    }
    if (n == 0 || capacity == 0) {
        return STATUS_OK;
    }

    size_t cap = (size_t)capacity + 1;
    if (cap == 0) {
        return STATUS_ERR_CAPACITY;
    }

    /* dp[j] — максимальная важность при вместимости j */
    unsigned long long *dp = calloc(cap, sizeof(unsigned long long));
    if (dp == NULL) {
        return STATUS_ERR_MEMORY;
    }

    /* Для восстановления: keep[i][j] = 1, если файл i входит в оптимальный набор для вместимости j */
    unsigned char *keep = calloc(n * cap, sizeof(unsigned char));
    if (keep == NULL) {
        free(dp);
        return STATUS_ERR_MEMORY;
    }

    for (size_t i = 0; i < n; ++i) {
        unsigned int w = files[i].size;
        unsigned long long v = (unsigned long long)files[i].importance;
        if (w > capacity) {
            continue;
        }
        for (size_t j = capacity; j >= (size_t)w; --j) {
            unsigned long long cand = dp[j - w] + v;
            if (cand > dp[j]) {
                dp[j] = cand;
                keep[i * cap + j] = 1;
            }
            if (j == (size_t)w) {
                break;
            }
        }
    }

    unsigned long long best = dp[capacity];

    /* Восстановление набора: идём от последнего файла к первому */
    size_t *chosen = malloc(n * sizeof(size_t));
    if (chosen == NULL) {
        free(dp);
        free(keep);
        return STATUS_ERR_MEMORY;
    }
    size_t chosen_cnt = 0;
    size_t j = capacity;
    for (size_t i = n; i > 0; --i) {
        size_t idx = i - 1;
        if (keep[idx * cap + j] == 1) {
            unsigned int w = files[idx].size;
            if (w > j) {
                continue;
            }
            chosen[chosen_cnt++] = idx;
            j -= w;
        }
    }

    /* Переворачиваем в порядке возрастания индекса (как во входных данных) */
    for (size_t i = 0; i < chosen_cnt / 2; ++i) {
        size_t t = chosen[i];
        chosen[i] = chosen[chosen_cnt - 1 - i];
        chosen[chosen_cnt - 1 - i] = t;
    }

    /* Копируем выбранные файлы в результат (только описатели, без владения именем) */
    struct file_info *sel = NULL;
    if (chosen_cnt > 0) {
        sel = malloc(chosen_cnt * sizeof(struct file_info));
        if (sel == NULL) {
            free(chosen);
            free(dp);
            free(keep);
            return STATUS_ERR_MEMORY;
        }
    }

    unsigned long long total_size = 0;
    unsigned long long total_imp = 0;
    for (size_t i = 0; i < chosen_cnt; ++i) {
        size_t idx = chosen[i];
        sel[i].filename = files[idx].filename;
        sel[i].size = files[idx].size;
        sel[i].importance = files[idx].importance;
        total_size += files[idx].size;
        total_imp += files[idx].importance;
    }

    free(chosen);
    free(dp);
    free(keep);

    result->files = sel;
    result->count = chosen_cnt;
    result->total_size = total_size;
    result->total_importance = total_imp;

    (void)best; /* best == total_imp */
    return STATUS_OK;
}

/* Вывод результата в заданном формате */
static void print_result(const struct selection *sel) {
    printf("TOTAL_SIZE %llu\n", sel->total_size);
    printf("TOTAL_IMPORTANCE %llu\n", sel->total_importance);
    printf("FILES %zu\n", sel->count);
    for (size_t i = 0; i < sel->count; ++i) {
        printf("%s\n", sel->files[i].filename);
    }
}

int main(void) {
    struct file_info *files = NULL;
    size_t n = 0;
    unsigned int capacity = 0;
    enum status st = STATUS_OK;

    /* Чтение первой строки: n capacity */
    {
        char *line = NULL;
        st = read_line(&line);
        if (st != STATUS_OK) {
            printf("Error: cannot read first line\n");
            return 1;
        }
        const char *p = line;
        unsigned int un = 0;
        if (!parse_uint(&p, &un)) {
            free(line);
            printf("Error: invalid number of files\n");
            return 1;
        }
        if (!parse_uint(&p, &capacity)) {
            free(line);
            printf("Error: invalid capacity\n");
            return 1;
        }
        if (!only_ws_left(p)) {
            free(line);
            printf("Error: extra characters after capacity\n");
            return 1;
        }
        n = (size_t)un;
        free(line);

        if (n == 0) {
            /* Пустой набор — валидный случай, но нужно дочитать ничего и вывести пусто */
            struct selection empty = { NULL, 0, 0, 0 };
            print_result(&empty);
            return 0;
        }
    }

    files = calloc(n, sizeof(struct file_info));
    if (files == NULL) {
        printf("Error: memory allocation failed\n");
        return 1;
    }

    /* Чтение n строк: filename size importance */
    for (size_t i = 0; i < n; ++i) {
        char *line = NULL;
        st = read_line(&line);
        if (st != STATUS_OK) {
            printf("Error: cannot read file description #%zu\n", i + 1);
            free_files(files, i);
            return 1;
        }
        const char *p = line;
        char *name = parse_name(&p);
        if (name == NULL) {
            free(line);
            printf("Error: invalid filename in line %zu\n", i + 1);
            free_files(files, i);
            return 1;
        }
        unsigned int sz = 0;
        unsigned int imp = 0;
        if (!parse_uint(&p, &sz) || sz == 0) {
            free(name);
            free(line);
            printf("Error: invalid size in line %zu\n", i + 1);
            free_files(files, i);
            return 1;
        }
        if (!parse_uint(&p, &imp)) {
            free(name);
            free(line);
            printf("Error: invalid importance in line %zu\n", i + 1);
            free_files(files, i);
            return 1;
        }
        if (!only_ws_left(p)) {
            free(name);
            free(line);
            printf("Error: extra characters in line %zu\n", i + 1);
            free_files(files, i);
            return 1;
        }
        free(line);

        /* Проверка уникальности имени */
        for (size_t j = 0; j < i; ++j) {
            const char *a = files[j].filename;
            const char *b = name;
            int same = 1;
            while (*a != '\0' && *b != '\0') {
                if (*a != *b) {
                    same = 0;
                    break;
                }
                ++a;
                ++b;
            }
            if (same && *a == '\0' && *b == '\0') {
                same = 1;
            } else {
                same = 0;
            }
            if (same) {
                free(name);
                printf("Error: duplicate filename in line %zu\n", i + 1);
                free_files(files, i);
                return 1;
            }
        }

        files[i].filename = name;
        files[i].size = sz;
        files[i].importance = imp;
    }

    struct selection result = { NULL, 0, 0, 0 };
    st = solve_knapsack(files, n, capacity, &result);
    if (st != STATUS_OK) {
        printf("Error: failed to solve knapsack (status %d)\n", (int)st);
        free_files(files, n);
        return 1;
    }

    print_result(&result);

    free(result.files);
    free_files(files, n);
    return 0;
}