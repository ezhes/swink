#include "exception.h"
#include "lib/types.h"
#include "lib/stdio.h"

#define STRINGIFY(x) #x
#define ENUM_TO_STR_TABLE(e) [e] = STRINGIFY(e)

#define RANGE_BITS(value, start, stop) \
    (((value) >> (start)) & ((1LLU << ((stop + 1) - (start))) - 1))

#define ESR_EC(esr) RANGE_BITS(esr, 26, 31)
#define ESR_ISS(esr) RANGE_BITS(esr, 0, 24)
#define SPSR_EXECUTION_LEVEL(spsr) RANGE_BITS(spsr, 0, 3)

enum exception_class {
    EXCEPTION_CLASS_UNKNOWN             = 0b000000,
    EXCEPTION_CLASS_TRAP_WF             = 0b000001,
    EXCEPTION_CLASS_TRAP_MCR_MRC1       = 0b000011,
    EXCEPTION_CLASS_TRAP_MCRR_MRRC1     = 0b000100,
    EXCEPTION_CLASS_TRAP_MCR_MRC2       = 0b000101,
    EXCEPTION_CLASS_TRAP_LDC_STC        = 0b000110,
    EXCEPTION_CLASS_TRAP_SVE_FP_NEON    = 0b000111,
    EXCEPTION_CLASS_TRAP_LD_ST64        = 0b001010,
    EXCEPTION_CLASS_TRAP_MCRR_MRRC2     = 0b001100,
    EXCEPTION_CLASS_BRANCH_TARGET_FAIL  = 0b001101,
    EXCEPTION_CLASS_ILLEGAL_EXEC_STATE  = 0b001110,
    EXCEPTION_CLASS_SVC_HVC3            = 0b010001,
    EXCEPTION_CLASS_SVC_HVC64           = 0b010101,
    EXCEPTION_CLASS_TRAP_MSR_MRS        = 0b011000,
    EXCEPTION_CLASS_TRAP_CPACR_S        = 0b011001,
    EXCEPTION_CLASS_POINTER_AUTH_FAIL   = 0b011100,
    EXCEPTION_CLASS_INST_ABORT_LOWER_EL = 0b100000,
    EXCEPTION_CLASS_INST_ABORT_SAME_EL  = 0b100001,
    EXCEPTION_CLASS_PC_ALIGNMENT_FAULT  = 0b100010,
    EXCEPTION_CLASS_DATA_ABORT_LOWER_EL = 0b100100,
    EXCEPTION_CLASS_DATA_ABORT_SAME_EL  = 0b100101,
    EXCEPTION_CLASS_SP_ALIGNMENT_FAULT  = 0b100110,
    EXCEPTION_CLASS_TRAP_FP32           = 0b101000,
    EXCEPTION_CLASS_TRAP_FP64           = 0b101100,
    EXCEPTION_CLASS_SERROR              = 0b101111,
    EXCEPTION_CLASS_BP_LOWER_EL         = 0b110000,
    EXCEPTION_CLASS_BP_SAME_EL          = 0b110001,
    EXCEPTION_CLASS_SW_STEP_LOWER_EL    = 0b110010,
    EXCEPTION_CLASS_SW_STEP_SAME_EL     = 0b110011,
    EXCEPTION_CLASS_WP_LOWER_EL         = 0b110100,
    EXCEPTION_CLASS_WP_SAME_EL          = 0b110101,
    EXCEPTION_CLASS_BRK_32              = 0b111000,
    EXCEPTION_CLASS_BRK_64              = 0b111100,
    EXCEPTION_CLASS_MAX
};

static const char *
get_exception_class_str(uint32_t esr) {
    static const char *exception_class_to_str[] = {
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_UNKNOWN),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_WF),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_MCR_MRC1),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_MCRR_MRRC1),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_MCR_MRC2),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_LDC_STC),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_SVE_FP_NEON),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_LD_ST64),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_MCRR_MRRC2),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_BRANCH_TARGET_FAIL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_ILLEGAL_EXEC_STATE),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_SVC_HVC3),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_SVC_HVC64),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_MSR_MRS),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_CPACR_S),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_POINTER_AUTH_FAIL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_INST_ABORT_LOWER_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_INST_ABORT_SAME_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_PC_ALIGNMENT_FAULT),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_DATA_ABORT_LOWER_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_DATA_ABORT_SAME_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_SP_ALIGNMENT_FAULT),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_FP32),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_TRAP_FP64),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_SERROR),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_BP_LOWER_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_BP_SAME_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_SW_STEP_LOWER_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_SW_STEP_SAME_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_WP_LOWER_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_WP_SAME_EL),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_BRK_32),
        ENUM_TO_STR_TABLE(EXCEPTION_CLASS_BRK_64),
    };

    uint64_t ec = ESR_EC(esr);
    if (ec >= EXCEPTION_CLASS_MAX) {
        return NULL;
    }

    return exception_class_to_str[ec];
}

UNUSED static bool 
cpsr_is_el0(uint32_t cpsr) {
    return SPSR_EXECUTION_LEVEL(cpsr) == 0b0000 /* EL0t */;
}

static const char *
get_execution_level_str(uint64_t cpsr) {
    switch (SPSR_EXECUTION_LEVEL(cpsr)) {
        case 0b0000:
            return "EL0_SP0";
        case 0b0100:
            return "EL1_SP0";
        case 0b0101:
            return "EL1_SP1";
        default:
            return "<unknown>";
    }
}

static void
dump_state(arm64_context_t context) {
    printf(
        "************REGISTER DUMP************\n"
        "context = %p\n"
        "ESR     = 0x%016x (ISS = 0x%08x, %s)\n"
        "FAR     = 0x%016llx\n"
        "CPSR    = 0x%016x (%s)\n"
        "TCR_EL1 = 0x%016lx\n"
        "SCTLR_EL1=0x%016lx\n",
        context,
        context->esr, (uint32_t)ESR_ISS(context->esr), 
            get_exception_class_str(context->esr),
        context->far,
        context->cpsr, get_execution_level_str(context->cpsr),
        __builtin_arm_rsr64("tcr_el1"), __builtin_arm_rsr64("sctlr_el1")
    );

    /* print x0...x28 */
    for (unsigned int i = 0; i + 4 < 29; i += 4) {
        printf(
            "X%02d: 0x%016llx "
            "X%02d: 0x%016llx "
            "X%02d: 0x%016llx "
            "X%02d: 0x%016llx\n",
            i + 0, context->gp.x[i + 0], 
            i + 1, context->gp.x[i + 1],
            i + 2, context->gp.x[i + 2],
            i + 3, context->gp.x[i + 3]
        );
    }

    printf(
        "FP : 0x%016llx "
        "LR : 0x%016llx "
        "SP : 0x%016llx "
        "PC : 0x%016llx\n",
        context->gp.fp, context->gp.lr, context->gp.sp, context->pc
    );
}

void exception_sync(arm64_context_t context) {
    dump_state(context);
    if (ESR_EC(context->esr) == EXCEPTION_CLASS_BRK_64) {
        printf("[debug] stepping over breakpoint...\n");
        context->pc += 4;
        return;
    }
    panic("Unhandled exception (synchronous)");
}

void exception_irq(arm64_context_t context) {
    dump_state(context);
    panic("Unhandled exception (IRQ)");
}

void exception_fiq(arm64_context_t context) {
    dump_state(context);
    panic("Unhandled exception (FIQ)");
}

void exception_error(arm64_context_t context) {
    dump_state(context);
    panic("Unhandled exception (error)");
}