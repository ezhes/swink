#ifndef PMAP_H
#define PMAP_H
#include "lib/types.h"
#include "core/vm/vm.h"

/** Describes a physical address */
typedef uint64_t phys_addr_t;
/** An invalid physical address */
#define PHYS_ADDR_ERROR (UINT64_MAX)

/** Converts a kernel virtual address to a physical address. 
 * Returns PHYS_ADDR_ERROR if the translation is not valid in this context
 */
phys_addr_t pmap_kva_to_pa(vm_addr_t kva);


#endif /* PMAP_H */