#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct {
    double x, y, z;
} Point;

static int num_points = 0;
static double* points = NULL;

static CRITICAL_SECTION cs;
static double max_area_par = 0.0;
static int best_i_par = -1, best_j_par = -1, best_k_par = -1;
static int next_i = 0;

static double calculate_area(Point a, Point b, Point c) {
    double ab = sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2));
    double bc = sqrt(pow(c.x - b.x, 2) + pow(c.y - b.y, 2) + pow(c.z - b.z, 2));
    double ca = sqrt(pow(a.x - c.x, 2) + pow(a.y - c.y, 2) + pow(a.z - c.z, 2));
    double p = (ab + bc + ca) / 2.0;
    double s_sq = p * (p - ab) * (p - bc) * (p - ca);
    if (s_sq < 0) return 0.0;
    return sqrt(s_sq);
}

static int read_points(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", filename);
        return 0;
    }
    if (fscanf(file, "%d", &num_points) != 1 || num_points < 3) {
        printf("Error: Invalid number of points\n");
        fclose(file);
        return 0;
    }
    points = (double*)malloc((size_t)num_points * 3 * sizeof(double));
    if (!points) {
        printf("Error: Out of memory\n");
        fclose(file);
        return 0;
    }
    for (int i = 0; i < num_points * 3; i++) {
        if (fscanf(file, "%lf", &points[i]) != 1) {
            printf("Error: Invalid data in file\n");
            free(points);
            fclose(file);
            return 0;
        }
    }
    fclose(file);
    return 1;
}

static void print_result(double area, int bi, int bj, int bk,
                         int threads, double elapsed) {
    printf("Max area: %f\n", area);
    printf("Points indices: %d, %d, %d\n", bi, bj, bk);
    printf("Coordinates:\n");
    printf("(%f, %f, %f)\n", points[bi*3], points[bi*3+1], points[bi*3+2]);
    printf("(%f, %f, %f)\n", points[bj*3], points[bj*3+1], points[bj*3+2]);
    printf("(%f, %f, %f)\n", points[bk*3], points[bk*3+1], points[bk*3+2]);
    printf("Threads used: %d\n", threads);
    printf("Elapsed time: %.6f seconds\n", elapsed);
}

static void run_sequential(void) {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    double best_area = 0.0;
    int bi = -1, bj = -1, bk = -1;

    for (int i = 0; i < num_points - 2; i++) {
        for (int j = i + 1; j < num_points - 1; j++) {
            for (int k = j + 1; k < num_points; k++) {
                Point p1 = {points[i*3],   points[i*3+1], points[i*3+2]};
                Point p2 = {points[j*3],   points[j*3+1], points[j*3+2]};
                Point p3 = {points[k*3],   points[k*3+1], points[k*3+2]};
                double area = calculate_area(p1, p2, p3);
                if (area > best_area) {
                    best_area = area;
                    bi = i; bj = j; bk = k;
                }
            }
        }
    }

    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;

    print_result(best_area, bi, bj, bk, 1, elapsed);
}

typedef struct {
    int thread_id;
} ThreadData;

static DWORD WINAPI worker_thread(LPVOID lpParam) {
    (void)lpParam;
    double local_max = 0.0;
    int li = -1, lj = -1, lk = -1;

    while (1) {
        EnterCriticalSection(&cs);
        int i = next_i++;
        LeaveCriticalSection(&cs);
        if (i >= num_points - 2) break;

        for (int j = i + 1; j < num_points - 1; j++) {
            for (int k = j + 1; k < num_points; k++) {
                Point p1 = {points[i*3],   points[i*3+1], points[i*3+2]};
                Point p2 = {points[j*3],   points[j*3+1], points[j*3+2]};
                Point p3 = {points[k*3],   points[k*3+1], points[k*3+2]};
                double area = calculate_area(p1, p2, p3);
                if (area > local_max) {
                    local_max = area; li = i; lj = j; lk = k;
                }
            }
        }
    }

    EnterCriticalSection(&cs);
    if (local_max > max_area_par) {
        max_area_par = local_max;
        best_i_par = li;
        best_j_par = lj;
        best_k_par = lk;
    }
    LeaveCriticalSection(&cs);
    return 0;
}

static void run_parallel(int max_threads) {
    max_area_par = 0.0;
    best_i_par = best_j_par = best_k_par = -1;
    next_i = 0;

    InitializeCriticalSection(&cs);

    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    HANDLE* threads = (HANDLE*)malloc((size_t)max_threads * sizeof(HANDLE));
    ThreadData* thread_data = (ThreadData*)malloc((size_t)max_threads * sizeof(ThreadData));
    if (!threads || !thread_data) {
        printf("Error: Out of memory\n");
        free(threads); free(thread_data);
        DeleteCriticalSection(&cs);
        return;
    }

    for (int i = 0; i < max_threads; i++) {
        thread_data[i].thread_id = i;
        threads[i] = CreateThread(NULL, 0, worker_thread, &thread_data[i], 0, NULL);
        if (threads[i] == NULL) {
            printf("Error: Failed to create thread %d\n", i);
            for (int t = 0; t < i; t++) CloseHandle(threads[t]);
            free(threads); free(thread_data);
            DeleteCriticalSection(&cs);
            return;
        }
    }

    WaitForMultipleObjects((DWORD)max_threads, threads, TRUE, INFINITE);

    QueryPerformanceCounter(&end);
    double elapsed = (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;

    print_result(max_area_par, best_i_par, best_j_par, best_k_par,
                 max_threads, elapsed);

    for (int i = 0; i < max_threads; i++) CloseHandle(threads[i]);
    free(threads);
    free(thread_data);
    DeleteCriticalSection(&cs);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("  %s seq <input_file>\n", argv[0]);
        printf("  %s par <num_threads> <input_file>\n", argv[0]);
        return 1;
    }

    int is_seq = (strcmp(argv[1], "seq") == 0);
    int is_par = (strcmp(argv[1], "par") == 0);
    int max_threads = 0;
    const char* filename = NULL;

    if (is_seq) {
        if (argc != 3) {
            printf("Usage: %s seq <input_file>\n", argv[0]);
            return 1;
        }
        filename = argv[2];
    } else if (is_par) {
        if (argc != 4) {
            printf("Usage: %s par <num_threads> <input_file>\n", argv[0]);
            return 1;
        }
        max_threads = atoi(argv[2]);
        if (max_threads <= 0) {
            printf("Error: num_threads must be > 0\n");
            return 1;
        }
        filename = argv[3];
    } else {
        printf("Error: unknown mode '%s' (expected 'seq' or 'par')\n", argv[1]);
        return 1;
    }

    if (!read_points(filename)) return 1;

    if (is_seq) {
        run_sequential();
    } else {
        run_parallel(max_threads);
    }

    free(points);
    return 0;
}
