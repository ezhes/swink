#include "lib/types.h"
#include "machine/io/gpio.h"
#include "machine/routines/routines.h"

void 
pmc_reset(void) {
    unsigned int r;
    // trigger a restart by instructing the GPU to boot from partition 0
    r = *PM_RSTS; r &= ~0xfffffaaa;
    *PM_RSTS = PM_WDOG_MAGIC | r;   // boot from partition 0
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;

    /* Idle while waiting for reset */
    routines_core_idle();
}

void 
pmc_shutdown(void) {
    unsigned long r;
    r = *PM_RSTS; r &= ~0xfffffaaa;
    r |= 0x555;    // partition 63 used to indicate halt
    *PM_RSTS = PM_WDOG_MAGIC | r;
    *PM_WDOG = PM_WDOG_MAGIC | 10;
    *PM_RSTC = PM_WDOG_MAGIC | PM_RSTC_FULLRST;
    
    /* Idle while waiting for shutdown */
    routines_core_idle();
}