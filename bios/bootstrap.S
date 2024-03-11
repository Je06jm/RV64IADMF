.section .bootstrap, "aw"

.globl _start
.extern putc
.extern main
.extern printf

_start:
    li t0, 1
    la sp, stack_top

    li a0, 1
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

.data

.align 4
argv_0: .string "bios.elf"

.align 4
argv: .word argv_0, 0

.bss

.align 16
stack_bottom:
.space 1024 * 16
stack_top: