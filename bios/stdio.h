#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

typedef struct FILE {
    void (*open)(struct FILE* file, const char* path, const char* mode);
    void (*close)(struct FILE* file);
    size_t (*read)(struct FILE* file, size_t count, void* buffer);
    size_t (*write)(struct FILE* file, size_t count, const void* buffer);
    int (*flush)(struct FILE* file);

    void* data;
} FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

int printf(const char* fmt, ...);
int fprintf(FILE* file, const char* fmt, ...);
int snprintf(char* buffer, size_t bufsize, const char* fmt, ...);
int vprintf(const char* fmt, va_list args);
int vfprintf(FILE* file, const char* fmt, va_list args);
int vsnprintf(char* buffer, size_t bufsize, const char* fmt, va_list args);

int fflush(FILE* file);

#endif