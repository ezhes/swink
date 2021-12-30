#include "debug.h"
#include "lib/stdio.h"
#include "machine/routines/routines.h"

#define PANIC_BANNER "\n****************PANIC****************\n"

NO_RETURN static void 
debug_panic(void)  {
    //TODO: dump debug info
    printf(
        "\n*************************************\n"
        "Spinning core...\n"
    );
    routines_core_idle();
}

void panic_simple(const char *message) {
    printf(
        PANIC_BANNER
        "%s",
        message
    );

    debug_panic();
}

void panic_formatted(const char *message, ...) {
    va_list args;
    
    printf(PANIC_BANNER);
    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    debug_panic();
}

void panic_macro(const char* file, int line, const char* function, 
                 const char* message, ...) {
    va_list args;

    printf(
        PANIC_BANNER
        "File: %s:%d\n"
        "Function: %s\n",
        file, line, function
    );

    va_start(args, message);
    vprintf(message, args);
    va_end(args);

    debug_panic();
}
