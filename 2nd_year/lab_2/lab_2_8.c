#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>

enum status {
    STATUS_OK = 0,
    STATUS_ERR_ARGS,
    STATUS_ERR_BASE,
    STATUS_ERR_ALLOC,
    STATUS_ERR_DIGIT,
    STATUS_ERR_EMPTY
};

static int char_to_digit(int c, int base) {
    int d = -1;
    if (c >= '0' && c <= '9') {
        d = c - '0';
    } else if (c >= 'a' && c <= 'z') {
        d = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'Z') {
        d = c - 'A' + 10;
    }
    if (d < 0 || d >= base) {
        return -1;
    }
    return d;
}

static char digit_to_char(int d) {
    if (d < 10) {
        return (char)('0' + d);
    }
    return (char)('a' + d - 10);
}

static enum status validate_number(const char *s, int base) {
    if (s == NULL || *s == '\0') {
        return STATUS_ERR_EMPTY;
    }
    if (s[0] == '0' && s[1] != '\0') {
        return STATUS_ERR_DIGIT;
    }
    for (const char *p = s; *p; ++p) {
        if (char_to_digit((unsigned char)*p, base) < 0) {
            return STATUS_ERR_DIGIT;
        }
    }
    return STATUS_OK;
}

static enum status add_in_base(const char *a, const char *b, int base, char **out) {
    if (a == NULL || b == NULL || out == NULL) {
        return STATUS_ERR_ARGS;
    }
    if (base < 2 || base > 36) {
        return STATUS_ERR_BASE;
    }

    size_t la = strlen(a);
    size_t lb = strlen(b);
    size_t maxlen = la > lb ? la : lb;

    char *res = malloc(maxlen + 2);
    if (res == NULL) {
        return STATUS_ERR_ALLOC;
    }

    int carry = 0;
    size_t pos = 0;
    size_t i = 0;

    while (i < maxlen || carry > 0) {
        int da = 0;
        int db = 0;
        if (i < la) {
            da = char_to_digit((unsigned char)a[la - 1 - i], base);
            if (da < 0) {
                free(res);
                return STATUS_ERR_DIGIT;
            }
        }
        if (i < lb) {
            db = char_to_digit((unsigned char)b[lb - 1 - i], base);
            if (db < 0) {
                free(res);
                return STATUS_ERR_DIGIT;
            }
        }
        int sum = da + db + carry;
        res[pos++] = digit_to_char(sum % base);
        carry = sum / base;
        ++i;
    }

    for (size_t k = 0; k < pos / 2; ++k) {
        char t = res[k];
        res[k] = res[pos - 1 - k];
        res[pos - 1 - k] = t;
    }
    res[pos] = '\0';

    size_t start = 0;
    while (res[start] == '0' && res[start + 1] != '\0') {
        ++start;
    }
    if (start > 0) {
        memmove(res, res + start, pos - start + 1);
    }

    *out = res;
    return STATUS_OK;
}

static enum status sum_in_base(int base, int count, char **out, ...) {
    if (out == NULL) {
        return STATUS_ERR_ARGS;
    }
    if (base < 2 || base > 36) {
        return STATUS_ERR_BASE;
    }
    if (count < 0) {
        return STATUS_ERR_ARGS;
    }

    *out = NULL;

    char *sum = malloc(2);
    if (sum == NULL) {
        return STATUS_ERR_ALLOC;
    }
    sum[0] = '0';
    sum[1] = '\0';

    if (count == 0) {
        *out = sum;
        return STATUS_OK;
    }

    va_list ap;
    va_start(ap, out);

    for (int i = 0; i < count; ++i) {
        const char *num = va_arg(ap, const char *);
        enum status st = validate_number(num, base);
        if (st != STATUS_OK) {
            free(sum);
            va_end(ap);
            return st;
        }
        char *new_sum = NULL;
        st = add_in_base(sum, num, base, &new_sum);
        if (st != STATUS_OK) {
            free(sum);
            va_end(ap);
            return st;
        }
        free(sum);
        sum = new_sum;
    }

    va_end(ap);

    *out = sum;
    return STATUS_OK;
}

int main(void) {
    char *result = NULL;
    enum status st = STATUS_OK;

    st = sum_in_base(10, 3, &result, "123", "456", "789");
    if (st == STATUS_OK) {
        printf("Test 1 (base 10): 123 + 456 + 789 = %s\n", result);
        free(result);
    } else {
        printf("Test 1 failed: %d\n", (int)st);
    }

    st = sum_in_base(2, 3, &result, "101", "11", "1");
    if (st == STATUS_OK) {
        printf("Test 2 (base 2): 101 + 11 + 1 = %s\n", result);
        free(result);
    } else {
        printf("Test 2 failed: %d\n", (int)st);
    }

    st = sum_in_base(16, 4, &result, "ff", "1", "a", "10");
    if (st == STATUS_OK) {
        printf("Test 3 (base 16): ff + 1 + a + 10 = %s\n", result);
        free(result);
    } else {
        printf("Test 3 failed: %d\n", (int)st);
    }

    st = sum_in_base(36, 3, &result, "z", "z", "z");
    if (st == STATUS_OK) {
        printf("Test 4 (base 36): z + z + z = %s\n", result);
        free(result);
    } else {
        printf("Test 4 failed: %d\n", (int)st);
    }

    st = sum_in_base(10, 3, &result,
                     "9999999999999999999999999999999999999999",
                     "1",
                     "1234567890123456789012345678901234567890");
    if (st == STATUS_OK) {
        printf("Test 5 (huge base 10): %s\n", result);
        free(result);
    } else {
        printf("Test 5 failed: %d\n", (int)st);
    }

    st = sum_in_base(10, 0, &result);
    if (st == STATUS_OK) {
        printf("Test 6 (count=0): '%s'\n", result);
        free(result);
    } else {
        printf("Test 6 failed: %d\n", (int)st);
    }

    st = sum_in_base(1, 2, &result, "1", "2");
    printf("Test 7 (bad base): status=%d\n", (int)st);

    st = sum_in_base(2, 1, &result, "102");
    printf("Test 8 (bad digit): status=%d\n", (int)st);

    st = sum_in_base(10, 1, &result, "0123");
    printf("Test 9 (leading zero): status=%d\n", (int)st);

    return 0;
}
