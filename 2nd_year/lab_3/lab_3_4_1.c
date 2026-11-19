#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Статус-коды */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARG,
    STATUS_ERR_ALLOC
} Status;

/* Тип String */
typedef struct {
    char *data;
    int length;
} String;

/* Создание экземпляра из C-строки */
static Status string_create(const char *src, String *out)
{
    size_t len;
    if (src == NULL || out == NULL) return STATUS_ERR_ARG;
    len = strlen(src);
    out->data = (char *)malloc(len + 1);
    if (out->data == NULL) return STATUS_ERR_ALLOC;
    memcpy(out->data, src, len + 1);
    out->length = (int)len;
    return STATUS_OK;
}

/* Удаление внутреннего содержимого */
static void string_destroy(String *s)
{
    if (s == NULL) return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

/* Отношение порядка: сначала по длине, потом лексикографически */
static Status string_compare_order(const String *a, const String *b, int *out)
{
    int cmp;
    if (a == NULL || b == NULL || out == NULL) return STATUS_ERR_ARG;
    if (a->length != b->length) {
        *out = (a->length < b->length) ? -1 : 1;
        return STATUS_OK;
    }
    cmp = strcmp(a->data, b->data);
    if (cmp < 0) *out = -1;
    else if (cmp > 0) *out = 1;
    else *out = 0;
    return STATUS_OK;
}

/* Эквивалентность (лексикографически) */
static Status string_equals(const String *a, const String *b, int *out)
{
    if (a == NULL || b == NULL || out == NULL) return STATUS_ERR_ARG;
    *out = (strcmp(a->data, b->data) == 0) ? 1 : 0;
    return STATUS_OK;
}

/* Копирование в существующий экземпляр */
static Status string_copy(const String *src, String *dst)
{
    char *new_data;
    size_t len;
    if (src == NULL || dst == NULL) return STATUS_ERR_ARG;
    if (src == dst) return STATUS_OK;
    len = (size_t)src->length;
    new_data = (char *)malloc(len + 1);
    if (new_data == NULL) return STATUS_ERR_ALLOC;
    memcpy(new_data, src->data, len + 1);
    free(dst->data);
    dst->data = new_data;
    dst->length = src->length;
    return STATUS_OK;
}

/* Копирование в новый экземпляр в куче */
static Status string_clone(const String *src, String **out)
{
    String *s;
    Status st;
    if (src == NULL || out == NULL) return STATUS_ERR_ARG;
    s = (String *)malloc(sizeof(String));
    if (s == NULL) return STATUS_ERR_ALLOC;
    s->data = NULL;
    s->length = 0;
    st = string_create(src->data, s);
    if (st != STATUS_OK) { free(s); return st; }
    *out = s;
    return STATUS_OK;
}

/* Конкатенация: дописать src к dst */
static Status string_concat(String *dst, const String *src)
{
    char *tmp_src = NULL;
    size_t new_len;
    char *new_data;
    if (dst == NULL || src == NULL) return STATUS_ERR_ARG;
    if (dst == src) {
        /* самоконкатенация: делаем временную копию */
        tmp_src = (char *)malloc((size_t)src->length + 1);
        if (tmp_src == NULL) return STATUS_ERR_ALLOC;
        memcpy(tmp_src, src->data, (size_t)src->length + 1);
        new_len = (size_t)dst->length * 2;
        new_data = (char *)realloc(dst->data, new_len + 1);
        if (new_data == NULL) { free(tmp_src); return STATUS_ERR_ALLOC; }
        memcpy(new_data + dst->length, tmp_src, (size_t)src->length);
        new_data[new_len] = '\0';
        dst->data = new_data;
        dst->length = (int)new_len;
        free(tmp_src);
        return STATUS_OK;
    }
    new_len = (size_t)dst->length + (size_t)src->length;
    new_data = (char *)realloc(dst->data, new_len + 1);
    if (new_data == NULL) return STATUS_ERR_ALLOC;
    memcpy(new_data + dst->length, src->data, (size_t)src->length + 1);
    dst->data = new_data;
    dst->length = (int)new_len;
    return STATUS_OK;
}

/* Печать строки */
static void string_print(const String *s, const char *label)
{
    if (s == NULL) return;
    printf("%s: \"%s\" (len=%d)\n", label ? label : "String", s->data, s->length);
}

/* Демонстрация */
int main(void)
{
    String a = {NULL, 0}, b = {NULL, 0}, c = {NULL, 0};
    String *clone = NULL;
    Status st;
    int cmp, eq;

    /* создание */
    st = string_create("hello", &a);
    if (st != STATUS_OK) { printf("Error: create a\n"); return 1; }
    st = string_create("world", &b);
    if (st != STATUS_OK) { printf("Error: create b\n"); string_destroy(&a); return 1; }

    string_print(&a, "a");
    string_print(&b, "b");

    /* порядок: длины равны (5 и 5), лексикографически "hello" < "world" */
    st = string_compare_order(&a, &b, &cmp);
    if (st != STATUS_OK) { printf("Error: compare\n"); goto cleanup; }
    printf("order(a,b) = %d\n", cmp);

    /* эквивалентность */
    st = string_equals(&a, &b, &eq);
    if (st != STATUS_OK) { printf("Error: equals\n"); goto cleanup; }
    printf("equals(a,b) = %d\n", eq);

    /* копирование в существующий */
    st = string_copy(&a, &c);
    if (st != STATUS_OK) { printf("Error: copy\n"); goto cleanup; }
    string_print(&c, "c (copy of a)");

    /* клонирование в новый */
    st = string_clone(&b, &clone);
    if (st != STATUS_OK) { printf("Error: clone\n"); goto cleanup; }
    string_print(clone, "clone of b");

    /* конкатенация: a += b */
    st = string_concat(&a, &b);
    if (st != STATUS_OK) { printf("Error: concat\n"); goto cleanup; }
    string_print(&a, "a after concat");

    /* самоконкатенация: c += c */
    st = string_concat(&c, &c);
    if (st != STATUS_OK) { printf("Error: self concat\n"); goto cleanup; }
    string_print(&c, "c after self-concat");

    /* сравнение после конкатенации: a длиннее c */
    st = string_compare_order(&a, &c, &cmp);
    if (st != STATUS_OK) { printf("Error: compare2\n"); goto cleanup; }
    printf("order(a_after, c_after) = %d\n", cmp);

    /* создание из пустой строки */
    {
        String empty = {NULL, 0};
        st = string_create("", &empty);
        if (st != STATUS_OK) { printf("Error: empty\n"); goto cleanup; }
        string_print(&empty, "empty");
        string_destroy(&empty);
    }

cleanup:
    string_destroy(&a);
    string_destroy(&b);
    string_destroy(&c);
    if (clone != NULL) { string_destroy(clone); free(clone); }
    return 0;
}