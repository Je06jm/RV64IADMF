.section .bootstrap, "aw"

.globl _start
.extern putc
.extern bios_main
.extern printf

_start:
    la sp, stack_top
    jal ra, bios_main

    li a0, 0
    call putc

    la a0, ret_error
    call printf
.loop:
    j .loop

.text

.globl machine_call
machine_call:
    ecall
    ret

.data
ret_error: .string "bios_main has returned\n"

.bss

.align 16
stack_bottom:
.space 1024 * 16
stack_top: