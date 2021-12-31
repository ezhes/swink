/*
 * Copyright (C) 2018 bzt (bztsrc@github)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#include "mini_uart.h"
#include "lib/string.h"
#include "machine/io/gpio.h"

static bool uart_initilized = false;

bool
mini_uart_is_initiliazed(void) {
    return uart_initilized;
}

void
mini_uart_init(void) {
    /*
    Set baud rate and characteristics (115200 8N1) and map to GPIO
    */
    ASSERT(!uart_initilized);

    register unsigned int r;

    /* initialize UART */
    *AUX_MU_ENABLE |=1;       // enable UART1, AUX mini uart
    *AUX_MU_CNTL = 0;
    *AUX_MU_LCR = 3;       // 8 bits
    *AUX_MU_MCR = 0;
    *AUX_MU_IER = 0;
    *AUX_MU_IIR = 0xc6;    // disable interrupts
    *AUX_MU_BAUD = 270;    // 115200 baud
    /* map UART1 to GPIO pins */
    r=*GPFSEL1;
    r&=~((7<<12)|(7<<15)); // gpio14, gpio15
    r|=(2<<12)|(2<<15);    // alt5
    *GPFSEL1 = r;
    *GPPUD = 0;            // enable pins 14 and 15
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = (1<<14)|(1<<15);
    r=150; while(r--) { asm volatile("nop"); }
    *GPPUDCLK0 = 0;        // flush GPIO setup
    *AUX_MU_CNTL = 3;      // enable Tx, Rx
    uart_initilized = true;
}

void
mini_uart_send_char(char c) {
    /* wait until we can send */
    while(!(*AUX_MU_LSR&0x20)) {}
    /* write the character to the buffer */
    *AUX_MU_IO=c;
    __builtin_arm_dsb(0xf);
    if (c == '\n') {
        mini_uart_send_char('\r');
    }
}

void
mini_uart_send_str(const char *str) {
    while (*str) {
        mini_uart_send_char(*str);
        str++;
    }
}


char
mini_uart_getc(void) {
    /* wait until something is in the buffer */
    while(!(*AUX_MU_LSR&0x01)) {}
    /* read it and return */
    return (char)(*AUX_MU_IO);
}