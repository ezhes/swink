#include "debug.h"
#include "lib/stdio.h"
#include "machine/routines/routines.h"
#include "machine/io/vc/vc_functions.h"

#define PANIC_BANNER "\n****************PANIC****************\n"

/** Only attempt to set the activity LED to high (signaling a panic) */
static uint8_t tried_set_activity_panic_marker;

NO_RETURN static void 
debug_panic(void)  {
    //TODO: dump debug info
    const char *led_msg;

    if (tried_set_activity_panic_marker) {
        led_msg = "[warning] double panic'd, will not attempt to set LED again";
    } else {
        /* set panic flag to prevent panic loops while trying to enable */
        tried_set_activity_panic_marker = 0x1;
        if (vc_functions_set_activity_led(0x1)) {
            led_msg = "Panic LED enabled!";
        } else {
            led_msg = "[warning] Failed to set panic LED!";
        }
    }
    
    printf(
        "\n*************************************\n"
        "%s\n"
        "Spinning core...\n",
        led_msg
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
