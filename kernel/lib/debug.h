#ifndef LIB_DEBUG
#define LIB_DEBUG

#define UNUSED __attribute__((unused))
#define NO_RETURN __attribute__((noreturn))
#define NO_INLINE __attribute__((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__((format(printf, FMT, FIRST)))

void panic_simple(const char *message) NO_RETURN;
void panic_formatted(const char *message, ...) PRINTF_FORMAT(1, 2) NO_RETURN;
void panic_macro(const char* file, int line, const char* function, const char* message, ...)
    PRINTF_FORMAT(4, 5) NO_RETURN;

#define panic(...) panic_macro(__FILE__, __LINE__, __func__, __VA_ARGS__)

#define HW_BRK  asm("brk #1")

#endif /* LIB_DEBUG */