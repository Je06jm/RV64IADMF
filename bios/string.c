#include "string.h"

#include <stdbool.h>
#include <stdint.h>

int isalnum(int chr) {
    return isalpha(chr) || isdigit(chr);
}

int isalpha(int chr) {
    if (chr >= 'a' && chr <= 'z') return true;
    if (chr >= 'A' && chr <= 'Z') return true;
    return false;
}

int islower(int chr) {
    return chr >= 'a' && chr <= 'z';
}

int isupper(int chr) {
    return chr >= 'A' && chr <= 'Z';
}

int isdigit(int chr) {
    return chr >= '0' && chr <= '9';
}

int isxdigit(int chr) {
    return (chr >= '0' && chr <= '9') || (chr >= 'a' && chr <= 'f') || (chr >= 'A' && chr <= 'F');
}

int iscntrl(int chr) {
    return chr <= 0x1f || chr == 0x7f;
}

int isgraph(int chr) {
    if (isalnum(chr)) return true;
    return ispunct(chr);
}

int isspace(int chr) {
    switch (chr) {
        case ' ':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case '\v':
            return true;
        
        default:
            return false;
    }
}

int isblank(int chr) {
    return chr == 0x20 || chr == 0x09;
}

int isprint(int chr) {
    if (isgraph(chr)) return true;
    return isblank(chr);
}

int ispunct(int chr) {
    const char* syms = "(!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~)";

    for (int i = 0; syms[i] != '\0'; i++) {
        if (syms[i] == chr)
            return true;
    }

    return false;
}

int tolower(int chr) {
    if (isupper(chr))
        return chr - 'A' + 'a';
    
    return chr;
}

int toupper(int chr) {
    if (islower(chr))
        return chr - 'a' + 'A';
    
    return chr;
}

size_t strnlen(const char* str, size_t strsz) {
    size_t i;
    for (i = 0; i < strsz; i++) {
        if (str[i] == '\0')
            break;
    }

    return i;
}

int strncmp(const char* lhs, const char* rhs, size_t count) {
    for (size_t i = 0; i < count; i++) {
        char lhs_c = lhs[i];
        char rhs_c = rhs[i];

        if (lhs_c == '\0' || rhs_c == '\0') {
            if (lhs_c != '\0') return 1;
            else if (rhs_c != '\0') return -1;
            return 0;
        }
        else if (lhs_c != rhs_c) {
            if (!isalnum(lhs_c) || !isalnum(rhs_c))
                return lhs_c < rhs_c ? -1 : 1;
            
            int lhs_upper = isupper(lhs_c);
            int rhs_upper = isupper(rhs_c);
            if (lhs_upper && !rhs_upper)
                return 1;
            
            else if (!lhs_upper && rhs_upper)
                return -1;
            
            return lhs_c < rhs_c ? -1 : 1;
        }
    }

    return 0;
}

void* memchr(const void* ptr, int ch, size_t count) {
    const char* str = (const char*)ptr;
    for (size_t i = 0; i < count; i++) {
        if (str[i] == ch)
            return (void*)(&str[i]);
    }
    return NULL;
}

int memcmp(const void* lhs, const void* rhs, size_t count) {
    const char* s_lhs = (const char*)lhs;
    const char* s_rhs = (const char*)rhs;

    for (size_t i = 0; i < count; i++) {
        char lhs_c = s_lhs[i];
        char rhs_c = s_rhs[i];

        if (lhs_c != rhs_c) {
            if (!isalnum(lhs_c) || !isalnum(rhs_c))
                return lhs_c < rhs_c ? -1 : 1;
            
            int lhs_upper = isupper(lhs_c);
            int rhs_upper = isupper(rhs_c);
            if (lhs_upper && !rhs_upper)
                return 1;
            
            else if (!lhs_upper && rhs_upper)
                return -1;
            
            return lhs_c < rhs_c ? -1 : 1;
        }
    }

    return 0;
}

void memset(void* dest, int ch, size_t count) {
    char* str = (char*)dest;
    for (size_t i = 0; i < count; i++)
        str[i] = (char)ch;
}

void memcpy(void* restrict dst, const void* restrict src, size_t count) {
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    uint64_t* restrict qword_dst = (uint64_t* restrict)dst;
    const uint64_t* restrict qword_src = (const uint64_t* restrict)src;

    for (size_t i = 0; i < qwords; i++)
        qword_dst[i] = qword_src[i];
    
    uint8_t* restrict byte_dst = (uint8_t* restrict)&qword_dst[qwords];
    const uint8_t* restrict byte_src = (uint8_t* restrict)&qword_src[qwords];

    for (size_t i = 0; i < bytes; i++)
        byte_dst[i] = byte_src[i];
}

void memmove(void* dst, const void* src, size_t count) {
    size_t qwords = count / 8;
    size_t bytes = count % 8;

    uint64_t* restrict qword_dst = (uint64_t* restrict)dst;
    const uint64_t* restrict qword_src = (const uint64_t* restrict)src;

    for (size_t i = 0; i < qwords; i++)
        qword_dst[i] = qword_src[i];
    
    uint8_t* restrict byte_dst = (uint8_t* restrict)&qword_dst[qwords];
    const uint8_t* restrict byte_src = (uint8_t* restrict)&qword_src[qwords];

    for (size_t i = 0; i < bytes; i++)
        byte_dst[i] = byte_src[i];
}