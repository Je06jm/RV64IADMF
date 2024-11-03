#include "stdlib.h"

#include "string.h"
#include "ctor.h"
#include "machine.h"
#include "memmap.h"
#include "stdio.h"

#define HEAP_SIZE (16 * 1024) // 16 KiBs

void abort(void) {
    raise(SIGABRT);
}

typedef void (*exit_func)(void);

typedef struct exit_func_array {
    exit_func* funcs;
    size_t count;
} exit_func_array;

exit_func_array exit_funcs;
exit_func_array quick_exit_funcs;

void exit(int code) {
    for (size_t i = exit_funcs.count - 1; i < exit_funcs.count; i++)
        exit_funcs.funcs[i]();
    
    ctor_cleanup_stdlib();
    
    machine_call(MACHINE_CALL_EXIT, code);
}

void quick_exit(int code) {
    for (size_t i = quick_exit_funcs.count - 1; i < quick_exit_funcs.count; i++)
        quick_exit_funcs.funcs[i]();
    
    machine_call(MACHINE_CALL_EXIT, code);
}

int atexit(void (*func)(void)) {
    if (!func) return 0;
    exit_funcs.funcs = (exit_func*)realloc(exit_funcs.funcs, (exit_funcs.count + 1) * sizeof(exit_func));
    exit_funcs.funcs[exit_funcs.count] = func;
    exit_funcs.count++;
    return 0;
}

int at_quick_exit(void (*func)(void)) {
    if (!func) return 0;
    quick_exit_funcs.funcs = (exit_func*)realloc(quick_exit_funcs.funcs, (quick_exit_funcs.count + 1) * sizeof(exit_func));
    quick_exit_funcs.funcs[quick_exit_funcs.count] = func;
    quick_exit_funcs.count++;
    return 0;
}

void _sig_dfl_impl(int code) {
    switch (code) {
        case SIGTERM:
            fprintf(stderr, "exited due to signal received (sigterm)\n");
            quick_exit(EXIT_FAILURE);
            break;
        
        case SIGSEGV:
            fprintf(stderr, "exited due to signal received (sigsegv)\n");
            quick_exit(EXIT_FAILURE);
            break;
        
        case SIGINT:
            fprintf(stderr, "exited due to signal received (sigint)\n");
            exit(EXIT_FAILURE);
            break;
        
        case SIGILL:
            fprintf(stderr, "exited due to signal received (sigill)\n");
            exit(EXIT_FAILURE);
            break;
        
        case SIGABRT:
            fprintf(stderr, "exited due to signal received (sigabrt)\n");
            quick_exit(EXIT_FAILURE);
            break;
        
        case SIGFPE:
            fprintf(stderr, "exited due to signal received (sigfpe)\n");
            exit(EXIT_FAILURE);
            break;
        
        default:
            fprintf(stderr, "exited due to unknown signal received (%i)\n", code);
            exit(EXIT_FAILURE);
            break;
    }
}

void _sig_ign_impl(int code) {

}

typedef void (*signal_func)(int);

signal_func signal_funcs[6];

int signal(int sig, void (*func)(int)) {
    if (sig > SIGFPE)
        return SIG_ERR;
    signal_funcs[sig] = func;
    return !SIG_ERR;
}

int raise(int sig) {
    if (sig > SIGFPE)
        return SIG_ERR;
    signal_funcs[sig](sig);
    return !SIG_ERR;
}

typedef enum HeapFlags {
    HEAP_FLAG_USED = (1ULL << 0)
} HeapFlags;

typedef struct HeapNode {
    struct HeapNode* last;
    struct HeapNode* next;
    HeapFlags flags;
} HeapNode;

HeapNode* heap_node;

void* malloc(size_t size) {
    HeapNode* current = heap_node;

    while (current) {
        if (current->flags & HEAP_FLAG_USED) {
            current = current->next;
            continue;
        }

        size_t node_size = (size_t)(current->next - current) - sizeof(HeapNode);
        if (node_size < size) {
            current = current->next;
            continue;
        }

        if (node_size >= (size + sizeof(HeapNode))) {
            uintptr_t new_addr = (uintptr_t)(current + 1) + size;
            new_addr = (new_addr + 7) & ~7;
            HeapNode* new_node = (HeapNode*)new_addr;
            
            new_node->last = current;
            new_node->next = current->next;
            if (current->next)
                current->next->last = new_node;
            
            new_node->flags = 0;
            current->next = new_node;
        }

        current->flags |= HEAP_FLAG_USED;
        void* ptr = (void*)(current + 1);
        return ptr;
    }

    return NULL;
}

void* calloc(size_t num, size_t size) {
    return malloc(num * size);
}

void* realloc(void* ptr, size_t new_size) {
    HeapNode* node = (HeapNode*)ptr;
    node--;
    size_t size = (size_t)(node->next - node) - sizeof(HeapNode);
    void* new_ptr = malloc(new_size);
    memcpy(new_ptr, ptr, size);
    free(ptr);
    return new_ptr;
}

void free(void* ptr) {
    HeapNode* node = (HeapNode*)ptr;
    node--;
    node->flags = 0;

    if (node->last && !(node->last->flags & HEAP_FLAG_USED)) {
        if (node->next)
            node->next->last = node->last;
        
        node->last->next = node->next;
        node = node->last;
    }

    if (node->next && !(node->next->flags & HEAP_FLAG_USED)) {
        if (node->last)
            node->last->next = node->next;
        
        node->next->last = node->last;
    }
}

void memprint() {
    HeapNode* current = heap_node;
    while (current) {
        uintptr_t size = (uintptr_t)(current->next - current) - sizeof(HeapNode);
        printf("%x  %i  %s\n", current, size, current->flags & HEAP_FLAG_USED ? "Used" : "Free");
        current = current->next;
    }
}

double atof(const char* str) {
    char whole[64];
    char frac[64];
    const char* addr = memchr(str, '.', 64);
    if (!addr)
        return atoll(str);
    
    size_t index = (size_t)(addr - str);
    memcpy(whole, str, index);
    size_t size = strnlen(str, 64);
    size_t f_size = size - index - 1;
    memcpy(frac, &str[index + 1], f_size);

    whole[index] = '\0';
    frac[f_size] = '\0';

    long long i_whole = atoll(whole);
    long long i_frac = atoll(frac);

    double tens = 1.0;
    for (size_t i = 0; i < f_size; i++)
        tens *= 10.0;

    double res = i_whole;
    res = (double)i_frac / tens;
    return res; 
}

int atoi(const char* str) {
    return atoll(str);
}

long atol(const char* str) {
    return atoll(str);
}

long long atoll(const char* str) {
    while (*str == ' ') str++;

    bool is_neg = false;
    if (*str == '-') {
        is_neg = true;
        str++;
    }

    long long value = 0;
    while (*str != '\0') {
        long long single = *str - '0';
        str++;
        value *= 10;
        value += single;
    }

    if (is_neg)
        return -value;
    
    return value;
}

uint8_t heap_ram_block[HEAP_SIZE];

int ctor_setup_stdlib(void) {
    uintptr_t end_addr = (uintptr_t)&heap_ram_block[HEAP_SIZE];
    HeapNode* end = (HeapNode*)end_addr;
    end--;
    HeapNode* start = (HeapNode*)heap_ram_block;

    start->last = NULL;
    start->next = end;
    start->flags = 0;

    end->last = start;
    end->next = NULL;
    end->flags = 0;

    heap_node = start;
    
    return true;
}

void ctor_cleanup_stdlib(void) {

}