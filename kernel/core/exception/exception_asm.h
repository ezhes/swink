#ifndef EXCEPTION_ASM_H
#define EXCEPTION_ASM_H

#define SYNC_EL1_SP1            0 
#define IRQ_EL1_SP1             1 
#define FIQ_EL1_SP1             2 
#define ERROR_EL1_SP1           3 

#define SYNC_EL1_SP0            4 
#define IRQ_EL1_SP0             5 
#define FIQ_EL1_SP0             6 
#define ERROR_EL1_SP0           7 

#define SYNC_EL0_64             8 
#define IRQ_EL0_64              9 
#define FIQ_EL0_64              10 
#define ERROR_EL0_64            11 

#define SYNC_EL0_32             12 
#define IRQ_EL0_32              13 
#define FIQ_EL0_32              14 
#define ERROR_EL0_32            15

#define ARM64_CONTEXT_SIZE ((33 + 64 + 3) * 8 + (2) * 4)

#define ARM64_GP_0  (8 * 0)
#define ARM64_GP_2  (8 * 2)
#define ARM64_GP_4  (8 * 4)
#define ARM64_GP_6  (8 * 6)
#define ARM64_GP_8  (8 * 8)
#define ARM64_GP_10 (8 * 10)
#define ARM64_GP_12 (8 * 12)
#define ARM64_GP_14 (8 * 14)
#define ARM64_GP_16 (8 * 16)
#define ARM64_GP_18 (8 * 18)
#define ARM64_GP_20 (8 * 20)
#define ARM64_GP_22 (8 * 22)
#define ARM64_GP_24 (8 * 24)
#define ARM64_GP_26 (8 * 26)
#define ARM64_GP_28 (8 * 28)
#define ARM64_GP_LR (8 * 30)
#define ARM64_GP_SP (8 * 31)
#define ARM64_GP_END (ARM64_GP_SP + 8)

#define ARM64_NEON_Q_0   (ARM64_GP_END + 16 * 0)
#define ARM64_NEON_Q_2   (ARM64_GP_END + 16 * 2)
#define ARM64_NEON_Q_4   (ARM64_GP_END + 16 * 4)
#define ARM64_NEON_Q_6   (ARM64_GP_END + 16 * 6)
#define ARM64_NEON_Q_8   (ARM64_GP_END + 16 * 8)
#define ARM64_NEON_Q_10  (ARM64_GP_END + 16 * 10)
#define ARM64_NEON_Q_12  (ARM64_GP_END + 16 * 12)
#define ARM64_NEON_Q_14  (ARM64_GP_END + 16 * 14)
#define ARM64_NEON_Q_16  (ARM64_GP_END + 16 * 16)
#define ARM64_NEON_Q_18  (ARM64_GP_END + 16 * 18)
#define ARM64_NEON_Q_20  (ARM64_GP_END + 16 * 20)
#define ARM64_NEON_Q_22  (ARM64_GP_END + 16 * 22)
#define ARM64_NEON_Q_24  (ARM64_GP_END + 16 * 24)
#define ARM64_NEON_Q_26  (ARM64_GP_END + 16 * 26)
#define ARM64_NEON_Q_28  (ARM64_GP_END + 16 * 28)
#define ARM64_NEON_Q_30  (ARM64_GP_END + 16 * 30)
#define ARM64_NEON_Q_END (ARM64_NEON_Q_30 + 16 * 2)
#define ARM64_NEON_FPSR  (ARM64_NEON_Q_END + 4 * 0)
#define ARM64_NEON_FPCR  (ARM64_NEON_Q_END + 4 * 1)
#define ARM64_NEON_END (ARM64_NEON_FPCR + 4)

#define ARM64_ESR  (ARM64_NEON_END + 8 * 0)
#define ARM64_FAR  (ARM64_NEON_END + 8 * 1)
#define ARM64_CPSR (ARM64_NEON_END + 8 * 2)
#define ARM64_PC   (ARM64_NEON_END + 8 * 3)

#endif /* EXCEPTION_ASM_H */