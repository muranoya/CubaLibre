#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void *
m_malloc(size_t size, const char *file, int line)
{
    void *p;
    p = malloc(size);
    if (!p)
    {
        fprintf(stderr,
                "Error(%s, %d): failed malloc\n",
                file, line);
        perror("malloc");
        abort();
    }
    return p;
}

void *
m_calloc(size_t n, size_t size, const char *file, int line)
{
    void *p;
    p = calloc(n, size);
    if (!p)
    {
        fprintf(stderr,
                "Error(%s, %d): failed calloc\n",
                file, line);
        perror("calloc");
        abort();
    }
    return p;
}

void *
m_realloc(void *ptr, size_t size, const char *file, int line)
{
    void *p;
    p = realloc(ptr, size);
    if (!p)
    {
        fprintf(stderr,
                "Error(%s, %d): failed realloc\n",
                file, line);
        perror("realloc");
        abort();
    }
    return p;
}

void
m_free(void *ptr, const char *file, int line)
{
    free(ptr);
}

char *
new_string(const char *s)
{
    char *str;
    int len = strlen(s)+1;
    str = mmalloc(len);
    strcpy(str, s);
    return str;
}

