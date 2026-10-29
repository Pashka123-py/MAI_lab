#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ================= Статус-коды ================= */

typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_FLAG,
    ST_ERR_FILE_IN,
    ST_ERR_FILE_OUT,
    ST_ERR_MEM,
    ST_ERR_PARSE,
    ST_ERR_NULL
} status_t;

/* ================= Структура ================= */

typedef struct {
    unsigned int id;
    char *name;
    char *surname;
    double salary;
} Employee;

typedef struct {
    Employee *data;
    size_t size;
    size_t capacity;
} EmpArray;

/* ================= Вспомогательные функции ================= */

static char *my_strdup(const char *s)
{
    size_t len;
    char *buf;

    if (s == NULL) return NULL;
    len = strlen(s);
    buf = (char *)malloc(len + 1U);
    if (buf == NULL) return NULL;
    memcpy(buf, s, len + 1U);
    return buf;
}

static int is_space_c(char c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static char *next_token(char *p)
{
    while (*p != '\0' && !is_space_c(*p)) ++p;
    if (*p == '\0') return p;
    *p = '\0';
    ++p;
    while (is_space_c(*p)) ++p;
    return p;
}

/* ================= Валидация и парсинг ================= */

static int is_valid_id(const char *s)
{
    size_t i = 0;
    if (s == NULL || s[0] == '\0') return 0;
    while (s[i] != '\0') {
        if (!isdigit((unsigned char)s[i])) return 0;
        ++i;
    }
    return 1;
}

static int is_valid_name(const char *s)
{
    size_t i = 0;
    if (s == NULL || s[0] == '\0') return 0;
    while (s[i] != '\0') {
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'))) {
            return 0;
        }
        ++i;
    }
    return 1;
}

static int is_valid_salary(const char *s)
{
    size_t i = 0;
    int has_dot = 0;
    if (s == NULL || s[0] == '\0') return 0;
    if (s[0] == '-') return 0;
    while (s[i] != '\0') {
        if (s[i] == '.') {
            if (has_dot) return 0;
            has_dot = 1;
        } else if (!isdigit((unsigned char)s[i])) {
            return 0;
        }
        ++i;
    }
    return 1;
}

static status_t parse_id(const char *s, unsigned int *out)
{
    unsigned long long acc = 0ULL;
    size_t i = 0;
    if (s == NULL || out == NULL) return ST_ERR_NULL;
    if (!is_valid_id(s)) return ST_ERR_PARSE;
    while (s[i] != '\0') {
        acc = acc * 10ULL + (unsigned long long)(s[i] - '0');
        if (acc > 4294967295ULL) return ST_ERR_PARSE;
        ++i;
    }
    *out = (unsigned int)acc;
    return ST_OK;
}

static status_t parse_double(const char *s, double *out)
{
    if (s == NULL || out == NULL) return ST_ERR_NULL;
    if (!is_valid_salary(s)) return ST_ERR_PARSE;
    *out = atof(s);
    return ST_OK;
}

/* ================= Работа с динамическим массивом ================= */

static status_t emp_array_init(EmpArray *arr)
{
    if (arr == NULL) return ST_ERR_NULL;
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
    return ST_OK;
}

static status_t emp_array_push(EmpArray *arr, Employee emp)
{
    if (arr == NULL) return ST_ERR_NULL;
    if (arr->size == arr->capacity) {
        size_t new_cap = (arr->capacity == 0) ? 16U : arr->capacity * 2U;
        Employee *new_data = (Employee *)realloc(arr->data, new_cap * sizeof(Employee));
        if (new_data == NULL) return ST_ERR_MEM;
        arr->data = new_data;
        arr->capacity = new_cap;
    }
    arr->data[arr->size++] = emp;
    return ST_OK;
}

static void emp_array_free(EmpArray *arr)
{
    size_t i;
    if (arr == NULL || arr->data == NULL) return;
    for (i = 0; i < arr->size; ++i) {
        free(arr->data[i].name);
        free(arr->data[i].surname);
    }
    free(arr->data);
    arr->data = NULL;
    arr->size = 0;
    arr->capacity = 0;
}

/* ================= Чтение и парсинг файла ================= */

static status_t parse_line(char *line, Employee *emp)
{
    char *p;
    char *tok_id;
    char *tok_name;
    char *tok_surname;
    char *tok_salary;
    unsigned int id_val;
    double sal_val;
    char *name_dup = NULL;
    char *surname_dup = NULL;
    status_t rc;

    /* обрезаем строку и получаем первый токен */
    p = line;
    while (is_space_c(*p)) ++p;
    if (*p == '\0') return ST_ERR_PARSE;

    tok_id = p;
    p = next_token(p);
    tok_name = p;
    p = next_token(p);
    tok_surname = p;
    p = next_token(p);
    tok_salary = p;
    p = next_token(p);
    if (*tok_name == '\0' || *tok_surname == '\0' || *tok_salary == '\0' || *p != '\0') {
        return ST_ERR_PARSE;
    }

    /* id */
    rc = parse_id(tok_id, &id_val);
    if (rc != ST_OK) return rc;

    /* name */
    if (!is_valid_name(tok_name)) return ST_ERR_PARSE;
    name_dup = my_strdup(tok_name);
    if (name_dup == NULL) return ST_ERR_MEM;

    /* surname */
    if (!is_valid_name(tok_surname)) { free(name_dup); return ST_ERR_PARSE; }
    surname_dup = my_strdup(tok_surname);
    if (surname_dup == NULL) { free(name_dup); return ST_ERR_MEM; }

    /* salary */
    rc = parse_double(tok_salary, &sal_val);
    if (rc != ST_OK) { free(name_dup); free(surname_dup); return rc; }

    emp->id = id_val;
    emp->name = name_dup;
    emp->surname = surname_dup;
    emp->salary = sal_val;
    return ST_OK;
}

static status_t read_employees(const char *path, EmpArray *arr)
{
    FILE *f;
    char buffer[4096];
    status_t rc;

    if (path == NULL || arr == NULL) return ST_ERR_NULL;
    f = fopen(path, "r");
    if (f == NULL) return ST_ERR_FILE_IN;

    while (fgets(buffer, sizeof(buffer), f) != NULL) {
        Employee emp;
        rc = parse_line(buffer, &emp);
        if (rc == ST_OK) {
            rc = emp_array_push(arr, emp);
            if (rc != ST_OK) {
                free(emp.name);
                free(emp.surname);
                fclose(f);
                return rc;
            }
        }
    }
    fclose(f);
    return ST_OK;
}

/* ================= Сортировка ================= */

static int cmp_salary(double a, double b, double eps)
{
    if (fabs(a - b) <= eps) return 0;
    return (a < b) ? -1 : 1;
}

static int cmp_asc_impl(const Employee *ea, const Employee *eb, double eps)
{
    int c;
    c = cmp_salary(ea->salary, eb->salary, eps);
    if (c != 0) return c;
    c = strcmp(ea->surname, eb->surname);
    if (c != 0) return c;
    c = strcmp(ea->name, eb->name);
    if (c != 0) return c;
    if (ea->id < eb->id) return -1;
    if (ea->id > eb->id) return 1;
    return 0;
}

static int cmp_asc(const void *a, const void *b)
{
    return cmp_asc_impl((const Employee *)a, (const Employee *)b, 1e-9);
}

static int cmp_desc(const void *a, const void *b)
{
    return -cmp_asc(a, b);
}

/* ================= Вывод ================= */

static status_t write_employees(const char *path, const EmpArray *arr)
{
    FILE *f;
    size_t i;
    if (path == NULL || arr == NULL) return ST_ERR_NULL;
    f = fopen(path, "w");
    if (f == NULL) return ST_ERR_FILE_OUT;
    for (i = 0; i < arr->size; ++i) {
        fprintf(f, "%u %s %s %g\n",
                arr->data[i].id,
                arr->data[i].name,
                arr->data[i].surname,
                arr->data[i].salary);
    }
    fclose(f);
    return ST_OK;
}

/* ================= Разбор флага ================= */

static status_t parse_flag(const char *flag, int *ascending)
{
    if (flag == NULL || ascending == NULL) return ST_ERR_NULL;
    if (flag[0] != '-' && flag[0] != '/') return ST_ERR_FLAG;
    if (flag[1] == 'a' && flag[2] == '\0') { *ascending = 1; return ST_OK; }
    if (flag[1] == 'd' && flag[2] == '\0') { *ascending = 0; return ST_OK; }
    return ST_ERR_FLAG;
}

/* ================= main ================= */

int main(int argc, char **argv)
{
    EmpArray arr;
    status_t rc;
    int ascending = 1;

    if (argc != 4) {
        printf("Error: expected 3 arguments\n");
        printf("Usage: prog <input> <-a|/a|-d|/d> <output>\n");
        return EXIT_FAILURE;
    }

    rc = parse_flag(argv[2], &ascending);
    switch (rc) {
    case ST_OK: break;
    case ST_ERR_FLAG:
        printf("Error: invalid flag '%s'\n", argv[2]);
        return EXIT_FAILURE;
    default:
        printf("Error: unexpected status %d\n", (int)rc);
        return EXIT_FAILURE;
    }

    rc = emp_array_init(&arr);
    switch (rc) {
    case ST_OK: break;
    default:
        printf("Error: unexpected status %d\n", (int)rc);
        return EXIT_FAILURE;
    }

    rc = read_employees(argv[1], &arr);
    switch (rc) {
    case ST_OK: break;
    case ST_ERR_FILE_IN:
        printf("Error: cannot open input file '%s'\n", argv[1]);
        emp_array_free(&arr);
        return EXIT_FAILURE;
    case ST_ERR_MEM:
        printf("Error: memory allocation failed\n");
        emp_array_free(&arr);
        return EXIT_FAILURE;
    default:
        printf("Error: unexpected status %d\n", (int)rc);
        emp_array_free(&arr);
        return EXIT_FAILURE;
    }

    if (arr.size == 0) {
        printf("Error: no valid records in input file\n");
        emp_array_free(&arr);
        return EXIT_FAILURE;
    }

    if (ascending) {
        qsort(arr.data, arr.size, sizeof(Employee), cmp_asc);
    } else {
        qsort(arr.data, arr.size, sizeof(Employee), cmp_desc);
    }

    rc = write_employees(argv[3], &arr);
    switch (rc) {
    case ST_OK:
        printf("Done: %zu records written\n", arr.size);
        break;
    case ST_ERR_FILE_OUT:
        printf("Error: cannot open output file '%s'\n", argv[3]);
        emp_array_free(&arr);
        return EXIT_FAILURE;
    default:
        printf("Error: unexpected status %d\n", (int)rc);
        emp_array_free(&arr);
        return EXIT_FAILURE;
    }

    emp_array_free(&arr);
    return EXIT_SUCCESS;
}