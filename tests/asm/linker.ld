ENTRY(_start)

SECTIONS
{
    . = 0x00010000;
    .text : {
        *(.text._start)
        *(.text*)
    }
    .rodata : {
        *(.rodata*)
    }
    .data : {
        *(.data*)
    }
    .bss : {
        *(.bss*)
    }
}
