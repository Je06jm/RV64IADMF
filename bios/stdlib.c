#include "stdlib.h"

#include "machine.h"

void abort(void) {
    raise(SIGABRT);
}

typedef int (*exit_func)(void);

typedef struct exit_func_array {
    exit_func* funcs;
    size_t count;
} exit_func_array;

exit_func_array exit_funcs;
exit_func_array quick_exit_funcs;

void exit(int code) {
    for (size_t i = 0; i < exit_funcs.count; i++)
        exit_funcs.funcs[i]();
    
    machine_call(MACHINE_CALL_EXIT, code);
}