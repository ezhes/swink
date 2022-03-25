#include "machine/io/console/console.h"
#include "machine/io/vc/vc_functions.h"
#include "machine/routines/routines.h"
#include "lib/stdio.h"
#include "machine/io/pmc/pmc.h"
#include "lib/string.h"
#include "machine/pmap/pmap.h"
#include "machine/pmap/pmap_init.h"
#include "machine/pmap/pmap_pfa.h"
#include "machine/platform_registers.h"

extern void pmap_pfa_dump_state(void);
extern void pmap_pfa_get_state(size_t *level_buffer, size_t count);

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

    pmap_page_metadata_s m;
    m.padding = 0;
    m.page_type = PMAP_PAGE_TYPE_KERNEL_DATA;
    size_t original_state[6];
    size_t temp_state[6];

    pmap_pfa_dump_state();
    pmap_pfa_get_state(original_state, 6);

    phys_addr_t addr;
    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        addr = pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &m);
        printf("alloc gave %d for %d pages\n", addr >> PAGE_SHIFT, pg_count);
        pmap_pfa_free_contig(addr, PAGE_SIZE * pg_count);
        printf("freed %d\n", pg_count);

        pmap_pfa_get_state(temp_state, 6);
        if (memcmp(original_state, temp_state, sizeof(temp_state))) {
            pmap_pfa_dump_state();
            panic("failed!");
        }
    }

    phys_addr_t addrs[32];
    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        addrs[pg_count - 1] = pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &m);
    }

    for (unsigned int pg_count = 31; pg_count >= 1; pg_count--) {
        printf("1 *** freeing %d (%d)\n", pg_count, addrs[pg_count - 1] >> PAGE_SHIFT);
        pmap_pfa_free_contig(addrs[pg_count - 1], PAGE_SIZE * pg_count);
    }

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        addrs[pg_count - 1] = pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &m);
    }

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        printf("2 *** freeing %d (%d)\n", pg_count, addrs[pg_count - 1] >> PAGE_SHIFT);
        pmap_pfa_free_contig(addrs[pg_count - 1], PAGE_SIZE * pg_count);
    }

    for (unsigned int pg_count = 31; pg_count >= 1; pg_count--) {
        addrs[pg_count - 1] = pmap_pfa_alloc_contig(PAGE_SIZE * pg_count, &m);
        printf("alloc gave %d for %d pages\n", addrs[pg_count - 1] >> PAGE_SHIFT, pg_count);
    }

    for (unsigned int pg_count = 1; pg_count < 32; pg_count++) {
        printf("3 *** freeing %d (%d)\n", pg_count, addrs[pg_count - 1] >> PAGE_SHIFT);
        pmap_pfa_free_contig(addrs[pg_count - 1], PAGE_SIZE * pg_count);
    }


    pmap_pfa_get_state(temp_state, 6);
    if (memcmp(original_state, temp_state, sizeof(temp_state))) {
        pmap_pfa_dump_state();
        panic("failed!");
    }

    printf("*** TEST PASS!\n");

    // uint64_t pages = 0;
    // while ((addr = pmap_pfa_alloc_contig(PAGE_SIZE, &m)) != PHYS_ADDR_INVALID) {
    //     printf("alloc %d\n", addr >> PAGE_SHIFT);
    //     pages++;
    // }
    // // 245065
    // printf("allocated %llu pages before OOM\n", pages);


    printf("[*] Shutting down...\n");
    // routines_adp_application_exit(0);
    pmc_shutdown();
    routines_core_idle();
    /* NO RETURN */
}