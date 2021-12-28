#include "machine/io/mini_uart/mini_uart.h"
#include "lib/stdio.h"
void main(void) {
    mini_uart_init();
    mini_uart_send_str("---SWINK BOOT--\n");
    printf("printf test! str = %s, number = %d, pointer = %p\n", "string!", 41, &main);
}