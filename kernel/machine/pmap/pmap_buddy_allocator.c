#include "pmap_buddy_allocator.h"
#include "machine/synchronization/synchs.h"

struct pmap_buddy_allocator {
    struct synchs_lock lock;
};

static struct pmap_buddy_allocator pb_allocator_s;
#define pb_allocator (&pb_allocator_s)

void pmap_buddy_allocator_init(phys_addr_t ram_base, 
                                phys_addr_t ram_size,
                                phys_addr_t bootstrap_pa_reserved) {
    synchs_lock_init(&pb_allocator->lock);
}