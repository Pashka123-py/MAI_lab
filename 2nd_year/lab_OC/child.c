#include <windows.h>
#include <string.h>

#define MAX_LINE 1024
#define IO_BUF   4096

typedef struct {
    HANDLE h;
    char   buf[IO_BUF];
    DWORD  pos, len;
} LineReader;

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

int main(void)
{
    return run_child();
}
