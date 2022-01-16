#ifndef MACHINE_ROUTINES
#define MACHINE_ROUTINES
#include "lib/debug.h"

/**
 * Infinite low-power loop
 */
extern void routines_core_idle(void) NO_RETURN;

/** Signal to a hosting debugger that the OS is exiting */
extern void routines_adp_application_exit(uint32_t exit_code) NO_RETURN;

#endif /* MACHINE_ROUTINES */
