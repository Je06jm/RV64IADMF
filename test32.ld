OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv", "elf32-littleriscv")
OUTPUT_ARCH(riscv)

ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x00001000, LENGTH = 512K
}

SECTIONS
{
    .bootstrap :
    {
        *(.bootstrap)
    } > ram
    
    .text :
    {
        *(.text)
    } > ram

    .rodata :
    {
        *(.rodata)
    } > ram

    .data :
    {
        *(.data)
    } > ram

    .bss :
    {
        *(.bss)
    } > ram

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }
}