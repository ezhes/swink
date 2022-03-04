#ifndef PMAP_H
#define PMAP_H
#include "lib/types.h"
#include "core/vm/vm.h"

/** Describes a physical address */
typedef uint64_t phys_addr_t;
/** An invalid physical address */
#define PHYS_ADDR_INVALID (UINT64_MAX)

/** 
 * Converts a kernel virtual address to a physical address. 
 * Returns PHYS_ADDR_ERROR if the translation is not valid in this context
 */
phys_addr_t pmap_kva_to_pa(vm_addr_t kva);

/** Converts a physical address to a kernel virtual address */
vm_addr_t pmap_pa_to_kva(phys_addr_t pa);

/** 
 * Converts a physmap KVA address to a physical address.
 * The advantage of this function over pmap_kva_to_pa is that it is faster for
 * physmap addresses as it does not require paging to translate an address
 */
phys_addr_t pmap_physmap_kva_to_pa(vm_addr_t kva);

#endif /* PMAP_H */