SECTIONS
{
    . = 0x80000;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        . = ALIGN(16);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    .primary_cpu_boot_stacks :
    {
        . = ALIGN(16);
        . = . + 16384;
        __primary_cpu_boot_stack  = .;
        . = . + 16384;
        __primary_cpu_exception_stack  = .;
    }

    _end = .;
    
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}