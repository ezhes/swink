#include "machine/io/console/console.h"
#include "machine/io/vc/vc_functions.h"
#include "machine/routines/routines.h"
#include "lib/stdio.h"
#include "machine/io/pmc/pmc.h"
#include "lib/string.h"
#include "machine/pmap/pmap.h"
#include "machine/pmap/pmap_init.h"
#include "machine/platform_registers.h"

extern uint8_t __bss_start;
extern uint8_t __bss_end;
/**
 * @brief The first C function invoked on boot. Executed once by the primary CPU
 * Virtual memory is active with both a P=V map and a KVA map. VM, however, is 
 * in "bootstrap" mode where everything is RWX (which will be solved later in 
 * boot).
 * 
 * The CPU is configured with both an application and exception stack which live
 * in the physical bootstrap region.
 * 
 * @param kernel_base The physical address of the first kernel symbol, aka the 
 * address of `__kernel_map_start`
 * 
 * @param bootstrap_pa_reserved The maximum physical address which is reserved
 * until the kernel has migrated off of bootstrap. [0, reserved) contains the
 * bootstrap page tables and the bootstrap stacks. Once the kernel ends its
 * bootstrap phase, this memory may be reclaimed.
 */
void 
main(phys_addr_t kernel_base, phys_addr_t bootstrap_pa_reserved) {
    /* zero BSS */
    memset(&__bss_start, 0x00, &__bss_end - &__bss_start);
    
    console_init();

    printf(
        "\033[2J"
        "SwinkOS kernel booting...\n"
        "Buildstamp: " __DATE__ "@" __TIME__ "\n"
        "(c) Allison Husain 2022\n\n"
    );

    phys_addr_t arm_ram_base = 0;
    uint32_t arm_ram_size = 0;
    uint32_t board_model = 0;
    uint32_t board_revision = 0;
    uint32_t videocore_fw = 0;
    
    if (!vc_functions_get_arm_memory_region(&arm_ram_base, &arm_ram_size)
        || !vc_functions_get_board_model(&board_model)
        || !vc_functions_get_vc_revision(&board_revision)
        || !vc_functions_get_vc_revision(&videocore_fw)) {
        panic("Failed to get VC properties!");
    }
    
    printf(
        "ARM DRAM = 0x%08llx -> 0x%08llx (->0x%08llx reserved)\n"
        "Model = 0x%08x, revision = 0x%08x\n"
        "VideoCore firmware version = 0x%08x\n\n",
        arm_ram_base, arm_ram_base + arm_ram_size, bootstrap_pa_reserved,
        board_model, board_revision,
        videocore_fw
    );

    pmap_vm_init(
        kernel_base, 
        arm_ram_base, arm_ram_size, 
        bootstrap_pa_reserved
    );

    printf("[*] Shutting down...\n");
    routines_adp_application_exit(0);
    pmc_shutdown();
    // routines_core_idle();
    /* NO RETURN */
}