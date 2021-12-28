#ifndef LIB_STDIO
#define LIB_STDIO
#include "lib/types.h"


typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

/* Standard functions. */
int printf(const char*, ...) PRINTF_FORMAT(1, 2);
int snprintf(char*, size_t, const char*, ...) PRINTF_FORMAT(3, 4);
int vprintf(const char*, va_list) PRINTF_FORMAT(1, 0);
int vsnprintf(char*, size_t, const char*, va_list) PRINTF_FORMAT(3, 0);
int putchar(int);
int puts(const char*);

/* Nonstandard functions. */
void hex_dump(uintptr_t ofs, const void*, size_t size, bool ascii);
void print_human_readable_size(uint64_t sz);

/* Internal functions. */
void __vprintf(const char* format, va_list args, void (*output)(char, void*), void* aux);
void __printf(const char* format, void (*output)(char, void*), void* aux, ...);

#endif /* LIB_STDIO */