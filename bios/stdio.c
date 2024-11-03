#include "stdio.h"

#include "machine.h"

#include <stdarg.h>
#include <stdbool.h>

FILE _stdin_impl;
FILE _stdout_impl;
FILE _stderr_impl;

FILE* stdin = &_stdin_impl;
FILE* stdout = &_stdout_impl;
FILE* stderr = &_stderr_impl;

typedef struct stdout_data {
    char buffer[512];
    size_t index;
} stdout_data;

void std_io_file_open(FILE* file, const char* path, const char* mode) {
    fprintf(stderr, "Cannot open stdin/stdout/stderr\n");
}

void std_io_file_close(FILE* file) {
    fprintf(stderr, "Cannot close stdin/stdout/stderr\n");
}

size_t std_io_file_read(FILE* file, size_t count, void* buffer) {
    fprintf(stderr, "Cannot read from stdout/stderr\n");
    return 0;
}

size_t std_stdin_read(FILE* file, size_t count, void* buffer) {
    fprintf(stderr, "Reading from stdin not implemented\n");
    return 0;
}

size_t std_stdout_write(FILE* file, size_t count, const void* buffer) {
    stdout_data* data = (stdout_data*)file->data;

    const char* str = (const char*)buffer;
    
    for (size_t i = 0; i < count; i++) {
        if (data->index >= 512)
            file->flush(file);
        
        data->buffer[data->index++] = str[i];

        if (str[i] == '\n')
            file->flush(file);
    }

    return count;
}

size_t std_stderr_write(FILE* file, size_t count, const void* buffer) {
    const char* str = (const char*)buffer;
    machine_break();
    machine_call(MACHINE_CALL_COUT, str, count);
    return count;
}

size_t std_stdin_write(FILE* file, size_t count, const void* buffer) {
    fprintf(stderr, "Cannot write to stdin\n");
    return 0;
}

int std_io_file_flush(FILE* file) {
    // Do nothing
    return 0;
}

int std_stdout_flush(FILE* file) {
    stdout_data* data = (stdout_data*)file->data;
    machine_break();
    machine_call(MACHINE_CALL_COUT, (const char*)data->buffer, data->index);
    data->index = 0;
    return 0;
}

stdout_data stdout_temp;

int ctor_setup_stdio(void) {
    _stdin_impl.open = std_io_file_open;
    _stdin_impl.close = std_io_file_close;
    _stdin_impl.read = std_stdin_read;
    _stdin_impl.write = std_stdin_write;
    _stdin_impl.flush = std_io_file_flush;
    _stdin_impl.data = NULL;

    _stdout_impl.open = std_io_file_open;
    _stdout_impl.close = std_io_file_close;
    _stdout_impl.read = std_io_file_read;
    _stdout_impl.write = std_stdout_write;
    _stdout_impl.flush = std_stdout_flush;
    _stdout_impl.data = &stdout_temp;

    _stderr_impl.open = std_io_file_open;
    _stderr_impl.close = std_io_file_close;
    _stderr_impl.read = std_io_file_read;
    _stderr_impl.write = std_stderr_write;
    _stderr_impl.flush = std_io_file_flush;
    _stderr_impl.data = NULL;

    return true;
}

void ctor_cleanup_stdio(void) {
    fflush(stdout);
}

int printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int chars = vfprintf(stdout, fmt, args);
    va_end(args);
    return chars;
}

int fprintf(FILE* file, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int chars = vfprintf(file, fmt, args);
    va_end(args);
    return chars;
}

int snprintf(char* buffer, size_t bufsize, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int chars = vsnprintf(buffer, bufsize, fmt, args);
    va_end(args);
    return chars;
}

int vprintf(const char* fmt, va_list args) {
    return fprintf(stdout, fmt, args);
}

typedef int (*str_writer)(void* obj, const char* str, size_t size);
int str_formatter(str_writer writer, void* obj, const char* fmt, va_list args);

int f_str_writer(void* obj, const char* str, size_t size) {
    FILE* file = (FILE*)obj;
    file->write(file, size, str);
    return size;
}

int vfprintf(FILE* file, const char* fmt, va_list args) {
    return str_formatter(f_str_writer, file, fmt, args);
}

typedef struct s_str_data {
    char* buffer;
    size_t bufsize;
    size_t written;
} s_str_data;

int s_str_writer(void* obj, const char* str, size_t size) {
    s_str_data* data = (s_str_data*)obj;

    for (size_t i = 0; i < size; i++) {
        if (data->written >= data->bufsize)
            return i;
        data->buffer[data->written] = str[i];
        data->written++;
    }

    return size;
}

int vsnprintf(char* buffer, size_t bufsize, const char* fmt, va_list args) {
    s_str_data data;
    data.buffer = buffer;
    data.bufsize = bufsize;
    data.written = 0;

    return str_formatter(s_str_writer, &data, fmt, args);
}

int num_formatter(str_writer writer, void* obj, uint64_t num, uint32_t base, bool is_signed, bool capital) {
    int chars = 0;
    if (is_signed && num & (1ULL<<63)) {
        chars += writer(obj, "-", 1);
        num = ~num + 1;
    }

    if (num == 0) {
        chars += writer(obj, "0", 1);
        return chars;
    }

    char buffer[64];
    size_t i = 0;

    while (num) {
        char c = num % base;
        num /= base;

        if (c >= 0 && c <= 9) c += '0';
        else if (c >= 10 && c <= 15 && capital) c += 'A' - 10;
        else if (c >= 10 && c <= 15) c += 'a' - 10;

        buffer[i] = c;
        i++;
    }

    char str[64];
    int j;
    for (j = 0; i; j++, j++)
        str[j] = buffer[--i];
    str[j] = '\0';

    chars += writer(obj, str, j);

    return chars;
}

int str_formatter(str_writer writer, void* obj, const char* fmt, va_list args) {
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
                    chars += num_formatter(writer, obj, num, 10, false, false);
                    break;
                }
                case 'i': {
                    int32_t num = va_arg(args, int32_t);
                    chars += num_formatter(writer, obj, *(uint32_t*)&num, 10, true, false);
                    break;
                }
                case 'o': {
                    uint64_t num = va_arg(args, uint64_t);
                    chars += num_formatter(writer, obj, num, 8, false, false);
                    break;
                }
                case 'x': {
                    uint64_t num = va_arg(args, uint64_t);
                    chars += num_formatter(writer, obj, num, 16, false, false);
                    break;
                }
                case 'X': {
                    uint64_t num = va_arg(args, uint64_t);
                    chars += num_formatter(writer, obj, num, 16, false, true);
                    break;
                }
                case 'b': {
                    uint64_t num = va_arg(args, uint64_t);
                    chars += num_formatter(writer, obj, num, 2, false, false);
                    break;
                }
                case 'f': {
                    int64_t num = va_arg(args, double) * 100.0;
                    chars += num_formatter(writer, obj, num / 100, 10, true, false);
                    chars += writer(obj, ".", 1);
                    chars += num_formatter(writer, obj, num % 100, 10, true, false);
                    break;
                }
                case 'c': {
                    char buff[2];
                    buff[0] = c;
                    buff[1] = '\0';
                    c = va_arg(args, uint32_t);
                    chars += writer(obj, buff, 1);
                    break;
                }
                case 's': {
                    const char* str = va_arg(args, const char*);
                    size_t size = 0;
                    for (size = 0; str[size] != '\0'; size++) {}
                    chars += writer(obj, str, size);
                    break;
                }
                case '%': {
                    chars += writer(obj, "%", 1);
                    break;
                }
                case 'l': {
                    c = *fmt;
                    fmt++;

                    switch (c) {
                        case 'l': {
                            c = *fmt;
                            fmt++;

                            switch (c) {
                                case 'u': {
                                    uint64_t num = va_arg(args, uint64_t);
                                    chars += num_formatter(writer, obj, num, 10, false, false);
                                    break;
                                }
                                default:{
                                    fmt--;
                                    int64_t num = va_arg(args, int64_t);
                                    chars += num_formatter(writer, obj, num, 10, true, false);
                                    break;
                                }
                            }
                            break;
                        }
                        default: {
                            fmt--; 
                            int32_t num = va_arg(args, int32_t);
                            chars += num_formatter(writer, obj, *(uint64_t*)&num, 10, true, false);
                            break;
                        }
                    }
                    break;
                }
                default: {
                    chars += writer(obj, " ", 1);
                    break;
                }
            }
        }
        else {
            char buff[2];
            buff[0] = c;
            buff[1] = '\0';
            chars += writer(obj, buff, 1);
        }
    }

    return chars;
}

int fflush(FILE* file) {
    return file->flush(file);
}