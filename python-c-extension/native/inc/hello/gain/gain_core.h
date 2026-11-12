/**
 * @file gain_core.h
 * @brief Gain component API.
 *
 * Lifecycle: create -> [step / steps / reset]* -> destroy
 *
 * Example:
 * @code
 * double k = 0; // your value
 * hello_gain_state_t *obj = hello_gain_create(k);
 * float _Complex y = hello_gain_step(obj, 0.0f + 0.0f * I);
 * hello_gain_destroy(obj);
 * @endcode
 */
#ifndef HELLO_GAIN_CORE_H
#define HELLO_GAIN_CORE_H

#include "hello/clib_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Gain state.
 *
 * Allocate with hello_gain_create().
 */
typedef struct {
    double k;

} hello_gain_state_t;

/**
 * @brief Create a gain instance.
 *
 * @param k  Initial k (default: 2.0).
 * @return Heap-allocated state, or NULL on allocation failure.
 * @note Caller must call hello_gain_destroy() when done.
 */
hello_gain_state_t *hello_gain_create(double k);

/**
 * @brief Destroy a gain instance and release all memory.
 * @param state  May be NULL.
 */
void hello_gain_destroy(hello_gain_state_t *state);

/**
 * @brief Reset Gain to its post-create state.
 * @param state  Must be non-NULL.
 */
void hello_gain_reset(hello_gain_state_t *state);

/**
 * @brief Get current k.
 * @param state  Must be non-NULL.
 */
double hello_gain_get_k(const hello_gain_state_t *state);

/**
 * @brief Set k.
 * @param state  Must be non-NULL.
 * @param val    New value.
 */
void hello_gain_set_k(hello_gain_state_t *state, double val);



/**
 * @brief Process one input sample.
 * @param state  Must be non-NULL.
 * @param x      Input sample (float _Complex).
 * @return Output sample (float _Complex).
 */
static inline float _Complex
hello_gain_step(const hello_gain_state_t *state, float _Complex x)
{
    (void)state; /* TODO: implement using state variables */
    return (float _Complex)x;
}

/**
 * @brief Process a block of samples.
 *
 * @param state   Component state (mutated).
 * @param input   Input array (length >= n).
 * @param output  Output array (length >= n; may alias input for in-place).
 * @param n       Number of samples.
 */
void hello_gain_steps(
    hello_gain_state_t *state,
    const float _Complex    *input,
    float _Complex          *output,
    size_t               n);

#ifdef __cplusplus
}
#endif

#endif /* HELLO_GAIN_CORE_H */
