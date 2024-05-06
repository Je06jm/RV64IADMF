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

.globl set_vtable_addr
set_vtable_addr:
    csrw satp, a0
    ret

.global enable_smode
enable_smode:
    // Set reset vector
    la t0, .smode
    csrw mepc, t0

    // Set reset privilege to supervisor
    csrr t0, mstatus
    li t1, 0b1100000000000
    not t1, t1
    and t0, t0, t1
    li t1, 0b100000000000
    or t0, t0, t1
    csrw mstatus, t0

    // Setup machine ebreak
    la t0, .mmode
    csrw mtvec, t0

    ebreak
    mret

.smode:
    // Enable virtual memory
    csrr t0, satp
    li t1, 0x80000000
    or t0, t0, t1
    csrw satp, t0
    
    ret

.mmode:
    // Forward machine calls
    ebreak
    ecall
    addi sp, sp, -8
    sw t0, (sp)
    sw t1, 4(sp)

    csrr t0, mcause
    li t1, 0x80000000
    and t0, t0, t1
    beq t0, t1, .dont_inc_pc

    csrr t0, mepc
    addi t0, t0, 4
    csrw mepc, t0

.dont_inc_pc:
    lw t0, (sp)
    lw t1, 4(sp)
    addi sp, sp, 8

    mret

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