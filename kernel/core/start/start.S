#include "lib/asm_utils.h"
#include "machine/pmap/pmap_asm.h"
#include "machine/platform_registers.h"

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
    /* Spin secondary cores */
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbnz    x1, _secondary_core_prepare

    /* Enable NEON */
    MOVC     x0, CPACR_EL1_CONFIG
    msr     cpacr_el1, x0

    /*
    We assume that the memory after the kernel is writable.
    Throughout init, we hold the ""allocator"" pointer in x0 which always refers
    to the next free slot

    Allocations (+__kernel_map_end):
    [0x00000, 0x01000) : Boot CPU exception stack
    [0x01000, 0x02000) : Boot CPU kernel stack
    [0x02000, 0x06000) : VM bootstrap tables (4 cnt.) -- see vm_bootstrap.S
    */

    /* Setup bootstrap data below the code */
    ADRL    x19, __kernel_map_end            /* stash the bootstrap PA region max (always page aligned) */
    ADRL    x20, __kernel_map_start          /* stash the start PA for pmap_init.c to load the kernel */
    mov     x0, x19
    /* Prepare exception stack */
    msr     SPSel, #1
    add     x0, x0, PAGE_SIZE
    mov     sp, x0                           /* end of first page */

    /* Prepare initial kernel thread stack */
    msr     SPSel, #0
    add     x0, x0, PAGE_SIZE
    mov     sp, x0                           /* end of second page */

    /* Setup bootstrap tables -- returns us back into KVA space */
    bl      EXT(vm_bootstrap)

    /* Setup bootstrap interrupts */
    ADRL    x1, exception_vectors_el1
    msr     vbar_el1, x1
    msr     DAIFClr, #DAIF_ALL              /* enable all exceptions */

    b      _primary_core_boot


_primary_core_boot:
    /* Launch ourselves into C */
    mov     x0, x20                         /* __kernel_map_start PA */
    add     x1, x19, #0x06000               /* Reserve static bootstrap region */
    bl      EXT(main)
    /* main should never return, panic if it does */
    ADRL    x0, Lstartup_returned
    b       EXT(panic_simple)

_secondary_core_prepare:
//TODO: Multi-core
1:
    wfe
    b 1b

.section ".rodata"
Lstartup_returned:
.string "main() must not return!"