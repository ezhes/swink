#include "pmap.h"
#include "pmap_internal.h"
#include "lib/assert.h"
#include "lib/ctype.h"
#include "core/vm/vm.h"
#include "core/vm/vm_page_allocator.h"
#include "pmap_asm.h"
#include "machine/platform_registers.h"
#include "machine/io/gpio.h"
#include "pmap_pfa.h"
#include "lib/stdio.h"
#include "lib/ctype.h"
#include "lib/string.h"

/*
pmap_init.c // January 15, 2022
These routines are responsible for performing VM bootstrap phase 2. This entails
creating the final kernel pmap, initializing the kernel physical memory manager,
and activating the new kernel map.
*/

/**
 * Converts a virtual address to its component translation parts.
 * The page table indices are placed in lx_i and the TTBRn value is returned
 * For example, l3_i will contain the index on the L3 table which contains
 * the translation for this VA.
 */
static inline unsigned int
va_to_phys_indexes(vm_addr_t va,
                    unsigned int *l1_i, unsigned int *l2_i, unsigned *l3_i) {
    /* 
    VA Anatomy:
    [63:63 - TCR_SZn]   TTBR select
    [63 - TCR_SZn:30]   L1 index
    [30:21]             L2 index
    [21:12]             L3 index
    [12:0]              Page offset
    */
    *l1_i = (va >> 30) & ((1LLU << ((64 - TCR_T1SZ_VALUE) - 30)) - 1);
    *l2_i = (va >> 21) & 0x1FF;
    *l3_i = (va >> 12) & 0x1FF;

    /* Get the TTBR select, avoid the top byte for TBI */
    return (va >> (64 - 8)) & 0x1;
}

/** Extract the output address from a PTE */
static inline phys_addr_t
pte_to_phys_addr(uint64_t pte) {
    if (!(pte & PTE_VALID)) {
        return PHYS_ADDR_INVALID;
    }

    return (pte & OUTPUT_ADDRESS_MASK) >> OUTPUT_ADDRESS_SHIFT;
}

/**
 * Allocate a new page (filled with `fill` bytes) using the allocation_ptr arena 
 * allocator. Returns the address of the allocated page
 */
static inline phys_addr_t allocation_ptr_page_alloc(uint64_t *allocation_ptr,
                                                    uint8_t fill) {
    phys_addr_t result = *allocation_ptr;
    *allocation_ptr += PAGE_SIZE;
    memset((void *)result, fill, PAGE_SIZE);
    return result;
}

/**
 * Looks for a table entry in `table_base` at index `table_i`. If one exists,
 * returns the physical address of the next table. If it does not exist,
 * allocates a new table from the `allocation_ptr` arena and installs it into
 * `table_base` at `table_i` using the provided `pte_template`. Returns the
 * physical address of the newly created table.
 */
static inline phys_addr_t table_lookup_allocating(phys_addr_t table_base, 
                                                    unsigned table_i,
                                                    uint64_t *allocation_ptr,
                                                    uint64_t pte_template) {
    /* Check for a pre-existing entry on table */
    phys_addr_t result = pte_to_phys_addr(((uint64_t *)table_base)[table_i]);
    if (result != PHYS_ADDR_INVALID) {
        /* Found one, return the lookup */
        return result;
    }
    
    /* Allocate the missing backing page, fill with invalid entries */
    result = allocation_ptr_page_alloc(allocation_ptr, 0x00);
    ((uint64_t *)table_base)[table_i] = pte_template 
                                            | OUTPUT_ADDRESS_TO_PTE(result);

    return result;
}

/**
 * Maps `page_count` pages starting from phys_base into `pmap` starting at the 
 * virtual address `vm_base`. Pages in this range will be mapped using 
 * `pte_template`. This function allocates all necessary intermediate tables
 * using the `allocation_ptr` arena.
 */
static void vm_init_map_contiguous(pmap_t pmap, uint64_t *allocation_ptr,
                            uint64_t pte_template, 
                            vm_addr_t vm_base, phys_addr_t phys_base,
                            size_t page_count) {
    printf(
        "[*] vm_init_map_contiguous vm_base = 0x%llx, " 
        "phys_base = 0x%llx, page_count = %zu\n", 
        vm_base, phys_base, page_count
    );

    phys_addr_t l1, l2, l3;
    unsigned int l1_i, l2_i, l3_i;
    l2 = l3 = PHYS_ADDR_INVALID;
    l1 = pmap->table_base;

    /* Init the Lx indexes for translating starting at vm_base */
    va_to_phys_indexes(vm_base, &l1_i, &l2_i, &l3_i);

    /* Map all the pages in range */
    for (size_t page_i = 0; page_i < page_count; page_i++) {
        if (l2 == PHYS_ADDR_INVALID) {
            /* Lookup the L2, allocating if needed */
            l2 = table_lookup_allocating(l1, l1_i,
                                        allocation_ptr,
                                        PTE_TEMPLATE_TABLE_KERN_ONLY);
        }

        if (l3 == PHYS_ADDR_INVALID) {
            l3 = table_lookup_allocating(l2, l2_i,
                                        allocation_ptr,
                                        PTE_TEMPLATE_TABLE_KERN_ONLY);
        }

        /* We now have an L1->L2->L3 table chain, write in the desired entry */
        ASSERT((((uint64_t *)l3)[l3_i] & PTE_VALID) == PTE_INVALID);
        ((uint64_t *)l3)[l3_i] = pte_template 
            | OUTPUT_ADDRESS_TO_PTE(phys_base + page_i * PAGE_SIZE);
        
        /* Advance lx_i, invalidating lx values as needed */
        l3_i++;
        if (l3_i >= PAGE_ENTRY_COUNT) {
            l3_i = 0;
            l3 = PHYS_ADDR_INVALID;
            l2_i++;
            if (l2_i >= PAGE_ENTRY_COUNT) {
                l2_i = 0;
                l2 = PHYS_ADDR_INVALID;
                l1_i++;
                if (l1_i >= PAGE_ENTRY_COUNT 
                    && page_i + 1 < page_count /* will the loop continue? */) {
                    panic("l1 idx rolled over with future mapping operations?");
                }
            }
        }

    }
}

/* Linker symbols, defined in link.ld */
extern char __kernel_map_start;
extern char __kernel_text_start;
extern char __kernel_text_end;
extern char __kernel_ro_data_start;
extern char __kernel_ro_data_end;
extern char __kernel_rw_data_start;
extern char __kernel_rw_data_end;

/** Switch the kernel from the bootstrap tables onto the pmap_kernel pmap */
extern void vm_bootstrap_switch_to_pmap_kernel(phys_addr_t page_table_base);

#define KERNEL_SECTION_PA_BASE_OFFSET(s)   \
    ((phys_addr_t)(&(s ## _start) - &__kernel_map_start))
#define KERNEL_SECTION_PA_BASE(s)   \
    (kernel_base + KERNEL_SECTION_PA_BASE_OFFSET(s))
#define KERNEL_SECTION_VA_BASE(s)   \
    ((vm_addr_t)(VM_KERNEL_BASE_ADDRESS + KERNEL_SECTION_PA_BASE_OFFSET(s)))
#define KERNEL_SECTION_SIZE(s) \
    (&(s ## _end) - &(s ## _start))
#define KERNEL_SECTION_PAGE_COUNT(s)    \
    (KERNEL_SECTION_SIZE(s) >> PAGE_SHIFT)

void pmap_vm_init(phys_addr_t kernel_base,
                  phys_addr_t ram_base, phys_addr_t ram_size,
                  phys_addr_t bootstrap_pa_reserved) {
    phys_addr_t allocation_ptr = bootstrap_pa_reserved;
    
    /* Init the kernel pmap */
    synchs_lock_init(&pmap_kernel->lock);
    pmap_kernel->table_base = allocation_ptr_page_alloc(&allocation_ptr, 0x00);

    /* Map the kernel regions */
    // text
    vm_init_map_contiguous(pmap_kernel, &allocation_ptr,
        PTE_TEMPLATE_PAGE_NORMAL_KERN_RX, /* PTE template */
        KERNEL_SECTION_VA_BASE(__kernel_text),
        KERNEL_SECTION_PA_BASE(__kernel_text),
        KERNEL_SECTION_PAGE_COUNT(__kernel_text)
    );

    // rodata
    vm_init_map_contiguous(pmap_kernel, &allocation_ptr,
        PTE_TEMPLATE_PAGE_NORMAL_KERN_RO, /* PTE template */
        KERNEL_SECTION_VA_BASE(__kernel_ro_data),
        KERNEL_SECTION_PA_BASE(__kernel_ro_data),
        KERNEL_SECTION_PAGE_COUNT(__kernel_ro_data)
    );

    // rwdata
    vm_init_map_contiguous(pmap_kernel, &allocation_ptr,
        PTE_TEMPLATE_PAGE_NORMAL_KERN_RW, /* PTE template */
        KERNEL_SECTION_VA_BASE(__kernel_rw_data),
        KERNEL_SECTION_PA_BASE(__kernel_rw_data),
        KERNEL_SECTION_PAGE_COUNT(__kernel_rw_data)
    );


    /* 
    Init the fine grained kernel physmap

    We want the physmap to be L1 aligned so that it never intermixes with other
    kernel memory (helpful for applying policy decisions, etc.)
    */
    size_t physmap_size = MAX(ram_size, MMIO_END);
    phys_addr_t physmap_l1_cnt = 
        ROUND_UP(physmap_size, VM_L1_ENTRY_SIZE) / VM_L1_ENTRY_SIZE;
    
    /* We place the physmap at the end of the KVA space */
    physmap_vm_base = VM_KERNEL_BASE_ADDRESS +  
        VM_L1_ENTRY_SIZE * (PAGE_ENTRY_COUNT - physmap_l1_cnt);
    
    // RAM physmap
    vm_init_map_contiguous(pmap_kernel, &allocation_ptr,
        PTE_TEMPLATE_PAGE_NORMAL_KERN_RW, /* PTE template */
        physmap_vm_base + 0x00,
        0x00 /* phys_base */, 
        ROUND_UP(ram_size, PAGE_SIZE) >> PAGE_SHIFT /* page count */
    );

    // MMIO physmap
    vm_init_map_contiguous(pmap_kernel, &allocation_ptr,
        PTE_TEMPLATE_PAGE_DEVICE_KERN_RW, /* PTE template */
        physmap_vm_base + MMIO_BASE,
        MMIO_BASE /* phys_base */, 
        ROUND_UP(MMIO_END - MMIO_BASE, PAGE_SIZE) >> PAGE_SHIFT /* page count */
    );


    printf("[*] pmap_init: Created kernel pmap (used %llu pages)\n", 
        (allocation_ptr - bootstrap_pa_reserved) >> PAGE_SHIFT
    );

    /* Activate the kernel pmap before going forwards to provide a stable KVA */
    vm_bootstrap_switch_to_pmap_kernel(pmap_kernel->table_base);
    printf("[*] Switched to kernel pmap!\n");
    
    /*
    Init the page frame allocator. This is the last time we are allowed to use 
    the allocation pointer since the PFA has now taken ownership of physical
    memory and marked both the now extended bootstrap region as allocated.
    */
   pmap_pfa_init(
        ram_base, 
        ram_size,
        KERNEL_SECTION_PA_BASE(__kernel_text),
        KERNEL_SECTION_SIZE(__kernel_text),
        KERNEL_SECTION_PA_BASE(__kernel_ro_data),
        KERNEL_SECTION_SIZE(__kernel_ro_data) 
            + KERNEL_SECTION_SIZE(__kernel_rw_data),
        allocation_ptr
   );
}
