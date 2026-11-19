#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

enum status {
    OK = 0,
    ERR_ARG,
    ERR_MEM
};

/*------------------------*/
static enum status convert_to_base(unsigned value, int r, unsigned **out, size_t *len)
{
    unsigned mask, shift, digit, tmp;
    size_t cap, pos, i;
    unsigned *buf;

    if (r < 1 || r > 5 || out == NULL || len == NULL)
        return ERR_ARG;

    shift = (unsigned)r;
    mask = (1u << shift) - 1u;

    cap = 1;
    tmp = value;
    while (tmp >>= shift)
        cap++;
    buf = (unsigned *)malloc(cap * sizeof(unsigned));
    if (buf == NULL)
        return ERR_MEM;

    pos = 0;
    tmp = value;
    do {
        digit = tmp & mask;
        tmp >>= shift; 
        buf[pos++] = digit;
    } while (tmp != 0);

    for (i = 0; i < pos / 2; i++) {
        unsigned t = buf[i];
        buf[i] = buf[pos - 1 - i];
        buf[pos - 1 - i] = t;
    }
    *out = buf;
    *len = pos;
    return OK;
}

/*-------------------------*/
static enum status print_in_base(long value, int r)
{
    unsigned mag;
    unsigned *out = NULL;
    size_t len = 0, i;
    int negative = 0;
    enum status st;

    if (value < 0) {
        negative = 1;
        mag = (unsigned)(-(value + 1)) + 1u;
    } else {
        mag = (unsigned)value;
    }

    st = convert_to_base(mag, r, &out, &len);
    if (st != OK)
        return st;

    printf("%ld base 2^%d = %s", value, r, negative ? "-" : "");
    for (i = 0; i < len; i++) {
        if (i > 0)
            putchar(' ');
        printf("%u", out[i]);
    }
    putchar('\n');
    free(out);
    return OK;
}

int main(void)
{
    long values[] = { 0, 1, 13, 255, 1024, -13, -255, 123456789L };
    size_t n = sizeof(values) / sizeof(values[0]);
    size_t i;
    int r;

    for (i = 0; i < n; i++) {
        for (r = 1; r <= 5; r++) {
            enum status st = print_in_base(values[i], r);
            switch (st) {
            case OK:
                break;
            case ERR_ARG:
                fprintf(stderr, "Invalid argument (r=%d)\n", r);
                return 1;
            case ERR_MEM:
                fprintf(stderr, "Memory allocation failed\n");
                return 1;
            default:
                fprintf(stderr, "Unknown error\n");
                return 1;
            }
        }
        putchar('\n');
    }
    return 0;
}