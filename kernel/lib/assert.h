#ifndef LIB_ASSERT
#define LIB_ASSERT
#include "debug.h"

#define likely(expr)    (__builtin_expect(!!(expr), 1))
#define unlikely(expr)  (__builtin_expect(!!(expr), 0))
#define STATIC_ASSERT(cond) _Static_assert((cond), "Assertion failed: " #cond)

/**
 * Asserts that a condition is true, and panics if it isn't.
 * ASSERT statements WILL BE OPTIMIZED OUT IN RELEASE BUILDS. They are NOT
 * suitable for load bearing checks. Use REQUIRE instead.
 */
#define ASSERT(CONDITION)                                                      \
  if (likely(CONDITION)) {                                                     \
  } else {                                                                     \
    panic("Assertion failed: %s", #CONDITION);                                 \
  }

/**
 * Requires that a condition be true, panics otherwise.
 * REQUIRE statements are included in release builds.
 */
#define REQUIRE(CONDITION)                                                     \
  if (unlikely(!(CONDITION))) {                                                \
    panic("Requirement failed: %s", #CONDITION);                               \
  }
#endif /* LIB_ASSERT */