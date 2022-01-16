#ifndef PLATFORM_REGISTERS_H
#define PLATFORM_REGISTERS_H
#include "machine/pmap/pmap_asm.h"

/* TCR */
#define TCR_T0SZ_SHIFT          (0)
#define TCR_IRGN0_SHIFT         (8)
#define TCR_ORGN0_SHIFT         (10)
#define TCR_SH0_SHIFT           (12)
#define TCR_TG0_SHIFT           (14)
#define TCR_T1SZ_SHIFT          (16)
#define TCR_A1_SHIFT            (22)
#define TCR_IRGN1_SHIFT         (24)
#define TCR_ORGN1_SHIFT         (26)
#define TCR_SH1_SHIFT           (28)
#define TCR_TG1_SHIFT           (30)
#define TCR_IPS_SHIFT           (32)

#define TCR_T0SZ_VALUE          (25)
#define TCR_T0SZ_CONFIG         (TCR_T0SZ_VALUE << TCR_T0SZ_SHIFT)    /* 64-27 bits of PA space */
#define TCR_IRGN0_CONFIG        (0b01 << TCR_IRGN0_SHIFT) /* write back, read/write allocate */
#define TCR_ORGN0_CONFIG        (0b01 << TCR_ORGN0_SHIFT) /* write back, read/write allocate */
#define TCR_SH0_CONFIG          (0b10 << TCR_SH0_SHIFT)   /* outer shareable */
#define TCR_TG0_CONFIG          (0b00 << TCR_TG0_SHIFT)   /* 4KB */
#define TCR_T1SZ_VALUE          (25)
#define TCR_T1SZ_CONFIG         (TCR_T1SZ_VALUE << TCR_T1SZ_SHIFT)    /* 64-27 bits of PA space */
#define TCR_A1_CONFIG           (0 << TCR_A1_SHIFT)       /* TTBR0 defines the ASID */
#define TCR_IRGN1_CONFIG        (0b01 << TCR_IRGN1_SHIFT) /* write back, read/write allocate */
#define TCR_ORGN1_CONFIG        (0b01 << TCR_ORGN1_SHIFT) /* write back, read/write allocate */
#define TCR_SH1_CONFIG          (0b10 << TCR_SH1_SHIFT)   /* outer sharable */
#define TCR_TG1_CONFIG          (0b10 << TCR_TG1_SHIFT)   /* 4KB - note: different encoding than TG0 */
#define TCR_IPS_CONFIG          (0b010 << TCR_IPS_SHIFT)  /* 40 bits of PA, 1TB of RAM */


#define TCR_EL1_CONFIG  ( \
    TCR_T0SZ_CONFIG | TCR_IRGN0_CONFIG | TCR_ORGN0_CONFIG | TCR_SH0_CONFIG \
    | TCR_TG0_CONFIG | TCR_T1SZ_CONFIG | TCR_A1_CONFIG | TCR_IRGN1_CONFIG \
    | TCR_ORGN1_CONFIG | TCR_SH1_CONFIG | TCR_TG1_CONFIG | TCR_IPS_CONFIG \
)

/* SCTLR */

/** MMU enabled */
#define SCTLR_M_SHIFT           (0)
#define SCTLR_M                 (1 << SCTLR_M_SHIFT)
/** Alignment check */
#define SCTLR_A_SHIFT           (1)
#define SCTLR_A                 (1 << SCTLR_A_SHIFT)
/** Data cache enable */
#define SCTLR_C_SHIFT           (2)
#define SCTLR_C                 (1 << SCTLR_C_SHIFT)
/** EL1 stack align check */
#define SCTLR_SA_SHIFT          (3)
#define SCTLR_SA                (1 << SCTLR_SA_SHIFT)
/** EL0 stack align check */
#define SCTLR_SA0_SHIFT         (4)
#define SCTLR_SA0               (1 << SCTLR_SA0_SHIFT)
/** Allow EL0 access to PSTATE.{DAIF} */
#define SCTLR_UMA_SHIFT         (9)
#define SCTLR_UMA               (1 << SCTLR_UMA_SHIFT)
/** Instruction cache enable */
#define SCTLR_I_SHIFT           (12)
#define SCTLR_I                 (1 << SCTLR_I_SHIFT)
/** W^X, write implies execute never */
#define SCTLR_WXN_SHIFT         (19)
#define SCTLR_WXN               (1 << SCTLR_WXN_SHIFT)

#define SCTLR_EL1_BOOTSTRAP_CONFIG (\
    SCTLR_C | SCTLR_SA | SCTLR_SA0 | SCTLR_I \
)

#define SCTLR_EL1_CONFIG (\
    SCTLR_C | SCTLR_SA | SCTLR_SA0 | SCTLR_I | SCTLR_M | SCTLR_WXN \
)

/* MAIR */
#define MAIR_ATTR_WIDTH         (8)
#define MAIR_ATTR_N_SHIFT(n)    (1 << (MAIR_ATTR_WIDTH * (n)))
#define MAIR_ATTR_N_MASK(n)     (0xFF << MAIR_ATTR_N_SHIFT(n))

/*
G/nG = (non)-gathering: can multiple accesses to the same/nearby locations be 
        merged?
R/nR = (non)-reordering: can memory accesses be re-ordered internally?
E/nE = (no)-early-write-ack: can memory writes be ack'd before they actually 
        reach the target device?
*/
/** non-gathering, non-reorderable, no-early ack */
#define MAIR_DD_nGnRnE          (0b00)
/** non-gathering, non-reorderable, early ack */
#define MAIR_DD_nGnRE           (0b01)
/** non-gathering, reorderable, early ack */
#define MAIR_DD_nGRE            (0b10)
/** gathering, reorderable, early ack */
#define MAIR_DD_GRE             (0b11)
#define MAIR_ATTR_DEVICE(dd)    ((dd) << 2)

/* r/w allocate is used to modify the outer/inner attributes and determines 
behavior on cache miss (i.e. should the item be inserted if it does not exist)
*/
#define MAIR_CACHE_READ_ALLOCATE (0b10)
#define MAIR_CACHE_WRITE_ALLOCATE (0b01)
#define MAIR_CACHE_NO_ALLOCATE  (0)
/** Cache write-through, transient */
#define MAIR_CACHE_WT_T(r, w)    (r | w | 0b00 << 2)
/** Cache write-back, transient */
#define MAIR_CACHE_WB_T(r, w)    (r | w | 0b01 << 2)
/** Caching is not allowed */
#define MAIR_CACHE_NON_CACHEABLE (0b0100)
/** Cache write-through, non-transient */
#define MAIR_CACHE_WT_NT(r, w)   (r | w | 0b10 << 2)
/** Cache write-back, non-transient */
#define MAIR_CACHE_WB_NT(r, w)   (r | w | 0b11 << 2)
#define MAIR_ATTR_NORMAL(oooo, iiii) ((oooo) << 4 | iiii)

#define MAIR_EL1_CONFIG         (\
    MAIR_ATTR_NORMAL( \
        MAIR_CACHE_WB_NT(MAIR_CACHE_READ_ALLOCATE, MAIR_CACHE_WRITE_ALLOCATE), \
        MAIR_CACHE_WB_NT(MAIR_CACHE_READ_ALLOCATE, MAIR_CACHE_WRITE_ALLOCATE) \
    ) << MAIR_ATTR_N_SHIFT(MAIR_IDX_NORMAL) \
    | MAIR_ATTR_DEVICE(MAIR_DD_nGnRnE) << MAIR_ATTR_N_SHIFT(MAIR_IDX_DEVICE) \
)

/* CPACR */
#define CPACR_FPEN_SHIFT        (20)
#define CPACR_FPEN_MASK         (0b11 << CPACR_FPEN_SHIFT)
/** Do not trap any FP/NEON instructions */
#define CPACR_FPEN_ALL_TRAP      (0b00 << CPACR_FPEN_SHIFT)
#define CPACR_FPEN_EL0_TRAP      (0b01 << CPACR_FPEN_SHIFT)
#define CPACR_FPEN_NO_TRAP      (0b11 << CPACR_FPEN_SHIFT)

#define CPACR_EL1_CONFIG        (\
    CPACR_FPEN_NO_TRAP \
)

#endif /* PLATFORM_REGISTERS_H */