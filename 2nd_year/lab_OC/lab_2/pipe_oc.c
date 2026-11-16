#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Глобальные переменные для синхронизации и результатов
CRITICAL_SECTION cs;
double max_area = 0.0;
int best_i = -1, best_j = -1, best_k = -1;
int total_tasks = 0;
int current_task_index = 0;
int num_points = 0;
double* points = NULL;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
} ThreadData;

// Структура точки
typedef struct {
    double x, y, z;
} Point;

// Функция вычисления площади треугольника по формуле Герона
double calculate_area(Point a, Point b, Point c) {
    double ab = sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2) + pow(b.z - a.z, 2));
    double bc = sqrt(pow(c.x - b.x, 2) + pow(c.y - b.y, 2) + pow(c.z - b.z, 2));
    double ca = sqrt(pow(a.x - c.x, 2) + pow(a.y - c.y, 2) + pow(a.z - c.z, 2));
    double p = (ab + bc + ca) / 2.0;
    double s_sq = p * (p - ab) * (p - bc) * (p - ca);
    if (s_sq < 0) return 0.0; // Защита от погрешностей округления
    return sqrt(s_sq);
}

// Функция потока
DWORD WINAPI worker_thread(LPVOID lpParam) {
    int task_idx;
    
    while (1) {
        // Блокировка для получения следующей задачи
        EnterCriticalSection(&cs);
        if (current_task_index >= total_tasks) {
            LeaveCriticalSection(&cs);
            break;
        }
        task_idx = current_task_index++;
        LeaveCriticalSection(&cs);

        // Восстановление индексов i, j, k из линейного индекса задачи
        int i = 0, j = 1, k = 2;
        int count = 0;
        int found = 0;
        for (i = 0; i < num_points - 2 && !found; i++) {
            for (j = i + 1; j < num_points - 1 && !found; j++) {
                for (k = j + 1; k < num_points && !found; k++) {
                    if (count == task_idx) {
                        found = 1;
                        break;
                    }
                    count++;
                }
            }
        }
        
        // Корректировка индексов после выхода из циклов
        i--; j--; k--;

        // Вычисление площади
        Point p1 = {points[i*3], points[i*3+1], points[i*3+2]};
        Point p2 = {points[j*3], points[j*3+1], points[j*3+2]};
        Point p3 = {points[k*3], points[k*3+1], points[k*3+2]};
        double area = calculate_area(p1, p2, p3);

        // Обновление глобального максимума (критическая секция)
        EnterCriticalSection(&cs);
        if (area > max_area) {
            max_area = area;
            best_i = i;
            best_j = j;
            best_k = k;
        }
        LeaveCriticalSection(&cs);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // Проверка аргументов командной строки
    if (argc != 3) {
        printf("Usage: %s <max_threads> <input_file>\n", argv[0]);
        return 1;
    }

    int max_threads = atoi(argv[1]);
    if (max_threads <= 0) {
        printf("Error: max_threads must be > 0\n");
        return 1;
    }

    // Чтение данных из файла
    FILE* file = fopen(argv[2], "r");
    if (!file) {
        printf("Error: Cannot open file %s\n", argv[2]);
        return 1;
    }

    if (fscanf(file, "%d", &num_points) != 1 || num_points < 3) {
        printf("Error: Invalid number of points\n");
        fclose(file);
        return 1;
    }

    points = (double*)malloc(num_points * 3 * sizeof(double));
    for (int i = 0; i < num_points * 3; i++) {
        if (fscanf(file, "%lf", &points[i]) != 1) {
            printf("Error: Invalid data in file\n");
            free(points);
            fclose(file);
            return 1;
        }
    }
    fclose(file);

    // Вычисление общего количества комбинаций (C(n, 3))
    total_tasks = (num_points * (num_points - 1) * (num_points - 2)) / 6;
    
    // Инициализация критической секции
    InitializeCriticalSection(&cs);

    // Создание потоков
    HANDLE* threads = (HANDLE*)malloc(max_threads * sizeof(HANDLE));
    ThreadData* thread_data = (ThreadData*)malloc(max_threads * sizeof(ThreadData));

    for (int i = 0; i < max_threads; i++) {
        thread_data[i].thread_id = i;
        threads[i] = CreateThread(NULL, 0, worker_thread, &thread_data[i], 0, NULL);
        if (threads[i] == NULL) {
            printf("Error: Failed to create thread %d\n", i);
            return 1;
        }
    }

    // Ожидание завершения всех потоков
    WaitForMultipleObjects(max_threads, threads, TRUE, INFINITE);

    // Вывод результатов
    printf("Max area: %f\n", max_area);
    printf("Points indices: %d, %d, %d\n", best_i, best_j, best_k);
    printf("Coordinates:\n");
    printf("(%f, %f, %f)\n", points[best_i*3], points[best_i*3+1], points[best_i*3+2]);
    printf("(%f, %f, %f)\n", points[best_j*3], points[best_j*3+1], points[best_j*3+2]);
    printf("(%f, %f, %f)\n", points[best_k*3], points[best_k*3+1], points[best_k*3+2]);

    // Освобождение ресурсов
    DeleteCriticalSection(&cs);
    for (int i = 0; i < max_threads; i++) {
        CloseHandle(threads[i]);
    }
    free(threads);
    free(thread_data);
    free(points);

    return 0;
}


//в test.c: первая строка - количество строк, далее - координаты x y z построчно