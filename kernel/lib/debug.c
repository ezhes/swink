#include "debug.h"
#include "lib/stdio.h"

void debug_panic(const char* file, int line, const char* function, const char* message, ...) {
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

    printf("\n*************************************\nSpinning core...\n");
    while (1) {
        //TODO: dump state, disable interrupts, suspend other cores, etc.
    }
}
