#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>

/* Перечислимый тип статусов */
enum status {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_FILE_OPEN,
    STATUS_ERR_FILE_READ,
    STATUS_ERR_ALLOC,
    STATUS_ERR_SUBSTR
};

/* Сравнение двух символов */
static int char_equal(char a, char b) {
    return a == b;
}

/* Проверка совпадения подстроки needle длиной needle_len в hay длиной hay_len */
static int substr_match(const char *hay, size_t hay_len, const char *needle, size_t needle_len) {
    if (needle_len == 0) {
        return 1;
    }
    if (hay_len < needle_len) {
        return 0;
    }
    for (size_t i = 0; i < needle_len; ++i) {
        if (!char_equal(hay[i], needle[i])) {
            return 0;
        }
    }
    return 1;
}

/* Длина C-строки без strlen */
static size_t str_length(const char *s) {
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

/* Чтение файла целиком в динамический буфер */
static enum status read_file(const char *path, char **out_buf, size_t *out_size) {
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return STATUS_ERR_FILE_OPEN;
    }

    size_t cap = 4096;
    size_t size = 0;
    char *buf = malloc(cap);
    if (buf == NULL) {
        fclose(f);
        return STATUS_ERR_ALLOC;
    }

    char chunk[4096];
    size_t nread;
    while ((nread = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (size + nread + 1 > cap) {
            size_t new_cap = cap;
            while (new_cap < size + nread + 1) {
                if (new_cap > SIZE_MAX / 2) {
                    free(buf);
                    fclose(f);
                    return STATUS_ERR_ALLOC;
                }
                new_cap *= 2;
            }
            char *tmp = realloc(buf, new_cap);
            if (tmp == NULL) {
                free(buf);
                fclose(f);
                return STATUS_ERR_ALLOC;
            }
            buf = tmp;
            cap = new_cap;
        }
        memcpy(buf + size, chunk, nread);
        size += nread;
    }

    if (ferror(f)) {
        free(buf);
        fclose(f);
        return STATUS_ERR_FILE_READ;
    }

    buf[size] = '\0';
    fclose(f);

    *out_buf = buf;
    *out_size = size;
    return STATUS_OK;
}

/* Поиск всех вхождений подстроки в одном файле с выводом номеров строк и столбцов */
static enum status search_in_file(const char *path, const char *needle, size_t needle_len) {
    char *buf = NULL;
    size_t size = 0;
    enum status st = read_file(path, &buf, &size);

    printf("File: %s\n", path);
    if (st != STATUS_OK) {
        switch (st) {
        case STATUS_ERR_FILE_OPEN:
            printf("  error: cannot open file\n");
            break;
        case STATUS_ERR_ALLOC:
            printf("  error: memory allocation failed\n");
            break;
        case STATUS_ERR_FILE_READ:
            printf("  error: file read error\n");
            break;
        default:
            printf("  error: unknown error\n");
            break;
        }
        return st;
    }

    size_t line = 1;
    size_t col = 1;
    int found = 0;

    for (size_t i = 0; i < size; ++i) {
        if (substr_match(buf + i, size - i, needle, needle_len)) {
            printf("  line %zu, column %zu\n", line, col);
            found = 1;
        }
        if (buf[i] == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
    }

    if (!found) {
        printf("  no matches\n");
    }

    free(buf);
    return STATUS_OK;
}

/* Variadic-функция: подстрока, количество файлов, пути к файлам */
static enum status search_substr_in_files(const char *substr, int count, ...) {
    if (substr == NULL || count < 1) {
        return STATUS_ERR_ARGS;
    }

    size_t needle_len = str_length(substr);
    if (needle_len == 0) {
        return STATUS_ERR_SUBSTR;
    }

    va_list ap;
    va_start(ap, count);

    enum status result = STATUS_OK;

    for (int i = 0; i < count; ++i) {
        const char *path = va_arg(ap, const char *);
        if (path == NULL) {
            printf("File: (null)\n  error: invalid file path\n");
            result = STATUS_ERR_ARGS;
            continue;
        }
        enum status st = search_in_file(path, substr, needle_len);
        if (st != STATUS_OK) {
            result = st;
        }
    }

    va_end(ap);
    return result;
}

/* Развёртка массива путей в вызов variadic-функции для конкретного count */
static enum status dispatch_search(const char *substr, int count, char *const *paths) {
    switch (count) {
    case 1:
        return search_substr_in_files(substr, 1, paths[0]);
    case 2:
        return search_substr_in_files(substr, 2, paths[0], paths[1]);
    case 3:
        return search_substr_in_files(substr, 3, paths[0], paths[1], paths[2]);
    case 4:
        return search_substr_in_files(substr, 4, paths[0], paths[1], paths[2], paths[3]);
    case 5:
        return search_substr_in_files(substr, 5, paths[0], paths[1], paths[2], paths[3], paths[4]);
    case 6:
        return search_substr_in_files(substr, 6, paths[0], paths[1], paths[2], paths[3], paths[4], paths[5]);
    case 7:
        return search_substr_in_files(substr, 7, paths[0], paths[1], paths[2], paths[3], paths[4], paths[5], paths[6]);
    case 8:
        return search_substr_in_files(substr, 8, paths[0], paths[1], paths[2], paths[3], paths[4], paths[5], paths[6], paths[7]);
    default:
        return STATUS_ERR_ARGS;
    }
}

/* Обёртка: при count > 8 обрабатываем блоками по 8 через dispatch_search */
static enum status search_all(const char *substr, int count, char *const *paths) {
    if (substr == NULL || count < 1 || paths == NULL) {
        return STATUS_ERR_ARGS;
    }
    if (str_length(substr) == 0) {
        return STATUS_ERR_SUBSTR;
    }

    enum status result = STATUS_OK;
    int processed = 0;

    while (processed < count) {
        int block = count - processed;
        if (block > 8) {
            block = 8;
        }

        enum status st = dispatch_search(substr, block, paths + processed);
        if (st != STATUS_OK) {
            result = st;
        }
        processed += block;
    }

    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <substring> <file1> [file2 ...]\n", argv[0]);
        return 1;
    }

    const char *substr = argv[1];
    int file_count = argc - 2;
    char **paths = &argv[2];

    enum status st = search_all(substr, file_count, paths);

    switch (st) {
    case STATUS_OK:
        return 0;
    case STATUS_ERR_ARGS:
        printf("Error: invalid arguments\n");
        return 2;
    case STATUS_ERR_SUBSTR:
        printf("Error: invalid substring\n");
        return 3;
    case STATUS_ERR_FILE_OPEN:
        printf("Error: cannot open file\n");
        return 4;
    case STATUS_ERR_FILE_READ:
        printf("Error: file read error\n");
        return 5;
    case STATUS_ERR_ALLOC:
        printf("Error: memory allocation failed\n");
        return 6;
    default:
        printf("Error: unknown error\n");
        return 7;
    }
}