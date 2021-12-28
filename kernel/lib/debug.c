#include "debug.h"
#include "lib/stdio.h"
#include "machine/routines/routines.h"

NO_RETURN static void 
debug_panic(void)  {
    //TODO: dump debug info
    printf("\n*************************************\nSpinning core...\n");
    routines_core_idle();
}


void panic_simple(const char *message) {
    printf(
        "\n\n****************PANIC****************\n"
        "%s",
        message
    );

    debug_panic();
}

void panic_verbose(const char* file, int line, const char* function, const char* message, ...) {
    va_list args;

    printf(
        "\n\n****************PANIC****************\n"
        "File: %s:%d\n"
        "Function: %s\n",
        file, line, function
    );

    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    debug_panic();
}
