// tool_func.h
#ifndef LIBC_MINIMAL_H
#define LIBC_MINIMAL_H

#include <stddef.h>  // for size_t

#ifdef __cplusplus
extern "C" {
#endif

/* ===================== memory ===================== */

void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int   memcmp(const void *a, const void *b, size_t n);
void *memchr(const void *s, int c, size_t n);

/* ===================== string ===================== */

size_t strlen(const char *s);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
size_t strnlen(const char* s, size_t maxlen);

/* ===================== stdlib ===================== */

unsigned long strtoul(const char *nptr, char **endptr, int base);

#ifdef __cplusplus
}
#endif

#endif /* LIBC_MINIMAL_H */