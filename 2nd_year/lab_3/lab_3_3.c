#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Статус-коды функций */
typedef enum {
    STATUS_OK = 0,
    STATUS_ERR_ARG,
    STATUS_ERR_ALLOC,
    STATUS_ERR_OPEN,
    STATUS_ERR_READ,
    STATUS_ERR_EMPTY_HEAP,
    STATUS_ERR_RANGE
} Status;

#define ALPHABET 256
#define EPS 1e-9

/* Узел дерева Хаффмана */
typedef struct Node {
    long long freq;
    int byte;                 /* -1 для внутренних узлов */
    struct Node *left;
    struct Node *right;
} Node;

/* Минимальная двоичная куча */
typedef struct {
    Node **data;
    size_t size;
    size_t cap;
} MinHeap;

/* Коды для каждого байта */
typedef struct {
    char code[ALPHABET][ALPHABET]; /* максимум 256 символов на код (вырожденный случай) */
    int len[ALPHABET];
} CodeTable;

/* Создание кучи */
static Status heap_init(MinHeap *h, size_t cap)
{
    if (h == NULL || cap == 0) return STATUS_ERR_ARG;
    h->data = (Node **)malloc(cap * sizeof(Node *));
    if (h->data == NULL) return STATUS_ERR_ALLOC;
    h->size = 0;
    h->cap = cap;
    return STATUS_OK;
}

/* Освобождение кучи (без узлов) */
static void heap_free(MinHeap *h)
{
    if (h == NULL) return;
    free(h->data);
    h->data = NULL;
    h->size = 0;
    h->cap = 0;
}

/* Обмен элементов */
static void heap_swap(Node **a, Node **b)
{
    Node *t = *a;
    *a = *b;
    *b = t;
}

/* Просеивание вверх */
static void heap_sift_up(MinHeap *h, size_t i)
{
    while (i > 0) {
        size_t parent = (i - 1) / 2;
        if (h->data[parent]->freq <= h->data[i]->freq) break;
        heap_swap(&h->data[parent], &h->data[i]);
        i = parent;
    }
}

/* Просеивание вниз */
static void heap_sift_down(MinHeap *h, size_t i)
{
    for (;;) {
        size_t l = 2 * i + 1;
        size_t r = 2 * i + 2;
        size_t smallest = i;
        if (l < h->size && h->data[l]->freq < h->data[smallest]->freq) smallest = l;
        if (r < h->size && h->data[r]->freq < h->data[smallest]->freq) smallest = r;
        if (smallest == i) break;
        heap_swap(&h->data[i], &h->data[smallest]);
        i = smallest;
    }
}

/* Вставка узла */
static Status heap_push(MinHeap *h, Node *node)
{
    if (h == NULL || node == NULL) return STATUS_ERR_ARG;
    if (h->size >= h->cap) return STATUS_ERR_ALLOC;
    h->data[h->size] = node;
    heap_sift_up(h, h->size);
    h->size++;
    return STATUS_OK;
}

/* Извлечение минимума */
static Status heap_pop(MinHeap *h, Node **out)
{
    if (h == NULL || out == NULL) return STATUS_ERR_ARG;
    if (h->size == 0) return STATUS_ERR_EMPTY_HEAP;
    *out = h->data[0];
    h->size--;
    if (h->size > 0) {
        h->data[0] = h->data[h->size];
        heap_sift_down(h, 0);
    }
    return STATUS_OK;
}

/* Создание узла */
static Node *node_create(long long freq, int byte, Node *l, Node *r)
{
    Node *n = (Node *)malloc(sizeof(Node));
    if (n == NULL) return NULL;
    n->freq = freq;
    n->byte = byte;
    n->left = l;
    n->right = r;
    return n;
}

/* Освобождение дерева (рекурсивно) */
static void tree_free(Node *root)
{
    if (root == NULL) return;
    tree_free(root->left);
    tree_free(root->right);
    free(root);
}

/* Построение дерева Хаффмана, возврат корня через out */
static Status build_huffman(const long long freq[ALPHABET], Node **out_root)
{
    MinHeap h;
    Status st;
    int i;
    int unique = 0;
    Node *root = NULL;

    if (freq == NULL || out_root == NULL) return STATUS_ERR_ARG;

    for (i = 0; i < ALPHABET; i++) if (freq[i] > 0) unique++;
    if (unique == 0) { *out_root = NULL; return STATUS_OK; }

    st = heap_init(&h, (size_t)unique);
    if (st != STATUS_OK) return st;

    /* Создаём листья */
    for (i = 0; i < ALPHABET; i++) {
        if (freq[i] > 0) {
            Node *leaf = node_create(freq[i], i, NULL, NULL);
            if (leaf == NULL) {
                /* освобождаем уже созданные листья в куче */
                while (h.size > 0) { Node *tmp; heap_pop(&h, &tmp); free(tmp); }
                heap_free(&h);
                return STATUS_ERR_ALLOC;
            }
            st = heap_push(&h, leaf);
            if (st != STATUS_OK) {
                free(leaf);
                while (h.size > 0) { Node *tmp; heap_pop(&h, &tmp); free(tmp); }
                heap_free(&h);
                return st;
            }
        }
    }

    /* Сливаем, пока не останется один узел */
    while (h.size > 1) {
        Node *a = NULL, *b = NULL, *parent = NULL;
        st = heap_pop(&h, &a);
        if (st != STATUS_OK) { tree_free(root); heap_free(&h); return st; }
        st = heap_pop(&h, &b);
        if (st != STATUS_OK) { free(a); tree_free(root); heap_free(&h); return st; }
        parent = node_create(a->freq + b->freq, -1, a, b);
        if (parent == NULL) {
            free(a); free(b);
            while (h.size > 0) { Node *tmp; heap_pop(&h, &tmp); tree_free(tmp); }
            heap_free(&h);
            return STATUS_ERR_ALLOC;
        }
        st = heap_push(&h, parent);
        if (st != STATUS_OK) {
            tree_free(parent);
            while (h.size > 0) { Node *tmp; heap_pop(&h, &tmp); tree_free(tmp); }
            heap_free(&h);
            return st;
        }
    }

    st = heap_pop(&h, &root);
    heap_free(&h);
    if (st != STATUS_OK) return st;
    *out_root = root;
    return STATUS_OK;
}

/* Обход дерева: заполняем коды */
static void fill_codes(Node *node, CodeTable *table, char *buf, int depth)
{
    if (node == NULL || table == NULL || buf == NULL) return;
    if (node->left == NULL && node->right == NULL) {
        if (depth == 0) {
            /* единственный байт в файле — код "0" */
            table->code[node->byte][0] = '0';
            table->code[node->byte][1] = '\0';
            table->len[node->byte] = 1;
        } else {
            memcpy(table->code[node->byte], buf, (size_t)depth);
            table->code[node->byte][depth] = '\0';
            table->len[node->byte] = depth;
        }
        return;
    }
    if (node->left != NULL) {
        buf[depth] = '0';
        fill_codes(node->left, table, buf, depth + 1);
    }
    if (node->right != NULL) {
        buf[depth] = '1';
        fill_codes(node->right, table, buf, depth + 1);
    }
}

/* Подсчёт частот из файла */
static Status count_freq(const char *path, long long freq[ALPHABET], long long *out_size)
{
    FILE *f;
    unsigned char buf[4096];
    size_t got;
    long long total = 0;
    int i;

    if (path == NULL || freq == NULL || out_size == NULL) return STATUS_ERR_ARG;

    for (i = 0; i < ALPHABET; i++) freq[i] = 0;

    f = fopen(path, "rb");
    if (f == NULL) return STATUS_ERR_OPEN;

    while ((got = fread(buf, 1, sizeof(buf), f)) > 0) {
        size_t j;
        for (j = 0; j < got; j++) freq[buf[j]]++;
        total += (long long)got;
    }
    if (ferror(f)) {
        fclose(f);
        return STATUS_ERR_READ;
    }
    fclose(f);
    *out_size = total;
    return STATUS_OK;
}

/* Вывод таблицы */
static void print_table(const long long freq[ALPHABET], const CodeTable *table, long long size)
{
    int i;
    if (size == 0) return;
    printf("Byte\tFrequency\tCode\n");
    for (i = 0; i < ALPHABET; i++) {
        if (freq[i] > 0) {
            printf("%d\t%lld\t%s\n", i, freq[i], table->code[i]);
        }
    }
}

/* Основная обработка */
static Status process(const char *path)
{
    long long freq[ALPHABET];
    long long size = 0;
    Node *root = NULL;
    CodeTable table;
    char buf[ALPHABET];
    Status st;
    long long total_bits = 0;
    long long total_bytes = 0;
    int i;

    memset(&table, 0, sizeof(table));

    st = count_freq(path, freq, &size);
    if (st != STATUS_OK) return st;

    if (size == 0) {
        printf("Byte\tFrequency\tCode\n");
        printf("Original size: 0 bytes\n");
        printf("Encoded size: 0 bits, 0 bytes\n");
        printf("Compression saving: 0.00%%\n");
        return STATUS_OK;
    }

    st = build_huffman(freq, &root);
    if (st != STATUS_OK) return st;

    fill_codes(root, &table, buf, 0);

    print_table(freq, &table, size);

    for (i = 0; i < ALPHABET; i++) {
        if (freq[i] > 0) total_bits += freq[i] * (long long)table.len[i];
    }
    total_bytes = (total_bits + 7) / 8;

    printf("Original size: %lld bytes\n", size);
    printf("Encoded size: %lld bits, %lld bytes\n", total_bits, total_bytes);
    {
        double saving = 100.0 * (1.0 - (double)total_bytes / (double)size);
        if (saving < EPS && saving > -EPS) saving = 0.0;
        printf("Compression saving: %.2f%%\n", saving);
    }

    tree_free(root);
    return STATUS_OK;
}

int main(int argc, char **argv)
{
    Status st;

    if (argc != 2) {
        printf("Usage: %s <file_path>\n", argc > 0 ? argv[0] : "program");
        return 1;
    }
    if (argv[1] == NULL || argv[1][0] == '\0') {
        printf("Error: empty file path\n");
        return 1;
    }

    st = process(argv[1]);
    switch (st) {
        case STATUS_OK:
            return 0;
        case STATUS_ERR_OPEN:
            printf("Error: cannot open file\n");
            return 1;
        case STATUS_ERR_READ:
            printf("Error: read failure\n");
            return 1;
        case STATUS_ERR_ALLOC:
            printf("Error: memory allocation failed\n");
            return 1;
        case STATUS_ERR_ARG:
            printf("Error: invalid argument\n");
            return 1;
        default:
            printf("Error: status %d\n", (int)st);
            return 1;
    }
}