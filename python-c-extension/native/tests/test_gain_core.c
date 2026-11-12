#include "hello/gain/gain_core.h"

/* gh-806: how many assertions just-makeit generated into this file.  Stamped
 * at scaffold time and compared against the runtime count by
 * JM_TEST_EPILOGUE(), so the "no assertions beyond the scaffold" note clears
 * itself the moment an author adds a check of their own -- a hard-coded
 * marker would still be claiming to be a placeholder long after the real
 * suite was written.
 *
 * Both defines come BEFORE the include: jm_test.h defaults each if the
 * including file has not set it, so a later define would be ignored. */
#define JM_TEST_NAME       "test_gain_core"
#define JM_SCAFFOLD_CHECKS 4

/* gh-934: CHECK, REQUIRE, the counters, the float helpers and the epilogue
 * all live in jm_test.h, written once per project.  They used to be stamped
 * into this template, so every component carried another private copy that
 * diverged from the day it was written. */
#include "jm_test.h"

int main(void)
{
    hello_gain_state_t *obj = hello_gain_create(2.0);
    REQUIRE(obj != NULL);

    /* k: getter / setter */
    CHECK(hello_gain_get_k(obj) == (double)(2.0));
    hello_gain_set_k(obj, 2.0);
    CHECK(hello_gain_get_k(obj) == 2.0);

    /* step: verify it runs without crashing */
    (void)hello_gain_step(obj, 0.0f + 0.0f * I);

    /* reset restores defaults */
    hello_gain_set_k(obj, 2.0);
    hello_gain_reset(obj);
    CHECK(hello_gain_get_k(obj) == (double)(2.0));

    hello_gain_destroy(obj);
    JM_TEST_EPILOGUE();
}
