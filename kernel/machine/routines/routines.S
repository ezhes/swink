#include "lib/asm_utils.h"
.section ".text"
.global EXT(routines_core_idle)
EXT(routines_core_idle):
    wfe
    b routines_core_idle

.global EXT(routines_adp_application_exit)
EXT(routines_adp_application_exit):
    /* x1 is the address of the paremeter control block. We place it on the
    stack */
    mov     x1, sp

    /* Exit status code in x0. Host QEMU process exits with that status. */
    /* 0x20026 == ADP_Stopped_ApplicationExit */
    MOVC    x2, 0x20026
    stp     x2, x0, [sp], 16

    /* SYS_EXIT */
    mov     w0, #0x18

    /* Do the semihosting call on aarch64. */
    hlt     0xf000
