#include "pmap.h"
#include "lib/assert.h"
#include "core/vm/vm.h"
#include "pmap_asm.h"
#include "machine/platform_registers.h"

STATIC_ASSERT(VM_KERNEL_BASE_ADDRESS == (UINT64_MAX - (1ULL << (64 - TCR_T1SZ_VALUE)) + 1));

phys_addr_t pmap_kva_to_pa(vm_addr_t kva) {
    //TODO: Add VM support
    return (phys_addr_t)kva;
}