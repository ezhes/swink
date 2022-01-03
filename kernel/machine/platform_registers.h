#ifndef PLATFORM_REGISTERS_H
#define PLATFORM_REGISTERS_H
#include "machine/pmap/pmap_asm.h"

#define BIT(shift)              (1 << (shift))

#define TCR_T0SZ_SHIFT          (0)
#define TCR_IRGN0_SHIFT         (8)
#define TCR_ORGN0_SHIFT         (10)
#define TCR_SH0_SHIFT           (12)
#define TCR_TG0_SHIFT           (14)
#define TCR_IPS_SHIFT           (32)

#define TCR_T0SZ_CONFIG         (27 << TCR_T0SZ_SHIFT)    /* 2^{64-27} bits of PA space */
#define TCR_IRGN0_CONFIG        (0b01 << TCR_IRGN0_SHIFT) /* write back, read/write allocate */
#define TCR_ORGN0_CONFIG        (0b01 << TCR_ORGN0_SHIFT) /* write back, read/write allocate */
#define TCR_SH0_CONFIG          (0b10 << TCR_SH0_SHIFT)   /* outer shareable */
#define TCR_TG0_CONFIG          (0b00 << TCR_TG0_SHIFT)   /* 4KB */
#define TCR_IPS_CONFIG          (0b010 << TCR_IPS_SHIFT)  /* 40 bits of PA, 1TB of RAM */

#define TCR_EL1_CONFIG  ( \
    TCR_T0SZ_CONFIG | TCR_IRGN0_CONFIG | TCR_ORGN0_CONFIG | TCR_SH0_CONFIG \
    | TCR_TG0_CONFIG |TCR_IPS_CONFIG \
)

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
/* Instruction cache enable */
#define SCTLR_I_SHIFT           (12)
#define SCTLR_I                 (1 << SCTLR_I_SHIFT)
/** W^X, write implies execute never */
#define SCTLR_WXN_SHIFT         (19)
#define SCTLR_WXN               (1 << SCTLR_WXN_SHIFT)

#define SCTLR_EL1_BOOTSTRAP_CONFIG (\
    SCTLR_C | SCTLR_SA | SCTLR_SA0 | SCTLR_I \
)

#define SCTLR_EL1_CONFIG (\
    SCTLR_C | SCTLR_SA | SCTLR_SA0 | SCTLR_I | SCTLR_WXN \
)


#undef BIT

#endif /* PLATFORM_REGISTERS_H */