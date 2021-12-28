#include "lib/types.h"
#include "console.h"
#include "machine/io/mini_uart/mini_uart.h"

void
console_write_str(const char *str) {
    if (mini_uart_is_initiliazed()) {
        mini_uart_send_str(str);
    }
}

static void 
vprintf_helper(char c, void* char_count_ptr) {
    int *char_count = char_count_ptr;
    *char_count += 1;
    mini_uart_send_char(c);
}

int
console_vprintf(const char *format, va_list args) {
    int char_cnt = 0;
    __vprintf(format, args, vprintf_helper, &char_cnt);
    return char_cnt;
}