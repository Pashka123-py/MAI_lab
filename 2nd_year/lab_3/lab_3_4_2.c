#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* Константы */
#define ID_LEN 14
#define INDEX_LEN 6
#define DATE_LEN 19
#define DELIVERY_TERM_DAYS 30
#define LINE_BUF 512

/* Статус-коды */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARG,
    STATUS_ERR_ALLOC,
    STATUS_ERR_INPUT,
    STATUS_ERR_RANGE,
    STATUS_ERR_FORMAT,
    STATUS_ERR_NOT_FOUND,
    STATUS_ERR_EMPTY,
    STATUS_ERR_FULL
} Status;

/* Тип String */
typedef struct {
    char *data;
    int length;
} String;

/* Address */
typedef struct {
    String city;
    String street;
    unsigned int house;
    String building;
    unsigned int flat;
    String index;
} Address;

/* Mail */
typedef struct {
    Address recipient;
    double weight;
    String post_id;
    String created_at;
    String delivered_at;
} Mail;

/* Post */
typedef struct {
    Address *current_office;
    Mail *mails;
    size_t count;
    size_t cap;
} Post;

/* ---------- String ---------- */
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

static void string_destroy(String *s)
{
    if (s == NULL) return;
    free(s->data);
    s->data = NULL;
    s->length = 0;
}

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

static Status string_equals(const String *a, const String *b, int *out)
{
    if (a == NULL || b == NULL || out == NULL) return STATUS_ERR_ARG;
    *out = (strcmp(a->data, b->data) == 0) ? 1 : 0;
    return STATUS_OK;
}

static Status string_compare_order(const String *a, const String *b, int *out)
{
    int cmp;
    if (a == NULL || b == NULL || out == NULL) return STATUS_ERR_ARG;
    if (a->length != b->length) {
        *out = (a->length < b->length) ? -1 : 1;
        return STATUS_OK;
    }
    cmp = strcmp(a->data, b->data);
    *out = (cmp < 0) ? -1 : (cmp > 0) ? 1 : 0;
    return STATUS_OK;
}

/* ---------- Валидация ---------- */
static int is_all_digits(const char *s, size_t n)
{
    size_t i;
    if (s == NULL || strlen(s) != n) return 0;
    for (i = 0; i < n; i++) if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

/* Парсинг "dd:MM:yyyy hh:mm:ss" и проверка валидности через mktime */
static Status parse_datetime(const char *s, time_t *out)
{
    struct tm tm_val;
    int d, M, y, h, m, sec;
    time_t t;
    if (s == NULL || out == NULL) return STATUS_ERR_ARG;
    if (strlen(s) != DATE_LEN) return STATUS_ERR_FORMAT;
    if (sscanf(s, "%2d:%2d:%4d %2d:%2d:%2d", &d, &M, &y, &h, &m, &sec) != 6)
        return STATUS_ERR_FORMAT;
    if (d < 1 || d > 31 || M < 1 || M > 12 || y < 1900 || y > 2100)
        return STATUS_ERR_RANGE;
    if (h < 0 || h > 23 || m < 0 || m > 59 || sec < 0 || sec > 59)
        return STATUS_ERR_RANGE;
    memset(&tm_val, 0, sizeof(tm_val));
    tm_val.tm_mday = d;
    tm_val.tm_mon = M - 1;
    tm_val.tm_year = y - 1900;
    tm_val.tm_hour = h;
    tm_val.tm_min = m;
    tm_val.tm_sec = sec;
    tm_val.tm_isdst = -1;
    t = mktime(&tm_val);
    if (t == (time_t)-1) return STATUS_ERR_RANGE;
    /* проверка, что mktime не нормализовал дату (например, 31 февраля) */
    if (tm_val.tm_mday != d || tm_val.tm_mon != M - 1 || tm_val.tm_year != y - 1900)
        return STATUS_ERR_RANGE;
    *out = t;
    return STATUS_OK;
}

/* ---------- Address ---------- */
static Status address_create(const char *city, const char *street,
                             unsigned int house, const char *building,
                             unsigned int flat, const char *index,
                             Address *out)
{
    Status st;
    if (city == NULL || street == NULL || building == NULL || index == NULL || out == NULL)
        return STATUS_ERR_ARG;
    if (city[0] == '\0' || street[0] == '\0') return STATUS_ERR_INPUT;
    if (house == 0 || flat == 0) return STATUS_ERR_RANGE;
    if (!is_all_digits(index, INDEX_LEN)) return STATUS_ERR_FORMAT;

    memset(out, 0, sizeof(*out));
    st = string_create(city, &out->city); if (st != STATUS_OK) goto fail;
    st = string_create(street, &out->street); if (st != STATUS_OK) goto fail;
    st = string_create(building, &out->building); if (st != STATUS_OK) goto fail;
    st = string_create(index, &out->index); if (st != STATUS_OK) goto fail;
    out->house = house;
    out->flat = flat;
    return STATUS_OK;
fail:
    string_destroy(&out->city);
    string_destroy(&out->street);
    string_destroy(&out->building);
    string_destroy(&out->index);
    return st;
}

static void address_destroy(Address *a)
{
    if (a == NULL) return;
    string_destroy(&a->city);
    string_destroy(&a->street);
    string_destroy(&a->building);
    string_destroy(&a->index);
}

static Status address_copy(const Address *src, Address *dst)
{
    Status st;
    if (src == NULL || dst == NULL) return STATUS_ERR_ARG;
    st = string_copy(&src->city, &dst->city); if (st != STATUS_OK) return st;
    st = string_copy(&src->street, &dst->street); if (st != STATUS_OK) return st;
    st = string_copy(&src->building, &dst->building); if (st != STATUS_OK) return st;
    st = string_copy(&src->index, &dst->index); if (st != STATUS_OK) return st;
    dst->house = src->house;
    dst->flat = src->flat;
    return STATUS_OK;
}

static void address_print(const Address *a)
{
    if (a == NULL) return;
    printf("  City: %s\n", a->city.data);
    printf("  Street: %s, house %u, building %s, flat %u\n",
           a->street.data, a->house, a->building.data, a->flat);
    printf("  Index: %s\n", a->index.data);
}

/* ---------- Mail ---------- */
static Status mail_create(const Address *recipient, double weight,
                          const char *post_id, const char *created_at,
                          const char *delivered_at, Mail *out)
{
    Status st;
    time_t t;
    if (recipient == NULL || post_id == NULL || created_at == NULL ||
        delivered_at == NULL || out == NULL) return STATUS_ERR_ARG;
    if (weight < 0.0) return STATUS_ERR_RANGE;
    if (!is_all_digits(post_id, ID_LEN)) return STATUS_ERR_FORMAT;
    st = parse_datetime(created_at, &t);
    if (st != STATUS_OK) return st;
    if (delivered_at[0] != '\0') {
        st = parse_datetime(delivered_at, &t);
        if (st != STATUS_OK) return st;
    }
    memset(out, 0, sizeof(*out));
    st = address_copy(recipient, &out->recipient); if (st != STATUS_OK) return st;
    st = string_create(post_id, &out->post_id); if (st != STATUS_OK) goto fail;
    st = string_create(created_at, &out->created_at); if (st != STATUS_OK) goto fail;
    st = string_create(delivered_at, &out->delivered_at); if (st != STATUS_OK) goto fail;
    out->weight = weight;
    return STATUS_OK;
fail:
    address_destroy(&out->recipient);
    string_destroy(&out->post_id);
    string_destroy(&out->created_at);
    string_destroy(&out->delivered_at);
    return st;
}

static void mail_destroy(Mail *m)
{
    if (m == NULL) return;
    address_destroy(&m->recipient);
    string_destroy(&m->post_id);
    string_destroy(&m->created_at);
    string_destroy(&m->delivered_at);
}

static Status mail_copy(const Mail *src, Mail *dst)
{
    Status st;
    if (src == NULL || dst == NULL) return STATUS_ERR_ARG;
    memset(dst, 0, sizeof(*dst));
    st = address_copy(&src->recipient, &dst->recipient); if (st != STATUS_OK) return st;
    st = string_copy(&src->post_id, &dst->post_id); if (st != STATUS_OK) return st;
    st = string_copy(&src->created_at, &dst->created_at); if (st != STATUS_OK) return st;
    st = string_copy(&src->delivered_at, &dst->delivered_at); if (st != STATUS_OK) return st;
    dst->weight = src->weight;
    return STATUS_OK;
}

static int mail_is_delivered(const Mail *m)
{
    return (m != NULL && m->delivered_at.length > 0);
}

static void mail_print(const Mail *m)
{
    if (m == NULL) return;
    printf("Mail ID: %s\n", m->post_id.data);
    printf("  Weight: %.3f kg\n", m->weight);
    printf("  Created: %s\n", m->created_at.data);
    if (mail_is_delivered(m)) printf("  Delivered: %s\n", m->delivered_at.data);
    else printf("  Delivered: (not delivered)\n");
    printf("  Recipient:\n");
    address_print(&m->recipient);
}

/* ---------- Post ---------- */
static Status post_init(Post *p, Address *office)
{
    if (p == NULL || office == NULL) return STATUS_ERR_ARG;
    p->current_office = office;
    p->mails = NULL;
    p->count = 0;
    p->cap = 0;
    return STATUS_OK;
}

static void post_destroy(Post *p)
{
    size_t i;
    if (p == NULL) return;
    for (i = 0; i < p->count; i++) mail_destroy(&p->mails[i]);
    free(p->mails);
    p->mails = NULL;
    p->count = 0;
    p->cap = 0;
}

static Status post_reserve(Post *p, size_t need)
{
    Mail *new_arr;
    size_t new_cap;
    if (p == NULL) return STATUS_ERR_ARG;
    if (need <= p->cap) return STATUS_OK;
    new_cap = (p->cap == 0) ? 4 : p->cap * 2;
    while (new_cap < need) new_cap *= 2;
    new_arr = (Mail *)realloc(p->mails, new_cap * sizeof(Mail));
    if (new_arr == NULL) return STATUS_ERR_ALLOC;
    p->mails = new_arr;
    p->cap = new_cap;
    return STATUS_OK;
}

/* Сортировка: индекс получателя, затем ID */
static int mail_cmp_index_id(const Mail *a, const Mail *b)
{
    int cmp;
    string_compare_order(&a->recipient.index, &b->recipient.index, &cmp);
    if (cmp != 0) return cmp;
    string_compare_order(&a->post_id, &b->post_id, &cmp);
    return cmp;
}

static void post_sort_main(Post *p)
{
    size_t i, j;
    if (p == NULL || p->count < 2) return;
    for (i = 1; i < p->count; i++) {
        Mail key;
        if (mail_copy(&p->mails[i], &key) != STATUS_OK) return;
        j = i;
        while (j > 0 && mail_cmp_index_id(&p->mails[j - 1], &key) > 0) {
            mail_destroy(&p->mails[j]);
            if (mail_copy(&p->mails[j - 1], &p->mails[j]) != STATUS_OK) {
                mail_destroy(&key);
                return;
            }
            j--;
        }
        mail_destroy(&p->mails[j]);
        p->mails[j] = key;
    }
}

static Status post_add(Post *p, const Mail *m)
{
    Status st;
    if (p == NULL || m == NULL) return STATUS_ERR_ARG;
    st = post_reserve(p, p->count + 1);
    if (st != STATUS_OK) return st;
    memset(&p->mails[p->count], 0, sizeof(Mail));
    st = mail_copy(m, &p->mails[p->count]);
    if (st != STATUS_OK) return st;
    p->count++;
    post_sort_main(p);
    return STATUS_OK;
}

static Status post_remove_by_id(Post *p, const String *id, int *removed)
{
    size_t i;
    if (p == NULL || id == NULL || removed == NULL) return STATUS_ERR_ARG;
    *removed = 0;
    for (i = 0; i < p->count; i++) {
        int eq;
        string_equals(&p->mails[i].post_id, id, &eq);
        if (eq) {
            mail_destroy(&p->mails[i]);
            if (i + 1 < p->count) {
                memmove(&p->mails[i], &p->mails[i + 1],
                        (p->count - i - 1) * sizeof(Mail));
            }
            p->count--;
            *removed = 1;
            return STATUS_OK;
        }
    }
    return STATUS_OK;
}

static Status post_find_by_id(const Post *p, const String *id, Mail **out)
{
    size_t i;
    if (p == NULL || id == NULL || out == NULL) return STATUS_ERR_ARG;
    for (i = 0; i < p->count; i++) {
        int eq;
        string_equals(&p->mails[i].post_id, id, &eq);
        if (eq) { *out = (Mail *)&p->mails[i]; return STATUS_OK; }
    }
    return STATUS_ERR_NOT_FOUND;
}

/* Сравнение по created_at (для сортировки указателей) */
static int ptr_cmp_created(const void *a, const void *b)
{
    const Mail *ma = *(const Mail * const *)a;
    const Mail *mb = *(const Mail * const *)b;
    time_t ta = 0, tb = 0;
    parse_datetime(ma->created_at.data, &ta);
    parse_datetime(mb->created_at.data, &tb);
    if (ta < tb) return -1;
    if (ta > tb) return 1;
    return 0;
}

/* Собрать указатели по предикату и вывести по created_at */
static Status post_print_filtered(const Post *p,
                                  int (*pred)(const Mail *, time_t),
                                  time_t now)
{
    Mail **arr;
    size_t i, n = 0;
    if (p == NULL) return STATUS_ERR_ARG;
    if (p->count == 0) { printf("No mails.\n"); return STATUS_OK; }
    arr = (Mail **)malloc(p->count * sizeof(Mail *));
    if (arr == NULL) return STATUS_ERR_ALLOC;
    for (i = 0; i < p->count; i++) {
        if (pred(&p->mails[i], now)) arr[n++] = &p->mails[i];
    }
    if (n == 0) { printf("No matching mails.\n"); free(arr); return STATUS_OK; }
    qsort(arr, n, sizeof(Mail *), ptr_cmp_created);
    for (i = 0; i < n; i++) {
        mail_print(arr[i]);
        printf("---\n");
    }
    free(arr);
    return STATUS_OK;
}

static int pred_delivered(const Mail *m, time_t now)
{
    (void)now;
    return mail_is_delivered(m);
}

static int pred_overdue(const Mail *m, time_t now)
{
    time_t created;
    if (mail_is_delivered(m)) return 0;
    if (parse_datetime(m->created_at.data, &created) != STATUS_OK) return 0;
    return (now - created) > (time_t)DELIVERY_TERM_DAYS * 86400;
}

/* ---------- Ввод ---------- */
static void strip_newline(char *s)
{
    size_t len = strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}

static int read_line(const char *prompt, char *buf, size_t size)
{
    if (prompt != NULL) printf("%s", prompt);
    if (fgets(buf, (int)size, stdin) == NULL) return 0;
    strip_newline(buf);
    return 1;
}

static Status read_uint(const char *prompt, unsigned int *out)
{
    char buf[64];
    char *end = NULL;
    unsigned long v;
    if (out == NULL) return STATUS_ERR_ARG;
    if (!read_line(prompt, buf, sizeof(buf))) return STATUS_ERR_INPUT;
    v = strtoul(buf, &end, 10);
    if (end == buf || *end != '\0') return STATUS_ERR_INPUT;
    if (v == 0 || v > 1000000) return STATUS_ERR_RANGE;
    *out = (unsigned int)v;
    return STATUS_OK;
}

static Status read_double(const char *prompt, double *out)
{
    char buf[64];
    char *end = NULL;
    double v;
    if (out == NULL) return STATUS_ERR_ARG;
    if (!read_line(prompt, buf, sizeof(buf))) return STATUS_ERR_INPUT;
    v = strtod(buf, &end);
    if (end == buf || *end != '\0') return STATUS_ERR_INPUT;
    if (v < 0.0) return STATUS_ERR_RANGE;
    *out = v;
    return STATUS_OK;
}


/* Ввод адреса */
static Status input_address(Address *out)
{
    char city[LINE_BUF], street[LINE_BUF], building[LINE_BUF], index[LINE_BUF];
    unsigned int house, flat;
    Status st;
    if (out == NULL) return STATUS_ERR_ARG;
    if (!read_line("City: ", city, sizeof(city))) return STATUS_ERR_INPUT;
    if (city[0] == '\0') return STATUS_ERR_INPUT;
    if (!read_line("Street: ", street, sizeof(street))) return STATUS_ERR_INPUT;
    if (street[0] == '\0') return STATUS_ERR_INPUT;
    st = read_uint("House (natural): ", &house);
    if (st != STATUS_OK) return st;
    if (!read_line("Building (may be empty): ", building, sizeof(building)))
        return STATUS_ERR_INPUT;
    st = read_uint("Flat (natural): ", &flat);
    if (st != STATUS_OK) return st;
    if (!read_line("Index (6 digits): ", index, sizeof(index))) return STATUS_ERR_INPUT;
    return address_create(city, street, house, building, flat, index, out);
}

/* Ввод письма */
static Status input_mail(Mail *out)
{
    Address rec;
    double weight;
    char id[LINE_BUF], created[LINE_BUF], delivered[LINE_BUF];
    Status st;
    if (out == NULL) return STATUS_ERR_ARG;
    printf("--- Recipient address ---\n");
    st = input_address(&rec);
    if (st != STATUS_OK) return st;
    st = read_double("Weight (>=0): ", &weight);
    if (st != STATUS_OK) { address_destroy(&rec); return st; }
    if (!read_line("Post ID (14 digits): ", id, sizeof(id))) {
        address_destroy(&rec); return STATUS_ERR_INPUT;
    }
    if (!read_line("Created at (dd:MM:yyyy hh:mm:ss): ", created, sizeof(created))) {
        address_destroy(&rec); return STATUS_ERR_INPUT;
    }
    if (!read_line("Delivered at (empty if not delivered): ", delivered, sizeof(delivered))) {
        address_destroy(&rec); return STATUS_ERR_INPUT;
    }
    st = mail_create(&rec, weight, id, created, delivered, out);
    address_destroy(&rec);
    return st;
}

/* ---------- Меню ---------- */
static void print_menu(void)
{
    printf("\n=== Post Office Menu ===\n");
    printf("1. Add mail\n");
    printf("2. Delete mail by ID\n");
    printf("3. Find mail by ID\n");
    printf("4. Show all delivered\n");
    printf("5. Show all overdue\n");
    printf("6. Show all mails (sorted)\n");
    printf("0. Exit\n");
    printf("Choice: ");
}

int main(void)
{
    Address office;
    Post post;
    Status st;
    int running = 1;
    char buf[LINE_BUF];

    st = address_create("Moscow", "Tverskaya", 1, "", 1, "000000", &office);
    if (st != STATUS_OK) { printf("Error: office init\n"); return 1; }
    post_init(&post, &office);

    while (running) {
        int choice;
        print_menu();
        if (!read_line("", buf, sizeof(buf))) break;
        choice = atoi(buf);
        switch (choice) {
            case 1: {
                Mail m;
                memset(&m, 0, sizeof(m));
                st = input_mail(&m);
                if (st != STATUS_OK) { printf("Error: input (status %d)\n", (int)st); break; }
                st = post_add(&post, &m);
                mail_destroy(&m);
                if (st != STATUS_OK) printf("Error: add (status %d)\n", (int)st);
                else printf("Mail added.\n");
                break;
            }
            case 2: {
                String id = {NULL, 0};
                int removed = 0;
                if (!read_line("Post ID: ", buf, sizeof(buf))) break;
                st = string_create(buf, &id);
                if (st != STATUS_OK) { printf("Error\n"); break; }
                st = post_remove_by_id(&post, &id, &removed);
                string_destroy(&id);
                if (st != STATUS_OK) printf("Error: remove\n");
                else if (removed) printf("Removed.\n");
                else printf("Not found.\n");
                break;
            }
            case 3: {
                String id = {NULL, 0};
                Mail *found = NULL;
                if (!read_line("Post ID: ", buf, sizeof(buf))) break;
                st = string_create(buf, &id);
                if (st != STATUS_OK) { printf("Error\n"); break; }
                st = post_find_by_id(&post, &id, &found);
                string_destroy(&id);
                if (st == STATUS_OK) mail_print(found);
                else printf("Not found.\n");
                break;
            }
            case 4:
                st = post_print_filtered(&post, pred_delivered, time(NULL));
                if (st != STATUS_OK) printf("Error\n");
                break;
            case 5:
                st = post_print_filtered(&post, pred_overdue, time(NULL));
                if (st != STATUS_OK) printf("Error\n");
                break;
            case 6: {
                size_t i;
                if (post.count == 0) { printf("No mails.\n"); break; }
                for (i = 0; i < post.count; i++) {
                    mail_print(&post.mails[i]);
                    printf("---\n");
                }
                break;
            }
            case 0:
                running = 0;
                break;
            default:
                printf("Invalid choice.\n");
        }
    }

    post_destroy(&post);
    address_destroy(&office);
    return 0;
}