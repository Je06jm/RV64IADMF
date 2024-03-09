.section .bootstrap, "aw"

.globl _start
.extern putc
.extern bios_main
.extern printf

_start:
    li sp, 0x4000
    jal ra, bios_main

    li a0, 0
    //call putc

    li a0, 0
    la a1, ret_error
    li a2, 22
    ecall
.loop:
    j .loop

.text

.globl machine_call
machine_call:
    ecall
    ret

.data
ret_error: .string "bios_main has returned"