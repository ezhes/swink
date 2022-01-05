SECTIONS
{
    . = 0xffffff8000000000;
    __kernel_map_start = .;
    __kernel_text_start = .;
    .text : { KEEP(*(.text.boot)) *(.text .text.* .gnu.linkonce.t*) }
    . = ALIGN(0x1000);
    __kernel_text_end = .;

    /* rodata is page aligned so we can memory protect it without relocation */
    __kernel_rodata_start = .;
    .rodata : { *(.rodata .rodata.* .gnu.linkonce.r*) }
    . = ALIGN(0x1000);
    __kernel_rodata_end = .;

    /* Data/BSS is page aligned so we can memory protect it without relocation */
    __kernel_rw_data_start = .;
    PROVIDE(_data = .);
    .data : { *(.data .data.* .gnu.linkonce.d*) }
    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }
    . = ALIGN(0x1000);
    __kernel_rw_data_end = .;
    __kernel_map_end = .;

    _end = .;
    
   /DISCARD/ : { *(.comment) *(.gnu*) *(.note*) *(.eh_frame*) }
}