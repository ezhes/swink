#include "runner.h"
#include "tests/tests.h"
#include "lib/stdio.h"
#include "machine/io/pmc/pmc.h"
#include "machine/routines/routines.h"

#define TAG "[runner] "


void
test_runner_run(void) {
    size_t test_count = 0;
    size_t test_pass_count = 0;
    size_t suite_count = COUNT_OF(suites);

    printf(TAG "Starting tests (%zu suites)...\n", suite_count);

    for (size_t suite_i = 0; suite_i < suite_count; suite_i++) {
        int result = 0;
        test_suite_t suite = suites[suite_i];
        
        printf(
            TAG "Running suite %s (sid=%zu, %zu tests)\n",
            suite->name, suite_i, suite->cases_count
        );

        if (suite->setup_function) {
            if ((result = suite->setup_function()) < 0) {
                panic(
                    "Test suite setup failed (result=%d, suite=%s, sid=%zu)",
                    result, suite->name, suite_i
                );
            }
        }

        for (size_t test_i = 0; test_i < suite->cases_count; test_i++) {
            test_case_t test = &suite->cases[test_i];

            test_count += 1;
            
            printf(
                TAG "\tRunning test %s (tid=%zu)\n",
                test->name, test_i
            );
            
            if ((result = test->function()) < 0) {
                printf(TAG "\tFAILED (result=%d)\n", result);
            } else {
                printf(TAG "\tPASSED\n");
                test_pass_count += 1;
            }
        }
    }

    printf(TAG "Run complete: %zu/%zu passed\n", test_pass_count, test_count);
    if (test_pass_count == test_count) {
        printf(TAG "** ALL TESTS PASSED **\n");
    }
    printf(TAG "Shutting down...\n");

    pmc_shutdown();
    routines_adp_application_exit(0);
    routines_core_idle();
}