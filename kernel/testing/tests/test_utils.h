#ifndef TESTS_TEST
#define TESTS_TEST
#include "lib/types.h"
#include "lib/string.h"
#include "lib/ctype.h"

typedef struct test_suite * test_suite_t;
typedef struct test_case * test_case_t;
typedef int (* test_function_t)(void);

/**
 * Represents a suite of tests. This is logically a single C file in the tests
 * directory
 */ 
struct test_suite {
    /**
     * A display name for the test suite
     */
    const char *name;

    /**
     * A function called once before any test in the suite is executed
     * If no function is needed, set to NULL.
     * Return negative to indicate a failure
     */
    test_function_t setup_function;

    /**
     * A function called once after the suite has finished executing any of the
     * requested tests.
     * If no function is needed, set to NULL.
     * Return negative to indicate a failure
     */
    test_function_t teardown_function;

    /**
     * Array of test cases
     */
    test_case_t cases;
    
    /**
     * The number of test cases in CASES
     */
    size_t cases_count;
};

/**
 * Represents a single test case/test function
 */
struct test_case {
    /**
     * The name of the test case
     */
    const char *name;

    /**
     * The function that represents the test case
     */
    test_function_t function;
};

#define TEST_CASE(f) {.name = #f , .function = f}

#endif /* TESTS_TEST */