.text

.globl _start

_start:
    li t0, 42
    li t1, 60
    sub t2, t0, t1
    add t3, t0, t1
    
    la t0, hello
    ecall

.loop:
    j .loop

.data

hello:
    .string "Hello there!"