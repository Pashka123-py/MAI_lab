#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

/* Статус-коды */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARG,
    STATUS_ERR_ALLOC,
    STATUS_ERR_OPEN,
    STATUS_ERR_READ,
    STATUS_ERR_FORMAT,
    STATUS_ERR_RANGE,
    STATUS_ERR_NOT_FOUND,
    STATUS_ERR_EMPTY
} Status;

#define GRADES_COUNT 5
#define LINE_BUF 512

/* Студент */
typedef struct {
    unsigned int id;
    char *name;
    char *surname;
    char *group;
    unsigned char *grades;
} Student;

/* Динамический массив студентов */
typedef struct {
    Student *data;
    size_t count;
    size_t cap;
} StudentArray;

/* ---------- String helpers ---------- */

/* Проверка: строка непустая и только латинские буквы */
static int is_latin_letters(const char *s)
{
    size_t i;
    if (s == NULL || s[0] == '\0') return 0;
    for (i = 0; s[i] != '\0'; i++) {
        if (!((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z')))
            return 0;
    }
    return 1;
}

/* Дублирование C-строки */
static char *dup_string(const char *s)
{
    size_t len;
    char *p;
    if (s == NULL) return NULL;
    len = strlen(s);
    p = (char *)malloc(len + 1);
    if (p == NULL) return NULL;
    memcpy(p, s, len + 1);
    return p;
}

/* ---------- Student ---------- */

static void student_destroy(Student *s)
{
    if (s == NULL) return;
    free(s->name);
    free(s->surname);
    free(s->group);
    free(s->grades);
    s->name = NULL;
    s->surname = NULL;
    s->group = NULL;
    s->grades = NULL;
    s->id = 0;
}

static Status student_create(unsigned int id, const char *name,
                             const char *surname, const char *group,
                             const unsigned char *grades, Student *out)
{
    int i;
    if (name == NULL || surname == NULL || group == NULL ||
        grades == NULL || out == NULL) return STATUS_ERR_ARG;
    if (!is_latin_letters(name)) return STATUS_ERR_FORMAT;
    if (!is_latin_letters(surname)) return STATUS_ERR_FORMAT;
    if (group[0] == '\0') return STATUS_ERR_FORMAT;

    memset(out, 0, sizeof(*out));
    out->name = dup_string(name);
    out->surname = dup_string(surname);
    out->group = dup_string(group);
    out->grades = (unsigned char *)malloc(GRADES_COUNT);
    if (out->name == NULL || out->surname == NULL ||
        out->group == NULL || out->grades == NULL) {
        student_destroy(out);
        return STATUS_ERR_ALLOC;
    }
    for (i = 0; i < GRADES_COUNT; i++) out->grades[i] = grades[i];
    out->id = id;
    return STATUS_OK;
}

static double student_average(const Student *s)
{
    int i;
    unsigned int sum = 0;
    if (s == NULL) return 0.0;
    for (i = 0; i < GRADES_COUNT; i++) sum += s->grades[i];
    return (double)sum / (double)GRADES_COUNT;
}

/* ---------- StudentArray ---------- */

static void array_init(StudentArray *a)
{
    if (a == NULL) return;
    a->data = NULL;
    a->count = 0;
    a->cap = 0;
}

static void array_destroy(StudentArray *a)
{
    size_t i;
    if (a == NULL) return;
    for (i = 0; i < a->count; i++) student_destroy(&a->data[i]);
    free(a->data);
    a->data = NULL;
    a->count = 0;
    a->cap = 0;
}

static Status array_reserve(StudentArray *a, size_t need)
{
    Student *new_data;
    size_t new_cap;
    if (a == NULL) return STATUS_ERR_ARG;
    if (need <= a->cap) return STATUS_OK;
    new_cap = (a->cap == 0) ? 4 : a->cap * 2;
    while (new_cap < need) new_cap *= 2;
    new_data = (Student *)realloc(a->data, new_cap * sizeof(Student));
    if (new_data == NULL) return STATUS_ERR_ALLOC;
    a->data = new_data;
    a->cap = new_cap;
    return STATUS_OK;
}

static Status array_push(StudentArray *a, const Student *s)
{
    Status st;
    if (a == NULL || s == NULL) return STATUS_ERR_ARG;
    st = array_reserve(a, a->count + 1);
    if (st != STATUS_OK) return st;
    a->data[a->count] = *s;
    a->count++;
    return STATUS_OK;
}

/* ---------- Валидация чисел ---------- */

static Status parse_uint(const char *s, unsigned int *out)
{
    char *end = NULL;
    unsigned long v;
    if (s == NULL || out == NULL) return STATUS_ERR_ARG;
    v = strtoul(s, &end, 10);
    if (end == s || *end != '\0') return STATUS_ERR_FORMAT;
    if (v > UINT_MAX) return STATUS_ERR_RANGE;
    *out = (unsigned int)v;
    return STATUS_OK;
}

static Status parse_grade(const char *s, unsigned char *out)
{
    char *end = NULL;
    unsigned long v;
    if (s == NULL || out == NULL) return STATUS_ERR_ARG;
    v = strtoul(s, &end, 10);
    if (end == s || *end != '\0') return STATUS_ERR_FORMAT;
    if (v > 255) return STATUS_ERR_RANGE;
    *out = (unsigned char)v;
    return STATUS_OK;
}

/* ---------- Чтение файла ---------- */

static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

static Status load_file(const char *path, StudentArray *out)
{
    FILE *f;
    char line[LINE_BUF];
    Status st = STATUS_OK;
    if (path == NULL || out == NULL) return STATUS_ERR_ARG;
    f = fopen(path, "r");
    if (f == NULL) return STATUS_ERR_OPEN;

    while (fgets(line, (int)sizeof(line), f) != NULL) {
        char *tok;
        unsigned int id;
        unsigned char grades[GRADES_COUNT];
        int i;
        char *name, *surname, *group;
        Student s;

        strip_newline(line);
        if (line[0] == '\0') continue;

        tok = strtok(line, " \t");
        if (tok == NULL) { st = STATUS_ERR_FORMAT; break; }
        st = parse_uint(tok, &id);
        if (st != STATUS_OK) break;

        name = strtok(NULL, " \t");
        surname = strtok(NULL, " \t");
        group = strtok(NULL, " \t");
        if (name == NULL || surname == NULL || group == NULL) {
            st = STATUS_ERR_FORMAT; break;
        }

        for (i = 0; i < GRADES_COUNT; i++) {
            tok = strtok(NULL, " \t");
            if (tok == NULL) { st = STATUS_ERR_FORMAT; break; }
            st = parse_grade(tok, &grades[i]);
            if (st != STATUS_OK) break;
        }
        if (st != STATUS_OK) break;

        memset(&s, 0, sizeof(s));
        st = student_create(id, name, surname, group, grades, &s);
        if (st != STATUS_OK) break;
        st = array_push(out, &s);
        if (st != STATUS_OK) { student_destroy(&s); break; }
    }
    fclose(f);
    return st;
}

/* ---------- Компараторы для qsort ---------- */

static int cmp_by_id(const void *a, const void *b)
{
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    if (sa->id < sb->id) return -1;
    if (sa->id > sb->id) return 1;
    return 0;
}

static int cmp_by_surname(const void *a, const void *b)
{
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->surname, sb->surname);
}

static int cmp_by_name(const void *a, const void *b)
{
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->name, sb->name);
}

static int cmp_by_group(const void *a, const void *b)
{
    const Student *sa = (const Student *)a;
    const Student *sb = (const Student *)b;
    return strcmp(sa->group, sb->group);
}

/* ---------- Поиск ---------- */

static void print_student(const Student *s)
{
    int i;
    if (s == NULL) return;
    printf("ID: %u, Name: %s, Surname: %s, Group: %s, Grades:",
           s->id, s->name, s->surname, s->group);
    for (i = 0; i < GRADES_COUNT; i++) printf(" %u", (unsigned)s->grades[i]);
    printf(", Avg: %.2f\n", student_average(s));
}

static size_t find_by_id(const StudentArray *a, unsigned int id)
{
    size_t i;
    if (a == NULL) return (size_t)-1;
    for (i = 0; i < a->count; i++) if (a->data[i].id == id) return i;
    return (size_t)-1;
}

static void find_by_field(const StudentArray *a, int field, const char *value)
{
    size_t i, found = 0;
    if (a == NULL || value == NULL) return;
    for (i = 0; i < a->count; i++) {
        const char *candidate = NULL;
        switch (field) {
            case 0: candidate = a->data[i].surname; break;
            case 1: candidate = a->data[i].name; break;
            case 2: candidate = a->data[i].group; break;
        }
        if (candidate != NULL && strcmp(candidate, value) == 0) {
            print_student(&a->data[i]);
            found++;
        }
    }
    if (found == 0) printf("Not found.\n");
}

/* ---------- Трассировка ---------- */

static Status trace_found_by_id(const char *path, const StudentArray *a,
                                unsigned int id)
{
    FILE *f;
    size_t idx;
    if (path == NULL || a == NULL) return STATUS_ERR_ARG;
    idx = find_by_id(a, id);
    if (idx == (size_t)-1) return STATUS_ERR_NOT_FOUND;
    f = fopen(path, "w");
    if (f == NULL) return STATUS_ERR_OPEN;
    fprintf(f, "ID: %u\n", a->data[idx].id);
    fprintf(f, "Full name: %s %s\n", a->data[idx].name, a->data[idx].surname);
    fprintf(f, "Group: %s\n", a->data[idx].group);
    fprintf(f, "Average grade: %.2f\n", student_average(&a->data[idx]));
    fclose(f);
    return STATUS_OK;
}

static Status trace_above_average(const char *path, const StudentArray *a)
{
    FILE *f;
    size_t i, n = 0;
    double total = 0.0, avg;
    if (path == NULL || a == NULL) return STATUS_ERR_ARG;
    if (a->count == 0) return STATUS_ERR_EMPTY;
    for (i = 0; i < a->count; i++) total += student_average(&a->data[i]);
    avg = total / (double)a->count;
    f = fopen(path, "w");
    if (f == NULL) return STATUS_ERR_OPEN;
    fprintf(f, "Overall average: %.2f\n", avg);
    fprintf(f, "Students above average:\n");
    for (i = 0; i < a->count; i++) {
        if (student_average(&a->data[i]) > avg) {
            fprintf(f, "%s %s\n", a->data[i].name, a->data[i].surname);
            n++;
        }
    }
    if (n == 0) fprintf(f, "(none)\n");
    fclose(f);
    return STATUS_OK;
}

/* ---------- Меню ---------- */

static void print_menu(void)
{
    printf("\n=== Students Menu ===\n");
    printf("1. Find by id\n");
    printf("2. Find by surname\n");
    printf("3. Find by name\n");
    printf("4. Find by group\n");
    printf("5. Sort by id\n");
    printf("6. Sort by surname\n");
    printf("7. Sort by name\n");
    printf("8. Sort by group\n");
    printf("9. Show all\n");
    printf("10. Trace: student by id -> file\n");
    printf("11. Trace: students above average -> file\n");
    printf("0. Exit\n");
    printf("Choice: ");
}

static int read_line(const char *prompt, char *buf, size_t size)
{
    if (prompt != NULL) printf("%s", prompt);
    if (fgets(buf, (int)size, stdin) == NULL) return 0;
    strip_newline(buf);
    return 1;
}

int main(int argc, char **argv)
{
    StudentArray arr;
    Status st;
    int running = 1;
    const char *trace_path;

    if (argc != 3) {
        printf("Usage: %s <students_file> <trace_file>\n",
               argc > 0 ? argv[0] : "program");
        return 1;
    }
    trace_path = argv[2];

    array_init(&arr);
    st = load_file(argv[1], &arr);
    if (st != STATUS_OK) {
        printf("Error: load file (status %d)\n", (int)st);
        array_destroy(&arr);
        return 1;
    }
    printf("Loaded %zu students.\n", arr.count);

    while (running) {
        char buf[LINE_BUF];
        int choice;
        print_menu();
        if (!read_line("", buf, sizeof(buf))) break;
        choice = atoi(buf);
        switch (choice) {
            case 1: {
                unsigned int id;
                char idbuf[64];
                size_t idx;
                if (!read_line("ID: ", idbuf, sizeof(idbuf))) break;
                st = parse_uint(idbuf, &id);
                if (st != STATUS_OK) { printf("Invalid id.\n"); break; }
                idx = find_by_id(&arr, id);
                if (idx == (size_t)-1) printf("Not found.\n");
                else print_student(&arr.data[idx]);
                break;
            }
            case 2:
            case 3:
            case 4: {
                char val[LINE_BUF];
                if (!read_line("Value: ", val, sizeof(val))) break;
                find_by_field(&arr, choice - 2, val);
                break;
            }
            case 5:
                qsort(arr.data, arr.count, sizeof(Student), cmp_by_id);
                printf("Sorted by id.\n");
                break;
            case 6:
                qsort(arr.data, arr.count, sizeof(Student), cmp_by_surname);
                printf("Sorted by surname.\n");
                break;
            case 7:
                qsort(arr.data, arr.count, sizeof(Student), cmp_by_name);
                printf("Sorted by name.\n");
                break;
            case 8:
                qsort(arr.data, arr.count, sizeof(Student), cmp_by_group);
                printf("Sorted by group.\n");
                break;
            case 9: {
                size_t i;
                if (arr.count == 0) { printf("Empty.\n"); break; }
                for (i = 0; i < arr.count; i++) print_student(&arr.data[i]);
                break;
            }
            case 10: {
                unsigned int id;
                char idbuf[64];
                if (!read_line("ID: ", idbuf, sizeof(idbuf))) break;
                st = parse_uint(idbuf, &id);
                if (st != STATUS_OK) { printf("Invalid id.\n"); break; }
                st = trace_found_by_id(trace_path, &arr, id);
                if (st == STATUS_OK) printf("Written to %s\n", trace_path);
                else if (st == STATUS_ERR_NOT_FOUND) printf("Not found.\n");
                else printf("Error: trace (status %d)\n", (int)st);
                break;
            }
            case 11:
                st = trace_above_average(trace_path, &arr);
                if (st == STATUS_OK) printf("Written to %s\n", trace_path);
                else if (st == STATUS_ERR_EMPTY) printf("No students.\n");
                else printf("Error: trace (status %d)\n", (int)st);
                break;
            case 0:
                running = 0;
                break;
            default:
                printf("Invalid choice.\n");
        }
    }

    array_destroy(&arr);
    return 0;
}