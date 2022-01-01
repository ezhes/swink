#ifndef LIB_ASM_UTILS
#define LIB_ASM_UTILS

 .macro ADRL reg, label
    adrp        \reg, \label
    add         \reg, \reg, :lo12:\label
 .endmacro

.macro	MOVC, reg, val
.if (((\val) >> 31) == 0 || ((\val) >> 31) == 0x1ffffffff)
    movz        \reg, :abs_g1_s:\val
.else
.if (((\val) >> 47) == 0 || ((\val) >> 47) == 0x1ffff)
    movz        \reg, :abs_g2_s:\val
.else
    movz        \reg, :abs_g3:\val
    movk        \reg, :abs_g2_nc:\val
.endif
    movk        \reg, :abs_g1_nc:\val
.endif
    movk        \reg, :abs_g0_nc:\val
.endmacro

/** Some compiler targets place an underscore before symbols but ours does not*/
#define EXT(label)  label

#define DAIF_DEBUG          (1 << 3)
#define DAIF_ASYNC          (1 << 2)
#define DAIF_IRQ            (1 << 1)
#define DAIF_FIQ            (1 << 0)
#define DAIF_ALL            (DAIF_DEBUG | DAIF_ASYNC | DAIF_IRQ | DAIF_FIQ)
#define DAIF_ALL_BUT_DEBUG  (DAIF_ASYNC | DAIF_IRQ | DAIF_FIQ)


#endif /* LIB_ASM_UTILS */