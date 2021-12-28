#ifndef LIB_DEBUG
#define LIB_DEBUG

#define UNUSED __attribute__((unused))
#define NO_RETURN __attribute__((noreturn))
#define NO_INLINE __attribute__((noinline))
#define PRINTF_FORMAT(FMT, FIRST) __attribute__((format(printf, FMT, FIRST)))

void debug_panic(const char* file, int line, const char* function, const char* message, ...)
    PRINTF_FORMAT(4, 5) NO_RETURN;

#define panic(...) debug_panic(__FILE__, __LINE__, __func__, __VA_ARGS__)

#endif /* LIB_DEBUG */