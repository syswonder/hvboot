// libc_minimal.c
#include <stddef.h>
#include <stdint.h>

/* ===================== memory ===================== */

// void *memcpy(void *dst, const void *src, size_t n)
// {
//     unsigned char *d = dst;
//     const unsigned char *s = src;
//     while (n--)
//         *d++ = *s++;
//     return dst;
// }

void *memmove(void *dst, const void *src, size_t n)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }

    return dst;
}

int memcmp(const void *a, const void *b, size_t n)
{
    const unsigned char *p1 = a;
    const unsigned char *p2 = b;

    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

void *memchr(const void *s, int c, size_t n)
{
    const unsigned char *p = s;
    while (n--) {
        if (*p == (unsigned char)c)
            return (void *)p;
        p++;
    }
    return NULL;
}

/* ===================== string ===================== */

size_t strlen(const char *s)
{
    const char *p = s;
    while (*p)
        p++;
    return p - s;
}

char *strchr(const char *s, int c)
{
    while (*s) {
        if (*s == (char)c)
            return (char *)s;
        s++;
    }
    return NULL;
}

char *strrchr(const char *s, int c)
{
    const char *last = NULL;
    while (*s) {
        if (*s == (char)c)
            last = s;
        s++;
    }
    return (char *)last;
}


size_t strnlen(const char* s, size_t maxlen) {
    const char* p = s;

    while (maxlen-- && *p) {
        ++p;
    }

    return p - s;
}

/* ===================== stdlib ===================== */

unsigned long strtoul(const char *nptr, char **endptr, int base)
{
    unsigned long result = 0;

    if (base == 0)
        base = 10;

    while (*nptr) {
        char c = *nptr;
        int value;

        if (c >= '0' && c <= '9')
            value = c - '0';
        else if (c >= 'a' && c <= 'f')
            value = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            value = c - 'A' + 10;
        else
            break;

        if (value >= base)
            break;

        result = result * base + value;
        nptr++;
    }

    if (endptr)
        *endptr = (char *)nptr;

    return result;
}