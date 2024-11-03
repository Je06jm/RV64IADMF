OUTPUT_FORMAT("elf64-littleriscv", "elf64-littleriscv", "elf64-littleriscv")
OUTPUT_ARCH(riscv)

ENTRY(_start)

MEMORY
{
    ram (rwx) : ORIGIN = 0x00001000, LENGTH = 512K
}

SECTIONS
{
    img_begin = 0x1000;

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

    img_end = .;

    /DISCARD/ :
    {
        *(.comment)
        *(.note*)
    }
}