#ifndef CONSOLE_H
#define CONSOLE_H
#include "lib/stdio.h"

/** Initialize the console and all enables outputs for writing */
void console_init(void);

/** Attempt to write `str` to the console */
void
console_write_str(const char *str);

int
console_vprintf(const char *format, va_list args);
#endif /* CONSOLE_H */