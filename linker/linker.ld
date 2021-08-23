/* Author: strawberryhacker */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm","elf32-littlearm")
OUTPUT_ARCH(arm)

MEMORY {
    flash (rx ) : ORIGIN = 0x00400000, LENGTH = 0x80000    /* 512 KiB */
    ram   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x20000    /* 128 KiB */
}

stack_size = 4096;

SECTIONS {
    .vector_table : {
        linker_vector_start = .;
        KEEP(*(.vector_table))
    } > flash

    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        . = ALIGN(4);
    } > flash

    .rodata : {
        . = ALIGN(4);
        KEEP(*(.rodata))
        KEEP(*(.rodata*))
        . = ALIGN(4);
    } > flash

    .data : {
        . = ALIGN(4);
        linker_data_start = .;
        KEEP(*(.data))
        KEEP(*(.data*))
        KEEP(*(.ramfunc))
        KEEP(*(.ramfunc*))
        . = ALIGN(4);
        linker_data_end = .;
    } > ram AT > flash

    linker_relocate_start = LOADADDR(.data);

    .bss (NOLOAD) : {
        . = ALIGN(4);
        linker_zero_start = .;
        *(.bss)
        *(.bss*)
        . = ALIGN(4);
        linker_zero_end = .;
    } > ram

    .stack (NOLOAD) : {
        . = ALIGN(8);
        . = . + stack_size;
        . = ALIGN(8);
        linker_stack_end = .;
    } > ram
}
