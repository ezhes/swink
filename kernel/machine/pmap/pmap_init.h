#ifndef PMAP_INIT_H
#define PMAP_INIT_H
#include "pmap.h"
#include "lib/types.h"
#include "core/vm/vm.h"

void pmap_vm_init(
    phys_addr_t kernel_base,
    phys_addr_t ram_base, 
    phys_addr_t ram_size,
    phys_addr_t bootstrap_pa_reserved);

#endif /* PMAP_INIT_H */