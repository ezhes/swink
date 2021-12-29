/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */
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
    adr     x2, Lstart_in_el2
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
    mov     x0, #(1 << 31)      // AArch64
    orr     x0, x0, #(1 << 1)   // SWIO hardwired on Pi3
    msr     hcr_el2, x0
    mrs     x0, hcr_el2
    /* return to EL1 */
    mov     x2, #0x3c4
    msr     spsr_el2, x2
    adr     x2, Lstart_in_el1
    msr     elr_el2, x2
    eret

    
Lstart_in_el1:
    /* Enable NEON */
    mov     x0, #(0b11<<20)                /* FPEN_ENABLE */
    msr     cpacr_el1, x0
    // read cpu id, stop secondary cores
    mrs     x1, mpidr_el1
    and     x1, x1, #3
    cbnz    x1, _secondary_core_prepare

    //Setup interrupts
    adr     x1, exception_vectors_el1
    msr     vbar_el1, x1
    msr     DAIFClr, #DAIF_ALL           /* enable all exceptions */

    /* Setup primary stack in SPSel 0 just above the code segment */
    adr     x1, _start
    mov     sp, x1
    /* Prepare exception stack */
    msr     SPSel, #1
    sub     x1, x1, 1024
    mov     sp, x1
    msr     SPSel, #0

    // clear bss
    ldr     x1, =__bss_start
    ldr     w2, =__bss_size
3:  cbz     w2, 4f
    str     xzr, [x1], #8
    sub     w2, w2, #1
    cbnz    w2, 3b

    // jump to C code, should not return
4:  b      _primary_core_boot


_primary_core_boot:
    bl      main
    /* main should never return, panic if it does */
    adr     x0, Lstartup_returned
    b       panic_simple

_secondary_core_prepare:
1:
    wfe
    b 1b

.section ".rodata"
Lstartup_returned:
.string "main() must not return!"