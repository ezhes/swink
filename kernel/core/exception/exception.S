#include "lib/asm_utils.h"
#include "exception_asm.h"

/* Spill registers to a context in SP, assuming x0, x1, and sp have already been
 spilled.
 Returns with the pointer to the context in x0
 Preserves x1
 */
.macro REGISTER_SPILL_NO_X0X1SP
    stp     x2, x3, [sp, #ARM64_GP_2]
    stp     x4, x5, [sp, #ARM64_GP_4]
    stp     x6, x7, [sp, #ARM64_GP_6]
    stp     x8, x9, [sp, #ARM64_GP_8]
    stp     x10, x11, [sp, #ARM64_GP_10]
    stp     x12, x13, [sp, #ARM64_GP_12]
    stp     x14, x15, [sp, #ARM64_GP_14]
    stp     x16, x17, [sp, #ARM64_GP_16]
    stp     x18, x19, [sp, #ARM64_GP_18]
    stp     x20, x21, [sp, #ARM64_GP_20]
    stp     x22, x23, [sp, #ARM64_GP_22]
    stp     x24, x25, [sp, #ARM64_GP_24]
    stp     x26, x27, [sp, #ARM64_GP_26]
    stp     x28, x29, [sp, #ARM64_GP_28]
    str     lr, [sp, #ARM64_GP_LR]

    stp     q0, q1, [sp, #ARM64_NEON_Q_0]
    stp     q2, q3, [sp, #ARM64_NEON_Q_2]
    stp     q4, q5, [sp, #ARM64_NEON_Q_4]
    stp     q6, q7, [sp, #ARM64_NEON_Q_6]
    stp     q8, q9, [sp, #ARM64_NEON_Q_8]
    stp     q10, q11, [sp, #ARM64_NEON_Q_10]
    stp     q12, q13, [sp, #ARM64_NEON_Q_12]
    stp     q14, q15, [sp, #ARM64_NEON_Q_14]
    stp     q16, q17, [sp, #ARM64_NEON_Q_16]
    stp     q18, q19, [sp, #ARM64_NEON_Q_18]
    stp     q20, q21, [sp, #ARM64_NEON_Q_20]
    stp     q22, q23, [sp, #ARM64_NEON_Q_22]
    stp     q24, q25, [sp, #ARM64_NEON_Q_24]
    stp     q26, q27, [sp, #ARM64_NEON_Q_26]
    stp     q28, q29, [sp, #ARM64_NEON_Q_28]
    stp     q30, q31, [sp, #ARM64_NEON_Q_30]

    mrs     x0, esr_el1
    str     x0, [sp, #ARM64_ESR]
    mrs     x2, spsr_el1
    str     x2, [sp, #ARM64_CPSR]
    mrs     x3, far_el1
    str     x3, [sp, #ARM64_FAR]
    mrs     x4, elr_el1
    str     x4, [sp, #ARM64_PC]
    mrs		x5, fpsr
    str     w5, [sp, #ARM64_NEON_FPSR]
	mrs		x6, fpcr
    str     w6, [sp, #ARM64_NEON_FPCR]

    mov     x0, sp              // Put the context pointer into x0
.endmacro

/** Unspills from a context saved on the stack */
.macro REGISTER_UNSPILL
    /* disable exceptions so we don't have to worry about spill-unspill races */
    msr		DAIFSet, #DAIF_ALL_BUT_DEBUG
    /* don't unspill x0, x1 yet, we use them for temporaries */
    ldp     x2, x3, [sp, #ARM64_GP_2]
    ldp     x4, x5, [sp, #ARM64_GP_4]
    ldp     x6, x7, [sp, #ARM64_GP_6]
    ldp     x8, x9, [sp, #ARM64_GP_8]
    ldp     x10, x11, [sp, #ARM64_GP_10]
    ldp     x12, x13, [sp, #ARM64_GP_12]
    ldp     x14, x15, [sp, #ARM64_GP_14]
    ldp     x16, x17, [sp, #ARM64_GP_16]
    ldp     x18, x19, [sp, #ARM64_GP_18]
    ldp     x20, x21, [sp, #ARM64_GP_20]
    ldp     x22, x23, [sp, #ARM64_GP_22]
    ldp     x24, x25, [sp, #ARM64_GP_24]
    ldp     x26, x27, [sp, #ARM64_GP_26]
    ldp     x28, x29, [sp, #ARM64_GP_28]
    /* don't unspill lr, sp to avoid trashing kernel sp while in use */

    ldp     q0, q1, [sp, #ARM64_NEON_Q_0]
    ldp     q2, q3, [sp, #ARM64_NEON_Q_2]
    ldp     q4, q5, [sp, #ARM64_NEON_Q_4]
    ldp     q6, q7, [sp, #ARM64_NEON_Q_6]
    ldp     q8, q9, [sp, #ARM64_NEON_Q_8]
    ldp     q10, q11, [sp, #ARM64_NEON_Q_10]
    ldp     q12, q13, [sp, #ARM64_NEON_Q_12]
    ldp     q14, q15, [sp, #ARM64_NEON_Q_14]
    ldp     q16, q17, [sp, #ARM64_NEON_Q_16]
    ldp     q18, q19, [sp, #ARM64_NEON_Q_18]
    ldp     q20, q21, [sp, #ARM64_NEON_Q_20]
    ldp     q22, q23, [sp, #ARM64_NEON_Q_22]
    ldp     q24, q25, [sp, #ARM64_NEON_Q_24]
    ldp     q26, q27, [sp, #ARM64_NEON_Q_26]
    ldp     q28, q29, [sp, #ARM64_NEON_Q_28]
    ldp     q30, q31, [sp, #ARM64_NEON_Q_30]
    
    ldr     w0, [sp, #ARM64_NEON_FPSR]
    ldr     w1, [sp, #ARM64_NEON_FPSR]
    msr     fpsr, x0
    msr     fpsr, x1

    ldr     x0, [sp, #ARM64_CPSR]
    ldr     x1, [sp, #ARM64_PC]
    msr     spsr_el1, x0
    msr     elr_el1, x1

    /* get lr and x1=sp */
    ldp     lr, x1, [sp, #ARM64_GP_LR]
    /* stash sp in x0 and restore old sp */
    mov     x0, sp
    mov     sp, x1
    /* restore x0, x1 */
    ldp     x0, x1, [x0, #(ARM64_GP_0)]
.endmacro

.macro EL1_SP0_VECTOR target
    /* 
    EL1_SP0 means we took an exception while on the service stack 
    SP0 might hold an invalid stack
    Returns with context at sp

    TODO: Before adding lifeguard, check that SP0 is valid before spilling
    */
    .align	7
    Lel1_sp0_vector_\target:
    msr     SPSel, #1                   // switch to exception stack
    sub     sp, sp, #ARM64_CONTEXT_SIZE
    stp     x0, x1, [sp, #ARM64_GP_0]   // spill a bit so we can get SP_EL0
    mrs     x0, SP_EL0
    str     x0, [sp, #ARM64_GP_SP]
    adr     x1, \target                 // setup handler target
    b fleh_dispatch
.endmacro

.macro EL1_SP1_VECTOR target
    /*
    EL1_SP1 means we took an exception while on the exception stack
    SP1 might hold an invalid stack
    Returns with context at sp

    TODO: Before adding lifeguard, check that SP0 is valid before spilling
    */
    .align	7
    Lel1_sp1_vector_\target:
    sub     sp, sp, #ARM64_CONTEXT_SIZE
    stp     x0, x1, [sp, #ARM64_GP_0]   // spill a bit so we can get SP_ELx
    add     x0, sp, #ARM64_CONTEXT_SIZE // recover sp
    str     x0, [sp, #ARM64_GP_SP]
    adr     x1, \target                 // setup handler target
    b fleh_dispatch
.endmacro

.macro EL0_64_SP1_VECTOR target
    .align	7
    Lel0_sp1_vector_\target:
    sub     sp, sp, #ARM64_CONTEXT_SIZE
    stp     x0, x1, [sp, #ARM64_GP_0]   // spill a bit so we can get SP_ELx
    add     x0, sp, #ARM64_CONTEXT_SIZE // recover sp
    str     x0, [sp, #ARM64_GP_SP]
    adr     x1, \target                 // setup handler target
    b fleh_dispatch
.endmacro

.macro HANDLE_INVALID_ENTRY_32 type
    .align	7
    Lel0_sp1_32_invalid_vector_\type:
    mov     x1, #\type
    b       unhandled_exception_32
.endm

.section ".text"
.align	12
.globl exception_vectors_el1 
exception_vectors_el1:
    EL1_SP1_VECTOR      EXT(exception_sync)
    EL1_SP1_VECTOR      EXT(exception_irq)
    EL1_SP1_VECTOR      EXT(exception_fiq)
    EL1_SP1_VECTOR      EXT(exception_error)

    EL1_SP0_VECTOR      EXT(exception_sync)
    EL1_SP0_VECTOR      EXT(exception_irq)
    EL1_SP0_VECTOR      EXT(exception_fiq)
    EL1_SP0_VECTOR      EXT(exception_error)

    EL0_64_SP1_VECTOR   EXT(exception_sync)
    EL0_64_SP1_VECTOR   EXT(exception_irq)
    EL0_64_SP1_VECTOR   EXT(exception_fiq)
    EL0_64_SP1_VECTOR   EXT(exception_error)

    HANDLE_INVALID_ENTRY_32	SYNC_EL0_32
    HANDLE_INVALID_ENTRY_32	IRQ_EL0_32
    HANDLE_INVALID_ENTRY_32	FIQ_EL0_32
    HANDLE_INVALID_ENTRY_32	ERROR_EL0_32

fleh_dispatch:
    REGISTER_SPILL_NO_X0X1SP
    blr     x1

    /* fallthrough  to return */

.global EXT(exception_return)
EXT(exception_return):
    REGISTER_UNSPILL
Lfleh_dispatch_eret:
    /* see ya :) */
    eret


unhandled_exception_32:
    /* We don't support 32-bit user processes, so this should never happen */
    mrs     x2, esr_el1
    mrs     x3, elr_el1
    adr 	x0, Lunhandled_exception_32_panic_str
    b EXT(panic_formatted)

.section ".rodata"
Lunhandled_exception_32_panic_str:
.string "Unexpected EL0_32 exception??? (type=%d, ESR=%p, ELR=%p)"
