#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_LINE 1024
#define SHM_SIZE 4096

typedef struct {
    volatile LONG length;
    char text[SHM_SIZE - sizeof(LONG)];
} ShmBuf;

static int write_all(HANDLE h, const char *s, size_t len)
{
    DWORD written = 0;
    if (!WriteFile(h, s, (DWORD)len, &written, NULL) || written != len)
        return 0;
    return 1;
}

static void out_str(HANDLE h, const char *s)
{
    DWORD written;
    WriteFile(h, s, (DWORD)strlen(s), &written, NULL);
}

static void u_to_str(unsigned long v, char *out)
{
    char tmp[24];
    int n = 0, i = 0;
    do {
        tmp[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (n > 0)
        out[i++] = tmp[--n];
    out[i] = '\0';
}

static void print_error(const char *where)
{
    char num[24];
    HANDLE err = GetStdHandle(STD_ERROR_HANDLE);

    u_to_str((unsigned long)GetLastError(), num);
    out_str(err, "Error: ");
    out_str(err, where);
    out_str(err, " failed (code ");
    out_str(err, num);
    out_str(err, ")\r\n");
}

static int run_child(const char *shm_name, const char *sem_name)
{
    HANDLE hMap, hSem;
    ShmBuf *shm;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    if (out == INVALID_HANDLE_VALUE || out == NULL) {
        print_error("GetStdHandle");
        return 1;
    }

    hMap = OpenFileMappingA(FILE_MAP_READ | FILE_MAP_WRITE, FALSE, shm_name);
    if (hMap == NULL) {
        print_error("OpenFileMapping");
        return 1;
    }

    shm = (ShmBuf *)MapViewOfFile(hMap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, SHM_SIZE);
    if (shm == NULL) {
        print_error("MapViewOfFile");
        CloseHandle(hMap);
        return 1;
    }

    hSem = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, sem_name);
    if (hSem == NULL) {
        print_error("OpenSemaphore");
        UnmapViewOfFile(shm);
        CloseHandle(hMap);
        return 1;
    }

    for (;;) {
        DWORD wr;
        LONG len;
        char buf[MAX_LINE + 2];

        wr = WaitForSingleObject(hSem, INFINITE);
        if (wr != WAIT_OBJECT_0) {
            print_error("WaitForSingleObject");
            break;
        }

        len = shm->length;

        if (len == (LONG)UINT32_MAX) {
            ReleaseSemaphore(hSem, 1, NULL);
            break;
        }

        if (len > 0) {
            size_t l = (size_t)len;

            if (l > MAX_LINE) l = MAX_LINE;
            memcpy(buf, shm->text, l);
            buf[l] = '\0';

            for (size_t i = 0; i < l / 2; i++) {
                char t = buf[i];
                buf[i] = buf[l - 1 - i];
                buf[l - 1 - i] = t;
            }

            if (!write_all(out, buf, l) || !write_all(out, "\r\n", 2)) {
                print_error("WriteFile");
                ReleaseSemaphore(hSem, 1, NULL);
                break;
            }

            shm->length = 0;
        }

        ReleaseSemaphore(hSem, 1, NULL);
    }

    CloseHandle(hSem);
    UnmapViewOfFile(shm);
    CloseHandle(hMap);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
        out_str(err, "Usage: child.exe <shm_name> <sem_name>\r\n");
        return 1;
    }
    return run_child(argv[1], argv[2]);
}