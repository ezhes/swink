#ifndef VC_FUNCTIONS_H
#define VC_FUNCTIONS_H
#include "lib/types.h"
#include "machine/pmap/pmap.h"
/** Gets the physical ARM DRAM memory region. Returns true on success */
bool vc_functions_get_arm_memory_region(phys_addr_t *addr, uint32_t *size);

/** Get the board model */
bool vc_functions_get_board_model(uint32_t *model);

/** Get the board revision */
bool vc_functions_get_board_revision(uint32_t *revision);

/** Get the VideoCore firmware revision */
bool vc_functions_get_vc_revision(uint32_t *revision);

/** Enable or disable the on-board activity LED */
bool vc_functions_set_activity_led(bool enabled);

#endif /* VC_FUNCTIONS_H */