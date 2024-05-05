.section .bootstrap, "aw"

.globl _start
.extern putc
.extern main
.extern printf

_start:
    li t0, 1
    la sp, stack_top

    la t0, handler_mtrap
    csrw mtvec, t0

    la t0, handler_strap
    csrw stvec, t0

    csrr t0, mvendorid
    csrr t1, marchid
    csrr t2, mimpid

    la t3, argv_0
    sw t1, 0(t3)
    sw t2, 4(t3)

    li a0, 2
    la a1, argv
    call main

    addi sp, sp, -4
    sw a0, (sp)

    li a0, 0
    call putc

    lw a1, (sp)
    add sp, sp, 4

    li a0, -1
    ecall

.text

.globl machine_call
machine_call:
    ecall
    ret

.globl machine_break
machine_break:
    ebreak
    ret

.global cust_mtrap
cust_mtrap:
    ebreak
    addi sp, sp, -4
    sw ra, (sp)
    mv x0, x0
    lw ra, (sp)
    add sp, sp, 4
    ret

.global cust_strap
cust_strap:
    ebreak
    addi sp, sp, -4
    sw ra, (sp)
    mv x0, x0
    lw ra, (sp)
    add sp, sp, 4
    ret

.extern mtrap_handler
.extern strap_handler

handler_mtrap:
    ebreak
    call mtrap_handler
    ebreak
    mret

handler_strap:
    ebreak
    call strap_handler
    ebreak
    sret

.data

.align 4
argv_0: .string "            "

.align 4
argv_1: .string "bios.elf"

.align 4
argv: .word argv_0, argv_1, 0

.bss

.align 16
stack_bottom:
.space 1024 * 16
stack_top: