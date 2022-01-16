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
    block entries (16) 

Allocations (sync with start.S):
[0x02000, 0x03000) : P=V L1
[0x03000, 0x04000) : KVA L1
[0x04000, 0x05000) : KVA L2
[0x05000, 0x06000) : KVA L3 
*/
.section ".text"
.global EXT(vm_bootstrap)
EXT(vm_bootstrap):
    /* physical allocation ptr in x0 */
    /* Allocate the P=V table into x1 */
    mov     x1, x0
    add     x0, x0, PAGE_SIZE
    /* 
    Allocate the first four slots of VA [0x0, 0x100000000] (4GB phys) 
    We don't actually bother doing this mapping ""correctly"" and only cover
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
    orr     x1, x1, TTBR_CNP
    msr     TTBR0_EL1, x1

    /* 
    Construct the KVA bootstrap table
    x1 = L1 table
    x2 = L2 table
    x3 = L3 table
    x4 = kernel table entry template
    x5 = scratch
    */
    mov     x1, x0
    add     x2, x1, PAGE_SIZE
    add     x3, x2, PAGE_SIZE
    add     x0, x0, PAGE_SIZE * 3
    ZERO_PAGE x1, x5, PAGE_SIZE * 3         /* zero tables L1, L2, L3 */
    /* Setup L1->L2 link in first slot */
    MOVC    x4, PTE_TEMPLATE_TABLE_KERN_ONLY
    /* Since the PA is aligned, the encoding works such that we can add the oPA*/
    orr     x5, x4, x2
    str     x5, [x1]
    /* Setup the L2->L3 link the first slot -- L3 starts at 0xffffff8000000000 */
    orr     x5, x4, x3
    str     x5, [x2]
    /* 
    Now map the kernel into the L3 table
    For simplicity, we map as RWX and later lockdown to finer permissions on
    vm_finalize 
    x1 = L1 table
    x2 = L3 table entry write remaining
    x3 = L3 table
    x4 = kernel bootstrap PTE template
    x5 = Physical address map iterator (incremented by pages)
    x6 = L3 table iterator
    x7 = scratch
    */
    /* we use adrp instead of ADRL since these are always page aligned */
    adrp    x5, __kernel_map_start
    adrp    x2, __kernel_map_end
    /* number of entries required */
    sub     x2, x2, x5
    lsr     x2, x2, PAGE_SHIFT              /* no rounding issues, page aligned */
    MOVC    x4, PTE_TEMPLATE_PAGE_NORMAL_KERN_BOOTSTRAP
    mov     x6, x3

    /* check that we have enough space to map the kernel */
    cmp     x2, #PAGE_ENTRY_COUNT
    b.lo    0f
    /* Kernel is too large for one L3 page, need to update code */
    ADRL    x0, Lmap_failed_too_large_str
    b       EXT(panic_simple)
    0:

Lvm_bootstrap_map_loop_again:
    /* have we mapped all entries? */
    cbz     x2, Lvm_bootstrap_map_loop_done
    /* map new entry, create entry in x7 = template | phys_addr_i */
    orr     x7, x4, x5
    /* store to l3 iterator i and iterator ptr */
    str     x7, [x6], #8

    add     x5, x5, #PAGE_SIZE              /* phys_addr_i += PAGE_SIZE */
    sub     x2, x2, #1                      /* l3_write_remainig-- */
    b       Lvm_bootstrap_map_loop_again

Lvm_bootstrap_map_loop_done:
    /* Load the KVA table into TTBR1 but don't activate it */
    orr     x1, x1, TTBR_CNP
    msr     ttbr1_el1, x1
    
    /* Configure SCTLR for bootstrap before going into the shared vm_init */
    MOVC    x1, SCTLR_EL1_BOOTSTRAP_CONFIG
    msr     sctlr_el1, x1

    /* attempt to init the VM */
    b vm_init



/**
Initializes the MMU with constant configuration and enables it
Returns position independent slide in x0
Assumes:
1. TTBR0/1 are already loaded
2. LR is a valid address physical address
3. SCTLR is alredy configured and valid
*/
vm_init:
    /* 
    Calculate the physical->VM slide
    slide = new_base - phys_base
    Assumes new_base > phys_base (which is always true given a higher-half kernel)
     */
    MOVC    x0, VM_KERNEL_BASE_ADDRESS
    adr     x1, _start
    sub     x0, x0, x1
    /* Relocate return address */
    add     lr, lr, x0
    /* Prepare to enable VM */
    MOVC    x1, TCR_EL1_CONFIG
    msr     tcr_el1, x1
    MOVC    x1, MAIR_EL1_CONFIG
    /* Clear caches before enabling because they may have garbage in them */
    msr     mair_el1, x1
    tlbi    vmalle1
    dsb     ish
    isb
    /* enable the MMU! */
    mrs     x1, sctlr_el1
    orr     x1, x1, SCTLR_M
    msr     sctlr_el1, x1
    isb

    /* Return to LR in KVA map */
    ret

/** 
Switches kernel page table to x0 and sets SCTLR to the booted configuration
*/
.global EXT(vm_bootstrap_switch_to_pmap_kernel)
EXT(vm_bootstrap_switch_to_pmap_kernel):
    dsb     ish                         /* wait for memory to sync */
    orr     x0, x0, TTBR_CNP
    msr     ttbr1_el1, x0               /* set new page table */
    tlbi    vmalle1
    dsb     ish                         /* wait for tlb purge */
    isb                                 /* wait for final activation */
    /* we are now on pmap_kernel -- set booted SCTLR config */
    MOVC    x1, SCTLR_EL1_CONFIG
    msr     sctlr_el1, x1
    isb                                 /* wait for config to apply */

    ret



.section ".rodata"
Lmap_failed_too_large_str:
.string "Kernel too large (>2MB) for single 4K L3 table"