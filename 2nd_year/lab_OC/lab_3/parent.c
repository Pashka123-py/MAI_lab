#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_LINE 1024
#define LIMIT    10
#define IO_BUF   4096
#define SHM_SIZE 4096

typedef struct {
    volatile LONG length;
    char text[SHM_SIZE - sizeof(LONG)];
} ShmBuf;

typedef struct {
    HANDLE h;
    char   buf[IO_BUF];
    DWORD  pos, len;
} LineReader;

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

static void print_code(HANDLE out, const char *label, DWORD code)
{
    char num[24];
    u_to_str((unsigned long)code, num);
    out_str(out, label);
    out_str(out, num);
    out_str(out, "\r\n");
}

static int read_line(LineReader *r, char *line, size_t cap)
{
    size_t n = 0;

    for (;;) {
        char c;

        if (r->pos >= r->len) {
            DWORD got = 0;
            if (!ReadFile(r->h, r->buf, IO_BUF, &got, NULL)) {
                if (GetLastError() != ERROR_BROKEN_PIPE)
                    return -1;
                got = 0;
            }
            if (got == 0) {
                line[n] = '\0';
                return n > 0 ? 1 : 0;
            }
            r->pos = 0;
            r->len = got;
        }

        c = r->buf[r->pos++];
        if (c == '\n') {
            line[n] = '\0';
            return 1;
        }
        if (c == '\r')
            continue;
        if (n + 1 < cap)
            line[n++] = c;
    }
}

static void build_child_path(char *out)
{
    char base[MAX_PATH];
    char *slash;

    if (GetModuleFileNameA(NULL, base, sizeof base) == 0) {
        out[0] = '\0';
        return;
    }

    strcpy(out, base);
    slash = strrchr(out, '\\');
    if (slash == NULL) {
        strcpy(out, "child.exe");
        return;
    }
    slash[1] = '\0';
    strcat(out, "child.exe");
}

typedef struct {
    char    shm_name[64];
    char    sem_name[64];
    HANDLE  hMap;
    HANDLE  hSem;
    ShmBuf *shm;
    PROCESS_INFORMATION pi;
} ChildChannel;

static int spawn_child(int index, const char *out_name, ChildChannel *ch)
{
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    HANDLE file = INVALID_HANDLE_VALUE;
    char child_exe[MAX_PATH];
    char cmd[MAX_PATH + 256];

    snprintf(ch->shm_name, sizeof ch->shm_name,
             "lab_3_OC_server_%lu_%d", (unsigned long)GetCurrentProcessId(), index);
    snprintf(ch->sem_name, sizeof ch->sem_name,
             "lab_3_OC_client_%lu_%d", (unsigned long)GetCurrentProcessId(), index);

    ch->hMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                  0, SHM_SIZE, ch->shm_name);
    if (ch->hMap == NULL) {
        print_error("CreateFileMapping");
        return 0;
    }

    ch->shm = (ShmBuf *)MapViewOfFile(ch->hMap, FILE_MAP_READ | FILE_MAP_WRITE,
                                      0, 0, SHM_SIZE);
    if (ch->shm == NULL) {
        print_error("MapViewOfFile");
        CloseHandle(ch->hMap);
        return 0;
    }
    ch->shm->length = 0;

    ch->hSem = CreateSemaphoreA(NULL, 1, 1, ch->sem_name);
    if (ch->hSem == NULL) {
        print_error("CreateSemaphore");
        UnmapViewOfFile(ch->shm);
        CloseHandle(ch->hMap);
        return 0;
    }

    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    file = CreateFileA(out_name, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        print_error("CreateFile");
        CloseHandle(ch->hSem);
        UnmapViewOfFile(ch->shm);
        CloseHandle(ch->hMap);
        return 0;
    }

    build_child_path(child_exe);
    if (child_exe[0] == '\0') {
        print_error("GetModuleFileName");
        CloseHandle(file);
        CloseHandle(ch->hSem);
        UnmapViewOfFile(ch->shm);
        CloseHandle(ch->hMap);
        return 0;
    }

    snprintf(cmd, sizeof cmd, "\"%s\" %s %s",
             child_exe, ch->shm_name, ch->sem_name);

    ZeroMemory(&si, sizeof si);
    ZeroMemory(&ch->pi, sizeof ch->pi);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = file;
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &ch->pi)) {
        print_error("CreateProcess");
        CloseHandle(file);
        CloseHandle(ch->hSem);
        UnmapViewOfFile(ch->shm);
        CloseHandle(ch->hMap);
        return 0;
    }

    CloseHandle(file);
    return 1;
}

static int send_block(ChildChannel *ch, const char *data, size_t len)
{
    DWORD wr = WaitForSingleObject(ch->hSem, INFINITE);
    if (wr != WAIT_OBJECT_0) {
        print_error("WaitForSingleObject");
        return 0;
    }

    if (len == (size_t)-1) {
        ch->shm->length = (LONG)UINT32_MAX;
    } else {
        if (len > sizeof ch->shm->text) len = sizeof ch->shm->text;
        memcpy(ch->shm->text, data, len);
        ch->shm->length = (LONG)len;
    }

    if (!ReleaseSemaphore(ch->hSem, 1, NULL)) {
        print_error("ReleaseSemaphore");
        return 0;
    }
    return 1;
}

static void cleanup_child(ChildChannel *ch)
{
    if (ch->hSem) CloseHandle(ch->hSem);
    if (ch->shm)  UnmapViewOfFile(ch->shm);
    if (ch->hMap) CloseHandle(ch->hMap);
    if (ch->pi.hProcess) CloseHandle(ch->pi.hProcess);
    if (ch->pi.hThread)  CloseHandle(ch->pi.hThread);
}

int main(int argc, char *argv[])
{
    ChildChannel ch1, ch2;
    LineReader in;
    char line[MAX_LINE + 2];
    int rc;
    DWORD code1 = 0, code2 = 0;
    HANDLE procs[2];
    HANDLE hin  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE herr = GetStdHandle(STD_ERROR_HANDLE);

    if (argc != 3) {
        out_str(herr, "Usage: lab1.exe <file1> <file2>\r\n");
        return 1;
    }
    if (hin == INVALID_HANDLE_VALUE || hin == NULL ||
        hout == INVALID_HANDLE_VALUE || hout == NULL) {
        print_error("GetStdHandle");
        return 1;
    }

    ZeroMemory(&ch1, sizeof ch1);
    ZeroMemory(&ch2, sizeof ch2);

    if (!spawn_child(1, argv[1], &ch1))
        return 1;
    if (!spawn_child(2, argv[2], &ch2)) {
        cleanup_child(&ch1);
        return 1;
    }

    in.h = hin;
    in.pos = 0;
    in.len = 0;

    out_str(hout, "Enter lines (Ctrl+Z, Enter to finish):\r\n");
    while ((rc = read_line(&in, line, MAX_LINE + 1)) == 1) {
        size_t len = strlen(line);
        ChildChannel *target = (len > LIMIT) ? &ch2 : &ch1;

        if (!send_block(target, line, len))
            break;
    }
    if (rc < 0)
        print_error("ReadFile");

    send_block(&ch1, NULL, (size_t)-1);
    send_block(&ch2, NULL, (size_t)-1);

    procs[0] = ch1.pi.hProcess;
    procs[1] = ch2.pi.hProcess;
    if (WaitForMultipleObjects(2, procs, TRUE, INFINITE) == WAIT_FAILED)
        print_error("WaitForMultipleObjects");

    GetExitCodeProcess(ch1.pi.hProcess, &code1);
    GetExitCodeProcess(ch2.pi.hProcess, &code2);
    print_code(hout, "child1 exit code: ", code1);
    print_code(hout, "child2 exit code: ", code2);

    cleanup_child(&ch1);
    cleanup_child(&ch2);
    return (code1 == 0 && code2 == 0) ? 0 : 1;
}