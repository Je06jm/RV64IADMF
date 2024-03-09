#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

void machine_call(uint32_t sys, ...);

#define MACHINE_CALL_PRINT 0

void putc(char c) {
    static char buffer[512];
    static size_t index = 0;
    static size_t size = 0;

    bool flush = false;
    if (c == 0 && size) flush = true;
    else if (c == 0) return;
    if (c == '\n') flush = true;
    if (c == '\r') {
        index = 0;
        return;
    }
    if (c == '\t') {
        do {
            buffer[index++] = ' ';
        } while (index % 4);
        if (index > size) size = index;
        return;
    }

    if (index > size) size = index;

    if (flush) {
        buffer[size] = 0;
        machine_call(MACHINE_CALL_PRINT, buffer, size);
        index = 0;
        size = 0;
        return;
    }

    buffer[index++] = c;
}

void print_num(uint32_t num, uint32_t base, bool is_signed, bool capital) {
    if (is_signed && num & (1U<<31)) {
        putc('-');
        num = ~num + 1;
    }

    if (num == 0) {
        putc('0');
        return;
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
        putc(buffer[--i]);
    }
}

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        char c = *fmt;
        fmt++;

        if (c == '%') {
            c = *fmt;
            fmt++;

            switch (c) {
                case 'u': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 10, false, false);
                    break;
                }
                case 'i': {
                    int32_t num = va_arg(args, int32_t);
                    print_num(*(uint32_t*)&num, 10, true, false);
                    break;
                }
                case 'o': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 8, false, false);
                    break;
                }
                case 'x': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 16, false, false);
                    break;
                }
                case 'X': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 16, false, true);
                    break;
                }
                case 'b': {
                    uint32_t num = va_arg(args, uint32_t);
                    print_num(num, 2, false, false);
                    break;
                }
                case 'c': {
                    c = va_arg(args, uint32_t);
                    putc(c);
                    break;
                }
                case 's': {
                    printf(va_arg(args, const char*));
                    break;
                }
                case '%': {
                    putc('%');
                    break;
                }
                default: {
                    putc(' ');
                    break;
                }
            }
        }
        else putc(c);
    }

    va_end(args);
}

void bios_main() {
    printf("Hello world!\n");
    printf("%i\n", 42);
    printf("0x%x\n", 0xdeadbeef);
    printf("0X%X\n", 0XDEADBEEF);
    printf("%i\n", -13);
    printf("0b%b\n", 0b11110001);
}