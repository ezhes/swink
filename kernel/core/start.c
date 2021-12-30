#include "machine/io/console/console.h"
#include "machine/routines/routines.h"
#include "lib/stdio.h"
#include "machine/io/pmc/pmc.h"

void 
main(void) {
    console_init();

    printf(
        "SwinkOS kernel booting...\n"
        "Buildstamp: " __DATE__ "@" __TIME__ "\n"
        "(c) Allison Husain 2022\n"
    );

    printf("[*] Shutting down...\n");
    pmc_shutdown();
    /* NO RETURN */
}