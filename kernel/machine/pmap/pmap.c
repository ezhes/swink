#include "pmap.h"

phys_addr_t pmap_kva_to_pa(vm_addr_t kva) {
    //TODO: Add VM support
    return (phys_addr_t)kva;
}