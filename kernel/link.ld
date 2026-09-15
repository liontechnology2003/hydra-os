ENTRY(loader)

SECTIONS {
    . = 0x00100000;

    kernel_start = .;

    .text ALIGN (0x1000) :
    {
        *(.text)
        *(.text.*)
    }

    .rodata ALIGN (0x1000) :
    {
        *(.rodata*)
        *(.rodata)
    }

    /* C++ exception tables */
    .eh_frame ALIGN (4) :
    {
        __eh_frame_start = .;
        *(.eh_frame)
        *(.eh_frame.*)
        __eh_frame_end = .;
    }

    .gcc_except_table ALIGN (4) :
    {
        *(.gcc_except_table)
        *(.gcc_except_table.*)
    }

    .data ALIGN (0x1000) :
    {
        *(.data)
        *(.data.*)
    }

    /* C++ constructor/destructor arrays */
    .init_array ALIGN (4) :
    {
        __init_array_start = .;
        *(.init_array)
        *(.init_array.*)
        __init_array_end = .;
    }

    .fini_array ALIGN (4) :
    {
        __fini_array_start = .;
        *(.fini_array)
        *(.fini_array.*)
        __fini_array_end = .;
    }

    .preinit_array ALIGN (4) :
    {
        *(.preinit_array)
    }

    .bss ALIGN (0x1000) :
    {
        *(COMMON)
        *(.common)
        *(.bss)
        *(.bss.*)
    }

    kernel_end = .;
}
