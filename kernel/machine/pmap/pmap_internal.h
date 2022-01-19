#ifndef PMAP_INTERNAL_H
#define PMAP_INTERNAL_H
#include "lib/types.h"
#include "machine/synchronization/synchs.h"
#include "pmap_buddy_allocator.h"

/** Represents a virtual memory translation set */
struct pmap {
    /** General lock for the entire pmap */
    struct synchs_lock lock;

    /** The physical address of the VM table */
    phys_addr_t table_base;    
};
typedef struct pmap * pmap_t;

extern struct pmap pmap_kernel_s;
#define pmap_kernel     (&pmap_kernel_s)
extern pmap_buddy_allocator_t pb_allocator;
extern vm_addr_t physmap_vm_base;

#endif /* PMAP_INTERNAL_H */