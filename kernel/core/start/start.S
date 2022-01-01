 #include "lib/asm_utils.h"

.section ".text.boot"

.global _start
_start:
    /* Get the CPU into EL1 since we do not support hypervisors  */
    mrs     x0, CurrentEL
    lsr     x0, x0, #2                  /* drop reserved bytes */

    cmp     x0, #3                      /* EL3 */
    beq     Lstart_in_el3
    cmp     x0, #2                      /* EL2 */
    beq     Lstart_in_el2
    b       Lstart_in_el1               /* EL1 */

Lstart_in_el3:
    /* If we started in EL3, we're in secure mode. Return to EL2-non-secure */
    mov     x2, #0x5b1
    msr     scr_el3, x2
    mov     x2, #0x3c9
    msr     spsr_el3, x2
    ADRL    x2, Lstart_in_el2
    msr     elr_el3, x2
    eret

Lstart_in_el2:
    msr     sp_el1, x1
    /* enable CNTP for EL1 */
    mrs     x0, cnthctl_el2
    orr     x0, x0, #3
    msr     cnthctl_el2, x0
    msr     cntvoff_el2, xzr
    /* enable AArch64 in EL1 */
    mrs     x0, hcr_el2
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    /* return to EL1 */
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    ADRL    x2, Lstart_in_el1
    msr     elr_el2, x2
    eret

    
Lstart_in_el1:
    /* Enable NEON */
    mov     x0, #(0b11<<20)                /* FPEN_ENABLE */
    msr     cpacr_el1, x0

    mrs     x0, sctlr_el1
    bic     x0, x0, 0b11111                /* disable A/C/SA/SA0 */
    msr     sctlr_el1, x0

    /* Spin secondary cores */
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbnz    x1, _secondary_core_prepare

    /* Setup bootstrap interrupts */
    ADRL    x1, exception_vectors_el1
    msr     vbar_el1, x1
    msr     DAIFClr, #DAIF_ALL           /* enable all exceptions */

    /* Setup primary stack in SPSel 0 to the pre-allocated boot stack */
    /* Prepare exception stack */
    msr     SPSel, #1
    ADRL    x1, __primary_cpu_exception_stack
    mov     sp, x1
    /* Prepare initial kernel thread stack */
    msr     SPSel, #0
    ADRL    x1, __primary_cpu_boot_stack
    mov     sp, x1

    b      _primary_core_boot


_primary_core_boot:
    bl      EXT(main)
    /* main should never return, panic if it does */
    ADRL    x0, Lstartup_returned
    b       EXT(panic_simple)

_secondary_core_prepare:
1:
    wfe
    b 1b

.section ".rodata"
Lstartup_returned:
.string "main() must not return!"