#ifndef LIB_ASSERT
#define LIB_ASSERT
#include "debug.h"

#define STATIC_ASSERT(cond) _Static_assert((cond), "Assertion failed: " #cond)
#define ASSERT(CONDITION)                                                      \
  if (CONDITION) {                                                             \
  } else {                                                                     \
    panic("Assertion failed: %s", #CONDITION);                                 \
  }
#endif /* LIB_ASSERT */