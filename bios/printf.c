#include "printf.h"

#include "machine.h"

#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>

void debug_trace(const char* str) {
    size_t size = 0;
    while (str[size]) size++;
    machine_call(MACHINE_CALL_COUT, str, size);
}

int putc(char c) {
    static char buffer[512];
    static size_t index = 0;

    bool flush = false;

    if (c == 0 && index == 0)
        return 0;
    
    else if ((index + 2) == sizeof(buffer))
        flush = true;
    
    else if (c == 0)
        flush = true;
    
    else {
        buffer[index++] = c;

        if (c == '\n')
            flush = true;
    }
    
    if (flush) {
        machine_call(MACHINE_CALL_COUT, buffer, index);
        index = 0;
    }

    return 1;
}

int print_num(uint32_t num, uint32_t base, bool is_signed, bool capital) {
    int chars = 0;
    if (is_signed && num & (1U<<31)) {
        chars += putc('-');
        num = ~num + 1;
    }

    if (num == 0) {
        chars += putc('0');
        return chars;
    }

    char buffer[32];
    size_t i = 0;

    while (num) {
        char c = num % base;
        num /= base;

        if (c >= 0 && c <= 9) c += '0';
        if (c >= 10 && c <= 15 && capital) c += 'A' - 10;
        else if (c >= 10 && c <= 15) c += 'a' - 10;

        buffer[i] = c;
        i++;
    }
    
    while (i) {
        chars += putc(buffer[--i]);
    }

    return chars;
}

int printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    int chars = 0;

    while (*fmt) {
        char c = *fmt;
        fmt++;

        if (c == '%') {
            c = *fmt;
            fmt++;

            switch (c) {
                case 'u': {
                    uint32_t num = va_arg(args, uint32_t);
                    chars += print_num(num, 10, false, false);
                    break;
                }
                case 'i': {
                    int32_t num = va_arg(args, int32_t);
                    chars += print_num(*(uint32_t*)&num, 10, true, false);
                    break;
                }
                case 'o': {
                    uint32_t num = va_arg(args, uint32_t);
                    chars += print_num(num, 8, false, false);
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    chars += print_num(num, 16, false, false);
                    break;
                }
                case 'X': {
                    uint32_t num = va_arg(args, uint32_t);
                    chars += print_num(num, 16, false, true);
                    break;
                }
                case 'b': {
                    uint32_t num = va_arg(args, uint32_t);
                    chars += print_num(num, 2, false, false);
                    break;
                }
                case 'f': {
                    int32_t num = va_arg(args, double) * 100.0;
                    chars += print_num(num / 100, 10, true, false);
                    chars += putc('.');
                    chars += print_num(num % 100, 10, true, false);
                    break;
                }
                case 'c': {
                    c = va_arg(args, uint32_t);
                    chars += putc(c);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    while (*str) {
                        chars += putc(*str);
                        str++;
                    }
                    break;
                }
                case '%': {
                    chars += putc('%');
                    break;
                }
                default: {
                    chars += putc(' ');
                    break;
                }
            }
        }
        else chars += putc(c);
    }

    va_end(args);
    
    return chars;
}

void flush() {
    putc(0);
}