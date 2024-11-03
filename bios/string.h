#ifndef STRING_H
#define STRING_H

#include <stddef.h>

int isalnum(int chr);
int isalpha(int chr);
int islower(int chr);
int isupper(int chr);
int isdigit(int chr);
int isxdigit(int chr);
int iscntrl(int chr);
int isgraph(int chr);
int isspace(int chr);
int isblank(int chr);
int isprint(int chr);
int ispunct(int chr);

int tolower(int chr);
int toupper(int chr);

size_t strnlen(const char* str, size_t strsz);
int strncmp(const char* lhs, const char* rhs, size_t count);

void* memchr(const void* ptr, int ch, size_t count);
int memcmp(const void* lhs, const void* rhs, size_t count);
void memset(void* dest, int ch, size_t count);
void memcpy(void* restrict dst, const void* restrict src, size_t count);
void memmove(void* dst, const void* src, size_t count);

#endif