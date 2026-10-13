#include <windows.h>
#include <string.h>

#define MAX_LINE 1024
#define LIMIT    10
#define IO_BUF   4096

/* Читатель строк поверх дескриптора: буферизует данные из ReadFile */
typedef struct {
    HANDLE h;
    char   buf[IO_BUF];
    DWORD  pos, len;
} LineReader;

/*  Вспомогательные функции вывода  */

/* Запись всего буфера в дескриптор (канал, файл, консоль) */
static int write_all(HANDLE h, const char *s, size_t len)
{
    DWORD written = 0;
    if (!WriteFile(h, s, (DWORD)len, &written, NULL) || written != len)
        return 0;
    return 1;
}

/* Вывод строки в дескриптор (для сообщений) */
static void out_str(HANDLE h, const char *s)
{
    DWORD written;
    WriteFile(h, s, (DWORD)strlen(s), &written, NULL);
}

/* Перевод числа в десятичную строку */
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

/* Вывод кода системной ошибки в stderr */
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

/* Вывод строки вида "<label><code>" в stdout */
static void print_code(HANDLE out, const char *label, DWORD code)
{
    char num[24];
    u_to_str((unsigned long)code, num);
    out_str(out, label);
    out_str(out, num);
    out_str(out, "\r\n");
}

/*  Чтение строки 
 * Возвращает: 1 - строка прочитана, 0 - конец данных, -1 - ошибка.
 * Символ '\r' пропускается, слишком длинная строка обрезается. */
static int read_line(LineReader *r, char *line, size_t cap)
{
    size_t n = 0;

    for (;;) {
        char c;

        if (r->pos >= r->len) {
            DWORD got = 0;
            if (!ReadFile(r->h, r->buf, IO_BUF, &got, NULL)) {
                /* Закрытый записывающий конец канала - это обычный EOF */
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

/*  Код дочернего процесса  */
static int run_child(void)
{
    LineReader r;
    char buf[MAX_LINE + 2];
    int rc;
    HANDLE in  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

    if (in == INVALID_HANDLE_VALUE || in == NULL ||
        out == INVALID_HANDLE_VALUE || out == NULL) {
        print_error("GetStdHandle");
        return 1;
    }
    r.h = in;
    r.pos = 0;
    r.len = 0;

    while ((rc = read_line(&r, buf, sizeof buf)) == 1) {
        size_t len = strlen(buf);

        /* Инвертируем строку на месте */
        for (size_t i = 0; i < len / 2; i++) {
            char t = buf[i];
            buf[i] = buf[len - 1 - i];
            buf[len - 1 - i] = t;
        }

        if (!write_all(out, buf, len) || !write_all(out, "\r\n", 2)) {
            print_error("WriteFile");
            return 1;
        }
    }
    if (rc < 0) {
        print_error("ReadFile");
        return 1;
    }
    return 0;
}

/*  Запуск одного ребёнка 
 * Создаёт канал и файл, запускает процесс.
 * В *pipe_write возвращает записывающий конец канала для родителя. */
static int spawn_child(const char *out_name, HANDLE *pipe_write,
                       PROCESS_INFORMATION *pi)
{
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOA si;
    HANDLE rd = NULL, wr = NULL, file = INVALID_HANDLE_VALUE;
    char exe[MAX_PATH];
    char cmd[MAX_PATH + 16];
    int ok = 0;

    sa.nLength = sizeof sa;
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;               /* дескрипторы наследуются */

    if (!CreatePipe(&rd, &wr, &sa, 0)) {
        print_error("CreatePipe");
        return 0;
    }
    /* Записывающий конец ребёнку не нужен, иначе EOF не наступит */
    if (!SetHandleInformation(wr, HANDLE_FLAG_INHERIT, 0)) {
        print_error("SetHandleInformation");
        goto cleanup;
    }

    /* Родитель открывает файл на запись */
    file = CreateFileA(out_name, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                       CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        print_error("CreateFile");
        goto cleanup;
    }

    if (GetModuleFileNameA(NULL, exe, sizeof exe) == 0) {
        print_error("GetModuleFileName");
        goto cleanup;
    }
    /* Командная строка: "<путь к exe>" --child */
    strcpy(cmd, "\"");
    strcat(cmd, exe);
    strcat(cmd, "\" --child");

    ZeroMemory(&si, sizeof si);
    ZeroMemory(pi, sizeof *pi);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = rd;                     /* stdin  = канал */
    si.hStdOutput = file;                   /* stdout = файл  */
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, pi)) {
        print_error("CreateProcess");
        goto cleanup;
    }

    *pipe_write = wr;
    wr = NULL;                              /* теперь им владеет вызывающий */
    ok = 1;

cleanup:
    /* Эти дескрипторы родителю больше не нужны */
    if (rd != NULL) CloseHandle(rd);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    if (wr != NULL) CloseHandle(wr);
    return ok;
}

/*  Код родительского процесса  */
int main(int argc, char *argv[])
{
    HANDLE pipe1_w = NULL, pipe2_w = NULL;
    PROCESS_INFORMATION pi1, pi2;
    HANDLE procs[2];
    DWORD code1 = 0, code2 = 0;
    LineReader in;
    char line[MAX_LINE + 2];
    int rc;
    HANDLE hin  = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE herr = GetStdHandle(STD_ERROR_HANDLE);

    if (argc == 2 && strcmp(argv[1], "--child") == 0)
        return run_child();

    if (argc != 3) {
        out_str(herr, "Usage: lab1.exe <file1> <file2>\r\n");
        return 1;
    }
    if (hin == INVALID_HANDLE_VALUE || hin == NULL ||
        hout == INVALID_HANDLE_VALUE || hout == NULL) {
        print_error("GetStdHandle");
        return 1;
    }

    if (!spawn_child(argv[1], &pipe1_w, &pi1))
        return 1;
    if (!spawn_child(argv[2], &pipe2_w, &pi2)) {
        /* Второй не запустился: корректно завершаем первого */
        CloseHandle(pipe1_w);
        WaitForSingleObject(pi1.hProcess, INFINITE);
        CloseHandle(pi1.hProcess);
        CloseHandle(pi1.hThread);
        return 1;
    }

    in.h = hin;
    in.pos = 0;
    in.len = 0;

    out_str(hout, "Enter lines (Ctrl+Z, Enter to finish):\r\n");
    while ((rc = read_line(&in, line, MAX_LINE + 1)) == 1) {
        size_t len = strlen(line);
        HANDLE target;

        /* Правило фильтрации: длина > 10 -> pipe2, иначе pipe1 */
        target = (len > LIMIT) ? pipe2_w : pipe1_w;

        line[len] = '\n';                   /* разделитель строк в канале */
        if (!write_all(target, line, len + 1)) {
            print_error("WriteFile");
            break;
        }
    }
    if (rc < 0)
        print_error("ReadFile");

    /* Закрываем каналы: дети получат EOF */
    CloseHandle(pipe1_w);
    CloseHandle(pipe2_w);

    procs[0] = pi1.hProcess;
    procs[1] = pi2.hProcess;
    if (WaitForMultipleObjects(2, procs, TRUE, INFINITE) == WAIT_FAILED)
        print_error("WaitForMultipleObjects");

    GetExitCodeProcess(pi1.hProcess, &code1);
    GetExitCodeProcess(pi2.hProcess, &code2);
    print_code(hout, "child1 exit code: ", code1);
    print_code(hout, "child2 exit code: ", code2);

    CloseHandle(pi1.hProcess); CloseHandle(pi1.hThread);
    CloseHandle(pi2.hProcess); CloseHandle(pi2.hThread);
    return (code1 == 0 && code2 == 0) ? 0 : 1;
}
