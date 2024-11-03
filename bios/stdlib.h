#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

void abort(void);
void exit(int code);
void quick_exit(int code);

int atexit(void (*func)(void));
int at_quick_exit(void (*func)(void));

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int signal(int sig, void (*func)(int));

#define SIG_ERR 1

#define SIGTERM 0
#define SIGSEGV 1
#define SIGINT 2
#define SIGILL 3
#define SIGABRT 4
#define SIGFPE 5

extern void _sig_dfl_impl(int code);
extern void _sig_ign_impl(int code);

#define SIG_DFLT _sig_dfl_impl
#define SIG_IGN _sig_ign_impl

int raise(int sig);

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);

double atof(const char* str);
int atoi(const char* str);
long atol(const char* str);
long long atoll(const char* str);

#endif