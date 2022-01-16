#ifndef PMAP_BUDDY_ALLOCATOR_H
#define PMAP_BUDDY_ALLOCATOR_H
#include "lib/types.h"
#include "pmap.h"

typedef struct pmap_buddy_allocator * pmap_buddy_allocator_t;

/** 
 * Initialize the buddy allocator with RAM_BASE of size RAM_SIZE. Reserves
 * a memory range [0, BOOTSTRAP_PA_RESERVED) for use in bootstrap.
 */
void pmap_buddy_allocator_init(phys_addr_t ram_base, 
                                phys_addr_t ram_size,
                                phys_addr_t bootstrap_pa_reserved);

#endif /* PMAP_BUDDY_ALLOCATOR_H */