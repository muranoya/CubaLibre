#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

void *
m_malloc(size_t size, const char *file, int line);

void *
m_calloc(size_t n, size_t size, const char *file, int line);

void *
m_realloc(void *ptr, size_t size, const char *file, int line);

void
m_free(void *ptr, const char *file, int line);

#define mmalloc(s)     m_malloc((s), __FILE__, __LINE__);
#define mcalloc(n, s)  m_calloc((n), (s), __FILE__, __LINE__);
#define mrealloc(p, s) m_realloc((p), (s), __FILE__, __LINE__);
#define mfree(p)       m_free((p), __FILE__, __LINE__);

char *
new_string(const char *s);

#endif /* UTIL_H */
