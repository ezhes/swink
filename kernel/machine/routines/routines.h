#ifndef MACHINE_ROUTINES
#define MACHINE_ROUTINES
#include "lib/debug.h"

/**
 * @brief Infinite low-power loop
 */
extern void routines_core_idle(void) asm ("_routines_core_idle") NO_RETURN;

#endif /* CORE_CORE_ROUTINES */