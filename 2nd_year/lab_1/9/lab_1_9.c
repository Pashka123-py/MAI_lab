#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define FIXED_N 100

/* статус-коды */
typedef enum {
    ST_OK = 0,
    ST_ERR_ARGC,
    ST_ERR_CMD,
    ST_ERR_NUM,
    ST_ERR_RANGE,
    ST_ERR_MEM,
    ST_ERR_UNKNOWN
} status_t;

/* разбор long */
static status_t parse_long(const char *s, long *out) {
    if (!s || !out || *s == '\0') return ST_ERR_NUM;
    errno = 0;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0') return ST_ERR_NUM;
    if (errno == ERANGE) return ST_ERR_NUM;
    *out = v;
    return ST_OK;
}

/* заполнение массива в [a..b] */
static void fill_range(int *arr, size_t n, int a, int b) {
    int span = b - a + 1;
    for (size_t i = 0; i < n; ++i) {
        arr[i] = a + rand() % span;
    }
}

/* поиск min/max и swap за один проход */
static status_t minmax_swap(int *arr, size_t n, int *min_out, int *max_out) {
    if (!arr || n == 0) return ST_ERR_RANGE;

    size_t i_min = 0, i_max = 0;
    int vmin = arr[0], vmax = arr[0];

    for (size_t i = 1; i < n; ++i) {
        if (arr[i] < vmin) { vmin = arr[i]; i_min = i; }
        if (arr[i] > vmax) { vmax = arr[i]; i_max = i; }
    }

    int tmp = arr[i_min];
    arr[i_min] = arr[i_max];
    arr[i_max] = tmp;

    if (min_out) *min_out = vmin;
    if (max_out) *max_out = vmax;
    return ST_OK;
}

/* печать массива */
static void print_arr(const int *arr, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        printf("%d", arr[i]);
        if (i + 1 < n) printf(" ");
    }
    printf("\n");
}

/*часть 1*/
static status_t cmd_part1(int argc, char *argv[]) {
    if (argc != 4) return ST_ERR_ARGC;

    long a, b;
    status_t st = parse_long(argv[2], &a);
    if (st != ST_OK) return st;
    st = parse_long(argv[3], &b);
    if (st != ST_OK) return st;

    if (a > b) return ST_ERR_RANGE;
    if (b - a + 1 > INT_MAX) return ST_ERR_RANGE;

    int arr[FIXED_N];
    fill_range(arr, FIXED_N, (int)a, (int)b);

    printf("array (before):\n");
    print_arr(arr, FIXED_N);

    int mn, mx;
    st = minmax_swap(arr, FIXED_N, &mn, &mx);
    if (st != ST_OK) return st;

    printf("\nmin = %d, max = %d\n", mn, mx);
    printf("\narray (after):\n");
    print_arr(arr, FIXED_N);
    return ST_OK;
}

/*часть 2*/

/* ближайший к x элемент в B */
static int nearest_in(const int *B, size_t nB, int x) {
    int best = B[0];
    long best_d = labs((long)B[0] - x);
    for (size_t j = 1; j < nB; ++j) {
        long d = labs((long)B[j] - x);
        if (d < best_d) { best_d = d; best = B[j]; }
    }
    return best;
}

static status_t cmd_part2(int argc, char *argv[]) {
    (void)argv;
    if (argc != 2) return ST_ERR_ARGC;

    size_t nA = 10 + (size_t)(rand() % 9991);
    size_t nB = 10 + (size_t)(rand() % 9991);

    int *A = (int *)malloc(nA * sizeof(int));
    int *B = (int *)malloc(nB * sizeof(int));
    int *C = (int *)malloc(nA * sizeof(int));
    if (!A || !B || !C) {
        free(A); free(B); free(C);
        return ST_ERR_MEM;
    }

    fill_range(A, nA, -1000, 1000);
    fill_range(B, nB, -1000, 1000);

    for (size_t i = 0; i < nA; ++i) {
        int nb = nearest_in(B, nB, A[i]);
        C[i] = A[i] + nb;
    }

    printf("nA = %zu, nB = %zu\n\n", nA, nB);

    /* Печатаем только первые 20 элементов — иначе вывод огромный */
    size_t showA = nA < 20 ? nA : 20;
    size_t showB = nB < 20 ? nB : 20;
    size_t showC = nA < 20 ? nA : 20;

    printf("A[0..%zu]: ", showA);
    print_arr(A, showA);

    printf("B[0..%zu]: ", showB);
    print_arr(B, showB);

    printf("C[0..%zu]: ", showC);
    print_arr(C, showC);

    /* Проверка: первый элемент вручную */
    if (nA > 0) {
        int nb = nearest_in(B, nB, A[0]);
        printf("\ncheck: C[0] = A[0] + nearest(B to A[0]) = %d + %d = %d\n",
               A[0], nb, A[0] + nb);
    }

    free(A);
    free(B);
    free(C);
    return ST_OK;
}

/*main*/
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s part1 <a> <b> | part2\n", argv[0]);
        return 1;
    }

    srand((unsigned)time(NULL));

    status_t st;
    const char *cmd = argv[1];

    if (strcmp(cmd, "part1") == 0) {
        st = cmd_part1(argc, argv);
    } else if (strcmp(cmd, "part2") == 0) {
        st = cmd_part2(argc, argv);
    } else {
        fprintf(stderr, "Error: unknown command '%s'\n", cmd);
        return 2;
    }

    if (st != ST_OK) {
        switch (st) {
            case ST_ERR_ARGC:  fprintf(stderr, "Error: wrong argc\n"); break;
            case ST_ERR_NUM:   fprintf(stderr, "Error: invalid number\n"); break;
            case ST_ERR_RANGE: fprintf(stderr, "Error: out of range\n"); break;
            case ST_ERR_MEM:   fprintf(stderr, "Error: out of memory\n"); break;
            default:           fprintf(stderr, "Error: unknown\n"); break;
        }
        return 3;
    }
    return 0;
}