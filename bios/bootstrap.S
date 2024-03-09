.section .bootstrap, "aw"

.globl _start
.extern putc
.extern bios_main
.extern printf

_start:
    li sp, 0x4000
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