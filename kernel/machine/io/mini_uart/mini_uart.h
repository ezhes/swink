#ifndef MINI_UART_H
#define MINI_UART_H
#include "lib/types.h"

/**
 * @brief Returns whether the mini UART subsystem is ready.
 */
bool
mini_uart_is_initiliazed(void);

/**
 * @brief Initilizes the mini UART subsystem. Call once.
 */
void
mini_uart_init(void);

/**
 * @brief Write a single charachter to UART
 * 
 * @param c The charachter to write out
 */
void
mini_uart_send_char(char c);

/**
 * @brief Send a string to UART
 * 
 * @param str The string to write out (NULL terminated)
 */
void
mini_uart_send_str(const char *str);

/**
 * @brief Read a single character from UART
 * 
 * @return char The read character
 */
char
mini_uart_getc(void);
#endif /* MINI_UART_H */