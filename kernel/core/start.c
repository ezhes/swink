#include "machine/io/mini_uart/mini_uart.h"
#include "machine/routines/routines.h"
#include "lib/stdio.h"
void main(void) {
    mini_uart_init();
    printf(
        "SwinkOS kernel booting...\n"
        "Buildstamp: " __DATE__ "@" __TIME__ "\n"
        "(c) Allison Husain 2022\n"
    );

    asm ("brk #1");

    routines_core_idle();
    /* NO RETURN */
}