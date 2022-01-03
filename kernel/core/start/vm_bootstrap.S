#include "lib/asm_utils.h"
#include "machine/pmap/pmap_asm.h"
#include "machine/platform_registers.h"

/** Fills \page_base with \count (which is a multiple of 16) bytes of zeroes */
.macro ZERO_PAGE page_base temp count
    add     \temp, \page_base, \count
    movi    v0.2d, #0
0:
    cmp     \page_base, \temp
    beq     1f
    stp     q0, q0, [\temp, -32]!
    b       0b
1:
.endmacro

/*
L3 = 512 * 1<<12 = 2MB
L2 = 512 * 512 * 1<<12 = 1GB
L1 = 512GB


L1 (TTBR0) P=V
	block entries @ 1GB/ea, map everything as device memory

L1 (TTBR1) KVA Bootstrap
0	L2
	0	L3
		0	Kernel, BSS, etc (2MB). We only use one table since the kernel is 
            small enough to fit on one table (at least for the moment)

Allocations (sync with start.S):
[0x7d000, 0x7e000) : P=V L1
[0x7c000, 0x7d000) : KVA L1
[0x7b000, 0x7c000) : KVA L2
[0x7a000, 0x7b000) : KVA L3 
*/
.section ".text"
.global EXT(vm_bootstrap)
EXT(vm_bootstrap):
    /* physical allocation ptr in x0 */
    /* Allocate the P=V table into x1 */
    mov     x1, x0
    sub     x0, x0, PAGE_SIZE
    /* 
    Allocate the first four slots of VA [0x0, 0x100000000] (4GB phys) 
    We don't actually both doing this mapping ""correctly"" and only covering
    the expected amount of physical memory (i.e. some physical memory may be
    invalid) but since the minimum amount of PA space is 32 on ARM, this is OK
    We leave the real physmap init to C since this table will be thrown away
    after bootstrap is complete
    */
    /* 
    Zero the rest of the table to mark invalid 
    x1 P=V base
    x2 P=V iterator 
    */
    ZERO_PAGE x1, x2, PAGE_SIZE

    /*
    x1 P=V phys base
    x2 bootstrap PTE template 1
    x3 bootstrap PTE template 2 (x2 + 1GB PA)
    x4 1GB constant
    */
    mov     x4, OUTPUT_ADDRESS_TO_PTE(0x40000000)
    MOVC    x2, PTE_TEMPLATE_BLOCK_DEVICE_BOOTSTRAP
    add     x3, x2, x4
    stp     x2, x3, [x1, #0]
    /* Advance both x2, x3 to get gigs 3 and 4  */
    add     x2, x3, x4
    add     x3, x2, x4
    stp     x2, x3, [x1, #16]
    /* Load P=V into TTBR0 but don't activate it yet */
    orr     x1, x1, #1 /* CNP */
    msr     TTBR0_EL1, x1

    /* Zero the KVA L1=x1, L2=x2, and L3=x3 tables */
    mov     x1, x0
    sub     x2, x1, PAGE_SIZE
    sub     x3, x2, PAGE_SIZE
    sub     x0, x0, PAGE_SIZE * 3
    ZERO_PAGE x3, x4, PAGE_SIZE * 3         /* zero FROM x3 since it's lower */

    msr     TTBR1_EL1, x1
    
    /* attempt to init the VM */
    b vm_init



/**
Initializes the MMU with constant configuration and enables it
Assumes:
1. TTBR0/1 are already loaded
2. LR is a valid address after the MMU is enabled (either on KVA or P=V) space
*/
vm_init:
    MOVC    x1, TCR_EL1_CONFIG
    msr     tcr_el1, x1
    MOVC    x1, MAIR_EL1_CONFIG
    /* Clear caches before enabling because they may have garbage in them */
    msr     mair_el1, x1
    tlbi    vmalle1
    dsb     ish
    isb
    /* enable the MMU! */
    mrs     x0, sctlr_el1
    orr     x0, x0, SCTLR_M
    msr     sctlr_el1, x0
    isb
    ret