#include <stddef.h>
#include <stdint.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void *memset(void *dest, int value, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    uint8_t v = (uint8_t)value;
    for (size_t i = 0; i < n; i++) {
        d[i] = v;
    }
    return dest;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    for (size_t i = 0; i < n; i++) {
        if (pa[i] != pb[i]) {
            return (int)pa[i] - (int)pb[i];
        }
    }
    return 0;
}

int strncmp(const char *a, const char *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return (unsigned char)a[i] - (unsigned char)b[i];
        }
        if (a[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n)
{
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    if (d == s || n == 0) {
        return dest;
    }
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

long strtol(const char *s, char **endptr, int base)
{
    (void)base;
    long value = 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    if (endptr != 0) {
        *endptr = (char *)s;
    }
    return value * sign;
}

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n] != '\0') {
        n++;
    }
    return n;
}

int atoi(const char *s)
{
    int value = 0;
    int sign = 1;
    while (*s == ' ' || *s == '\t') {
        s++;
    }
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        value = value * 10 + (*s - '0');
        s++;
    }
    return value * sign;
}

const unsigned short *__ctype_b_loc(void)
{
    static const unsigned short table[256] = {
        [0 ... 255] = 0
    };
    return table;
}

void *__memcpy_chk(void *dest, const void *src, size_t n, size_t destlen)
{
    (void)destlen;
    return memcpy(dest, src, n);
}
