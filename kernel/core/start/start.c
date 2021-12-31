#include "machine/io/console/console.h"
#include "machine/io/vc/vc_functions.h"
#include "machine/routines/routines.h"
#include "lib/stdio.h"
#include "machine/io/pmc/pmc.h"

void 
main(void) {
    console_init();

    printf(
        "\033[2J"
        "SwinkOS kernel booting...\n"
        "Buildstamp: " __DATE__ "@" __TIME__ "\n"
        "(c) Allison Husain 2022\n\n"
    );

    phys_addr_t arm_base = 0;
    uint32_t arm_size = 0;
    uint32_t board_model = 0;
    uint32_t board_revision = 0;
    uint32_t videocore_fw = 0;
    
    if (!vc_functions_get_arm_memory_region(&arm_base, &arm_size)
        || !vc_functions_get_board_model(&board_model)
        || !vc_functions_get_vc_revision(&board_revision)
        || !vc_functions_get_vc_revision(&videocore_fw)) {
        panic("Failed to get VC properties!");
    }
    
    printf(
        "ARM DRAM = 0x%08llx -> 0x%08llx\n"
        "Model = 0x%08x, revision = 0x%08x\n"
        "VideoCore firmware version = 0x%08x\n\n",
        arm_base, arm_base + arm_size,
        board_model, board_revision,
        videocore_fw
    );

    asm ("brk #1");


    printf("[*] Shutting down...\n");
    pmc_shutdown();
    routines_core_idle();
    /* NO RETURN */
}