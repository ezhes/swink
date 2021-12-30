#ifndef EXCEPTION_H
#define EXCEPTION_H
#include "lib/types.h"
#include "lib/assert.h"
#include "exception_asm.h"

struct arm64_context {
    struct gp {
        uint64_t x[29]; /* x0-28 */
        uint64_t fp;    /* x29 */
        uint64_t lr;    /* x30 */
        uint64_t sp;    /* x31 */
    } gp;

    struct neon {
        /* 32 qwords, so 32 * 2 dwords */
        uint64_t q[32 * 2];
        uint32_t fpsr;
    	uint32_t fpcr;
    } neon;

    uint64_t esr;
    uint64_t far;
    uint64_t cpsr;
    uint64_t pc;
};

typedef struct arm64_context * arm64_context_t;

/* exception_asm.h preprocessor definitions */
STATIC_ASSERT(ARM64_CONTEXT_SIZE == sizeof(struct arm64_context));
STATIC_ASSERT(ARM64_GP_END == sizeof(struct gp));
STATIC_ASSERT(ARM64_NEON_END == sizeof(struct gp) + sizeof(struct neon));
STATIC_ASSERT(ARM64_CPSR == ARM64_NEON_END + sizeof(uint64_t) * 2);
#endif /* EXCEPTION_H */