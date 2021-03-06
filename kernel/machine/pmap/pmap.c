#include "pmap.h"
#include "pmap_internal.h"
#include "lib/assert.h"
#include "lib/ctype.h"
#include "core/vm/vm.h"
#include "core/vm/vm_page_allocator.h"
#include "pmap_asm.h"
#include "machine/platform_registers.h"
#include "machine/io/gpio.h"
#include "lib/stdio.h"
#include "lib/ctype.h"
#include "lib/string.h"

/* 
Ensure our VM base from vm_asm.h is actually correct for the address space size
*/
STATIC_ASSERT(VM_KERNEL_BASE_ADDRESS == 
    (UINT64_MAX - (1ULL << (64 - TCR_T1SZ_VALUE)) + 1));

struct pmap pmap_kernel_s;
#define pmap_kernel     (&pmap_kernel_s)

vm_addr_t physmap_vm_base;

phys_addr_t pmap_kva_to_pa(vm_addr_t kva) {
    //TODO: Add VM support
    return (phys_addr_t)kva;
}

vm_addr_t pmap_pa_to_kva(phys_addr_t pa) {
    return physmap_vm_base + pa;
}

phys_addr_t pmap_physmap_kva_to_pa(vm_addr_t kva) {
    /* 
    The physmap is at the end of the KVA space, which means we can very easily
    check and translate addresses in the physmap back to PAs
    */
   
    REQUIRE(kva >= physmap_vm_base);
    return kva - physmap_vm_base;
}