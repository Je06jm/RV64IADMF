.section .bootstrap, "aw"

.globl _start
.extern putc
.extern main
.extern printf

_start:
    li t0, 1
    la sp, stack_top

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

.globl current_hart
current_hart:
    csrr a0, mhartid
    ret

.globl cpu_bootstrap
cpu_bootstrap:
    lw t0, cpu_bootstrap_stack_address
    mv sp, t0
    lw t0, cpu_bootstrap_start_address
    jr t0

.globl load_reserve
load_reserve:
    lr.w a0, (a0)
    ret

.globl store_conditional
store_conditional:
    sc.w a0, a1, (a0)
    ret

.data

.align 4
argv_0: .string "            "

.align 4
argv_1: .string "bios.elf"

.align 4
argv: .word argv_0, argv_1, 0

.align 4
.globl cpu_bootstrap_stack_address
cpu_bootstrap_stack_address: .word 0
.globl cpu_bootstrap_start_address
cpu_bootstrap_start_address: .word 0

.bss

.align 16
stack_bottom:
.space 1024 * 16
stack_top: