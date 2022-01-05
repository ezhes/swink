#ifndef PMAP_ASM_H
#define PMAP_ASM_H

#include "core/vm/vm_asm.h"

/** 
 * Common-not-private: allow page table caching to be shared across PEs 
 * CPUs that do not support CNP simply ignore this bit.
 */
#define TTBR_CNP_SHIFT          (0)
#define TTBR_CNP                (1 << TTBR_CNP_SHIFT)

/* Table PTE constants */
#define NS_TABLE_SHIFT          (63)
#define NS_TABLE                (1 << NS_TABLE_SHIFT)
#define AP_TABLE_SHIFT          (61)
#define AP_TABLE_MASK           (0b11 << AP_TABLE_SHIFT)
#define UXN_TABLE_SHIFT         (60)
#define UXN_TABLE               (1 << UXN_TABLE_SHIFT)
#define PXN_TABLE_SHIFT         (59)
#define PXN_TABLE               (1 << PXN_TABLE_SHIFT)

/* Block PTE constants */
#define SOFTWARE_BLOCK_SHIFT    (55)
#define SOFTWARE_BLOCK_MASK     (0b1111 << SOFTWARE_BLOCK_SHIFT)
#define UXN_BLOCK_SHIFT         (54)
#define UXN_BLOCK               (1 << UXN_BLOCK_SHIFT)
#define PXN_BLOCK_SHIFT         (53)
#define PXN_BLOCK               (1 << PXN_BLOCK_SHIFT)
#define NOT_GLOBAL_BLOCK_SHIFT  (11)
#define NOT_GLOBAL_BLOCK        (1 << NOT_GLOBAL_BLOCK_SHIFT)
#define ACCESS_FLAG_BLOCK_SHIFT (10)
#define ACCESS_FLAG_BLOCK       (1 << ACCESS_FLAG_BLOCK_SHIFT)
#define SH_BLOCK_SHIFT          (8)
#define SH_BLOCK_MASK           (0b11 << SH_BLOCK_SHIFT)
#define AP_BLOCK_SHIFT          (6)
#define AP_BLOCK_MASK           (0b11 << AP_BLOCK_SHIFT)
#define MAIR_BLOCK_SHIFT        (2)
#define MAIR_BLOCK_MASK         (0b111 << MAIR_BLOCK_SHIFT)

#define OUTPUT_ADDRESS_SHIFT    (PAGE_SHIFT)
#define OUTPUT_ADDRESS_MASK     (((1 << (47 - OUTPUT_ADDRESS_SHIFT)) - 1) << OUTPUT_ADDRESS_SHIFT)
#define OUTPUT_ADDRESS_TO_PTE(oa) ((oa & OUTPUT_ADDRESS_MASK) << OUTPUT_ADDRESS_SHIFT)

/* APTable constants */
#define AP_KERN_RW_USER_NA      (0b00)
#define AP_KERN_RW_USER_RW      (0b01)
#define AP_KERN_RO_USER_NA      (0b10)
#define AP_KERN_RO_USER_RO      (0b11)
#define AP_BLOCK_TO_PTE(ap)     ((ap) << AP_BLOCK_SHIFT)
#define AP_TABLE_TO_PTE(ap)     ((ap) << AP_TABLE_SHIFT)

/* Shareability constants */
#define SH_NON_SHAREABLE        (0b00)
#define SH_OUTER_SHAREABLE      (0b10)
#define SH_INNER_SHAREABLE      (0b11)
#define SH_TO_PTE(sh)           ((sh) << SH_BLOCK_SHIFT)

/* MAIR page table configuration */
/* 
Attributes are the indirection indexes
These indexes are defined arbitrarily and configured in platform_registers.h
*/
#define MAIR_IDX_NORMAL         (0)
#define MAIR_IDX_DEVICE         (1)
#define MAIR_IDX_TO_PTE(mair)   ((mair) << MAIR_BLOCK_SHIFT)

/* Templates */
/** Invalid block or template */
#define PTE_INVALID             (0)
#define PTE_VALID               (0b1)
#define PTE_TYPE_SHIFT          (1)
#define PTE_TYPE_MASK           (0b10)
#define PTE_TYPE_BLOCK          (0b00)
#define PTE_TYPE_TABLE          (0b10)
#define PTE_VALID_BLOCK         (PTE_TYPE_BLOCK | PTE_VALID)
#define PTE_VALID_TABLE         (PTE_TYPE_TABLE | PTE_VALID)

/** Template for a kernel rw device memory */
#define PTE_TEMPLATE_BLOCK_DEVICE_RW     ( \
    PTE_VALID_BLOCK | UXN_BLOCK | PXN_BLOCK | SH_TO_PTE(SH_NON_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RW_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_DEVICE) \
    | ACCESS_FLAG_BLOCK \
)

/** Template for kernel RO device memory */
#define PTE_TEMPLATE_BLOCK_DEVICE_RO     ( \
    PTE_VALID_BLOCK | UXN_BLOCK | PXN_BLOCK | SH_TO_PTE(SH_NON_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RO_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_DEVICE) \
    | ACCESS_FLAG_BLOCK \
)

/** Template for a kernel rwx device memory PTE (useful for bootstrap) */
#define PTE_TEMPLATE_BLOCK_DEVICE_BOOTSTRAP     ( \
    PTE_VALID_BLOCK | UXN_BLOCK | SH_TO_PTE(SH_NON_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RW_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_DEVICE) \
    | ACCESS_FLAG_BLOCK \
)

/** Template for regular kernel RW memory */
#define PTE_TEMPLATE_PAGE_NORMAL_KERN_RW     ( \
    PTE_VALID_TABLE | UXN_BLOCK | PXN_BLOCK | SH_TO_PTE(SH_OUTER_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RW_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_NORMAL) \
    | ACCESS_FLAG_BLOCK \
)

/** Template for regular kernel RO memory */
#define PTE_TEMPLATE_PAGE_NORMAL_KERN_RO     ( \
    PTE_VALID_TABLE | UXN_BLOCK | PXN_BLOCK | SH_TO_PTE(SH_OUTER_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RO_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_NORMAL) \
    | ACCESS_FLAG_BLOCK \
)

/** Template for regular kernel RX memory */
#define PTE_TEMPLATE_PAGE_NORMAL_KERN_RX     ( \
    PTE_VALID_TABLE | UXN_BLOCK | SH_TO_PTE(SH_OUTER_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RO_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_NORMAL) \
    | ACCESS_FLAG_BLOCK \
)

/** Template for kernel rwx normal memory (useful for bootstrap only) */
#define PTE_TEMPLATE_PAGE_NORMAL_KERN_BOOTSTRAP     ( \
    PTE_VALID_TABLE | UXN_BLOCK | SH_TO_PTE(SH_OUTER_SHAREABLE) \
    | AP_BLOCK_TO_PTE(AP_KERN_RW_USER_NA) | MAIR_IDX_TO_PTE(MAIR_IDX_NORMAL) \
    | ACCESS_FLAG_BLOCK \
)

/** 
 * Table pointer template for use on kernel tables only
 * Prevents user-execute or user-access on any downstream blocks
 */
#define PTE_TEMPLATE_TABLE_KERN_ONLY            ( \
    PTE_VALID_TABLE | UXN_TABLE | AP_TABLE_TO_PTE(AP_KERN_RW_USER_NA) \
    | ACCESS_FLAG_BLOCK\
)

/** 
 * Table pointer template for use on user tables only
 * Prevents kernel-execute on any downstream blocks
 */
#define PTE_TEMPLATE_TABLE_USER                 ( \
    PTE_VALID_TABLE | PXN_TABLE | AP_TABLE_TO_PTE(AP_KERN_RW_USER_RW) \
)

#endif /* PMAP_ASM_H */