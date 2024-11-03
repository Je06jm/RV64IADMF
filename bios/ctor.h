#ifndef CTOR_H
#define CTOR_H

#include <stdbool.h>

int ctor_init();
void ctor_cleanup();

int ctor_setup_stdlib(void);
void ctor_cleanup_stdlib(void);
int ctor_setup_stdio(void);
void ctor_cleanup_stdio(void);

#endif